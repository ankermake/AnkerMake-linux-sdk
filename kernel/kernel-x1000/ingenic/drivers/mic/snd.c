/*
 * f_audio.c -- USB Audio Class 2.0 Function
 *
 * Copyright (C) 2011
 *    Yadwinder Singh (yadi.brar01@gmail.com)
 *    Jaswinder Singh (jaswinder.singh@linaro.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/usb/audio.h>
#include <linux/usb/audio-v2.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/memory.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/control.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <common.h>
#include "snd.h"
#include "i2s.h"

static const char *codec_name = "snd_audio";
static struct audio_dev *g_audio = NULL;

#define PERIOD_BYTES_MIN (1024)
#define PERIODS_MIN 4
#define PERIODS_MAX 120
#define JZ_DMA_BUFFERSIZE (PERIODS_MAX * PERIOD_BYTES_MIN)

static const struct snd_pcm_hardware codec_pcm_hardware = {
	.info           = SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_MMAP_VALID,
	.formats        = SNDRV_PCM_FMTBIT_S16_LE,
	.channels_min       = 2,
	.channels_max       = 2,
	.buffer_bytes_max   = JZ_DMA_BUFFERSIZE,
	.period_bytes_min   = PERIOD_BYTES_MIN,
	.period_bytes_max   = JZ_DMA_BUFFERSIZE / PERIODS_MIN,
	.periods_min        = PERIODS_MIN,
	.periods_max        = PERIODS_MAX,
	.rate_min	= 48000,
	.rate_max	= 48000,
	.fifo_size	= 0,
};

static inline struct audio_dev *pdev_to_audio(struct platform_device *p)
{
	return container_of(p, struct audio_dev, pdev);
}

static int codec_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct audio_dev *audio = snd_pcm_substream_chip(substream);
	struct audio_rtd_params *prm = &audio->p_prm;
	unsigned long flags;
	int err = 0;

	assert(substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
	spin_lock_irqsave(&prm->lock, flags);
	prm->offset = 0;
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
			prm->ss = substream;
			snd_hrtimer_start(prm);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
			prm->ss = NULL;
			snd_hrtimer_stop(prm);
			break;
		default:
			err = -EINVAL;
	}
	spin_unlock_irqrestore(&prm->lock, flags);

	return err;
}

static snd_pcm_uframes_t codec_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct audio_dev *audio = snd_pcm_substream_chip(substream);
	struct audio_rtd_params *prm = &audio->p_prm;
	assert(substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
	return bytes_to_frames(substream->runtime, prm->offset);
}

static int codec_pcm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *hw_params)
{
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
}

static int codec_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct audio_dev *audio = (struct audio_dev *)snd_pcm_substream_chip(substream);
	struct audio_rtd_params *prm = &audio->p_prm;

	assert(substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
	prm->dma_area = 0;
	prm->dma_bytes = 0;
	prm->period_size = 0;
	return snd_pcm_lib_free_pages(substream);
}

static int codec_pcm_open(struct snd_pcm_substream *substream)
{
	struct audio_dev *audio = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	audio->p_residue = 0;
	assert(substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
	runtime->hw = codec_pcm_hardware;
	snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	return 0;
}

static int codec_pcm_prepare(struct snd_pcm_substream *substream)
{
	return codec_dac_power(true);
}

static int codec_pcm_close(struct snd_pcm_substream *substream)
{
	return codec_dac_power(false);
}

static struct snd_pcm_ops codec_pcm_ops = {
	.open = codec_pcm_open,
	.close = codec_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = codec_pcm_hw_params,
	.hw_free = codec_pcm_hw_free,
	.trigger = codec_pcm_trigger,
	.pointer = codec_pcm_pointer,
	.prepare = codec_pcm_prepare,
};

static int snd_codec_info_volume(struct snd_kcontrol *kctrl,
		struct snd_ctl_elem_info *uinfo)
{
	struct audio_dev *audio = snd_kcontrol_chip(kctrl);
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = audio->codec_params.vol_min_val;
	uinfo->value.integer.max = audio->codec_params.vol_max_val;
	return 0;
}

static int snd_codec_get_volume(struct snd_kcontrol *kctrl,
		struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_get_dac_vol();
	return 0;
}

static int snd_codec_put_volume(struct snd_kcontrol *kctrl,
		struct snd_ctl_elem_value *ucontrol)
{
	u32 vol = ucontrol->value.integer.value[0];
	return codec_set_dac_vol(vol);
}

static int snd_codec_vol_tlv(struct snd_kcontrol *kcontrol, int op_flag,
		unsigned int size, unsigned int __user *_tlv)
{
	struct audio_dev *audio = kcontrol->private_data;
	DECLARE_TLV_DB_SCALE(scale, 0, 0, 0);
	if (size < sizeof(scale))
		return -EINVAL;
	scale[2] = audio->codec_params.vol_min_DB;
	scale[3] = (audio->codec_params.vol_step_DB & TLV_DB_SCALE_MASK) | \
		   (audio->codec_params.vol_mute ? TLV_DB_SCALE_MUTE : 0);
	if (copy_to_user(_tlv, scale, sizeof(scale)))
		return -EFAULT;
	return 0;
}

static struct snd_kcontrol_new playback_controls = {
	.iface      = SNDRV_CTL_ELEM_IFACE_MIXER,
	.access	= SNDRV_CTL_ELEM_ACCESS_READWRITE
		| SNDRV_CTL_ELEM_ACCESS_TLV_READ
		| SNDRV_CTL_ELEM_ACCESS_TLV_CALLBACK,
	.name       = "Master Playback Volume",
	.index      = 0,
	.tlv = { .c =  snd_codec_vol_tlv},
	.info       = snd_codec_info_volume,
	.get        = snd_codec_get_volume,
	.put        = snd_codec_put_volume,
};

static int snd_codec_probe(struct platform_device *pdev)
{
	struct audio_dev *audio = pdev_to_audio(pdev);
	struct snd_kcontrol *kctrl;
	struct snd_card *card;
	struct snd_pcm *pcm;
	int err;

	err = snd_card_create(-1, "internalcodec", THIS_MODULE, 0, &card);
	if (err < 0) {
		printk("internal codec snd card create failed(%d)\n", err);
		return err;
	}
	audio->card = card;
	strcpy(audio->card->mixername, "Headset Mixer");
	kctrl = snd_ctl_new1(&playback_controls, audio);
	if (!kctrl)
		return -ENOMEM;
	err = snd_ctl_add(card, kctrl);
	if (err < 0)
		return err;

	err = snd_pcm_new(audio->card, "internal codec PCM", 0, 1, 0, &pcm);
	if (err < 0) {
		printk("UAC2 snd pcm new failed(%d)\n", err);
		goto snd_fail;
	}
	strcpy(pcm->name, "internal codec pcm");
	pcm->private_data = audio;
	audio->pcm = pcm;
	spin_lock_init(&audio->p_prm.lock);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &codec_pcm_ops);
	strcpy(card->driver, "internal codec pcm");
	strcpy(card->shortname, "internal codec pcm");
	sprintf(card->longname, "internal codec pcm %i", pdev->id);
	snd_card_set_dev(card, &pdev->dev);
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
			&pdev->dev, JZ_DMA_BUFFERSIZE, JZ_DMA_BUFFERSIZE);
	err = snd_card_register(card);
	if (!err) {
		platform_set_drvdata(pdev, card);
		printk("%s probe success\n", __func__);
		return 0;
	}

snd_fail:
	snd_card_free(card);
	audio->pcm = NULL;
	audio->card = NULL;
	return err;
}

static int snd_codec_remove(struct platform_device *pdev)
{
	struct snd_card *card = platform_get_drvdata(pdev);

	if (card)
		return snd_card_free(card);
	return 0;
}

static void snd_codec_release(struct device *dev)
{
	dev_dbg(dev, "releasing '%s'\n", dev_name(dev));
}

int snd_codec_init(struct dma_param *dma)
{
	int err;
	struct audio_dev *audio;

	if (g_audio)
		return 0;

	audio = (struct audio_dev *)
		kzalloc(sizeof(struct audio_dev), GFP_KERNEL);

	audio->pdrv.probe = snd_codec_probe;
	audio->pdrv.remove = snd_codec_remove;
	audio->pdrv.driver.name = codec_name;

	audio->pdev.id = 0;
	audio->pdev.name = codec_name;
	audio->pdev.dev.release = snd_codec_release;

	audio->p_prm.dma = dma;
	snd_hrtimer_init(&audio->p_prm);

	err = codec_dac_init(dma->iomem, &audio->codec_params);
	if (err)
		return err;

	/* Register snd_audio driver */
	err = platform_driver_register(&audio->pdrv);
	if (err)
		return err;

	/* Register snd_audio device */
	err = platform_device_register(&audio->pdev);
	if (err)
		platform_driver_unregister(&audio->pdrv);
	g_audio = audio;
	return err;
}

void snd_codec_exit(void)
{
	if (g_audio) {
		platform_driver_unregister(&g_audio->pdrv);
		platform_device_unregister(&g_audio->pdev);
		g_audio = NULL;
	}
}
