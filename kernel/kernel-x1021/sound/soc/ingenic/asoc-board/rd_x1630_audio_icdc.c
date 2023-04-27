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
#include "../asoc-v12/asoc-aic-v12.h"

#define PD_X1630_SPEAKER_EN     GPIO_PB(31)
#define PD_X1630_SPEAKER_LEVEL  1

static bool speaker_is_on = false;
static DEFINE_MUTEX(speaker_state_mutex);

static int spk_switch_unlock(bool is_on)
{
	if (is_on) {
		gpio_direction_output(PD_X1630_SPEAKER_EN, PD_X1630_SPEAKER_LEVEL);
		printk("gpio speaker enable %d\n", gpio_get_value(PD_X1630_SPEAKER_EN));
	} else {
		speaker_is_on = false;
		gpio_direction_output(PD_X1630_SPEAKER_EN, !PD_X1630_SPEAKER_LEVEL);
		printk("gpio speaker disable %d\n", gpio_get_value(PD_X1630_SPEAKER_EN));
	}
	return 0;
}

static int pd_x1630_audio_spk_power(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	int ret;
	if (!PD_X1630_SPEAKER_EN)
		return 0;

	mutex_lock(&speaker_state_mutex);
	speaker_is_on = !!SND_SOC_DAPM_EVENT_ON(event);
	ret = spk_switch_unlock(speaker_is_on);
	mutex_unlock(&speaker_state_mutex);
	return ret;
}

int pd_x1630_audio_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret, rate = params_rate(params);

	ret = snd_soc_dai_set_sysclk(cpu_dai, JZ_I2S_INNER_CODEC, rate * 256, SND_SOC_CLOCK_OUT);
	if (ret)
		return ret;
	return 0;
};

static struct snd_soc_ops pd_x1630_audio_i2s_ops = {
	.hw_params = pd_x1630_audio_i2s_hw_params,
};

static const struct snd_soc_dapm_widget pd_x1630_audio_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Speaker", pd_x1630_audio_spk_power),
	SND_SOC_DAPM_MIC("Buildin-mic", NULL),
	SND_SOC_DAPM_MIC("Digital-mic", NULL),
};

/* pd_x1630_audio machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	{"Speaker", NULL, "HPOUT"},
	{"MICP", NULL, "Buildin-mic"},
	{"MICN", NULL, "Buildin-mic"},
	{"AIP", NULL, "Digital-mic"},
};

static int pd_x1630_audio_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = rtd->card;
	int err;
	if (PD_X1630_SPEAKER_EN) {
		err = devm_gpio_request(card->dev, PD_X1630_SPEAKER_EN, "speaker_en");
		if (err)
			return err;
	}
	err = snd_soc_dapm_new_controls(dapm, pd_x1630_audio_dapm_widgets,
		ARRAY_SIZE(pd_x1630_audio_dapm_widgets));
	if (err){
		printk("pd_x1630_audio dai add controls err!!\n");
		return err;
	}
	/* Set up rx1950 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
			ARRAY_SIZE(audio_map));
	if (err){
		printk("add pd_x1630_audio dai routes err!!\n");
		return err;
	}
	snd_soc_dapm_enable_pin(dapm, "Speaker");
	snd_soc_dapm_enable_pin(dapm, "Buildin-mic");
	snd_soc_dapm_enable_pin(dapm, "Digital-mic");
	snd_soc_dapm_sync(dapm);
	return 0;
}

static struct snd_soc_dai_link pd_x1630_audio_dais[] = {
	[0] = {
		.name = "pd_x1630_audio ICDC",
		.stream_name = "pd_x1630_audio ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = pd_x1630_audio_dlv_dai_link_init,
		.codec_dai_name = "icdc-d4-hifi",
		.codec_name = "icdc-d4",
                .dai_fmt = SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM,
		.ops = &pd_x1630_audio_i2s_ops,
	},
	[1] = {
		.name = "canna DMIC",
		.stream_name = "canna DMIC",
		.platform_name = "jz-asoc-dmic-dma",
		.cpu_dai_name = "jz-asoc-dmic",
		.codec_dai_name = "dmic-codec-hifi",
		.codec_name = "jz-asoc-dmic",
	},

};

static struct snd_soc_card pd_x1630_audio = {
	.name = "audiosound",
	.owner = THIS_MODULE,
	.dai_link = pd_x1630_audio_dais,
	.num_links = ARRAY_SIZE(pd_x1630_audio_dais),
};

void speaker_close_delaywork(struct work_struct *work)
{
	mutex_lock(&speaker_state_mutex);
	if (!speaker_is_on)
		spk_switch_unlock(speaker_is_on);
	mutex_unlock(&speaker_state_mutex);
}
static DECLARE_DELAYED_WORK(speaker_work, speaker_close_delaywork);

static int snd_pd_x1630_audio_probe(struct platform_device *pdev)
{
	int ret = 0;

        pd_x1630_audio.dev = &pdev->dev;
	ret = snd_soc_register_card(&pd_x1630_audio);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
	/*
	 * pd x1630 audio board's SD card cd mode pin and speaker en pin
	 * was conflict, To prevent SPI from initializing the SD card,
	 * this pin should maintain high level during SD card initialization.
	 */
	schedule_delayed_work(&speaker_work, msecs_to_jiffies(5000));
	return ret;
}

static int snd_pd_x1630_audio_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&pd_x1630_audio);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver snd_pd_x1630_audio_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-alsa",
		.pm = &snd_soc_pm_ops,
	},
	.probe = snd_pd_x1630_audio_probe,
	.remove = snd_pd_x1630_audio_remove,
};
module_platform_driver(snd_pd_x1630_audio_driver);

MODULE_AUTHOR("chen.li<chen.li@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC PDX1630_audio Snd Card");
MODULE_LICENSE("GPL");
