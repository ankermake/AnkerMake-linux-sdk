#include <linux/err.h>

#include "snd.h"

static int dma_param_config(struct dma_param *dma,
		enum dma_transfer_direction direction) {
	int ret;
	struct dma_slave_config slave_config;
	int buswidth;
	int maxburst;
	struct dma_chan *dma_chan;


	if (direction == DMA_DEV_TO_MEM) {
		slave_config.src_addr = dma->dma_addr;
		buswidth = dma->rx_buswidth;
		maxburst = dma->rx_maxburst;
		dma_chan = dma->dma_chan[SNDRV_PCM_STREAM_CAPTURE];
	}
	else {
		slave_config.dst_addr = dma->dma_addr;
		buswidth = dma->tx_buswidth;
		maxburst = dma->tx_maxburst;
		dma_chan = dma->dma_chan[SNDRV_PCM_STREAM_PLAYBACK];
	}

	slave_config.direction = direction;
	slave_config.dst_addr_width = buswidth;
	slave_config.dst_maxburst = maxburst;
	slave_config.src_addr_width = buswidth;
	slave_config.src_maxburst = slave_config.dst_maxburst;
	ret = dmaengine_slave_config(dma_chan, &slave_config);
	if (ret) {
		printk("Failed to config dma chan\n");
		return -1;
	}

	return 0;
}

int dma_param_submit_cyclic(struct dma_param *dma,
		enum dma_transfer_direction direction)
{
	int ret;
	struct dma_async_tx_descriptor *desc;
	unsigned long flags = DMA_CTRL_ACK;
	dma_addr_t buf = 0;

	int buf_len;
	int period_size;
	struct dma_chan *dma_chan;

	if (direction == DMA_DEV_TO_MEM) {
		buf = dma->rx_buf;
		buf_len = dma->rx_buf_len;
		period_size = dma->rx_period_size;
		dma_chan = dma->dma_chan[SNDRV_PCM_STREAM_CAPTURE];
	}
	else {
		buf = dma->tx_buf;
		buf_len = dma->tx_buf_len;
		period_size = dma->tx_period_size;
		dma_chan = dma->dma_chan[SNDRV_PCM_STREAM_PLAYBACK];
	}

	ret = dma_param_config(dma, direction);
	if (ret < 0)
		return ret;

	desc = dma_chan->device->device_prep_dma_cyclic(dma_chan,
			buf,
			buf_len,
			period_size,
			direction,
			flags,
			NULL);
	if (!desc) {
		printk("failed to prepare dma desc\n");
		return -1;
	}

	desc->callback = NULL;
	desc->callback_param = dma;

	dmaengine_submit(desc);
	dma_async_issue_pending(dma_chan);
	return 0;
}

void dma_param_terminate(struct dma_param *dma, enum dma_transfer_direction direction)
{
	if (direction == DMA_DEV_TO_MEM) {
		dmaengine_terminate_all(dma->dma_chan[SNDRV_PCM_STREAM_CAPTURE]);
	}
	else {
		dmaengine_terminate_all(dma->dma_chan[SNDRV_PCM_STREAM_PLAYBACK]);
	}
}

static bool dma_filter(struct dma_chan *chan, void *filter_param)
{
	struct dma_param *dma = (struct dma_param *)filter_param;

	return dma->dma_type == (int)chan->private;
}

struct dma_param *dma_param_init(struct platform_device *pdev,
		int res_no, int chan_cnt)
{
	int i;
	dma_cap_mask_t mask;
	struct dma_param *dma =
		kmalloc(sizeof(struct dma_param), GFP_KERNEL);

	memset(dma, 0, sizeof(struct dma_param));

	dma->dma_res = platform_get_resource(pdev, IORESOURCE_DMA, res_no);
	if (!dma->dma_res) {
		dev_err(&pdev->dev, "failed to get platform dma resource, res_no: %d\n", res_no);
		return ERR_PTR(-1);
	}

	dma->io_res = platform_get_resource(pdev, IORESOURCE_MEM, res_no);
	if (!dma->io_res) {
		dev_err(&pdev->dev, "failed to get platform IO resource, res_no: %d\n", res_no);
		return ERR_PTR(-1);
	}

	if (NULL == devm_request_mem_region(&pdev->dev,
				dma->io_res->start, resource_size(dma->io_res),
				pdev->name)) {
		dev_err(&pdev->dev, "failed to request mem region, res_no: %d\n", res_no);
		return ERR_PTR(-EBUSY);
	}

	dma->iomem = devm_ioremap_nocache(&pdev->dev,
			dma->io_res->start, resource_size(dma->io_res));
	if (!dma->iomem) {
		dev_err(&pdev->dev, "Failed to ioremap mmio memory, res_no: %d\n", res_no);
		return ERR_PTR(-ENOMEM);
	}

	dma->dma_type = GET_MAP_TYPE(dma->dma_res->start);
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_cap_set(DMA_CYCLIC, mask);
	chan_cnt = chan_cnt > 2 ? 2 : chan_cnt;
	for (i = 0; i < chan_cnt; i++) {
		dma->dma_chan[i] = dma_request_channel(mask, dma_filter, dma);
		if (!dma->dma_chan[i]) {
			printk("Failed to request dma chan\n");
			return ERR_PTR(-ENOMEM);
		}
	}
	return dma;
}
