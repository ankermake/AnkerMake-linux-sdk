/*
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author: zhihao.xiao <zhihao.xiao@ingenic.com>
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
#include <linux/of_gpio.h>
#include "../as-v1/asoc-aic.h"
#include "../ecodec/nau8822.h"
static struct snd_soc_card *knot_card;

struct knot {
	struct gpio_desc * gpio_audio_select;
	struct gpio_desc * gpio_spken;
};
struct knot * x2500_knot;

int x2500_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret;
	unsigned int sysclk = 24576000;
	int cpu_clk_id = 0;
	int codec_clk_id = 0;
	int codec_mode = 0;
	//int div = 24576000 / params_rate(params) / 64 - 1;

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		codec_clk_id |= NAU8822_CLK_MCLK;
		cpu_clk_id |= INGENIC_I2S_PLAYBACK;
	}else{
		codec_clk_id |= NAU8822_CLK_MCLK;
		cpu_clk_id |= INGENIC_I2S_CAPTURE;
	}
	cpu_clk_id |= INGENIC_I2S_EX_CODEC;

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBS_CFM);
	if (ret)
		return ret;

	codec_mode |= SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_I2S;
	ret = snd_soc_dai_set_fmt(codec_dai, codec_mode);
	if (ret)
		return ret;

	sysclk = 256 * params_rate(params);
	ret = snd_soc_dai_set_sysclk(cpu_dai, cpu_clk_id, sysclk, SND_SOC_CLOCK_OUT);
	if (ret)
		return ret;

	return 0;
};

int x2500_i2s_hw_free(struct snd_pcm_substream *substream)
{
	/*notify release pll*/
	return 0;
};

static struct snd_soc_ops x2500_i2s_ecodec_ops = {
	.hw_params = x2500_i2s_hw_params,
	.hw_free = x2500_i2s_hw_free,
};



static const struct snd_soc_dapm_widget x2500_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Speaker", NULL),
	SND_SOC_DAPM_MIC("MICBIAS", NULL),
	SND_SOC_DAPM_MIC("MICL", NULL),
	SND_SOC_DAPM_MIC("MICR", NULL),
};


static int x2500_i2s_cdc_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_card *card = rtd->card;
	return 0;
}

static int audio_select_get(struct snd_kcontrol *kcontrol,
                                 struct snd_ctl_elem_value *ucontrol)
{
	if(x2500_knot->gpio_audio_select)
		ucontrol->value.integer.value[0] = gpiod_get_value(x2500_knot->gpio_audio_select);
	return 0;
}

static int audio_select_put(struct snd_kcontrol *kcontrol,
                                 struct snd_ctl_elem_value *ucontrol)
{
	unsigned int audio_select_state = ucontrol->value.integer.value[0] ? 1 : 0;
	if(x2500_knot->gpio_audio_select)
		gpiod_direction_output(x2500_knot->gpio_audio_select, audio_select_state);
	return 0;
}

static int spken_get(struct snd_kcontrol *kcontrol,
                                 struct snd_ctl_elem_value *ucontrol)
{
	if(x2500_knot->gpio_spken)
		ucontrol->value.integer.value[0] = gpiod_get_value(x2500_knot->gpio_spken);
	return 0;
}

static int spken_put(struct snd_kcontrol *kcontrol,
                                 struct snd_ctl_elem_value *ucontrol)
{
	unsigned int spken_state = ucontrol->value.integer.value[0] ? 1 : 0;

	if(x2500_knot->gpio_spken)
		gpiod_direction_output(x2500_knot->gpio_spken, spken_state);
	return 0;
}

static const struct snd_kcontrol_new knot_controls[] = {
	SOC_SINGLE_BOOL_EXT("Analog Headphone", 0, audio_select_get, audio_select_put),
	SOC_SINGLE_BOOL_EXT("Speaker Enable", 0, spken_get, spken_put),
};

static int snd_x2500_probe(struct platform_device *pdev)
{
	struct device_node *snd_node = pdev->dev.of_node;
	struct snd_soc_card *card;
	struct snd_soc_dai_link *dai_link;
	enum of_gpio_flags flags;
	struct knot *machine;
	int num_links;
	int ret = 0, i;

	machine = devm_kzalloc(&pdev->dev,
			sizeof(struct knot), GFP_KERNEL);
	if (!machine) {
		dev_err(&pdev->dev, "Can't allocate knot\n");
		return -ENOMEM;
	}
	x2500_knot = machine;

	machine->gpio_audio_select = devm_gpiod_get_optional(&pdev->dev, "ingenic,audio-select", GPIOD_ASIS);
	if (IS_ERR_OR_NULL(machine->gpio_audio_select))
		machine->gpio_audio_select = NULL;

	machine->gpio_spken = devm_gpiod_get_optional(&pdev->dev, "ingenic,spken", GPIOD_OUT_HIGH);
	if (IS_ERR_OR_NULL(machine->gpio_spken))
		machine->gpio_spken = NULL;

	num_links = of_property_count_strings(snd_node, "ingenic,dai-link");
	if (num_links < 0)
		return num_links;
	BUG_ON(!num_links);

	card = (struct snd_soc_card *)devm_kzalloc(&pdev->dev,
			sizeof(struct snd_soc_card), GFP_KERNEL);
	if (!card)
		return -ENOMEM;
	knot_card = card;
	dai_link = (struct snd_soc_dai_link *)devm_kzalloc(&pdev->dev,
			sizeof(struct snd_soc_dai_link) * num_links, GFP_KERNEL);
	if (!dai_link)
		return -ENOMEM;
	//card->num_dapm_widgets = ARRAY_SIZE(x2500_dapm_widgets);
	//card->dapm_widgets = x2500_dapm_widgets;
	card->num_links = num_links;
	card->dai_link = dai_link;
	card->owner = THIS_MODULE;
	card->dev = &pdev->dev;
	card->controls = knot_controls,
	card->num_controls = ARRAY_SIZE(knot_controls),

	ret = snd_soc_of_parse_card_name(card, "ingenic,model");
	if (ret)
		return ret;
	/* ret = snd_soc_of_parse_audio_routing(card, "ingenic,audio-routing"); */
	/* if (ret) */
	/* 	return ret; */

	for (i = 0; i < card->num_links; i++) {
		dai_link[i].cpu_of_node = of_parse_phandle(snd_node, "ingenic,cpu-dai" , i);
		dai_link[i].platform_of_node = of_parse_phandle(snd_node, "ingenic,platform", i);
		dai_link[i].codec_of_node = of_parse_phandle(snd_node, "ingenic,codec", i);
		ret = of_property_read_string_index(snd_node, "ingenic,codec-dai", i,
				&(dai_link[i].codec_dai_name));
		if (ret || !dai_link[i].cpu_of_node ||
				!dai_link[i].codec_of_node ||
				!dai_link[i].platform_of_node)
			return -ENODEV;
		ret = of_property_read_string_index(snd_node, "ingenic,dai-link", i,
				&(dai_link[i].name));
		if (ret)
			return -ENODEV;
		ret = of_property_read_string_index(snd_node, "ingenic,stream", i,
				&(dai_link[i].stream_name));
		if (ret)
			dai_link[i].stream_name = dai_link[i].name;

		dev_dbg(&pdev->dev, "dai_link %s\n", dai_link[i].name);
		dev_dbg(&pdev->dev, "stream_name %s\n", dai_link[i].stream_name);
		dev_dbg(&pdev->dev, "cpu %s(%s)\n", dai_link[i].cpu_of_node->name,
				dai_link[i].cpu_of_node->full_name);
		dev_dbg(&pdev->dev, "platform %s(%s)\n", dai_link[i].platform_of_node->name,
				dai_link[i].platform_of_node->full_name);
		dev_dbg(&pdev->dev, "codec dai %s\n", dai_link[i].codec_dai_name);
		dev_dbg(&pdev->dev, "codec %s(%s)\n", dai_link[i].codec_of_node->name,
				dai_link[i].codec_of_node->full_name);


		if (!strcmp(dai_link[i].name, "i2s-ecodec") ||
				!strcmp(dai_link[i].codec_dai_name, "dmic")) {
			dai_link->ops = &x2500_i2s_ecodec_ops;
			dai_link->init = x2500_i2s_cdc_dai_link_init;
		}
	}

	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
		return ret;
	}

	snd_soc_card_set_drvdata(card, machine);
	platform_set_drvdata(pdev, card);
	dev_info(&pdev->dev, "Sound Card successed\n");
	return ret;
}

static int snd_x2500_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(knot_card);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id sound_dt_match[] = {
	{ .compatible = "ingenic,x2500-sound", .data = NULL },
	{},
};
MODULE_DEVICE_TABLE(of, sound_dt_match);

static struct platform_driver snd_x2500_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "x2500-sound",
		.pm = &snd_soc_pm_ops,
		.of_match_table = of_match_ptr(sound_dt_match),
	},
	.probe = snd_x2500_probe,
	.remove = snd_x2500_remove,
};
module_platform_driver(snd_x2500_driver);

MODULE_AUTHOR("zhxiao<zhihao.xiao@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC x2500 Snd Card");
MODULE_LICENSE("GPL");
