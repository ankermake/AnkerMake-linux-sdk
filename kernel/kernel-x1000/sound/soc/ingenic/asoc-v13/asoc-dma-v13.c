/*
 *  sound/soc/ingenic/asoc-dma.c
 *  ALSA Soc Audio Layer -- ingenic audio dma platform driver
 *
 *  Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *	cscheng <shicheng.cheng@ingenic.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/hrtimer.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/soc-dai.h>
#include <sound/pcm_params.h>
#include <mach/jzdma.h>
#include <linux/delay.h>
#include "asoc-dma-v13.h"
#include <mach/jzsnd.h>


#include "asoc-utils.c"
static int asoc_dma_debug = 0;
module_param(asoc_dma_debug, int, 0644);

#define DMA_DEBUG_MSG(msg...)			\
	do {					\
		if (asoc_dma_debug)		\
			printk(KERN_DEBUG"ADMA: " msg);	\
	} while(0)
#define DMA_SUBSTREAM_MSG(substream, msg...)	\
	do {					\
		if (asoc_dma_debug) {		\
			printk(KERN_DEBUG"ADMA[%s][%s]:", \
					substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? \
					"replay" : "record",	\
					substream->pcm->id);	\
			printk(KERN_DEBUG msg);		\
		} \
	} while(0)


struct jz_dma_pcm {
	struct dma_chan *chan[2];
	enum jzdma_type dma_type;
	unsigned int capture_soft_mute:1;
	unsigned int to_user:1;
	unsigned int playback_oncache:1;
};

/*
 * fake using a continuous buffer
 */
static inline void *
snd_pcm_get_ptr(struct snd_pcm_substream *substream, unsigned int ofs)
{
	return substream->runtime->dma_area + ofs;
}

static int jz_pcm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct jz_pcm_dma_params *dma_params = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);
	struct dma_slave_config slave_config;
	int ret;
	DMA_SUBSTREAM_MSG(substream, "%s enter\n", __func__);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		slave_config.direction = DMA_TO_DEVICE;
		slave_config.dst_addr = dma_params->dma_addr;
	} else {
		slave_config.direction = DMA_FROM_DEVICE;
		slave_config.src_addr = dma_params->dma_addr;
	}
	slave_config.dst_addr_width = dma_params->buswidth;
	slave_config.dst_maxburst = dma_params->max_burst;
	slave_config.src_addr_width = dma_params->buswidth;	/*jz dmaengine needed*/
	slave_config.src_maxburst = dma_params->max_burst;
	ret = dmaengine_slave_config(prtd->dma_chan, &slave_config);
	if (ret)
		return ret;

	prtd->delayed_jiffies =  2 * (params_period_size(params) * HZ /params_rate(params));
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
}

static void dma_stop_watchdog(struct work_struct *work)
{
	struct jz_pcm_runtime_data *prtd =
		container_of(work, struct jz_pcm_runtime_data, dwork_stop_dma.work);
	struct snd_pcm_substream *substream = prtd->substream;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	if (!atomic_dec_if_positive(&prtd->stopped_pending)) {
		DMA_SUBSTREAM_MSG(substream,"stop real\n");
		dmaengine_terminate_all(prtd->dma_chan);
		if (cpu_dai->driver->ops->trigger)
			cpu_dai->driver->ops->trigger(substream, prtd->stopped_cmd, cpu_dai);
	}
}

static size_t
snd_pcm_get_pos_algin_period(struct snd_pcm_substream *substream, dma_addr_t addr)
{
	return (addr - substream->runtime->dma_addr -
			(addr - substream->runtime->dma_addr)%
			snd_pcm_lib_period_bytes(substream));
}

static size_t
snd_pcm_get_pos(struct snd_pcm_substream *substream, dma_addr_t addr)
{
	return (addr - substream->runtime->dma_addr);
}

static void jz_asoc_handle_cb_data(struct snd_pcm_substream *substream, bool is_playback)
{
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = (void *)snd_pcm_substream_chip(substream);
	struct jz_dma_pcm  *jz_pcm = snd_soc_platform_get_drvdata(rtd->platform);
	struct dma_chan *dma_chan = prtd->dma_chan;
	size_t buffer_bytes = snd_pcm_lib_buffer_bytes(substream);
	size_t curr_pos = 0;
	void *first_addr = NULL, *second_addr = NULL;
	size_t first_size = 0, second_size = 0;
	dma_addr_t pdma_addr = 0;

	pdma_addr = dma_chan->device->get_current_trans_addr(dma_chan,
			NULL, NULL,
			is_playback ? DMA_MEM_TO_DEV : DMA_DEV_TO_MEM);

	if (is_playback)
		curr_pos = snd_pcm_get_pos_algin_period(substream, pdma_addr) % buffer_bytes;
	else
		curr_pos = snd_pcm_get_pos(substream, pdma_addr) % buffer_bytes;

	if (curr_pos == prtd->pos) {
		prtd->pos = curr_pos;
		return;
	}

	if (prtd->pos < curr_pos) {
		first_addr = snd_pcm_get_ptr(substream, prtd->pos);
		first_size = curr_pos - prtd->pos;
	} else {
		first_addr = snd_pcm_get_ptr(substream, prtd->pos);
		first_size = buffer_bytes - prtd->pos;
		if (curr_pos) {
			second_addr = snd_pcm_get_ptr(substream, 0);
			second_size = curr_pos;
		}
	}

	if (is_playback) {
		if (jz_pcm->playback_oncache) {
			size_t period_bytes = snd_pcm_lib_period_bytes(substream);
			size_t dma1pos = 0, dma2pos = 0;
			dma1pos = curr_pos;
			dma2pos = (dma1pos + period_bytes) % buffer_bytes;
			dma_cache_sync(NULL, snd_pcm_get_ptr(substream, dma1pos), period_bytes, DMA_TO_DEVICE);
			dma_cache_sync(NULL, snd_pcm_get_ptr(substream, dma2pos), period_bytes, DMA_TO_DEVICE);
		}
		if (first_size && jz_pcm->to_user)
			send_data_back_to_user(first_addr, first_size);
		if (second_size && jz_pcm->to_user)
			send_data_back_to_user(second_addr, second_size);
		if (first_size && IS_BUILTIN(CONFIG_JZ_ASOC_DMA_AUTO_CLR_DRT_MEM))
			memset(first_addr, 0, first_size);
		if (second_size && IS_BUILTIN(CONFIG_JZ_ASOC_DMA_AUTO_CLR_DRT_MEM))
			memset(second_addr, 0, second_size);
	} else {
		if (first_size && jz_pcm->capture_soft_mute)
			memset(first_addr, 0, first_size);
		if (second_size && jz_pcm->capture_soft_mute)
			memset(second_addr, 0, second_size);
		if (first_size && !jz_pcm->capture_soft_mute)
			dma_cache_sync(NULL, first_addr, first_size, DMA_FROM_DEVICE);
		if (second_size && !jz_pcm->capture_soft_mute)
			dma_cache_sync(NULL, second_addr, second_size, DMA_FROM_DEVICE);
	}

	prtd->pos = curr_pos;

	//printk(KERN_DEBUG"curr_pos = %d buffer_bytes = %d\n", curr_pos, buffer_bytes);
	return;
}

static void jz_asoc_dma_callback(void *data)
{
	struct snd_pcm_substream *substream = data;
	struct jz_pcm_runtime_data *prtd;

	if (PCM_RUNTIME_CHECK(substream))
		return;

	prtd = substream->runtime->private_data;
	if (!atomic_dec_if_positive(&prtd->stopped_pending)) {
		struct snd_soc_pcm_runtime *rtd =  (void *)snd_pcm_substream_chip(substream);
		struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
		DMA_SUBSTREAM_MSG(substream,"stop real\n");
		cancel_delayed_work(&prtd->dwork_stop_dma);
		dmaengine_terminate_all(prtd->dma_chan);
		if (cpu_dai->driver->ops->trigger)
			cpu_dai->driver->ops->trigger(substream, prtd->stopped_cmd, cpu_dai);
		return;
	}

	jz_asoc_handle_cb_data(substream, substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
	snd_pcm_period_elapsed(substream);
}

static int jz_asoc_dma_prepare_and_submit(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
	struct jz_dma_pcm *jz_pcm = snd_soc_platform_get_drvdata(rtd->platform);
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct dma_async_tx_descriptor *desc;
	unsigned long flags = DMA_CTRL_ACK;
	enum dma_data_direction direction =
		(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ?
		DMA_MEM_TO_DEV:
		DMA_DEV_TO_MEM;

	prtd->pos = 0;
	desc = prtd->dma_chan->device->device_prep_dma_cyclic(prtd->dma_chan,
			substream->runtime->dma_addr,
			snd_pcm_lib_buffer_bytes(substream),
			snd_pcm_lib_period_bytes(substream),
			direction,
			flags,
			NULL);
	if (!desc) {
		dev_err(rtd->dev, "cannot prepare slave dma\n");
		return -EINVAL;
	}

	desc->callback = jz_asoc_dma_callback;
	desc->callback_param = substream;

	prtd->cookie = dmaengine_submit(desc);

	if (!jz_pcm->playback_oncache)
		return 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_cache_sync(NULL, snd_pcm_get_ptr(substream, 0),
				2 * snd_pcm_lib_period_bytes(substream), DMA_TO_DEVICE);
	return 0;
}

static int jz_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
	struct jz_dma_pcm  *jz_pcm = snd_soc_platform_get_drvdata(rtd->platform);
	int timeout = 100;

	if (atomic_read(&prtd->stopped_pending))
		printk(KERN_DEBUG"prepare wait dma stopping\n");
	while(atomic_read(&prtd->stopped_pending) && (--timeout))
		msleep(1);
	printk(KERN_DEBUG"prepare wait dma stopping ok\n");

	if (!jz_pcm->playback_oncache)
		return 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		substream->runtime->start_threshold =
			substream->runtime->start_threshold <= (2 * substream->runtime->period_size) ?
			(2 * substream->runtime->period_size) :
			substream->runtime->start_threshold;
		substream->runtime->stop_threshold =
			substream->runtime->stop_threshold <= substream->runtime->period_size ?
			substream->runtime->period_size : substream->runtime->stop_threshold;
	}
	return 0;
}

static int jz_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;

#ifdef CONFIG_JZ_ASOC_DMA_AUTO_CLR_DRT_MEM
	size_t buffer_bytes = snd_pcm_lib_buffer_bytes(substream);
#endif
	int ret;

	DMA_SUBSTREAM_MSG(substream,"%s enter cmd %d\n", __func__, cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (atomic_read(&prtd->stopped_pending))
			return -EPIPE;
		DMA_SUBSTREAM_MSG(substream,"start trigger\n");
		ret = jz_asoc_dma_prepare_and_submit(substream);
		if (ret)
			return ret;
		dma_async_issue_pending(prtd->dma_chan);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		DMA_SUBSTREAM_MSG(substream,"stop trigger\n");
		/* To make sure there is not data transfer on AHB bus,
		 * then we can stop the dma, Wait dma callback happen
		 */
		if (dmaengine_terminate_all(prtd->dma_chan)) {
			prtd->stopped_cmd = cmd;
			atomic_set(&prtd->stopped_pending, 1);
			schedule_delayed_work(&prtd->dwork_stop_dma, prtd->delayed_jiffies);
		}
#ifdef CONFIG_JZ_ASOC_DMA_AUTO_CLR_DRT_MEM
		printk(KERN_DEBUG"show the time memset1 %d\n", buffer_bytes);
		memset(snd_pcm_get_ptr(substream, 0), 0, buffer_bytes);
		printk(KERN_DEBUG"show the time memset2 %d\n", buffer_bytes);
#endif
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static snd_pcm_uframes_t jz_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;

	DMA_DEBUG_MSG("%s: %d %ld\n", __func__, prtd->pos,
			bytes_to_frames(substream->runtime, prtd->pos));
	return bytes_to_frames(substream->runtime, prtd->pos);
}


#define JZ_DMA_BUFFERSIZE (32*1024)
static const struct snd_pcm_hardware jz_pcm_hardware = {
	.info = SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_PAUSE |
		SNDRV_PCM_INFO_RESUME |
		SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.formats = SNDRV_PCM_FMTBIT_S24_LE |
		SNDRV_PCM_FMTBIT_S20_3LE |
		SNDRV_PCM_FMTBIT_S18_3LE |
		SNDRV_PCM_FMTBIT_S16_LE |
		SNDRV_PCM_FMTBIT_S8,
	.rates                  = SNDRV_PCM_RATE_8000_192000,
	.rate_min               = 8000,
	.rate_max               = 192000,
	.channels_min           = 1,
	.channels_max           = 2,
	.buffer_bytes_max       = JZ_DMA_BUFFERSIZE,
	.period_bytes_min       = PAGE_SIZE / 4, /* 1K */
	.period_bytes_max       = PAGE_SIZE * 2, /* 64K */
	.periods_min            = 4,
	.periods_max            = 64,
	.fifo_size              = 0,
};

static int jz_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
	struct jz_dma_pcm *jz_pcm = snd_soc_platform_get_drvdata(rtd->platform);
	struct dma_chan *chan = jz_pcm->chan[substream->stream];
	struct jz_pcm_runtime_data *prtd = NULL;
	int ret;
	DMA_DEBUG_MSG("%s enter\n", __func__);
	ret = snd_soc_set_runtime_hwparams(substream, &jz_pcm_hardware);
	if (ret)
		return ret;

	ret = snd_pcm_hw_constraint_integer(substream->runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		return ret;

	prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
	if (!prtd)
		return -ENOMEM;

	atomic_set(&prtd->stopped_pending, 0);
	INIT_DELAYED_WORK(&prtd->dwork_stop_dma, dma_stop_watchdog);
	prtd->dma_chan = chan;
	prtd->substream = substream;
	substream->runtime->private_data = prtd;

	return 0;
}

static int jz_pcm_close(struct snd_pcm_substream *substream)
{
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;

	DMA_DEBUG_MSG("%s enter\n", __func__);
	flush_delayed_work(&prtd->dwork_stop_dma);
	substream->runtime->private_data = NULL;
	kfree(prtd);
	return 0;
}

static int jz_pcm_mmap(struct snd_pcm_substream *substream,
		struct vm_area_struct *vma)
{

	if (substream->dma_buffer.dev.type == SNDRV_DMA_TYPE_DEV)
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	return remap_pfn_range(vma, vma->vm_start,
			substream->dma_buffer.addr >> PAGE_SHIFT,
			vma->vm_end - vma->vm_start, vma->vm_page_prot);
}

struct snd_pcm_ops jz_pcm_ops = {
	.open		= jz_pcm_open,
	.close		= jz_pcm_close,
	.prepare	= jz_pcm_prepare,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= jz_pcm_hw_params,
	.hw_free	= snd_pcm_lib_free_pages,
	.trigger	= jz_pcm_trigger,
	.pointer	= jz_pcm_pointer,
	.mmap		= jz_pcm_mmap,
};

static bool filter(struct dma_chan *chan, void *filter_param)
{
	struct jz_dma_pcm *jz_pcm = (struct jz_dma_pcm*)filter_param;
	return jz_pcm->dma_type == (int)chan->private;
}

static void jz_pcm_free(struct snd_pcm *pcm)
{
	struct snd_soc_pcm_runtime *rtd = snd_pcm_chip(pcm);
	struct jz_dma_pcm *jz_pcm = snd_soc_platform_get_drvdata(rtd->platform);
	struct snd_pcm_substream *substream;
	int i;

	DMA_DEBUG_MSG("%s enter\n", __func__);

	for (i = SNDRV_PCM_STREAM_PLAYBACK; i <= SNDRV_PCM_STREAM_CAPTURE; i++) {
		substream = pcm->streams[i].substream;
		if (!substream)
			continue;
		if (jz_pcm && jz_pcm->chan[i])
			dma_release_channel(jz_pcm->chan[i]);
	}
	snd_pcm_lib_preallocate_free_for_all(pcm);
	return;
}

static int jz_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_pcm *pcm = rtd->pcm;
	struct jz_dma_pcm *jz_pcm = snd_soc_platform_get_drvdata(rtd->platform);
	struct snd_pcm_substream *substream;
	dma_cap_mask_t mask;
	size_t buffer_size = JZ_DMA_BUFFERSIZE;
	size_t buffer_bytes_max = JZ_DMA_BUFFERSIZE;
	int ret = -EINVAL;
	int i;

	DMA_DEBUG_MSG("%s enter\n", __func__);

	for (i = 0; i < 2; i++) {
		substream = pcm->streams[i].substream;
		if (!substream)
			continue;
		if (!jz_pcm->chan[i]) {
			int type;
			dma_cap_zero(mask);
			dma_cap_set(DMA_SLAVE, mask);
			dma_cap_set(DMA_CYCLIC, mask);
			jz_pcm->chan[i] = dma_request_channel(mask, filter, jz_pcm);

			if (!jz_pcm->chan[i])
				goto out;

			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
				type = jz_pcm->playback_oncache ? SNDRV_DMA_TYPE_DEV2: SNDRV_DMA_TYPE_DEV;
			else
				type = SNDRV_DMA_TYPE_DEV2;

			ret = snd_pcm_lib_preallocate_pages(substream, type,
					jz_pcm->chan[i]->device->dev,
					buffer_size,
					buffer_bytes_max);
			if (ret)
				goto out;
			printk("sound dma %p %x", substream->dma_buffer.area, substream->dma_buffer.addr);
		}
	}
	return 0;
out:
	dev_err(rtd->dev, "Failed to alloc dma buffer %d\n", ret);
	jz_pcm_free(pcm);
	return ret;
}


static int dma_soft_mute_get(struct snd_kcontrol *kcontrol,
			    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct jz_dma_pcm  *jz_pcm = snd_soc_platform_get_drvdata(platform);
	ucontrol->value.integer.value[0] = jz_pcm->capture_soft_mute;
	return 0;
}


static int dma_soft_mute_put(struct snd_kcontrol *kcontrol,
			    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct jz_dma_pcm  *jz_pcm = snd_soc_platform_get_drvdata(platform);
	unsigned int value = ucontrol->value.integer.value[0];
	jz_pcm->capture_soft_mute = value;
	return 0;
}

static const struct snd_kcontrol_new  dma_soft_controls[] = {
	SOC_SINGLE_EXT(NULL, 0, 0, 1, 0,
		       dma_soft_mute_get, dma_soft_mute_put),
};

static int jz_pcm_platform_probe(struct platform_device *pdev)
{
	struct jz_dma_pcm *jz_pcm;
	struct resource *res;
	struct snd_soc_platform_driver *snd_soc_platform_driver;
	struct snd_dma_data *snd_dma_data = pdev->dev.platform_data;
	int ret;

	jz_pcm  = devm_kzalloc(&pdev->dev, sizeof(*jz_pcm), GFP_KERNEL);
	if (!jz_pcm)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (!res)
		return -ENOENT;
	jz_pcm->dma_type = GET_MAP_TYPE(res->start);
	platform_set_drvdata(pdev, jz_pcm);

	snd_soc_platform_driver = devm_kzalloc(&pdev->dev, sizeof(*snd_soc_platform_driver), GFP_KERNEL);
	if (!snd_soc_platform_driver)
		return -ENOMEM;
	snd_soc_platform_driver->ops = &jz_pcm_ops;
	snd_soc_platform_driver->pcm_new = jz_pcm_new;
        snd_soc_platform_driver->pcm_free = jz_pcm_free;

       if (snd_dma_data && snd_dma_data->dma_soft_mute == SUPPLY_DMA_SOFT_MUTE_AMIXER){
                struct snd_kcontrol_new *controls = NULL;
                char *controls_name = NULL;

                controls_name =devm_kzalloc(&pdev->dev, 20, GFP_KERNEL);
                if (!controls_name){
                        dev_err(&pdev->dev, "Failed to malloc kcontrols_name\n");
                        goto skip_kctl;
                }

                controls = devm_kzalloc(&pdev->dev, sizeof(struct snd_kcontrol_new), GFP_KERNEL);
                if(!controls){
                        dev_err(&pdev->dev, "Failed to malloc kcontrols\n");
                        goto skip_kctl;
                }
                memcpy(controls, dma_soft_controls, sizeof(struct snd_kcontrol_new));
                snprintf(controls_name, 20, "%s %s", dev_name(&pdev->dev)+ strlen("jz-asoc-"), "soft mute");
                controls->name = controls_name;

                snd_soc_platform_driver->num_controls = 1;
                snd_soc_platform_driver->controls = controls;
        }
	if (snd_dma_data->dma_write_oncache)
		jz_pcm->playback_oncache = 1;
skip_kctl:
	if (!strcmp(pdev->name, "jz-asoc-aic-dma"))
		jz_pcm->to_user = 1;

	ret = snd_soc_register_platform(&pdev->dev, snd_soc_platform_driver);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register platfrom\n");
		platform_set_drvdata(pdev, NULL);
		return ret;
	}

	jz_pcm->capture_soft_mute = 0;
	dev_dbg(&pdev->dev, "Audio dma platfrom probe success\n");
	return 0;
}

static int jz_pcm_platform_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "Audio dma platfrom removed\n");
	snd_soc_unregister_platform(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const  struct platform_device_id jz_dma_id_table[] = {
	{
		.name = "jz-asoc-aic-dma",	/*aic dma*/

	},
	{
		.name = "jz-asoc-pcm-dma",	/*pcmc dma*/
	},
	{
		.name = "jz-asoc-dmic-dma",	/*dmic dma*/
	},
	{},
};

struct platform_driver jz_pcm_platfrom_driver = {
	.probe  = jz_pcm_platform_probe,
	.remove = jz_pcm_platform_remove,
	.driver = {
		.name   = "jz-asoc-dma",
		.owner  = THIS_MODULE,
	},
	.id_table = jz_dma_id_table,
};

static int jz_pcm_init(void)
{
	send_data_back_to_user_init();
	return platform_driver_register(&jz_pcm_platfrom_driver);
}
module_init(jz_pcm_init);

static void jz_pcm_exit(void)
{
	platform_driver_unregister(&jz_pcm_platfrom_driver);
}
module_exit(jz_pcm_exit);

MODULE_DESCRIPTION("JZ ASOC Platform driver");
MODULE_AUTHOR("shicheng.cheng@ingenic.com");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:jz-asoc-dma");
