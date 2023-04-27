
#include <mach/jzsnd.h>
#include "board_base.h"

#ifdef CONFIG_SOUND_OSS_CORE
struct snd_codec_data codec_data = {
	.codec_sys_clk = 0 ,  //0:12M  1:13M
	.codec_dmic_clk = 1,
	/* volume */
	.replay_volume_base = 6,//(-25 ~ 6 VS 0 ~ 31 the former is more bigger , more louder)
	.record_volume_base = 20,
	.record_digital_volume_base = 15,
	.replay_digital_volume_base = 0,//(-31 ^ -1 0 1 ^ 32 VS 33 ^1 0 63 ^ 32)the former is more bigger, more louder

	/* default route */
	.replay_def_route = {
					.route = REPLAY_HP_STEREO_WITH_CAP,
					.gpio_hp_mute_stat = STATE_ENABLE,
					.gpio_buildin_mic_en_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE,
					.replay_volume_base = 6},//hwang
	.record_def_route = {
					.route = RECORD_DMIC,
					.gpio_buildin_mic_en_stat = STATE_ENABLE,
					.gpio_spk_en_stat = STATE_ENABLE,
					.gpio_hp_mute_stat = STATE_DISABLE},
	.record_buildin_mic_route = {
					.route = RECORD_DMIC,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	.replay_speaker_route = {
					.route = REPLAY_HP_STEREO_WITH_CAP,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},

	.replay_headset_route = {.route = REPLAY_HP_STEREO_WITH_CAP,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_DISABLE},

	.record_linein_route = {.route = BYPASS_LINEIN_TO_HP_WITH_CAP,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_DISABLE},

	.record_linein1_route = {.route = BYPASS_LINEIN_TO_LINEOUT,
					.gpio_hp_mute_stat = STATE_ENABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	.record_linein2_route = {.route= RECORD_STEREO_MIC_DIFF_WITH_BIAS_BYPASS_MIXER_MIC2_TO_HP_CAP_LESS,
					.gpio_hp_mute_stat = STATE_ENABLE,
					.gpio_spk_en_stat = STATE_DISABLE},

	.gpio_spk_en = {.gpio = GPIO_SPEAKER_EN, .active_level = GPIO_SPEAKER_EN_LEVEL},
	.gpio_hp_mute = {.gpio = GPIO_HP_MUTE, .active_level = GPIO_HP_MUTE_LEVEL},
	.gpio_hp_detect = {.gpio = GPIO_HP_DETECT, .active_level = GPIO_HP_INSERT_LEVEL},
#if 0
	.gpio_handset_en = {.gpio = GPIO_HANDSET_EN, .active_level = GPIO_HANDSET_EN_LEVEL},
	.gpio_mic_detect = {.gpio = GPIO_MIC_DETECT,.active_level = GPIO_MIC_INSERT_LEVEL},
	.gpio_buildin_mic_select = {.gpio = GPIO_MIC_SELECT,.active_level = GPIO_BUILDIN_MIC_LEVEL},
	.gpio_mic_detect_en = {.gpio = GPIO_MIC_DETECT_EN,.active_level = GPIO_MIC_DETECT_EN_LEVEL},
	.gpio_adc_en = {.gpio = GPIO_ADC_EN,.active_level = GPIO_ADC_EN_LEVEL},
    //.gpio_usb_detect = {.gpio = GPIO_USB_DETE,.active_level = GPIO_USB_INSERT_LEVEL},
	/* gpio */
	//.hook_active_level = -1,
	.hpsense_active_level = 1,
#endif
};
#endif

#ifdef CONFIG_SND_ASOC_JZ_INCODEC
static struct snd_codec_data snd_alsa_platform_data = {
	.gpio_spk_en = {.gpio = GPIO_SPEAKER_EN, .active_level = GPIO_SPEAKER_EN_LEVEL},
};

struct platform_device snd_alsa_device = {
	.name = "ingenic-alsa",
	.dev = {
		.platform_data = &snd_alsa_platform_data,
	},
};
#endif

#if defined(CONFIG_SND) && defined(CONFIG_SND_ASOC_INGENIC)
#ifdef CONFIG_SND_ASOC_JZ_AIC_V12
struct snd_dma_data aic_dma_data = {
	.dma_soft_mute = NO_SUPPLY_DMA_SOFT_MUTE_AMIXER,
};
#endif
#if defined(CONFIG_SND_ASOC_JZ_PCM_V13)
struct snd_dma_data pcm_dma_data = {
	.dma_soft_mute = NO_SUPPLY_DMA_SOFT_MUTE_AMIXER,
};
#endif
#if defined(CONFIG_SND_ASOC_JZ_DMIC_V13)
struct snd_dma_data dmic_dma_data = {
	.dma_soft_mute = SUPPLY_DMA_SOFT_MUTE_AMIXER,
};
#endif
#endif
