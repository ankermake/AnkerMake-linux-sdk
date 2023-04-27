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

#include <sound/soc.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <common.h>
#include "audio_dev.h"

static const char *internal_codec_name = "snd_audio";

#define PERIOD_BYTES_MIN (1024)
#define PERIODS_MIN 4
#define PERIODS_MAX 120
#define JZ_DMA_BUFFERSIZE (PERIODS_MAX * PERIOD_BYTES_MIN)

static const struct snd_pcm_hardware internal_codec_pcm_hardware = {
    .info           = SNDRV_PCM_INFO_INTERLEAVED |
                    SNDRV_PCM_INFO_MMAP |
                    SNDRV_PCM_INFO_MMAP_VALID,
    .formats        = SNDRV_PCM_FMTBIT_S16_LE |
                    SNDRV_PCM_FMTBIT_U16_LE |
                    SNDRV_PCM_FMTBIT_U8 |
                    SNDRV_PCM_FMTBIT_S8,
    .channels_min       = 1,
    .channels_max       = 1,
    .buffer_bytes_max   = JZ_DMA_BUFFERSIZE,
    .period_bytes_min   = PERIOD_BYTES_MIN,
    .period_bytes_max   = JZ_DMA_BUFFERSIZE / PERIODS_MIN,
    .periods_min        = PERIODS_MIN,
    .periods_max        = PERIODS_MAX,
    .fifo_size      = 0,
};

static inline
struct audio_dev *pdev_to_audio(struct platform_device *p)
{
    return container_of(p, struct audio_dev, pdev);
}

static int
internal_codec_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
    struct audio_dev *audio = snd_pcm_substream_chip(substream);
    struct audio_rtd_params *prm;
    unsigned long flags;
    int err = 0;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        prm = &audio->p_prm;
    else
        prm = &audio->c_prm;

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
        snd_hrtimer_stop(prm);
        prm->ss = NULL;
        break;
    default:
        err = -EINVAL;
    }

    spin_unlock_irqrestore(&prm->lock, flags);

    return err;
}

static snd_pcm_uframes_t internal_codec_pcm_pointer(struct snd_pcm_substream *substream)
{
    struct audio_dev *audio = snd_pcm_substream_chip(substream);
    struct audio_rtd_params *prm;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        prm = &audio->p_prm;
    else
        prm = &audio->c_prm;

    return bytes_to_frames(substream->runtime, prm->offset);
}

static int internal_codec_pcm_hw_params(struct snd_pcm_substream *substream,
                   struct snd_pcm_hw_params *hw_params)
{
    struct audio_dev *audio = snd_pcm_substream_chip(substream);
    struct audio_rtd_params *prm;
    int err;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        prm = &audio->p_prm;
    else
        prm = &audio->c_prm;

    return snd_pcm_lib_malloc_pages(substream,
                params_buffer_bytes(hw_params));
}

static int internal_codec_pcm_hw_free(struct snd_pcm_substream *substream)
{
    struct audio_dev *audio = snd_pcm_substream_chip(substream);
    struct audio_rtd_params *prm;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        prm = &audio->p_prm;
    else
        prm = &audio->c_prm;

    prm->dma_area = NULL;
    prm->dma_bytes = 0;
    prm->period_size = 0;

    return snd_pcm_lib_free_pages(substream);
}

void codec_enable_hpout(void);
void codec_disable_hpout(void);

static int internal_codec_pcm_open(struct snd_pcm_substream *substream)
{
    struct audio_dev *audio = snd_pcm_substream_chip(substream);
    struct snd_pcm_runtime *runtime = substream->runtime;
    audio->p_residue = 0;

    snd_soc_set_runtime_hwparams(substream, &internal_codec_pcm_hardware);

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        runtime->hw.rate_min = 48000;
        runtime->hw.rate_max = 48000;
        runtime->hw.formats = SNDRV_PCM_FMTBIT_S16_LE;
        runtime->hw.channels_min = 1;
        runtime->hw.channels_max = runtime->hw.channels_min;
    } else {
        runtime->hw.rate_min = 16000;
        runtime->hw.rate_max = runtime->hw.rate_min;
        runtime->hw.formats = SNDRV_PCM_FMTBIT_S16_LE;
        runtime->hw.channels_min = 5;
        runtime->hw.channels_max = runtime->hw.channels_min;
    }

    snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        codec_enable_hpout();

    return 0;
}

extern void snd_hrtimer_wait_stop(struct audio_rtd_params *prm);

/* ALSA cries without these function pointers */
static int internal_codec_pcm_close(struct snd_pcm_substream *substream)
{
    struct audio_dev *audio = snd_pcm_substream_chip(substream);
    struct audio_rtd_params *prm;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        prm = &audio->p_prm;
    else
        prm = &audio->c_prm;

    snd_hrtimer_wait_stop(prm);

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        codec_disable_hpout();

    return 0;
}

static int internal_codec_pcm_null(struct snd_pcm_substream *substream)
{
    return 0;
}

static struct snd_pcm_ops internal_codec_pcm_ops = {
    .open = internal_codec_pcm_open,
    .close = internal_codec_pcm_close,
    .ioctl = snd_pcm_lib_ioctl,
    .hw_params = internal_codec_pcm_hw_params,
    .hw_free = internal_codec_pcm_hw_free,
    .trigger = internal_codec_pcm_trigger,
    .pointer = internal_codec_pcm_pointer,
    .prepare = internal_codec_pcm_null,
};

static int snd_internal_codec_probe(struct platform_device *pdev)
{
    struct audio_dev *audio = pdev_to_audio(pdev);
    struct snd_card *card;
    struct snd_pcm *pcm;
    int err;

    err = snd_card_create(-1, "internalcodec", THIS_MODULE, 0, &card);
    if (err < 0) {
        printk("internal codec snd card create failed(%d)\n", err);
        return err;
    }

    audio->card = card;

    snd_internal_codec_add_volume_control(audio);

    /*
     * Create first PCM device
     * Create a substream only for non-zero channel streams
     */
    err = snd_pcm_new(audio->card,
            "internal codec PCM", 0, 1, 1, &pcm);
    if (err < 0) {
            printk("UAC2 snd pcm new failed(%d)\n", err);
            goto snd_fail;
    }

    strcpy(pcm->name, "internal codec pcm");
    pcm->private_data = audio;

    audio->pcm = pcm;

    spin_lock_init(&audio->p_prm.lock);
    spin_lock_init(&audio->c_prm.lock);
    snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &internal_codec_pcm_ops);
    snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &internal_codec_pcm_ops);

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

static int snd_internal_codec_remove(struct platform_device *pdev)
{
    struct snd_card *card = platform_get_drvdata(pdev);

    if (card)
        return snd_card_free(card);

    return 0;
}

static void snd_internal_codec_release(struct device *dev)
{
        dev_dbg(dev, "releasing '%s'\n", dev_name(dev));
}

int snd_internal_codec_init(struct dma_param *playback_dma, struct dma_param *capture_dma)
{
    int err;
    struct audio_dev *audio = (struct audio_dev *)
            kmalloc(sizeof(struct audio_dev), GFP_KERNEL);

    memset(audio, 0, sizeof(struct audio_dev));

    audio->pdrv.probe = snd_internal_codec_probe;
    audio->pdrv.remove = snd_internal_codec_remove;
    audio->pdrv.driver.name = internal_codec_name;

    audio->pdev.id = 0;
    audio->pdev.name = internal_codec_name;
    audio->pdev.dev.release = snd_internal_codec_release;

    audio->p_prm.dma = playback_dma;
    //audio->c_prm.dma = capture_dma;
    snd_hrtimer_init(&audio->p_prm);
    //snd_hrtimer_init(&audio->c_prm);

    /* Register snd_audio driver */
    err = platform_driver_register(&audio->pdrv);
    if (err)
        return err;

    /* Register snd_audio device */
    err = platform_device_register(&audio->pdev);
    if (err)
        platform_driver_unregister(&audio->pdrv);

    return err;
}

static void snd_internal_codec_exit(struct audio_dev *audio)
{
    platform_driver_unregister(&audio->pdrv);
    platform_device_unregister(&audio->pdev);
}













