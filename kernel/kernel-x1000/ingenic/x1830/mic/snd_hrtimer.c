#include "audio_dev.h"
#include <linux/delay.h>
#include <linux/mutex.h>

static DEFINE_MUTEX(m_lock);

void snd_start_config(struct audio_rtd_params *prm) {
    struct snd_pcm_substream *substream = prm->ss;

    int sample_size;
    switch (substream->runtime->hw.formats) {
    case SNDRV_PCM_FMTBIT_S16_LE:
        sample_size = 2;
        break;
    case SNDRV_PCM_FMTBIT_S24_LE:
        sample_size = 4;
        break;
    default:
        assert(0);
    }

    prm->dma_bytes = snd_pcm_lib_buffer_bytes(substream);
    prm->dma_area = substream->runtime->dma_addr;
    prm->period_size = snd_pcm_lib_period_bytes(substream);

    prm->period_ms = MSEC_PER_SEC * prm->period_size / substream->runtime->rate
            / substream->runtime->hw.channels_min / sample_size;

    prm->periods = prm->dma_bytes / prm->period_size;
    prm->stream = prm->ss->stream;
}

static void amic_submit_dma(void *data)
{
    struct audio_rtd_params *prm = data;
    struct dma_param *dma = prm->dma;

    dma->rx_buf = prm->dma_area;
    dma->rx_buf_len = prm->dma_bytes;
    dma->rx_period_size = prm->period_size;
    dma_param_submit_cyclic(prm->dma, DMA_DEV_TO_MEM);
}

void snd_start(struct audio_rtd_params *prm) {
    struct snd_pcm_substream *substream = prm->ss;
    struct dma_param *dma = prm->dma;

    if (prm->stream == SNDRV_PCM_STREAM_PLAYBACK)
        i2s_enable_playback_dma(substream->runtime->rate);
    else
        i2s_enable_capture_dma(amic_submit_dma, prm);

    if (prm->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        dma->tx_buf = prm->dma_area;
        dma->tx_buf_len = prm->dma_bytes;
        dma->tx_period_size = prm->period_size;
        dma_param_submit_cyclic(prm->dma, DMA_MEM_TO_DEV);
    }

    prm->hrtimer_is_running = 1;

    hrtimer_forward_now(&prm->hrtimer,
            ns_to_ktime(prm->period_ms * NSEC_PER_MSEC));
    hrtimer_start_expires(&prm->hrtimer, HRTIMER_MODE_ABS);
}

void snd_stop(struct audio_rtd_params *prm) {
    dma_param_terminate(prm->dma, DMA_MEM_TO_DEV);
    hrtimer_cancel(&prm->hrtimer);

    prm->hrtimer_is_running = 0;

    if (prm->stream == SNDRV_PCM_STREAM_PLAYBACK)
        i2s_disable_playback_dma();
    else
        i2s_disable_capture_dma();
}

void snd_work(struct work_struct *work) {
    struct audio_rtd_params *prm =
            container_of(work, struct audio_rtd_params, work);
    static int is_enable = 0;

    mutex_lock(&m_lock);

    if (!is_enable && prm->hrtimer_uesd_count == 1) {
        is_enable = 1;
        snd_start(prm);
    }
    else if (is_enable && !prm->hrtimer_uesd_count){
        is_enable = 0;
        snd_stop(prm);
    }

    mutex_unlock(&m_lock);
}

static enum hrtimer_restart snd_hrtimer_callback(struct hrtimer *hrtimer)
{
    struct audio_rtd_params *prm =
            container_of(hrtimer, struct audio_rtd_params, hrtimer);
    struct dma_param *dma = prm->dma;

    if (!prm->hrtimer_uesd_count)
        return HRTIMER_NORESTART;

    if (!prm->ss) {
        hrtimer_forward(hrtimer, hrtimer->_softexpires,
                ns_to_ktime(prm->period_ms * NSEC_PER_MSEC));

        return HRTIMER_NORESTART;
    }

    enum dma_data_direction direction;
    dma_addr_t dma_addr;
    struct dma_chan *dma_chan;

    if (prm->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        direction = DMA_MEM_TO_DEV;
        dma_chan = dma->dma_chan[SNDRV_PCM_STREAM_PLAYBACK];
    }
    else {
        direction = DMA_DEV_TO_MEM;
        dma_chan = dma->dma_chan[SNDRV_PCM_STREAM_CAPTURE];
    }

    dma_addr = dma_chan->device->get_current_trans_addr(
            dma_chan, NULL, NULL, direction);
	if(dma_addr > prm->dma_area && dma_addr < prm->dma_area + prm->dma_bytes){
		hrtimer_forward(hrtimer, hrtimer->_softexpires,
			ns_to_ktime(prm->period_ms * NSEC_PER_MSEC));

		prm->offset = dma_addr - prm->dma_area;
        if (prm->hrtimer_uesd_count)
		    snd_pcm_period_elapsed(prm->ss);
	}else{
		/* dma addr is illegal,and retry */
		pr_warning("snd: dma address[0x%08x] illegal!\n",dma_addr);
		hrtimer_forward(hrtimer, hrtimer->_softexpires,
			ns_to_ktime(NSEC_PER_MSEC));
	}
	return HRTIMER_RESTART;
}

void snd_hrtimer_start(struct audio_rtd_params *prm) {
    prm->hrtimer_uesd_count++;
    if (prm->hrtimer_uesd_count > 1)
        return ;

    snd_start_config(prm);

    schedule_work(&prm->work);
}

void snd_hrtimer_stop(struct audio_rtd_params *prm) {
    prm->hrtimer_uesd_count--;
    if (prm->hrtimer_uesd_count > 0)
        return ;

    schedule_work(&prm->work);
}

void snd_hrtimer_wait_stop(struct audio_rtd_params *prm) {
    while (prm->hrtimer_is_running)
        msleep(1);
}

void snd_hrtimer_init(struct audio_rtd_params *prm) {
    hrtimer_init(&prm->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    prm->hrtimer.function = snd_hrtimer_callback;
    INIT_WORK(&prm->work, snd_work);
}
