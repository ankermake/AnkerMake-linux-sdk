/*
 *  sound/soc/ingenic/asoc-dma.c
 *  ALSA Soc Audio Layer -- ingenic audio dma platform driver
 *
 *  Copyright 2018 Ingenic Semiconductor Co.,Ltd
 *	cli <chen.li@ingenic.com>
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
#include <linux/platform_data/asoc-ingenic.h>

#include "asoc-dma-encrypt.h"
#define DEBUGFILE
#include "asoc-dma-hrtimer.h"

static int asoc_dma_debug = 0;
module_param(asoc_dma_debug, int, 0644);

#define DMA_DEBUG_MSG(msg...)			\
	do {					\
		if (asoc_dma_debug)		\
			printk("ADMA-HRTIMER: " msg);	\
	} while(0)
#define DMA_SUBSTREAM_MSG(substream, msg...)	\
	do {					\
		if (asoc_dma_debug) {		\
			printk("ADMA[%s][%s]:", \
					substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? \
					"replay" : "record",	\
					substream->pcm->id);	\
			printk(msg);		\
		} \
	} while(0)

struct jz_dma_pcm {
	struct dma_chan *chan[2];
	enum jzdma_type dma_type;
	void *data_encrypt_handle;
	bool playback_oncache;
};


static int jz_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
	struct jz_dma_pcm *jz_pcm = snd_soc_platform_get_drvdata(rtd->platform);

	DMA_SUBSTREAM_MSG(substream, "prepare enter\n");
	hrtimer_cancel(&prtd->hr_timer);
	DMA_SUBSTREAM_MSG(substream, "prepare ok\n");

	if (jz_pcm->playback_oncache && substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
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
	struct jz_dma_pcm *jz_pcm = snd_soc_platform_get_drvdata(rtd->platform);
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct jz_pcm_dma_params *dma_params = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);
	struct dma_slave_config slave_config;
	unsigned long long time_ns;
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

	time_ns = 1000LL * 1000 * 1000 * params_period_size(params);
	do_div(time_ns, params_rate(params));
	prtd->expires = ns_to_ktime(time_ns);

	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (ret < 0)
		return ret;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		data_encrypt_init(jz_pcm->data_encrypt_handle,
				substream->runtime->dma_area,
				params);
	DMA_SUBSTREAM_MSG(substream, "%s leave \n\
			buf(%p, %dbytes) period(%d frames, %lldns)\n\
			rate %d channel %d, format %d\n", __func__,
			substream->runtime->dma_area,
			params_buffer_bytes(params),
			params_period_size(params),
			time_ns,
			params_rate(params),
			params_channels(params),
			snd_pcm_format_physical_width(params_format(params))/8);
	return 0;
}

static size_t
snd_pcm_get_pos_algin_period(struct snd_pcm_substream *substream, dma_addr_t addr)
{
	return (addr - substream->runtime->dma_addr -
			(addr - substream->runtime->dma_addr)%
			snd_pcm_lib_period_bytes(substream));
}

#ifdef DEBUGFILE
static void debug_file_fill_dbg_buf(void *buffer, int size);
static char plt_name[];
static struct file *debug_file;
#endif

static enum hrtimer_restart jz_asoc_hrtimer_callback(struct hrtimer *hr_timer) {
	struct jz_pcm_runtime_data *prtd = container_of(hr_timer,
			struct jz_pcm_runtime_data, hr_timer);
	struct snd_pcm_substream *substream = prtd->substream;
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
	struct jz_dma_pcm *jz_pcm = snd_soc_platform_get_drvdata(rtd->platform);
	struct dma_chan *dma_chan = prtd->dma_chan;
	dma_addr_t pdma_addr = 0;
	size_t buffer_bytes = snd_pcm_lib_buffer_bytes(substream);
	size_t curr_pos = 0;
	void *first_addr = NULL, *second_addr = NULL;
	size_t first_size = 0, second_size = 0;
	bool is_playback = substream->stream == SNDRV_PCM_STREAM_PLAYBACK;

	if (atomic_read(&prtd->stopped_pending) == 0)
		hrtimer_start(&prtd->hr_timer, prtd->expires, HRTIMER_MODE_REL);
	else
		return HRTIMER_NORESTART;

	pdma_addr = dma_chan->device->get_current_trans_addr(dma_chan,
			NULL,
			NULL,
			is_playback ? DMA_MEM_TO_DEV : DMA_DEV_TO_MEM);

	if (pdma_addr < substream->runtime->dma_addr ||
			pdma_addr > (substream->runtime->dma_addr + snd_pcm_lib_buffer_bytes(substream))) {
		pr_info("%s %s stream dma out limit pdma_addr %x\n", __func__,
				substream->pcm->id, pdma_addr);
		return HRTIMER_NORESTART;
	}

	curr_pos = snd_pcm_get_pos_algin_period(substream, pdma_addr) % buffer_bytes;
	if (curr_pos == prtd->pos) {
		prtd->pos = curr_pos;
		goto out;
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
		bool debug_file_need_write = !strncmp(plt_name, rtd->platform->name, 15);
		if (jz_pcm->playback_oncache) {
			size_t period_bytes = snd_pcm_lib_period_bytes(substream);
			size_t dma1pos = 0, dma2pos = 0;
			dma1pos = curr_pos;
			dma2pos = (dma1pos + period_bytes) % buffer_bytes;
			dma_cache_sync(NULL, snd_pcm_get_ptr(substream, dma1pos), period_bytes, DMA_TO_DEVICE);
			dma_cache_sync(NULL, snd_pcm_get_ptr(substream, dma2pos), period_bytes, DMA_TO_DEVICE);
		}
#ifdef DEBUGFILE
		if (first_size && debug_file_need_write)
			debug_file_fill_dbg_buf(first_addr, first_size);
		if (second_size && debug_file_need_write)
			debug_file_fill_dbg_buf(second_addr, second_size);
#endif
		if (first_size && IS_BUILTIN(CONFIG_JZ_ASOC_DMA_AUTO_CLR_DRT_MEM))
			memset(first_addr, 0, first_size);
		if (second_size && IS_BUILTIN(CONFIG_JZ_ASOC_DMA_AUTO_CLR_DRT_MEM))
			memset(second_addr, 0, second_size);
	} else {
		if (first_size) {
			dma_cache_sync(NULL, first_addr, first_size, DMA_FROM_DEVICE);\
			data_encrypt(jz_pcm->data_encrypt_handle, first_addr, first_size);
		}
		if (second_size) {
			dma_cache_sync(NULL, second_addr, second_size, DMA_FROM_DEVICE);
			data_encrypt(jz_pcm->data_encrypt_handle, second_addr, second_size);
		}
	}
	prtd->pos = curr_pos;
out:
	snd_pcm_period_elapsed(substream);
	return HRTIMER_NORESTART;
}

static int jz_asoc_dma_prepare_and_submit(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct jz_dma_pcm *jz_pcm = snd_soc_platform_get_drvdata(rtd->platform);
	struct dma_async_tx_descriptor *desc;
	unsigned long flags = DMA_CTRL_ACK;
	enum dma_transfer_direction direction =
		(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ?
		DMA_MEM_TO_DEV:
		DMA_DEV_TO_MEM;

	prtd->pos = 0;

	DMA_SUBSTREAM_MSG(substream, "dma prepare 0x%08x, %x, %x\n", substream->runtime->dma_addr,
			snd_pcm_lib_buffer_bytes(substream),
			snd_pcm_lib_period_bytes(substream));

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
	prtd->cookie = dmaengine_submit(desc);

	if (jz_pcm->playback_oncache &&
			substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_cache_sync(NULL, snd_pcm_get_ptr(substream, 0),
				2 * snd_pcm_lib_period_bytes(substream), DMA_TO_DEVICE);
	return 0;
}

static void trigger_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct jz_pcm_runtime_data *prtd = container_of(dwork, struct jz_pcm_runtime_data, trigger_work);
	spin_lock(&prtd->spinlock);
	if (!prtd->is_stop) {
		dma_async_issue_pending(prtd->dma_chan);
		atomic_set(&prtd->stopped_pending, 0);
		hrtimer_start(&prtd->hr_timer, prtd->expires , HRTIMER_MODE_REL);
		prtd->is_start = 1;
	}
	spin_unlock(&prtd->spinlock);
}

static int jz_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;

	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	size_t buffer_bytes = snd_pcm_lib_buffer_bytes(substream);
	int ret;

	DMA_SUBSTREAM_MSG(substream,"%s enter cmd %d\n", __func__, cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		DMA_SUBSTREAM_MSG(substream,"start trigger\n");
		ret = jz_asoc_dma_prepare_and_submit(substream);
		if (ret)
			return ret;
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			prtd->is_stop = 0;
			prtd->is_start = 0;
			schedule_delayed_work(&prtd->trigger_work, msecs_to_jiffies(100));
		} else {
			dma_async_issue_pending(prtd->dma_chan);
			atomic_set(&prtd->stopped_pending, 0);
			hrtimer_start(&prtd->hr_timer, prtd->expires , HRTIMER_MODE_REL);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		DMA_SUBSTREAM_MSG(substream,"stop trigger\n");
		spin_lock(&prtd->spinlock);
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			prtd->is_stop = 1;
			cancel_delayed_work(&prtd->trigger_work);
		}
		spin_unlock(&prtd->spinlock);
		/* To make sure there is not data transfer on AHB bus,
		 * then we can stop the dma, Wait tur or ror happen
		 */
		if (cpu_dai->driver->ops->trigger) {
			ret = cpu_dai->driver->ops->trigger(substream, cmd, cpu_dai);
			if (ret < 0)
				return ret;
		}
		if (prtd->is_start || substream->stream != SNDRV_PCM_STREAM_PLAYBACK) {
			ret = hrtimer_try_to_cancel(&prtd->hr_timer);
			if (ret < 0)
				atomic_set(&prtd->stopped_pending, 1);
			DMA_SUBSTREAM_MSG(substream,"stop trigger mid\n");
			dmaengine_terminate_all(prtd->dma_chan);
			memset(snd_pcm_get_ptr(substream, 0), 0, buffer_bytes);
			DMA_SUBSTREAM_MSG(substream,"stop trigger ok\n");
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static snd_pcm_uframes_t jz_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;

	/*DMA_DEBUG_MSG("%s: %d %ld\n", __func__, prtd->pos,
			bytes_to_frames(substream->runtime, prtd->pos));*/
	return bytes_to_frames(substream->runtime, prtd->pos);
}


#define PERIOD_BYTES_MIN (1024)
#define PERIODS_MIN	4
#define PERIODS_MAX	120
#define JZ_DMA_BUFFERSIZE (PERIODS_MAX * PERIOD_BYTES_MIN)
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
	.period_bytes_min       = PERIOD_BYTES_MIN,
	.period_bytes_max       = JZ_DMA_BUFFERSIZE / PERIODS_MIN,
	.periods_min            = PERIODS_MIN,
	.periods_max            = PERIODS_MAX,
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

	hrtimer_init(&prtd->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	prtd->hr_timer.function = jz_asoc_hrtimer_callback;
	prtd->dma_chan = chan;
	prtd->substream = substream;
	substream->runtime->private_data = prtd;
	spin_lock_init(&prtd->spinlock);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		INIT_DELAYED_WORK(&prtd->trigger_work, trigger_work_func);
	return 0;
}

static int jz_pcm_close(struct snd_pcm_substream *substream)
{
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;

	if (substream->stream != SNDRV_PCM_STREAM_PLAYBACK)
		cancel_delayed_work_sync(&prtd->trigger_work);

	DMA_DEBUG_MSG("%s enter\n", __func__);
	hrtimer_cancel(&prtd->hr_timer);
	substream->runtime->private_data = NULL;
	kfree(prtd);
	return 0;
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

			ret = snd_pcm_lib_preallocate_pages(substream,
					type,
					jz_pcm->chan[i]->device->dev,
					buffer_size,
					buffer_bytes_max);
			if (ret)
				goto out;
		}
	}
	return 0;
out:
	dev_err(rtd->dev, "Failed to alloc dma buffer %d\n", ret);
	jz_pcm_free(pcm);
	return ret;
}

static struct snd_soc_platform_driver jz_pcm_platform = {
	.ops            = &jz_pcm_ops,
	.pcm_new        = jz_pcm_new,
	.pcm_free       = jz_pcm_free,
};

static int jz_pcm_platform_probe(struct platform_device *pdev)
{
	struct jz_dma_pcm *jz_pcm;
	struct resource *res;
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

	jz_pcm->data_encrypt_handle = data_encrypt_acquire(&pdev->dev);

	if (snd_dma_data && snd_dma_data->dma_write_oncache)
		jz_pcm->playback_oncache = true;

	ret = snd_soc_register_platform(&pdev->dev, &jz_pcm_platform);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register platfrom\n");
		platform_set_drvdata(pdev, NULL);
		return ret;
	}

	dev_info(&pdev->dev, "Audio dma platfrom probe success\n");
	return 0;
}

static int jz_pcm_platform_remove(struct platform_device *pdev)
{
	struct jz_dma_pcm *jz_pcm = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "Audio dma platfrom removed\n");

	data_encrypt_release(jz_pcm->data_encrypt_handle);

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
	return platform_driver_register(&jz_pcm_platfrom_driver);
}
module_init(jz_pcm_init);

static void jz_pcm_exit(void)
{
	platform_driver_unregister(&jz_pcm_platfrom_driver);
}
module_exit(jz_pcm_exit);


#ifdef DEBUGFILE
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/moduleparam.h>
#define MAX_FILE_SIZE   (5 * 1024 * 1024)
#define MAX_PATH_LEN    (128)
static DEFINE_SPINLOCK(dbg_lock);
static LIST_HEAD(buf_filled);
static DECLARE_WAIT_QUEUE_HEAD(dbg_wq);
static struct file *debug_file = NULL;
static char file_name[MAX_PATH_LEN] = "/usr/data/replay.pcm";
static char plt_name[MAX_PATH_LEN] = "jz-asoc-aic-dma";
static struct task_struct *dbg_task;
static unsigned long max_file_size = MAX_FILE_SIZE;
static bool dbg_enabled = false;

struct debug_file_dbg_buf {
        struct list_head node;
        int size;
        void *buf;
};

static struct debug_file_dbg_buf *debug_file_alloc_dbgbuf(int size)
{
        struct debug_file_dbg_buf *tmp;
        if (size <= 0)
                return NULL;
        tmp = (struct debug_file_dbg_buf *)kmalloc(size, GFP_ATOMIC);
        if (tmp) {
                tmp->buf = tmp + 1;
                tmp->size = size;
                INIT_LIST_HEAD(&tmp->node);
        }
        return tmp;
}

static void debug_file_free_dbgbuf(struct debug_file_dbg_buf *dbgbuf)
{
        if (!dbgbuf) return;
        list_del(&dbgbuf->node);
        kfree(dbgbuf);
        return;
}

static void debug_file_fill_dbg_buf(void *buffer, int size)
{
        struct debug_file_dbg_buf *tmp;
        unsigned long flags;

        if (!debug_file || size <= 0)
                return;

        spin_lock_irqsave(&dbg_lock, flags);
        if (!debug_file) goto out;    /*kthread stop*/
        tmp = debug_file_alloc_dbgbuf(size);
        if (tmp) {
                memcpy(tmp->buf, buffer, tmp->size);
                list_add_tail(&tmp->node, &buf_filled);
                wake_up_interruptible(&dbg_wq);
        }
out:
        spin_unlock_irqrestore(&dbg_lock, flags);
        return;
}

static int debug_file_debug_threadfn(void *data)
{
        struct debug_file_dbg_buf *tmp, *n;
        unsigned long flags;
        struct file *filp;
        int error_write = 0;
        int ret;

        set_fs(KERNEL_DS);
        filp = filp_open(file_name, O_RDWR|O_CREAT|O_TRUNC, 0644);
        if (IS_ERR_OR_NULL(filp)) {
                pr_err("[%s] debug_file_debug_thread open %s failed(%ld)\n",
                                __FILE__, file_name, PTR_ERR(filp));
                return -ENOENT;
        }
        pr_info("[%s] debug_file_debug_thread start ok\n", __FILE__);
        debug_file = filp;

        while (!kthread_should_stop()) {
                if (error_write || debug_file->f_pos > max_file_size) {
                        filp_close(debug_file, NULL);
                        debug_file = filp_open(file_name, O_RDWR|O_TRUNC, 0644);
                        if (IS_ERR_OR_NULL(debug_file)) {
                                pr_err("[%s]fail open %s %ld\n", __FILE__, file_name, PTR_ERR(debug_file));
                                debug_file = NULL;
                                break;
                        }
                        debug_file->f_pos = 0;
                }

                wait_event_interruptible(dbg_wq, (!list_empty(&buf_filled) || kthread_should_stop()));
                if (kthread_should_stop())
                        break;

                spin_lock_irqsave(&dbg_lock, flags);
                tmp = list_first_entry(&buf_filled, struct debug_file_dbg_buf, node);
                list_del_init(&tmp->node);
                spin_unlock_irqrestore(&dbg_lock, flags);

                ret = vfs_write(debug_file, (char *)tmp->buf, tmp->size, &debug_file->f_pos);
                if (ret != tmp->size) {
                        pr_err("[%s]vfs write failed ret %d\n", __FILE__, ret);
                        error_write = 1;
                        continue;
                }
                pr_debug("[%s] vfs write %s ops %u\n", __FILE__, file_name, (unsigned int)debug_file->f_pos);
                spin_lock_irqsave(&dbg_lock, flags);
                debug_file_free_dbgbuf(tmp);
                spin_unlock_irqrestore(&dbg_lock, flags);
        }

        filp = debug_file;
        spin_lock_irqsave(&dbg_lock, flags);
        debug_file = NULL;
        list_for_each_entry_safe(tmp, n, &buf_filled, node)
                debug_file_free_dbgbuf(tmp);
        spin_unlock_irqrestore(&dbg_lock, flags);
        if (filp)
                filp_close(filp, NULL);
        dbg_enabled = false;
        return 0;
}

static int set_enabled(const char *val, const struct kernel_param *kp)
{
        int rv = param_set_bool(val, kp);

        if (rv)
                return rv;

        pr_info("[%s] debug %s\n", __FILE__, dbg_enabled ? "enabled" : "disabled");
        if (dbg_enabled) {
                dbg_task =  kthread_run(debug_file_debug_threadfn,
                                (void *)NULL, "emc debug");
                if (IS_ERR_OR_NULL(dbg_task)) {
                        dbg_enabled = false;
                        return PTR_ERR(dbg_task);
                }
        } else
                kthread_stop(dbg_task);
        return 0;
}

static struct kernel_param_ops enabled_param_ops = {
        .set = set_enabled,
        .get = param_get_bool,
};

module_param_string(dbg_plt, plt_name, MAX_PATH_LEN, 0644);
MODULE_PARM_DESC(dbg_plt, "asoc dma debug platform");
module_param_string(dbg_path, file_name, MAX_PATH_LEN, 0644);
MODULE_PARM_DESC(dbg_path, "asoc dma debug file path");
module_param_named(dbg_file_len, max_file_size, ulong, 0644);
MODULE_PARM_DESC(dbg_file_len, "limit asoc dma debug file size");
module_param_cb(dbg_enable, &enabled_param_ops, &dbg_enabled, 0644);
MODULE_PARM_DESC(dbg_enable, "enable asoc dma out debug");
#endif

MODULE_DESCRIPTION("JZ ASOC Platform driver");
MODULE_AUTHOR("cli<chen.li@ingenic.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:jz-asoc-dma");
