/*
 *  sound/soc/ingenic/asoc-dma.h
 *  ALSA Soc Audio Layer -- ingenic dma platform driver
 *
 *  Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *	cscheng <shicheng.cheng@ingenic.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#ifndef __ASOC_DMA_H__
#define __ASOC_DMA_H__

#include <linux/device.h>
#include <linux/mm.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/hrtimer.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/soc-dai.h>
#include <sound/pcm_params.h>
#include <linux/dmaengine.h>
#include <linux/types.h>
#include <linux/fs.h>

struct jz_pcm_dma_params {
	dma_addr_t dma_addr;
	enum dma_slave_buswidth buswidth;
	int max_burst;
};

struct jz_pcm_runtime_data {
	struct snd_pcm_substream *substream;
	struct dma_chan *dma_chan;
	dma_cookie_t cookie;
	unsigned int pos;
	atomic_t stopped_pending;
	int stopped_cmd;
	struct delayed_work dwork_stop_dma;
	unsigned long delayed_jiffies;
};
#endif
