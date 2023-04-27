#include "hw.h"
#include "snd.h"

static void snd_start_config(struct audio_rtd_params *prm) {
	struct snd_pcm_substream *substream = prm->ss;
	int sample_size = 2;
	prm->dma_bytes = snd_pcm_lib_buffer_bytes(substream);
	prm->dma_area = substream->runtime->dma_addr;
	prm->period_size = snd_pcm_lib_period_bytes(substream);
	prm->period_ms = MSEC_PER_SEC * prm->period_size / substream->runtime->rate
		/ substream->runtime->hw.channels_min / sample_size;
	prm->periods = prm->dma_bytes / prm->period_size;
	prm->stream = substream->stream;
}

static void snd_start(struct audio_rtd_params *prm)
{
	struct dma_param *dma = prm->dma;

	if (!prm->ss)
		return;

	/*dma start*/
	snd_start_config(prm);
	dma->tx_buf = prm->dma_area;
	dma->tx_buf_len = prm->dma_bytes;
	dma->tx_period_size = prm->period_size;
	dma_param_submit_cyclic(prm->dma, DMA_MEM_TO_DEV);
	hrtimer_forward_now(&prm->hrtimer,
			ns_to_ktime(prm->period_ms * NSEC_PER_MSEC));
	hrtimer_start_expires(&prm->hrtimer, HRTIMER_MODE_ABS);
	/*cpu start*/
	i2s_enable_playback_dma();
}

static void snd_stop(struct audio_rtd_params *prm)
{
	/*cpu stop*/
	i2s_disable_playback_dma();
	/*dma stop*/
	dma_param_terminate(prm->dma, DMA_MEM_TO_DEV);
	hrtimer_cancel(&prm->hrtimer);
}

static void snd_work(struct work_struct *work) {
	struct audio_rtd_params *prm =
		container_of(work, struct audio_rtd_params, work);
	static int is_enable = 0;

	if (!is_enable && prm->hrtimer_uesd_count == 1) {
		is_enable = 1;
		snd_start(prm);
	}
	else if (is_enable && !prm->hrtimer_uesd_count){
		is_enable = 0;
		snd_stop(prm);
	}
}

static enum hrtimer_restart snd_hrtimer_callback(struct hrtimer *hrtimer)
{
	struct audio_rtd_params *prm =
		container_of(hrtimer, struct audio_rtd_params, hrtimer);
	struct dma_param *dma = prm->dma;
	struct dma_chan *dma_chan = dma->dma_chan[SNDRV_PCM_STREAM_PLAYBACK];
	dma_addr_t dma_addr = 0;

	if (!prm->hrtimer_uesd_count)
		return HRTIMER_NORESTART;

	if (!prm->ss) {
		hrtimer_forward(hrtimer, hrtimer->_softexpires,
				ns_to_ktime(prm->period_ms * NSEC_PER_MSEC));
		return HRTIMER_NORESTART;
	}

	dma_addr = dma_chan->device->get_current_trans_addr(
			dma_chan, NULL, NULL, DMA_MEM_TO_DEV);
	if(dma_addr >= prm->dma_area &&
			dma_addr < prm->dma_area + prm->dma_bytes){
		hrtimer_forward(hrtimer, hrtimer->_softexpires,
				ns_to_ktime(prm->period_ms * NSEC_PER_MSEC));
		prm->offset = dma_addr - prm->dma_area;
		snd_pcm_period_elapsed(prm->ss);
	} else {
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
	schedule_work(&prm->work);
}

void snd_hrtimer_stop(struct audio_rtd_params *prm) {
	prm->hrtimer_uesd_count--;
	if (prm->hrtimer_uesd_count > 0)
		return ;
	schedule_work(&prm->work);
}

void snd_hrtimer_init(struct audio_rtd_params *prm) {
	hrtimer_init(&prm->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	prm->hrtimer.function = snd_hrtimer_callback;
	INIT_WORK(&prm->work, snd_work);
}
