
#include <mach/jzsnd.h>
#include "board_base.h"

#ifdef CONFIG_SOUND_OSS_CORE
struct snd_codec_data codec_data = {
};

#ifdef CONFIG_AKM4951_EXTERNAL_CODEC
struct snd_codec_data akm4951_codec_data = {
	.codec_sys_clk = 0,
	/* volume */
	/* akm4951 -104db~24db, 0.5db step */
	.replay_digital_volume_base = -10,

	.gpio_spk_en = {.gpio = GPIO_SPEAKER_EN, .active_level = GPIO_SPEAKER_EN_LEVEL},
	.gpio_amp_power = {.gpio = GPIO_AMP_POWER_EN, .active_level = GPIO_AMP_POWER_EN_LEVEL},
	.gpio_hp_detect = {.gpio = GPIO_HP_DETECT, .active_level = GPIO_HP_INSERT_LEVEL},
	.gpio_linein_detect = {.gpio = GPIO_LINEIN_DETECT, .active_level = GPIO_LINEIN_INSERT_LEVEL},

	.priv = &akm4951_priv,
};

struct platform_device akm4951_codec_device = {
	.name = "akm4951_codec",
};
#endif
#endif

#ifdef CONFIG_SND_ASOC_INGENIC
static struct snd_codec_data snd_alsa_platform_data = {
	.gpio_spk_en = {.gpio = GPIO_SPEAKER_EN, .active_level = GPIO_SPEAKER_EN_LEVEL},
	.gpio_dmic_en = {.gpio = GPIO_DMIC_EN, .active_level = GPIO_DMIC_EN_LEVEL},
	.gpio_hp_detect = {.gpio = GPIO_HP_DETECT, .active_level = GPIO_HP_INSERT_LEVEL},
	.gpio_linein_detect = {.gpio = GPIO_LINEIN_DETECT, .active_level = GPIO_LINEIN_INSERT_LEVEL},
	.gpio_lineout_detect = {.gpio = GPIO_LINEOUT_DETECT, .active_level = GPIO_LINEOUT_INSERT_LEVEL},
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
