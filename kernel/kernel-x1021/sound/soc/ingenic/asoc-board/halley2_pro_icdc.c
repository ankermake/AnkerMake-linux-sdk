 /*
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author: cli <chen.li@ingenic.com>
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
#include "../icodec/icdc_d1.h"

#define halley2pro_SPK_GPIO	 GPIO_PB(1)
#define halley2pro_SPK_EN	1

static int halley2pro_spk_power(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		gpio_direction_output(halley2pro_SPK_GPIO, halley2pro_SPK_EN);
		printk("gpio speaker enable %d\n", gpio_get_value(halley2pro_SPK_GPIO));
	} else {
		gpio_direction_output(halley2pro_SPK_GPIO, !halley2pro_SPK_EN);
		printk("gpio speaker disable %d\n", gpio_get_value(halley2pro_SPK_GPIO));
	}
	return 0;
}

static const struct snd_soc_dapm_widget halley2pro_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Speaker", halley2pro_spk_power),
	SND_SOC_DAPM_MIC("Mic Buildin", NULL),
};

/* halley2pro machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	/* ext speaker connected to AOLOP/N  */
	{"Speaker", NULL, "AOHPL"},
	{"Speaker", NULL, "AOHPR"},

	/* mic is connected to AIP/N1 */
	{"AIP1", NULL, "Mic Buildin"},
	{"AIN1", NULL, "Mic Buildin"},
	{"AIP2", NULL, "Mic Buildin"},
};

static int halley2pro_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = rtd->card;
	int err;
	int jack = 0;
	err = devm_gpio_request(card->dev, halley2pro_SPK_GPIO, "Speaker_en");
	if (err)
		return err;

	err = snd_soc_dapm_new_controls(dapm, halley2pro_dapm_widgets,
			ARRAY_SIZE(halley2pro_dapm_widgets));
	if (err)
		return err;

	/* Set up rx1950 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
			ARRAY_SIZE(audio_map));
	if (err)
		return err;

	snd_soc_dapm_enable_pin(dapm, "Speaker");
	snd_soc_dapm_enable_pin(dapm, "Mic Buildin");
	snd_soc_dapm_sync(dapm);
	return 0;
}

int halley2pro_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	/*FIXME snd_soc_dai_set_pll*/
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM);
	if (ret)
		return ret;
	ret = snd_soc_dai_set_sysclk(cpu_dai, JZ_I2S_INNER_CODEC, 12000000, SND_SOC_CLOCK_OUT);
	if (ret)
		return ret;
	return 0;
};

int halley2pro_i2s_hw_free(struct snd_pcm_substream *substream)
{
	/*notify release pll*/
	return 0;
};

static struct snd_soc_ops halley2pro_i2s_ops = {
	.hw_params = halley2pro_i2s_hw_params,
	.hw_free = halley2pro_i2s_hw_free,
};

static struct snd_soc_dai_link halley2pro_dais[] = {
	[0] = {
		.name = "halley2pro ICDC",
		.stream_name = "halley2pro ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = halley2pro_dlv_dai_link_init,
		.codec_dai_name = "icdc-d1-hifi",
		.codec_name = "icdc-d1",
		.ops = &halley2pro_i2s_ops,
	},
	[1] = {
		.name = "halley2pro PCMBT",
		.stream_name = "halley2pro PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
	[2] = {
		.name = "halley2pro DMIC",
		.stream_name = "halley2pro DMIC",
		.platform_name = "jz-asoc-dmic-dma",
		.cpu_dai_name = "jz-asoc-dmic",
		.codec_dai_name = "dmic dump dai",
		.codec_name = "dmic dump",
	},

};

static struct snd_soc_card halley2pro = {
	.name = "halley2pro",
	.owner = THIS_MODULE,
	.dai_link = halley2pro_dais,
	.num_links = ARRAY_SIZE(halley2pro_dais),
};

static int snd_halley2pro_probe(struct platform_device *pdev)
{
	int ret = 0;
	halley2pro.dev = &pdev->dev;
	ret = snd_soc_register_card(&halley2pro);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
	return ret;
}

static int snd_halley2pro_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&halley2pro);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver snd_halley2pro_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-halley2plus",
		.pm = &snd_soc_pm_ops,
	},
	.probe = snd_halley2pro_probe,
	.remove = snd_halley2pro_remove,
};
module_platform_driver(snd_halley2pro_driver);

MODULE_AUTHOR("cli<chen.li@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC halley2pro Snd Card");
MODULE_LICENSE("GPL");
