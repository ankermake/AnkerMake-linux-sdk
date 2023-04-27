
#ifndef __DMA_OPS_H__
#define __DMA_OPS_H__
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/hrtimer.h>
#include <linux/dmaengine.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/circ_buf.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/memory.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <mach/jzdma.h>
#include <common.h>

struct dma_param {
    struct dma_chan *dma_chan[2];
    struct resource *dma_res;
    struct resource *io_res;
    int dma_type;

    dma_addr_t dma_addr;
    void __iomem    *iomem;

    dma_addr_t tx_buf;
    int tx_buf_len;
    int tx_period_size;
    int tx_buswidth;
    int tx_maxburst;

    dma_addr_t rx_buf;
    int rx_buf_len;
    int rx_period_size;
    int rx_buswidth;
    int rx_maxburst;

    int enable_cnt;
};

int dma_param_submit_cyclic(struct dma_param *dma,
        enum dma_transfer_direction direction);

void dma_param_terminate(struct dma_param *dma, enum dma_transfer_direction direction);

struct dma_param *dma_param_init(
        struct platform_device *pdev, int res_no, int chan_cnt);

#endif
