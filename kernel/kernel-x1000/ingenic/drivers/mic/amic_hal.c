#include "mic.h"
#include "hw.h"
#include "dma.h"

static struct codec_params codec_params;
void amic_set_gain(struct mic_dev *mic_dev, int gain) {
	int val;
	struct mic *amic = &mic_dev->amic;

	printk("entry: %s, set: %d\n", __func__, gain);

	val = (gain * 100 - codec_params.vol_min_DB) / codec_params.vol_step_DB;
	if (val < codec_params.vol_min_val)
		val = codec_params.vol_min_val;
	if (val > codec_params.vol_max_val)
		val = codec_params.vol_max_val;

	gain = (((val - codec_params.vol_min_val) *
				codec_params.vol_step_DB + codec_params.vol_min_DB)) / 100;
	amic->gain = gain;
	codec_set_adc_vol(val);
}

void amic_get_gain_range(struct gain_range *range)
{
	range->min_dB = codec_params.vol_min_DB / 100;
	range->max_dB = (codec_params.vol_min_DB +
			(codec_params.vol_max_val - codec_params.vol_min_val) *
			codec_params.vol_step_DB) / 100;
}

int amic_init(struct mic_dev *mic_dev)
{
	int ret = i2s_init(mic_dev->pdev, &mic_dev->amic);
	if (ret)
		return ret;
	return codec_adc_init(mic_dev->amic.dma->iomem, &codec_params);
}

void amic_enable(struct mic_dev *mic_dev)
{
	codec_adc_power(true);
	i2s_enable_capture_dma();
}

void amic_disable(struct mic_dev *mic_dev)
{
	i2s_disable_capture_dma();
	codec_adc_power(false);
}
