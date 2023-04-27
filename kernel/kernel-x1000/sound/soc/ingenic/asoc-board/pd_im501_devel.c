/*
 * Copyright (C) 2017 Ingenic Semiconductor Co., Ltd.
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
#include "board.h"
#include "../asoc-v12/asoc-aic-v12.h"

static int suning_alarm_spk_power(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	return jz_speaker_enable(!!SND_SOC_DAPM_EVENT_ON(event));
}

int suning_alarm_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
        int ret, rate = params_rate(params);

	ret = snd_soc_dai_set_sysclk(cpu_dai, JZ_I2S_INNER_CODEC, rate * 256, SND_SOC_CLOCK_OUT);
	if (ret)
		return ret;
	return 0;
};

static struct snd_soc_ops suning_alarm_i2s_ops = {
	.hw_params = suning_alarm_i2s_hw_params,
};

static const struct snd_soc_dapm_widget suning_alarm_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Speaker", suning_alarm_spk_power),
	SND_SOC_DAPM_MIC("Buildin-mic", NULL),
	SND_SOC_DAPM_MIC("Digital-mic", NULL),
};

/* suning_alarm machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	{"Speaker", NULL, "HPOUT"},
	{"MICP", NULL, "Buildin-mic"},
	{"MICN", NULL, "Buildin-mic"},
	{"AIP", NULL, "Digital-mic"},
};

static int suning_alarm_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = rtd->card;
	int err;
	err = jz_speaker_init(card->dev);
	if (err)
		return err;
        err = snd_soc_dapm_new_controls(dapm, suning_alarm_dapm_widgets,
			ARRAY_SIZE(suning_alarm_dapm_widgets));
	if (err){
		printk("suning_alarm dai add controls err!!\n");
		return err;
	}
	/* Set up rx1950 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
			ARRAY_SIZE(audio_map));
	if (err){
		printk("add suning_alarm dai routes err!!\n");
		return err;
	}
	snd_soc_dapm_enable_pin(dapm, "Speaker");
	snd_soc_dapm_enable_pin(dapm, "Buildin-mic");
	snd_soc_dapm_enable_pin(dapm, "Digital-mic");
	snd_soc_dapm_sync(dapm);
	return 0;
}

static struct snd_soc_dai_link suning_alarm_dais[] = {
	[0] = {
		.name = "suning_alarm ICDC",
		.stream_name = "suning_alarm ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = suning_alarm_dlv_dai_link_init,
		.codec_dai_name = "icdc-d4-hifi",
		.codec_name = "icdc-d4",
                .dai_fmt = SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM,
		.ops = &suning_alarm_i2s_ops,
	},
	[1] = {
		.name = "DMIC",
		.stream_name = "DMIC",
		.platform_name = "jz-asoc-dmic-dma",
		.cpu_dai_name = "jz-asoc-dmic",
		.codec_dai_name = "dmic-codec-hifi",
		.codec_name = "jz-asoc-dmic",
	},
};

static struct snd_soc_card suning_alarm = {
	.name =  UNI_INNER_SOUND_CARD_NAME,
	.owner = THIS_MODULE,
	.dai_link = suning_alarm_dais,
	.num_links = ARRAY_SIZE(suning_alarm_dais),
};

static int snd_suning_alarm_probe(struct platform_device *pdev)
{
	int ret = 0;

        suning_alarm.dev = &pdev->dev;
	ret = snd_soc_register_card(&suning_alarm);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
	return ret;
}

static int snd_suning_alarm_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&suning_alarm);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver snd_suning_alarm_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-alsa",
		.pm = &snd_soc_pm_ops,
	},
	.probe = snd_suning_alarm_probe,
	.remove = snd_suning_alarm_remove,
};
module_platform_driver(snd_suning_alarm_driver);

MODULE_AUTHOR("chen.li<chen.li@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC suning Snd Card");
MODULE_LICENSE("GPL");
