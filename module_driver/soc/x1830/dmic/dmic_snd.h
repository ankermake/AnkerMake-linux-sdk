#ifndef __MD_DMIC_SND_H__
#define __MD_DMIC_SND_H__

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <soc/irq.h>

#include <linux/cdev.h>
#include <linux/hrtimer.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/memory.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <assert.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dpcm.h>

#include <utils/gpio.h>

struct dma_param {
    struct dma_chan *dma_chan;
    struct resource *dma_res;
    struct resource *io_res;
    int dma_type;

    dma_addr_t dma_addr;
    void __iomem    *iomem;

    char *rx_buf;
    int rx_buf_len;
    int rx_period_size;
    int rx_buswidth;
    int rx_maxburst;
};

struct dmic_data {
    struct device *dev;
    struct platform_device *pdev;

    struct snd_card *card;
    struct snd_pcm *pcm;

    int irq;
    void *buffer;
    unsigned int dma_pos;
    unsigned int read_pos;
    unsigned int data_size;
    unsigned int buffer_size;
    unsigned int period_time;
    unsigned int unit_size;
    unsigned int period_size;
    unsigned int frame_size;
    unsigned int channels;
    unsigned int rate;

    struct dma_param *dma;

    struct hrtimer hrtimer;
    unsigned int working_flag;
    wait_queue_head_t wait_queue;
    spinlock_t lock;
    unsigned int is_enable;

    struct snd_pcm_substream *substream;
};

#endif