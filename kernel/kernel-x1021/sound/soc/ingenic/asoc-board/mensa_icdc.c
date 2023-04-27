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
#include <mach/jzsnd.h>
#include "../icodec/icdc_d2.h"

static struct snd_codec_data *codec_platform_data = NULL;


unsigned long codec_sysclk = -1;


static int mensa_spk_power(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		if (codec_platform_data && (codec_platform_data->gpio_spk_en.gpio) != -1) {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio, codec_platform_data->gpio_spk_en.active_level);
			printk("gpio speaker enable %d\n", gpio_get_value(codec_platform_data->gpio_spk_en.gpio));
		} else
			printk("set speaker enable failed. please check codec_platform_data\n");
	} else {
		if (codec_platform_data && (codec_platform_data->gpio_spk_en.gpio) != -1) {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio, !(codec_platform_data->gpio_spk_en.active_level));
			printk("gpio speaker disable %d\n", gpio_get_value(codec_platform_data->gpio_spk_en.gpio));
		} else
			printk("set speaker disable failed. please check codec_platform_data\n");
	}
	return 0;
}

void mensa_spk_sdown(struct snd_pcm_substream *sps){

	if (codec_platform_data && (codec_platform_data->gpio_spk_en.gpio) != -1) {
		gpio_direction_output(codec_platform_data->gpio_spk_en.gpio, !(codec_platform_data->gpio_spk_en.active_level));
	}

	return;
}

int mensa_spk_sup(struct snd_pcm_substream *sps){
	if (codec_platform_data && (codec_platform_data->gpio_spk_en.gpio) != -1) {
		gpio_direction_output(codec_platform_data->gpio_spk_en.gpio, codec_platform_data->gpio_spk_en.active_level);
	}
	return 0;
}

static struct snd_soc_ops mensa_i2s_ops = {
	.startup = mensa_spk_sup,
	.shutdown = mensa_spk_sdown,
};

static const struct snd_soc_dapm_widget mensa_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker", mensa_spk_power),
	SND_SOC_DAPM_MIC("Mic Buildin", NULL),
	SND_SOC_DAPM_LINE("Line In",NULL),
};

static struct snd_soc_jack mensa_icdc_d2_hp_jack;
static struct snd_soc_jack_pin mensa_icdc_d2_hp_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};

/* mensa machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	/* headphone connected to AOHPL/R */
	{"Headphone Jack", NULL, "AOHPL"},
	{"Headphone Jack", NULL, "AOHPR"},

	/* ext speaker connected to AOLOP/N  */
	{"Speaker", NULL, "AOLOP"},
	{"Speaker", NULL, "AOLON"},


	/* mic is connected to AIP/N1 */
	{"MICP1", NULL, "MICBIAS"},
	{"MICN1", NULL, "MICBIAS"},
	{"MICP2", NULL, "MICBIAS"},
	{"MICN2", NULL, "MICBIAS"},
	{"MICBIAS", NULL, "Mic Buildin"},

	/* line in is connected to AIR/L*/
	{"AIR1", NULL, "Line In"},
	{"AIL1", NULL, "Line In"},
};

static int mensa_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = rtd->card;
	int err;
	if ((codec_platform_data) && ((codec_platform_data->gpio_spk_en.gpio) != -1)) {
		err = devm_gpio_request(card->dev, codec_platform_data->gpio_spk_en.gpio, "Speaker_en");
		if (err) {
			pr_err("%s, %d: request spk gpio(%d) fail: %d\n", __func__, __LINE__,
					codec_platform_data->gpio_spk_en.gpio, err);
			return err;
		}
	} else {
		pr_err("codec_platform_data gpio_spk_en is NULL\n");
		return err;
	}
	err = snd_soc_dapm_new_controls(dapm, mensa_dapm_widgets,
			ARRAY_SIZE(mensa_dapm_widgets));
	if (err){
		printk("mensa dai add controls err!!\n");
		return err;
	}
	/* Set up rx1950 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
			ARRAY_SIZE(audio_map));
	if (err){
		printk("add mensa dai routes err!!\n");
		return err;
	}
	snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &mensa_icdc_d2_hp_jack);

	snd_soc_jack_add_pins(&mensa_icdc_d2_hp_jack,ARRAY_SIZE(mensa_icdc_d2_hp_jack_pins),mensa_icdc_d2_hp_jack_pins);
	icdc_d2_hp_detect(codec, &mensa_icdc_d2_hp_jack, SND_JACK_HEADSET);

	snd_soc_dapm_enable_pin(dapm, "Speaker");
	snd_soc_dapm_enable_pin(dapm, "Mic Buildin");
	snd_soc_dapm_enable_pin(dapm, "Line In");

	snd_soc_dapm_sync(dapm);
	return 0;
}

static struct snd_soc_dai_link mensa_dais[] = {
	[0] = {
		.name = "MENSA ICDC",
		.stream_name = "MENSA ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = mensa_dlv_dai_link_init,
		.codec_dai_name = "icdc-d2-hifi",
		.codec_name = "icdc-d2",
		.ops = &mensa_i2s_ops,
	},
	[1] = {
		.name = "MENSA PCMBT",
		.stream_name = "MENSA PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
};

static struct snd_soc_card mensa = {
	.name = "mensa",
	.owner = THIS_MODULE,
	.dai_link = mensa_dais,
	.num_links = ARRAY_SIZE(mensa_dais),
};

static int snd_mensa_probe(struct platform_device *pdev)
{
	int ret = 0;
	mensa.dev = &pdev->dev;
	codec_platform_data = (struct snd_codec_data *)mensa.dev->platform_data;
	ret = snd_soc_register_card(&mensa);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
	return ret;
}

static int snd_mensa_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&mensa);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver snd_mensa_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-alsa",
		.pm = &snd_soc_pm_ops,
	},
	.probe = snd_mensa_probe,
	.remove = snd_mensa_remove,
};
module_platform_driver(snd_mensa_driver);

MODULE_AUTHOR("cli<chen.li@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC mensa Snd Card");
MODULE_LICENSE("GPL");
