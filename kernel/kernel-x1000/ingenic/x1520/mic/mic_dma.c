#include "mic.h"

int mic_dma_submit_cyclic(struct mic_dev *mic_dev, struct mic *mic) {
    enum dma_data_direction direction = DMA_DEV_TO_MEM;
    struct dma_param *dma = mic->dma;

    dma->rx_buf = virt_to_phys(mic->buf[0]);
    dma->rx_buf_len = mic->buf_len * mic->buf_cnt;
    dma->rx_period_size = mic->buf_len;

    dma_param_submit_cyclic(mic->dma, DMA_DEV_TO_MEM);

    return 0;
}

void mic_dma_terminate(struct mic *mic) {
    dma_param_terminate(mic->dma->dma_chan, DMA_DEV_TO_MEM);
}
