#include "mic.h"

static void dmic_hrtimer(struct mic_dev *mic_dev) {
    struct mic *dmic = &mic_dev->dmic;
    enum dma_data_direction direction = DMA_DEV_TO_MEM;
    dma_addr_t dmic_dst;
    struct dma_param *dma = dmic->dma;
    struct dma_chan *dma_chan = dma->dma_chan[SNDRV_PCM_STREAM_CAPTURE];

    if (!dmic->is_enabled) {
        dmic->offset += dmic->buf_len;
        dmic->offset %= dmic->buf_len * dmic->buf_cnt;
        return ;
    }

    dmic_dst = dma_chan->device->get_current_trans_addr(
            dma_chan, NULL, NULL, direction);

    int pdmic_addr_start = virt_to_phys(dmic->buf[0]);
    int dmic_buf_total_len = dmic->buf_len * dmic->buf_cnt;
    if(dmic_dst < pdmic_addr_start ||
        dmic_dst >= pdmic_addr_start + dmic_buf_total_len){
        pr_warning("dmic:dma address[0x%08x] illegal!\n",dmic_dst);
        return;
    }

    int dmic_offset = dmic_dst - pdmic_addr_start;
    int *dmic_buf = (int *)(dmic->buf[0] + dmic->offset);
    int dmic_flush_off = (dmic_offset + dmic_buf_total_len
        - dmic->offset) % dmic_buf_total_len;
    int nr = dmic_buf_total_len - dmic->offset;
    nr = nr > dmic_flush_off ? dmic_flush_off : nr;

    if (nr) {
        dma_cache_sync(NULL,
            (void *)dmic_buf, nr, DMA_DEV_TO_MEM);
    }

    if (nr < dmic_flush_off) {
        nr = dmic_flush_off - nr;
        dmic_buf = (int *)dmic->buf[0];

        dma_cache_sync(NULL,
            (void *)dmic_buf, nr, DMA_DEV_TO_MEM);
    }

    dmic->offset = dmic_offset % dmic_buf_total_len;
}

static void amic_hrtimer(struct mic_dev *mic_dev) {
    struct mic *amic = &mic_dev->amic;
    enum dma_data_direction direction = DMA_DEV_TO_MEM;
    dma_addr_t amic_dst;
    struct dma_param *dma = amic->dma;
    struct dma_chan *dma_chan = dma->dma_chan[SNDRV_PCM_STREAM_CAPTURE];

    if (!amic->is_enabled) {
        amic->offset += amic->buf_len;
        amic->offset %= amic->buf_len * amic->buf_cnt;
        return ;
    }

    amic_dst = dma_chan->device->get_current_trans_addr(
            dma_chan, NULL, NULL, direction);

    int amic_buf_total_len = amic->buf_len * amic->buf_cnt;
    int pamic_addr_start = virt_to_phys(amic->buf[0]);

    if(amic_dst < pamic_addr_start ||
        amic_dst >= pamic_addr_start + amic_buf_total_len){
        pr_warning("amic:dma address[0x%08x] illegal!\n",amic_dst);
        return;
    }

    int amic_offset = amic_dst - pamic_addr_start;
    int *amic_buf = (int *)(amic->buf[0] + amic->offset);
    int amic_flush_off = (amic_offset + amic_buf_total_len
        - amic->offset) % amic_buf_total_len;

    int nr = amic_buf_total_len - amic->offset;
    nr = nr > amic_flush_off ? amic_flush_off : nr;

    if (nr) {
        dma_cache_sync(NULL,
                (void *)amic_buf, nr, DMA_DEV_TO_MEM);
    }


    if (nr < amic_flush_off) {
        nr = amic_flush_off - nr;
        amic_buf = (int *)amic->buf[0];

        dma_cache_sync(NULL,
                (void *)amic_buf, nr, DMA_DEV_TO_MEM);
    }

    amic->offset = amic_offset % amic_buf_total_len;
}

static enum hrtimer_restart mic_hrtimer_callback(struct hrtimer *hrtimer)
{
    struct mic_dev *mic_dev = container_of(hrtimer, struct mic_dev, hrtimer);

    if (!mic_dev->hrtimer_uesd_count)
        return HRTIMER_NORESTART;

    hrtimer_forward(hrtimer, hrtimer->_softexpires,
            ns_to_ktime(mic_dev->periods_ms * NSEC_PER_MSEC));

    dmic_hrtimer(mic_dev);
    amic_hrtimer(mic_dev);

    /**
     * wake up mic read
     */
    mic_dev->cnt++;
    wake_up_all(&mic_dev->wait_queue);
//    mic_dma_submit_cyclic(mic_dev, &mic_dev->dmic);
//    mic_dma_submit_cyclic(mic_dev, &mic_dev->amic);

    return HRTIMER_RESTART;
}

void mic_hrtimer_start(struct mic_dev *mic_dev) {
    struct mic *dmic = &mic_dev->dmic;
    struct mic *amic = &mic_dev->amic;

    mic_dev->hrtimer_uesd_count++;
    if (mic_dev->hrtimer_uesd_count > 1)
        return ;

    hrtimer_forward_now(&mic_dev->hrtimer,
            ns_to_ktime(mic_dev->periods_ms * NSEC_PER_MSEC));
    hrtimer_start_expires(&mic_dev->hrtimer, HRTIMER_MODE_ABS);
}

void mic_hrtimer_stop(struct mic_dev *mic_dev) {

    mic_dev->hrtimer_uesd_count--;
    if (mic_dev->hrtimer_uesd_count > 0)
        return ;

    hrtimer_cancel(&mic_dev->hrtimer);
}

void mic_hrtimer_init(struct mic_dev *mic_dev) {
    hrtimer_init(&mic_dev->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    mic_dev->hrtimer.function = mic_hrtimer_callback;
}
