#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <mach/jzdma.h>

static dma_addr_t get_dma_addr(struct dma_chan *dma_chan, unsigned long start, unsigned int len, enum dma_transfer_direction direction)
{
    dma_addr_t aic_dst;

    int count = 1;

    do {
        aic_dst = dma_chan->device->get_current_trans_addr(
            dma_chan, NULL, NULL, direction);
        if ( aic_dst >= start && aic_dst < start + len)
            return aic_dst;
    } while (count--);

    pr_warning("AIC: DMA address[0x%08x] illegal!\n", aic_dst);

    return 0;
}

int dma_param_config(struct dma_param *dma,
        enum dma_transfer_direction direction) {
    int ret;
    struct dma_slave_config slave_config;
    int buswidth;
    int maxburst;
    struct dma_chan *dma_chan;

    buswidth = dma->buswidth;
    maxburst = dma->maxburst;
    dma_chan = dma->dma_chan;

    if (direction == DMA_DEV_TO_MEM) {
        slave_config.src_addr = dma->dma_addr;
    } else {
        slave_config.dst_addr = dma->dma_addr;
    }

    slave_config.direction = direction;
    slave_config.dst_addr_width = buswidth;
    slave_config.dst_maxburst = maxburst;
    slave_config.src_addr_width = buswidth;
    slave_config.src_maxburst = maxburst;

    ret = dmaengine_slave_config(dma_chan, &slave_config);
    if (ret) {
        printk(KERN_ERR "AIC: Failed to config dma chan\n");
        return -1;
    }

    return 0;
}

int dma_param_submit_cyclic(struct dma_param *dma,
        enum dma_transfer_direction direction) {
    struct dma_async_tx_descriptor *desc;
    unsigned long flags = DMA_CTRL_ACK;

    char *buf = NULL;
    int buf_len;
    int period_size;
    struct dma_chan *dma_chan;

    buf = dma->buf;
    buf_len = dma->buf_len;
    period_size = dma->period_size;
    dma_chan = dma->dma_chan;

    int ret = dma_param_config(dma, direction);
    if (ret < 0)
        return ret;

    desc = dma_chan->device->device_prep_dma_cyclic(dma_chan,
            (unsigned long)buf,
            buf_len,
            period_size,
            direction,
            flags,
            NULL);
    if (!desc) {
        printk(KERN_ERR "AIC: Failed to prepare dma desc\n");
        return -1;
    }

    desc->callback = NULL;
    desc->callback_param = dma;

    dmaengine_submit(desc);
    dma_async_issue_pending(dma_chan);

    return 0;
}

bool dma_filter(struct dma_chan *chan, void *filter_param)
{
    struct dma_param *dma = (struct dma_param *)filter_param;

    return dma->dma_type == (int)chan->private;
}

void dma_param_init(struct platform_device *pdev, struct dma_param *dma)
{
    struct resource *dma_res;
    struct resource *io_res;

    dma_res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
    if (!dma_res)
        panic("AIC: Failed to get platform dma resource\n");

    io_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!io_res)
        panic("AIC: Failed to get platform IO resource\n");

    dma->dma_type = GET_MAP_TYPE(dma_res->start);

    dma->dma_addr = io_res->start + AICDR;
}

int aic_dma_submit_cyclic(struct aic_data *aic, enum dma_transfer_direction direction)
{
    struct dma_param *dma = &aic->dma;

    dma->buf = (void *)virt_to_phys(aic->dma_buffer);
    dma->buf_len = aic->dma_buffer_size;
    dma->period_size = aic->period_size;

    dma_param_submit_cyclic(dma, direction);

    return 0;
}

void aic_dma_terminate(struct aic_data *aic, enum dma_transfer_direction direction)
{
    dmaengine_terminate_all(aic->dma.dma_chan);
}