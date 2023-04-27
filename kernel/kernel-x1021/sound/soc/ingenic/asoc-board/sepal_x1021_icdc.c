/*
 * Copyright (C) 2017 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author: shuan.xin <shuan.xin@ingenic.com>
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
#include "../asoc-v12/asoc-aic-v12.h"

#define SEPAL_X1021_SPEAKER_EN     GPIO_PC(1)
#define SEPAL_X1021_SPEAKER_LEVEL  1

static int sepal_x1021_spk_power(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	if (gpio_is_valid(SEPAL_X1021_SPEAKER_EN)) {
		if (SND_SOC_DAPM_EVENT_ON(event)) {
			gpio_set_value(SEPAL_X1021_SPEAKER_EN, SEPAL_X1021_SPEAKER_LEVEL);
			printk("gpio speaker enable\n");
		} else {
			gpio_set_value(SEPAL_X1021_SPEAKER_EN, !SEPAL_X1021_SPEAKER_LEVEL);
			printk("gpio speaker disable\n");
		}
	}

	return 0;
}

int sepal_x1021_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    int ret, rate = params_rate(params);

	ret = snd_soc_dai_set_sysclk(cpu_dai, JZ_I2S_INNER_CODEC, rate * 256, SND_SOC_CLOCK_OUT);

	return ret;
};

static struct snd_soc_ops sepal_x1021_i2s_ops = {
	.hw_params = sepal_x1021_i2s_hw_params,
};

static const struct snd_soc_dapm_widget sepal_x1021_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Speaker", sepal_x1021_spk_power),
	SND_SOC_DAPM_MIC("Buildin-mic", NULL),
};

/* sepal_x1021 machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	{"Speaker", NULL, "HPOUT"},
	{"MICP", NULL, "Buildin-mic"},
	{"MICN", NULL, "Buildin-mic"},
};

static int sepal_x1021_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = rtd->card;
        int err;
        if (gpio_is_valid(SEPAL_X1021_SPEAKER_EN)) {
                err = devm_gpio_request_one(card->dev, SEPAL_X1021_SPEAKER_EN,
                                SEPAL_X1021_SPEAKER_LEVEL ? GPIOF_OUT_INIT_LOW : GPIOF_OUT_INIT_HIGH, "speaker_en");
                if (err)
                        return err;
        }
        err = snd_soc_dapm_new_controls(dapm, sepal_x1021_dapm_widgets,
			ARRAY_SIZE(sepal_x1021_dapm_widgets));
	if (err){
		printk("sepal_x1021 dai add controls err!!\n");
		return err;
	}
	/* Set up rx1950 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
			ARRAY_SIZE(audio_map));
	if (err){
		printk("add sepal_x1021 dai routes err!!\n");
		return err;
	}
	snd_soc_dapm_enable_pin(dapm, "Speaker");
	snd_soc_dapm_enable_pin(dapm, "Buildin-mic");
	snd_soc_dapm_sync(dapm);
	return 0;
}

static struct snd_soc_dai_link sepal_x1021_dais[] = {
	[0] = {
		.name = "sepal_x1021",
		.stream_name = "sepal_x1021",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = sepal_x1021_dlv_dai_link_init,
		.codec_dai_name = "icdc-d4-hifi",
		.codec_name = "icdc-d4",
        .dai_fmt = SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM,
		.ops = &sepal_x1021_i2s_ops,
	},
};

static struct snd_soc_card sepal_x1021 = {
	.name = "x1021_codec",
	.owner = THIS_MODULE,
	.dai_link = sepal_x1021_dais,
	.num_links = ARRAY_SIZE(sepal_x1021_dais),
};

static int snd_sepal_x1021_probe(struct platform_device *pdev)
{
	int ret = 0;

    sepal_x1021.dev = &pdev->dev;
	ret = snd_soc_register_card(&sepal_x1021);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);

	return ret;
}

static int snd_sepal_x1021_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&sepal_x1021);
	return 0;
}

static struct platform_driver snd_sepal_x1021_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-alsa",
		.pm = &snd_soc_pm_ops,
	},
	.probe = snd_sepal_x1021_probe,
	.remove = snd_sepal_x1021_remove,
};
module_platform_driver(snd_sepal_x1021_driver);

MODULE_AUTHOR("shuan.xin<shuan.xin@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC Sepal_x1021 Snd Card");
MODULE_LICENSE("GPL");
