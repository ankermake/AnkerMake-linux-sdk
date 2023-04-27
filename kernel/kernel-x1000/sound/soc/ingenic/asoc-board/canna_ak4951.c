/*
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author: tjin <tao.jin@ingenic.com>
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
#include <linux/switch.h>
#include <linux/delay.h>
#include "board.h"

#define SPK_EN_GPIO	GPIO_PA(2)
#define SPK_EN_LEVEL	1

#define LINEIN_DETECT_GPIO	GPIO_PD(3)
#define LINEIN_INSERT_LEVEL	0

static struct snd_switch_data {
	struct switch_dev sdev;
	int irq;
	struct work_struct work;
	int gpio;
	int valid_level;
	int state;
}linein_switch;

unsigned long codec_sysclk = 24000000;
static int spk_en_power(struct snd_soc_dapm_widget *w,
						struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		if (SPK_EN_GPIO != -1){
			gpio_direction_output(SPK_EN_GPIO, SPK_EN_LEVEL);
			printk("gpio speaker enable %d\n", gpio_get_value(SPK_EN_GPIO));
		}
	} else {
		if (SPK_EN_GPIO != -1){
			gpio_direction_output(SPK_EN_GPIO, !SPK_EN_LEVEL);
			printk("gpio speaker disable %d\n", gpio_get_value(SPK_EN_GPIO));
		}
	}
	return 0;
}

void spk_mute(void)
{
	if (SPK_EN_GPIO != -1)
		gpio_direction_output(SPK_EN_GPIO, !SPK_EN_LEVEL);
	return;
}

void spk_unmute(void)
{
	if (SPK_EN_GPIO != -1)
		gpio_direction_output(SPK_EN_GPIO, SPK_EN_LEVEL);
	return;
}

static void set_switch_state(struct snd_switch_data *switch_data, int state)
{
	if (switch_data->state != state) {
		switch_set_state(&switch_data->sdev, state);
		switch_data->state = state;
	}
}

static void linein_switch_work(struct work_struct *linein_work)
{
	int i;
	int state = 0;
	int tmp_state = 0;
	int delay = 0;
	struct snd_switch_data *switch_data =
		container_of(linein_work, struct snd_switch_data, work);

	if(switch_data->state == 1){
        /*
         * The event of linein plugout should check more time to avoid frequently plug action.
         * You can change the delay time(ms) according to your needs.
         */
		delay = 20;
	}else{
        /*
         * The event of linein plugin should report immediately.
         */
		delay = 20;
	}

	state = gpio_get_value(switch_data->gpio);
	for (i = 0; i < 5; i++) {
		msleep(delay);
		tmp_state = gpio_get_value(switch_data->gpio);
		if (tmp_state != state) {
			i = -1;
			state = gpio_get_value(switch_data->gpio);
			continue;
		}
	}

	if (state == 1)
		irq_set_irq_type(switch_data->irq, IRQF_TRIGGER_LOW);
	else if (state == 0)
		irq_set_irq_type(switch_data->irq, IRQF_TRIGGER_HIGH);

	enable_irq(switch_data->irq);

	if (state == (int)switch_data->valid_level){
		state = 1;
	}else{
		state = 0;
	}

	set_switch_state(switch_data, state);
	return;
}

static irqreturn_t linein_irq_handler(int irq, void *dev_id)
{
	struct snd_switch_data *switch_data =
		(struct snd_switch_data *)dev_id;

	disable_irq_nosync(switch_data->irq);

	schedule_work(&switch_data->work);

	return IRQ_HANDLED;
}

static ssize_t switch_linein_print_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf,"%s.\n",sdev->name);
}

static ssize_t switch_linein_print_state(struct switch_dev *sdev, char *buf)
{
	char *state[2] = {"0", "1"};
	unsigned int state_val = switch_get_state(sdev);

	if (state_val == 1)
		return sprintf(buf, "%s\n", state[1]);
	else
		return sprintf(buf, "%s\n", state[0]);
}

int canna_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	/*FIXME snd_soc_dai_set_pll*/
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM);
	if (ret)
		return ret;
	ret = snd_soc_dai_set_sysclk(cpu_dai, 1, 24000000, SND_SOC_CLOCK_OUT);
	if (ret)
		return ret;
	return 0;
};

int canna_i2s_hw_free(struct snd_pcm_substream *substream)
{
	/*notify release pll*/
	return 0;
};

static struct snd_soc_ops canna_i2s_ops = {
//	.startup = phoenix_spk_sup,
//	.shutdown = phoenix_spk_sdown,
	.hw_params = canna_i2s_hw_params,
	.hw_free = canna_i2s_hw_free,

};

static const struct snd_soc_dapm_widget i2s_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker", spk_en_power),
	SND_SOC_DAPM_MIC("Mic", NULL),
};

static struct snd_soc_jack icdc_d3_hp_jack;
static struct snd_soc_jack_pin icdc_d3_hp_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};

static struct snd_soc_jack_gpio icdc_d3_jack_gpio[] = {
	{
		.name = "Headphone detection",
		.report = SND_JACK_HEADPHONE,
		.debounce_time = 150,
	}
};

/* machine audio_map */
static const struct snd_soc_dapm_route i2s_audio_map[] = {
	/* ext speaker connected to DAC out */
	{"Speaker", NULL, "DAC OUT"},

	/* mic is connected to ADC in */
	{"Mic", NULL, "ADC IN"},

};

static int i2s_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int err;

	err = snd_soc_dapm_new_controls(dapm, i2s_dapm_widgets,
									ARRAY_SIZE(i2s_dapm_widgets));
	if (err){
		printk("add dapm controls err!!\n");
		return err;
	}

	/* Set up specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, i2s_audio_map,
								  ARRAY_SIZE(i2s_audio_map));
	if (err){
		printk("add dapm routes err!!\n");
		return err;
	}

	snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &icdc_d3_hp_jack);
	snd_soc_jack_add_pins(&icdc_d3_hp_jack,ARRAY_SIZE(icdc_d3_hp_jack_pins), icdc_d3_hp_jack_pins);
#ifdef HAVE_HEADPHONE
	if (gpio_is_valid(DORADO_HP_DET)) {
		icdc_d3_jack_gpio[jack].gpio = PHOENIX_HP_DET;
		icdc_d3_jack_gpio[jack].invert = !PHOENIX_HP_DET_LEVEL;
		snd_soc_jack_add_gpios(&icdc_d3_hp_jack, 1, icdc_d3_jack_gpio);
	}
#else
	snd_soc_dapm_disable_pin(dapm, "Headphone Jack");
#endif

	/* snd_soc_dapm_enable_pin(dapm, "Speaker"); */
	snd_soc_dapm_force_enable_pin(dapm, "Speaker");
	snd_soc_dapm_force_enable_pin(dapm, "Mic");

	snd_soc_dapm_sync(dapm);
	return 0;
}

static struct snd_soc_dai_link soc_dais[] = {
	[0] = {
		.name = "aslmom ak4951",
		.stream_name = "aslmom ak4951",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = i2s_dai_link_init,
		.codec_dai_name = "ak4951-dai",
		.codec_name = "ak4951.0-0012",
		.ops = &canna_i2s_ops,
	},
	[1] = {
		.name = "aslmom PCMBT",
		.stream_name = "aslmom PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
	[2] = {
		.name = "aslmom DMIC",
		.stream_name = "aslmom DMIC",
		.platform_name = "jz-asoc-dmic-dma",
		.cpu_dai_name = "jz-asoc-dmic",
		.codec_dai_name = "dmic-codec-hifi",
		.codec_name = "jz-asoc-dmic",
	},

};

static struct snd_soc_card aslmom_card = {
	.name = UNI_SOUND_CARD_NAME,
	.owner = THIS_MODULE,
	.dai_link = soc_dais,
	.num_links = ARRAY_SIZE(soc_dais),
};

static struct platform_device *soc_audio_device;

static int board_extcodec_init(void)
{
	/*struct jz_aic_gpio_func *gpio_info;*/
	int ret;

	linein_switch.sdev.name = "linein";
	linein_switch.sdev.print_state = switch_linein_print_state;
	linein_switch.sdev.print_name  = switch_linein_print_name;
	linein_switch.state = -1;

	ret = switch_dev_register(&linein_switch.sdev);
	if (ret < 0) {
		printk("linein switch dev register fail.\n");
		return ret;
	}

	/* linein detect register */
	if (LINEIN_DETECT_GPIO != -1 && LINEIN_INSERT_LEVEL != -1) {
		linein_switch.gpio = LINEIN_DETECT_GPIO;
		linein_switch.valid_level = LINEIN_INSERT_LEVEL;
		if (!gpio_is_valid(linein_switch.gpio)){
			printk("linein detect gpio error.\n");
			goto err_request_linein_gpio;
		}

		ret = gpio_request(linein_switch.gpio, "linein_detect");
		if (ret < 0)
			goto err_request_linein_gpio;

		linein_switch.irq = gpio_to_irq(linein_switch.gpio);
		if (linein_switch.irq < 0) {
			printk("get linein_irq error.\n");
			ret = linein_switch.irq;
			goto err_linein_irq_num_failed;
		}

		INIT_WORK(&linein_switch.work, linein_switch_work);

		ret = request_irq(linein_switch.irq, linein_irq_handler,
						  IRQF_TRIGGER_FALLING, "linein_detect", &linein_switch);
		if (ret < 0) {
			printk("request linein detect irq fail.\n");
			goto err_request_linein_irq;
		}
		disable_irq(linein_switch.irq);

		linein_switch_work(&linein_switch.work);
	} else {
		linein_switch.irq = -1;
	}

	if (SPK_EN_GPIO != -1) {
		ret = gpio_request(SPK_EN_GPIO, "Speaker_en");
		if (ret)
			goto err_request_spk_gpio;
	}
	soc_audio_device = platform_device_alloc("soc-audio", -1);
	if (!soc_audio_device) {
		ret = -ENOMEM;
		goto err_platform_device_alloc;
	}

	platform_set_drvdata(soc_audio_device, &aslmom_card);
	ret = platform_device_add(soc_audio_device);
	if (ret)
		goto err_platform_device_add;

	dev_info(&soc_audio_device->dev, "Alsa sound card init ok!!!\n");
	return ret;

err_platform_device_add:
	platform_device_put(soc_audio_device);
err_platform_device_alloc:
	if (SPK_EN_GPIO != -1)
		gpio_free(SPK_EN_GPIO);
err_request_spk_gpio:
err_request_linein_irq:
err_linein_irq_num_failed:
	if (LINEIN_DETECT_GPIO != -1)
		gpio_free(LINEIN_DETECT_GPIO);
err_request_linein_gpio:
	switch_dev_unregister(&linein_switch.sdev);
	return ret;
}

static void board_extcodec_exit(void)
{
	platform_device_unregister(soc_audio_device);
	if (SPK_EN_GPIO != -1)
		gpio_free(SPK_EN_GPIO);

	if(LINEIN_DETECT_GPIO != -1){
		cancel_work_sync(&linein_switch.work);
		gpio_free(LINEIN_DETECT_GPIO);
	}

	switch_dev_unregister(&linein_switch.sdev);
}

module_init(board_extcodec_init);
module_exit(board_extcodec_exit);

MODULE_AUTHOR("tjin<tao.jin@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC Aslmom Snd Card");
MODULE_LICENSE("GPL");




/* /\* */
/*  * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd. */
/*  *	http://www.ingenic.com */
/*  * Author: cli <chen.li@ingenic.com> */
/*  * */
/*  * This program is free software; you can redistribute it and/or modify */
/*  * it under the terms of the GNU General Public License as published by */
/*  * the Free Software Foundation; either version 2 of the License, or */
/*  * (at your option) any later version. */
/*  * */
/*  * This program is distributed in the hope that it will be useful, */
/*  * but WITHOUT ANY WARRANTY; without even the implied warranty of */
/*  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the */
/*  * GNU General Public License for more details. */
/*  * */
/*  * You should have received a copy of the GNU General Public License */
/*  * along with this program; if not, write to the Free Software */
/*  * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */
/*  * */
/*  *\/ */

/* #include <linux/module.h> */
/* #include <linux/init.h> */
/* #include <linux/platform_device.h> */
/* #include <sound/soc.h> */
/* #include <sound/jack.h> */
/* #include <linux/gpio.h> */
/* #include <mach/jzsnd.h> */
/* #include "../icodec/icdc_d3.h" */

/* static struct snd_codec_data *codec_pdata = NULL; */

/* unsigned long codec_sysclk = -1; */
/* static int phoenix_spk_power(struct snd_soc_dapm_widget *w, */
/* 							 struct snd_kcontrol *kcontrol, int event) */
/* { */
/* 	if (SND_SOC_DAPM_EVENT_ON(event)) { */
/* 		if (codec_pdata && (codec_pdata->gpio_spk_en.gpio) != -1) { */
/* 			gpio_direction_output(codec_pdata->gpio_spk_en.gpio, codec_pdata->gpio_spk_en.active_level); */
/* 			printk("gpio speaker enable %d\n", gpio_get_value(codec_pdata->gpio_spk_en.gpio)); */
/* 		} else */
/* 			printk("set speaker enable failed. please check codec_pdata\n"); */
/* 	} else { */
/* 		if (codec_pdata && (codec_pdata->gpio_spk_en.gpio) != -1) { */
/* 			gpio_direction_output(codec_pdata->gpio_spk_en.gpio, !(codec_pdata->gpio_spk_en.active_level)); */
/* 			printk("gpio speaker disable %d\n", gpio_get_value(codec_pdata->gpio_spk_en.gpio)); */
/* 		} else */
/* 			printk("set speaker disable failed. please check codec_pdata\n"); */
/* 	} */
/* 	return 0; */
/* } */

/* void phoenix_spk_sdown(struct snd_pcm_substream *sps){ */

/* 	if (codec_pdata && (codec_pdata->gpio_spk_en.gpio) != -1) { */
/* 		gpio_direction_output(codec_pdata->gpio_spk_en.gpio, !(codec_pdata->gpio_spk_en.active_level)); */
/* 	} */

/* 	return; */
/* } */

/* int phoenix_spk_sup(struct snd_pcm_substream *sps){ */
/* 	if (codec_pdata && (codec_pdata->gpio_spk_en.gpio) != -1) { */
/* 		gpio_direction_output(codec_pdata->gpio_spk_en.gpio, codec_pdata->gpio_spk_en.active_level); */
/* 	} */
/* 	return 0; */
/* } */

/* int phoenix_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) { */
/* 	struct snd_soc_pcm_runtime *rtd = substream->private_data; */
/* 	struct snd_soc_dai *cpu_dai = rtd->cpu_dai; */
/* 	int ret; */

/* 	/\*FIXME snd_soc_dai_set_pll*\/ */
/* 	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM); */
/* 	if (ret) */
/* 		return ret; */
/* 	ret = snd_soc_dai_set_sysclk(cpu_dai, JZ_I2S_INNER_CODEC, 24000000, SND_SOC_CLOCK_OUT); */
/* 	if (ret) */
/* 		return ret; */
/* 	return 0; */
/* }; */

/* int phoenix_i2s_hw_free(struct snd_pcm_substream *substream) */
/* { */
/* 	/\*notify release pll*\/ */
/* 	return 0; */
/* }; */


/* static struct snd_soc_ops phoenix_i2s_ops = { */
/* 	.startup = phoenix_spk_sup, */
/* 	.shutdown = phoenix_spk_sdown, */
/* 	.hw_params = phoenix_i2s_hw_params, */
/* 	.hw_free = phoenix_i2s_hw_free, */

/* }; */
/* static const struct snd_soc_dapm_widget phoenix_dapm_widgets[] = { */
/* 	SND_SOC_DAPM_HP("Headphone Jack", NULL), */
/* 	SND_SOC_DAPM_SPK("Speaker", phoenix_spk_power), */
/* 	SND_SOC_DAPM_MIC("Mic Buildin", NULL), */
/* 	SND_SOC_DAPM_MIC("DMic", NULL), */
/* }; */

/* static struct snd_soc_jack phoenix_icdc_d3_hp_jack; */
/* static struct snd_soc_jack_pin phoenix_icdc_d3_hp_jack_pins[] = { */
/* 	{ */
/* 		.pin = "Headphone Jack", */
/* 		.mask = SND_JACK_HEADPHONE, */
/* 	}, */
/* }; */

/* #ifdef HAVE_HEADPHONE */
/* static struct snd_soc_jack_gpio phoenix_icdc_d3_jack_gpio[] = { */
/* 	{ */
/* 		.name = "Headphone detection", */
/* 		.report = SND_JACK_HEADPHONE, */
/* 		.debounce_time = 150, */
/* 	} */
/* }; */
/* #endif */

/* /\* phoenix machine audio_map *\/ */
/* static const struct snd_soc_dapm_route audio_map[] = { */
/* 	/\* ext speaker connected to DO_LO_PWM  *\/ */
/* 	{"Speaker", NULL, "DO_LO_PWM"}, */

/* 	/\* mic is connected to AIP/N1 *\/ */
/* 	{"MICBIAS", NULL, "Mic Buildin"}, */
/* 	{"DMIC", NULL, "DMic"}, */

/* }; */

/* static int board_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd) */
/* { */
/* 	struct snd_soc_codec *codec = rtd->codec; */
/* 	struct snd_soc_dapm_context *dapm = &codec->dapm; */
/* 	struct snd_soc_card *card = rtd->card; */
/* 	int err; */

/* 	if ((codec_pdata) && ((codec_pdata->gpio_spk_en.gpio) != -1)) { */
/* 		err = devm_gpio_request(card->dev, codec_pdata->gpio_spk_en.gpio, "Speaker_en"); */
/* 		if (err) */
/* 			return err; */
/* 	} else { */
/* 		pr_err("codec_pdata gpio_spk_en is NULL\n"); */
/* 		return err; */
/* 	} */
/* 	err = snd_soc_dapm_new_controls(dapm, phoenix_dapm_widgets, */
/* 									ARRAY_SIZE(phoenix_dapm_widgets)); */
/* 	if (err){ */
/* 		printk("phoenix dai add controls err!!\n"); */
/* 		return err; */
/* 	} */
/* 	/\* Set up rx1950 specific audio path audio_mapnects *\/ */
/* 	err = snd_soc_dapm_add_routes(dapm, audio_map, */
/* 								  ARRAY_SIZE(audio_map)); */
/* 	if (err){ */
/* 		printk("add phoenix dai routes err!!\n"); */
/* 		return err; */
/* 	} */
/* 	snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &phoenix_icdc_d3_hp_jack); */
/* 	snd_soc_jack_add_pins(&phoenix_icdc_d3_hp_jack,ARRAY_SIZE(phoenix_icdc_d3_hp_jack_pins),phoenix_icdc_d3_hp_jack_pins); */
/* #ifdef HAVE_HEADPHONE */
/* 	if (gpio_is_valid(DORADO_HP_DET)) { */
/* 		phoenix_icdc_d3_jack_gpio[jack].gpio = PHOENIX_HP_DET; */
/* 		phoenix_icdc_d3_jack_gpio[jack].invert = !PHOENIX_HP_DET_LEVEL; */
/* 		snd_soc_jack_add_gpios(&phoenix_icdc_d3_hp_jack, 1, phoenix_icdc_d3_jack_gpio); */
/* 	} */
/* #else */
/* 	snd_soc_dapm_disable_pin(dapm, "Headphone Jack"); */
/* #endif */

/* 	snd_soc_dapm_enable_pin(dapm, "Speaker"); */
/* 	snd_soc_dapm_enable_pin(dapm, "Mic Buildin"); */
/* 	snd_soc_dapm_enable_pin(dapm, "DMic"); */

/* 	snd_soc_dapm_sync(dapm); */
/* 	return 0; */
/* } */

/* static struct snd_soc_dai_link board_card_dais[] = { */
/* 	[0] = { */
/* 		.name = "phoenix ICDC", */
/* 		.stream_name = "phoenix ICDC", */
/* 		.platform_name = "jz-asoc-aic-dma", */
/* 		.cpu_dai_name = "jz-asoc-aic-i2s", */
/* 		.init = phoenix_dlv_dai_link_init, */
/* 		.codec_dai_name = "icdc-d3-hifi", */
/* 		.codec_name = "icdc-d3", */
/* 		.ops = &phoenix_i2s_ops, */
/* 	}, */
/* 	[1] = { */
/* 		.name = "phoenix PCMBT", */
/* 		.stream_name = "phoenix PCMBT", */
/* 		.platform_name = "jz-asoc-pcm-dma", */
/* 		.cpu_dai_name = "jz-asoc-pcm", */
/* 		.codec_dai_name = "pcm dump dai", */
/* 		.codec_name = "pcm dump", */
/* 	}, */

/* #ifdef CONFIG_SND_ASOC_JZ_DMIC_MODULE */
/* 	[2] = { */
/* 		.name = "PHOENIX DMIC", */
/* 		.stream_name = "PHOENIX DMIC", */
/* 		.platform_name = "jz-asoc-dmic-module-dma", */
/* 		.cpu_dai_name = "jz-asoc-dmic-module", */
/* 		.codec_dai_name = "dmic dump dai", */
/* 		.codec_name = "dmic dump", */
/* 	}, */
/* #else */
/* 	[2] = { */
/* 		.name = "PHOENIX DMIC", */
/* 		.stream_name = "PHOENIX DMIC", */
/* 		.platform_name = "jz-asoc-dmic-dma", */
/* 		.cpu_dai_name = "jz-asoc-dmic", */
/* 		.codec_dai_name = "dmic dump dai", */
/* 		.codec_name = "dmic dump", */
/* 	}, */

/* #endif */

/* }; */

/* static struct snd_soc_card board_card = { */
/* 	.name = "ak", */
/* 	.owner = THIS_MODULE, */
/* 	.dai_link = board_dais, */
/* 	.num_links = ARRAY_SIZE(board_dais), */
/* }; */

/* static int snd_board_probe(struct platform_device *pdev) */
/* { */
/* 	int ret = 0; */
/* 	board_card.dev = &pdev->dev; */
/* 	codec_pdata = (struct snd_codec_data *)board_card.dev->platform_data; */
/* 	ret = snd_soc_register_card(&board_card); */
/* 	if (ret) */
/* 		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret); */
/* 	return ret; */
/* } */

/* static int snd_board_remove(struct platform_device *pdev) */
/* { */
/* 	snd_soc_unregister_card(&phoenix); */
/* 	platform_set_drvdata(pdev, NULL); */
/* 	return 0; */
/* } */

/* static struct platform_driver snd_board_driver = { */
/* 	.driver = { */
/* 		.owner = THIS_MODULE, */
/* 		.name = "ingenic-alsa", */
/* 		.pm = &snd_soc_pm_ops, */
/* 	}, */
/* 	.probe = snd_board_probe, */
/* 	.remove = snd_board_remove, */
/* }; */
/* module_platform_driver(snd_board_driver); */

/* MODULE_AUTHOR("bo.liu<bo.liu@ingenic.com>"); */
/* MODULE_DESCRIPTION("ALSA SoC Snd Card"); */
/* MODULE_LICENSE("GPL"); */
