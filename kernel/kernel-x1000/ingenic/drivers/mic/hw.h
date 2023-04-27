#ifndef __JZ_SOUND_HW_H__
#define __JZ_SOUND_HW_H__

#include <linux/types.h>
#include <linux/platform_device.h>

struct mic;
/*I2S interface*/
int i2s_init(struct platform_device *pdev, struct mic *mic);
void codec_icdc_reset(void);
int i2s_clk_switch(bool is_on);
void i2s_enable_playback_dma(void);
void i2s_disable_playback_dma(void);
void i2s_enable_capture_dma(void);
void i2s_disable_capture_dma(void);

/*Codec interface*/
struct codec_params {
	int vol_step_DB;	/*unit DB/100*/
	int vol_min_DB;	/*unit DB/100*/
	int vol_mute;
	int vol_min_val;
	int vol_max_val;
};

int codec_adc_init(void *iomem, struct codec_params* params);
int codec_dac_init(void *iomem, struct codec_params* params);
int codec_set_dac_vol(int val);
int codec_get_dac_vol(void);
int codec_set_adc_vol(int val);
int codec_get_adc_vol(void);
int codec_dac_power(bool on);
int codec_adc_power(bool on);
#endif /*__JZ_SOUND_HW_H__*/
