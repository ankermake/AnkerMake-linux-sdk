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
#include "board.h"
#include "../asoc-v12/asoc-aic-v12.h"

int pd_x1830_audio_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
        struct snd_soc_pcm_runtime *rtd = substream->private_data;
        struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
        int ret, rate = params_rate(params);
        
        ret = snd_soc_dai_set_sysclk(cpu_dai, JZ_I2S_EX_CODEC, rate * 256, SND_SOC_CLOCK_OUT);
        if (ret)
                return ret;
        ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, 256);
        if (ret)
                return ret;
        return 0;
};

static struct snd_soc_ops pd_x1830_audio_i2s_ops = {
	.hw_params = pd_x1830_audio_i2s_hw_params,
};

static struct snd_soc_dai_link pd_x1830_audio_dais[] = {
	[0] = {
		.name = "PDX1830 SY6026L",
		.stream_name = "PDX1830 SY6026L",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.codec_dai_name = "sy6026l-hifi",
		.codec_name = "sy6026l.1-002a",
		.ops = &pd_x1830_audio_i2s_ops,
                .dai_fmt = SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBS_CFS,
	},
};

static struct snd_soc_card pd_x1830_audio = {
	.name = UNI_SOUND_CARD_NAME,
	.owner = THIS_MODULE,
	.dai_link = pd_x1830_audio_dais,
	.num_links = ARRAY_SIZE(pd_x1830_audio_dais),
};

static int snd_pd_x1830_audio_probe(struct platform_device *pdev)
{
	int ret = 0;
	
        pd_x1830_audio.dev = &pdev->dev;
	ret = snd_soc_register_card(&pd_x1830_audio);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
	return ret;
}

static int snd_pd_x1830_audio_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&pd_x1830_audio);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver snd_pd_x1830_audio_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-alsa",
		.pm = &snd_soc_pm_ops,
	},
	.probe = snd_pd_x1830_audio_probe,
	.remove = snd_pd_x1830_audio_remove,
};
module_platform_driver(snd_pd_x1830_audio_driver);

MODULE_AUTHOR("chen.li<chen.li@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC PDX1830_audio Snd Card");
MODULE_LICENSE("GPL");
