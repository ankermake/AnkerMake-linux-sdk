#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <mach/jzdma.h>

static dma_addr_t get_dma_addr(struct dma_chan *dma_chan, unsigned long start, unsigned int len)
{
    dma_addr_t dmic_dst;
    enum dma_data_direction direction = DMA_DEV_TO_MEM;

    int count = 1;

    do {
        dmic_dst = dma_chan->device->get_current_trans_addr(
            dma_chan, NULL, NULL, direction);
        if ( dmic_dst >= start && dmic_dst < start + len)
            return dmic_dst;
    } while (count--);

    pr_warning("DMIC: DMA address[0x%08x] illegal!\n", dmic_dst);

    return 0;
}

static int dma_param_config(struct dma_param *dma,
        enum dma_transfer_direction direction) {
    int ret;
    struct dma_slave_config slave_config;
    int buswidth;
    int maxburst;
    struct dma_chan *dma_chan;

    slave_config.src_addr = dma->dma_addr;
    buswidth = dma->rx_buswidth;
    maxburst = dma->rx_maxburst;
    dma_chan = dma->dma_chan;

    slave_config.direction = direction;
    slave_config.dst_addr_width = buswidth;
    slave_config.dst_maxburst = maxburst;
    slave_config.src_addr_width = buswidth;
    slave_config.src_maxburst = slave_config.dst_maxburst;

    ret = dmaengine_slave_config(dma_chan, &slave_config);
    if (ret) {
        printk(KERN_ERR "DMIC: Failed to config dma chan\n");
        return -1;
    }

    return 0;
}

static int dma_param_submit_cyclic(struct dma_param *dma,
        enum dma_transfer_direction direction) {
    struct dma_async_tx_descriptor *desc;
    unsigned long flags = DMA_CTRL_ACK;

    char *buf = NULL;
    int buf_len;
    int period_size;
    struct dma_chan *dma_chan;

    buf = dma->rx_buf;
    buf_len = dma->rx_buf_len;
    period_size = dma->rx_period_size;
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
        printk(KERN_ERR "DMIC: Failed to prepare dma desc\n");
        return -1;
    }

    desc->callback = NULL;
    desc->callback_param = dma;

    dmaengine_submit(desc);
    dma_async_issue_pending(dma_chan);

    return 0;
}

static bool dma_filter(struct dma_chan *chan, void *filter_param)
{
    struct dma_param *dma = (struct dma_param *)filter_param;

    return dma->dma_type == (int)chan->private;
}

static struct dma_param *dma_param_init(struct platform_device *pdev)
{
    struct dma_param *dma = kzalloc(sizeof(*dma), GFP_KERNEL);

    dma->dma_res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
    if (!dma->dma_res) {
        dev_err(&pdev->dev, "DMIC: Failed to get platform dma resource\n");
        return NULL;
    }

    dma->io_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!dma->io_res) {
        dev_err(&pdev->dev, "DMIC: Failed to get platform IO resource\n");
        return NULL;
    }

    struct resource *res = devm_request_mem_region(&pdev->dev,
            dma->io_res->start, resource_size(dma->io_res), pdev->name);
    if (!res) {
        dev_err(&pdev->dev, "DMIC: Failed to request mem region\n");
        return NULL;
    }

    dma->iomem = devm_ioremap_nocache(&pdev->dev,
            dma->io_res->start, resource_size(dma->io_res));
    if (!dma->iomem) {
        dev_err(&pdev->dev, "DMIC: Failed to ioremap mmio memory\n");
        return NULL;
    }

    dma->dma_type = GET_MAP_TYPE(dma->dma_res->start);

    dma_cap_mask_t mask;
    dma_cap_zero(mask);
    dma_cap_set(DMA_SLAVE, mask);
    dma_cap_set(DMA_CYCLIC, mask);

    dma->dma_chan = dma_request_channel(mask, dma_filter, dma);
    if (!dma->dma_chan) {
        panic("DMIC: dmic dma chan requested failed.\n");
    }

    dma->dma_addr = dma->io_res->start + DMIC_DR;
    dma->rx_buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
    dma->rx_maxburst = 128;

    return dma;
}

static int dmic_dma_submit_cyclic(struct dmic_data *dmic)
{
    struct dma_param *dma = dmic->dma;

    dma->rx_buf = (void *)virt_to_phys(dmic->buffer);
    dma->rx_buf_len = dmic->buffer_size;
    dma->rx_period_size = dmic->period_size;

    dma_param_submit_cyclic(dma, DMA_DEV_TO_MEM);

    return 0;
}

static void dmic_dma_terminate(struct dmic_data *dmic)
{
    dmaengine_terminate_all(dmic->dma->dma_chan);
}