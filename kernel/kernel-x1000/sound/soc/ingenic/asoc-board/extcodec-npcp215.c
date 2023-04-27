/*
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author: dlzhang <daolin.zhang@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <linux/gpio.h>
#include "../icodec/icdc_d3.h"

#define WB73_SPK_GPIO	-1
#define WB73_SPK_EN	1

unsigned long codec_sysclk = 24000000;
static int wb73_spk_power(struct snd_soc_dapm_widget *w,
						  struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		if (WB73_SPK_GPIO != -1){
			gpio_direction_output(WB73_SPK_GPIO, WB73_SPK_EN);
			printk("gpio speaker enable %d\n", gpio_get_value(WB73_SPK_GPIO));
		}
	} else {
		if (WB73_SPK_GPIO != -1){
			gpio_direction_output(WB73_SPK_GPIO, !WB73_SPK_EN);
			printk("gpio speaker disable %d\n", gpio_get_value(WB73_SPK_GPIO));
		}
	}
	return 0;
}

void wb73_spk_sdown(struct snd_pcm_substream *sps)
{
	if (WB73_SPK_GPIO != -1)
		gpio_direction_output(WB73_SPK_GPIO, !WB73_SPK_EN);
	return;
}

int wb73_spk_sup(struct snd_pcm_substream *sps)
{
	if (WB73_SPK_GPIO != -1)
		gpio_direction_output(WB73_SPK_GPIO, WB73_SPK_EN);
	return 0;
}
int canna_npcp215_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	/*FIXME snd_soc_dai_set_pll*/
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM);
	if (ret)
		return ret;
	ret = snd_soc_dai_set_sysclk(cpu_dai, 1, 24000000, SND_SOC_CLOCK_IN);
	if (ret)
		return ret;
	return 0;
};
static struct snd_soc_ops wb73_i2s_ops = {
	.startup = wb73_spk_sup,
	.shutdown = wb73_spk_sdown,
	.hw_params = canna_npcp215_hw_params,
};
static const struct snd_soc_dapm_widget wb73_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker", wb73_spk_power),
	SND_SOC_DAPM_MIC("Mic", NULL),
};

static struct snd_soc_jack canna_icdc_d3_hp_jack;
static struct snd_soc_jack_pin canna_icdc_d3_hp_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};
#ifdef HAVE_HEADPHONE
static struct snd_soc_jack_gpio canna_icdc_d3_jack_gpio[] = {
	{
		.name = "Headphone detection",
		.report = SND_JACK_HEADPHONE,
		.debounce_time = 150,
	}
};
#endif
/* m1 machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	/* ext speaker connected to DAC out */
	{"Speaker", NULL, "DAC OUT"},

	/* mic is connected to ADC in */
	{"Mic", NULL, "ADC IN"},

};

static int wb73_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int err;

	err = snd_soc_dapm_new_controls(dapm, wb73_dapm_widgets,
									ARRAY_SIZE(wb73_dapm_widgets));
	if (err){
		printk("wb73 dai add controls err!!\n");
		return err;
	}

	/* Set up specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
								  ARRAY_SIZE(audio_map));
	if (err){
		printk("add wb73 dapm routes err!!\n");
		return err;
	}

	snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &canna_icdc_d3_hp_jack);
	snd_soc_jack_add_pins(&canna_icdc_d3_hp_jack,ARRAY_SIZE(canna_icdc_d3_hp_jack_pins),canna_icdc_d3_hp_jack_pins);
#ifdef HAVE_HEADPHONE
	if (gpio_is_valid(DORADO_HP_DET)) {
		canna_icdc_d3_jack_gpio[jack].gpio = PHOENIX_HP_DET;
		canna_icdc_d3_jack_gpio[jack].invert = !PHOENIX_HP_DET_LEVEL;
		snd_soc_jack_add_gpios(&canna_icdc_d3_hp_jack, 1, canna_icdc_d3_jack_gpio);
	}
#else
	snd_soc_dapm_disable_pin(dapm, "Headphone Jack");
#endif

	snd_soc_dapm_force_enable_pin(dapm, "Speaker");
	snd_soc_dapm_force_enable_pin(dapm, "Mic");

	snd_soc_dapm_sync(dapm);
	return 0;
}

static struct snd_soc_dai_link wb73_dais[] = {
	[0] = {
		.name = "WB73 npcp215",
		.stream_name = "WB73 npcp215",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = wb73_dlv_dai_link_init,
		.codec_dai_name = "npcp215-dai",
		.codec_name = "npcp215.0-0073",
		.ops = &wb73_i2s_ops,
	},
	[1] = {
		.name = "WB73 PCMBT",
		.stream_name = "WB73 PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
	[2] = {
		.name = "WB73 DMIC",
		.stream_name = "WB73 DMIC",
		.platform_name = "jz-asoc-dmic-dma",
		.cpu_dai_name = "jz-asoc-dmic",
		.codec_dai_name = "dmic dump dai",
		.codec_name = "dmic dump",
	},

};

static struct snd_soc_card wb73 = {
	.name = "wb73",
	.owner = THIS_MODULE,
	.dai_link = wb73_dais,
	.num_links = ARRAY_SIZE(wb73_dais),
};

static struct platform_device *wb73_snd_device;

static int wb73_init(void)
{
	/*struct jz_aic_gpio_func *gpio_info;*/
	int ret;

	if (WB73_SPK_GPIO != -1) {
		ret = gpio_request(WB73_SPK_GPIO, "Speaker_en");
		if (ret)
			return ret;
	}
	wb73_snd_device = platform_device_alloc("soc-audio", -1);
	if (!wb73_snd_device) {
		if (WB73_SPK_GPIO != -1)
			gpio_free(WB73_SPK_GPIO);
		return -ENOMEM;
	}

	platform_set_drvdata(wb73_snd_device, &wb73);
	ret = platform_device_add(wb73_snd_device);
	if (ret) {
		platform_device_put(wb73_snd_device);
		if (WB73_SPK_GPIO != -1)
			gpio_free(WB73_SPK_GPIO);
	}

	dev_info(&wb73_snd_device->dev, "Alsa sound card:wb73 init ok!!!\n");
	return ret;
}

static void wb73_exit(void)
{
	platform_device_unregister(wb73_snd_device);
	if (WB73_SPK_GPIO != -1)
		gpio_free(WB73_SPK_GPIO);
}

module_init(wb73_init);
module_exit(wb73_exit);

MODULE_AUTHOR("dlzhang<daolin.zhang@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC M1 Snd Card");
MODULE_LICENSE("GPL");
