
#include <mach/jzsnd.h>
#include "board_base.h"

#ifdef CONFIG_SOUND_OSS_CORE
struct snd_codec_data codec_data = {
	/* volume */

	.record_volume_base = 0,
	.record_digital_volume_base = 0,
	.replay_digital_volume_base = 50,

	/* default route */
	.replay_def_route = {
		.route = SND_ROUTE_REPLAY_SPK,
		.gpio_spk_en_stat = STATE_ENABLE,
		.replay_digital_volume_base = 50,
	},

	.record_def_route = {
		.route = SND_ROUTE_RECORD_AMIC,
		.gpio_spk_en_stat = STATE_DISABLE,
		.record_digital_volume_base = 50,
	},

	.record_buildin_mic_route = {
		.route = SND_ROUTE_NONE,
		.gpio_spk_en_stat = KEEP_OR_IGNORE,
	},

	.record_linein_route = {
		.route = SND_ROUTE_LINEIN_REPLAY_MIXER_LOOPBACK,
		.gpio_spk_en_stat = STATE_ENABLE,
		.record_digital_volume_base = 50,
	},

	.replay_speaker_route = {
		.route = SND_ROUTE_REPLAY_SOUND_MIXER_LOOPBACK,
		.gpio_spk_en_stat = STATE_ENABLE,
		.replay_digital_volume_base = 50,
	},

	.replay_speaker_record_buildin_mic_route = {
		.route = SND_ROUTE_RECORD_AMIC_AND_REPLAY_SPK,
		.gpio_spk_en_stat = STATE_ENABLE,
		.replay_digital_volume_base = 50,
		.record_digital_volume_base = 50,
	},

	.gpio_spk_en = {.gpio = GPIO_SPEAKER_EN, .active_level = GPIO_SPEAKER_EN_LEVEL},
	.gpio_amp_power = {.gpio = GPIO_AMP_POWER_EN, .active_level = GPIO_AMP_POWER_EN_LEVEL},
	.gpio_hp_mute = {.gpio = GPIO_HP_MUTE, .active_level = GPIO_HP_MUTE_LEVEL},
	.gpio_hp_detect = {.gpio = GPIO_HP_DETECT, .active_level = GPIO_HP_INSERT_LEVEL},
	.gpio_linein_detect = {.gpio = GPIO_LINEIN_DETECT, .active_level = GPIO_LINEIN_INSERT_LEVEL},

};
#endif

#ifdef CONFIG_SND_ASOC_JZ_INCODEC
static struct ingenic_snd_codec_data snd_alsa_platform_data = {
	.gpio_spk_en = {.gpio = GPIO_SPEAKER_EN, .active_level = GPIO_SPEAKER_EN_LEVEL},
	.gpio_linein_sw = {.gpio = GPIO_LINEIN_DETECT, .active_level = GPIO_LINEIN_INSERT_LEVEL},
};

struct platform_device snd_alsa_device = {
	.name = "ingenic-alsa",
	.dev = {
		.platform_data = &snd_alsa_platform_data,
	},
};
#endif
