/*
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author: bo.liu <bo.liu@ingenic.com>
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
#include <mach/jzsnd.h>

static struct snd_switch_data {
	struct switch_dev sdev;
	int irq;
	struct work_struct work;
	int gpio;
	int valid_level;
	int state;
}lineout_switch;

unsigned long codec_sysclk = 24000000;
unsigned int spk_force_mute = 0;
static struct snd_codec_data *snd_pdata = NULL;

#define GPIO_SPEAKER (snd_pdata->gpio_spk_en)
#define GPIO_DMIC (snd_pdata->gpio_dmic_en)
#define GPIO_HP (snd_pdata->gpio_hp_detect)

/* #define USE_JACK_FOR_HP_DETECT */

void spk_mute(unsigned int force)
{
	if (GPIO_SPEAKER.gpio != -1)
		gpio_direction_output(GPIO_SPEAKER.gpio, !GPIO_SPEAKER.active_level);

	if (force)
		spk_force_mute = force;
	return;
}

void spk_unmute(unsigned int force)
{
	if (GPIO_SPEAKER.gpio != -1)
		gpio_direction_output(GPIO_SPEAKER.gpio, GPIO_SPEAKER.active_level);

	if (force)
		spk_force_mute = force;
	return;
}

void mic_mute(void)
{
	if (GPIO_DMIC.gpio != -1)
		gpio_direction_output(GPIO_DMIC.gpio, !GPIO_DMIC.active_level);
	return;
}

void mic_unmute(void)
{
	if (GPIO_DMIC.gpio != -1)
		gpio_direction_output(GPIO_DMIC.gpio, GPIO_DMIC.active_level);
	return;
}

static int spk_en_power(struct snd_soc_dapm_widget *w,
						struct snd_kcontrol *kcontrol, int event)
{
	if (spk_force_mute == 2)
		return 0;

	if (SND_SOC_DAPM_EVENT_ON(event)) {
		spk_unmute(0);
	} else {
		spk_mute(0);
	}
	return 0;
}

static int mic_en_power(struct snd_soc_dapm_widget *w, struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		mic_unmute();
	} else {
		mic_mute();
	}
	return 0;
}

static void set_switch_state(struct snd_switch_data *switch_data, int state)
{
	if (switch_data->state != state) {
		switch_set_state(&switch_data->sdev, state);
		switch_data->state = state;
	}
}

static void lineout_switch_work(struct work_struct *lineout_work)
{
	int i;
	int state = 0;
	int tmp_state = 0;
	int delay = 0;
	struct snd_switch_data *switch_data =
		container_of(lineout_work, struct snd_switch_data, work);

	if(switch_data->state == 1){
        /*
         * The event of lineout plugout should check more time to avoid frequently plug action.
         * You can change the delay time(ms) according to your needs.
         */
		delay = 20;
	}else{
        /*
         * The event of lineout plugin should report immediately.
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

static irqreturn_t lineout_irq_handler(int irq, void *dev_id)
{
	struct snd_switch_data *switch_data =
		(struct snd_switch_data *)dev_id;

	disable_irq_nosync(switch_data->irq);

	schedule_work(&switch_data->work);

	return IRQ_HANDLED;
}

static ssize_t switch_lineout_print_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf,"%s.\n",sdev->name);
}

static ssize_t switch_lineout_print_state(struct switch_dev *sdev, char *buf)
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

static const struct snd_soc_dapm_widget dmic_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("MIC_POWER", mic_en_power),
};

static const struct snd_soc_dapm_route dmic_audio_map[] = {
	{"AIP", NULL, "MIC_POWER"},
};
static struct snd_soc_jack icdc_d3_hp_jack;
static struct snd_soc_jack_pin icdc_d3_hp_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};

#ifdef USE_JACK_FOR_HP_DETECT
static struct snd_soc_jack_gpio icdc_d3_jack_gpio[] = {
	{
		.name = "Headphone detection",
		.report = SND_JACK_HEADPHONE,
		.debounce_time = 150,
	}
};
#endif

/* machine audio_map */
static const struct snd_soc_dapm_route i2s_audio_map[] = {
	/* ext speaker connected to DAC out */
	{"Speaker", NULL, "SPK OUT"},

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
#ifdef USE_JACK_FOR_HP_DETECT
	if (gpio_is_valid(GPIO_HP.gpio)) {
		icdc_d3_jack_gpio[0].gpio = GPIO_HP.gpio;
		icdc_d3_jack_gpio[0].invert = !GPIO_HP.active_level;
		snd_soc_jack_add_gpios(&icdc_d3_hp_jack, ARRAY_SIZE(icdc_d3_jack_gpio), icdc_d3_jack_gpio);
	}
#else
	snd_soc_dapm_disable_pin(dapm, "Headphone Jack");
#endif

	snd_soc_dapm_enable_pin(dapm, "Speaker");
	/* snd_soc_dapm_force_enable_pin(dapm, "Speaker"); */
	snd_soc_dapm_force_enable_pin(dapm, "Mic");

	snd_soc_dapm_sync(dapm);
	return 0;
}

static int dmic_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int err;

	err = snd_soc_dapm_new_controls(dapm, dmic_dapm_widgets, ARRAY_SIZE(dmic_dapm_widgets));
	if (err){
		printk("add dapm controls err!!\n");
		return err;
	}

	err = snd_soc_dapm_add_routes(dapm, dmic_audio_map, ARRAY_SIZE(dmic_audio_map));
	if (err){
		printk("add dapm routes err!!\n");
		return err;
	}
	return 0;
}

static struct snd_soc_dai_link soc_dais[] = {
	[0] = {
		.name           = "d0085 ak4951",
		.stream_name    = "d0085 ak4951",
		.platform_name  = "jz-asoc-aic-dma",
		.cpu_dai_name   = "jz-asoc-aic-i2s",
		.init           = i2s_dai_link_init,
		.codec_dai_name = "ak4951-dai",
		.codec_name     = "ak4951.0-0012",
		.ops            = &canna_i2s_ops,
	},
	[1] = {
		.name           = "d0085 PCMBT",
		.stream_name    = "d0085 PCMBT",
		.platform_name  = "jz-asoc-pcm-dma",
		.cpu_dai_name   = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name     = "pcm dump",
	},
	[2] = {
		.name           = "d0085 DMIC",
		.stream_name    = "d0085 DMIC",
		.platform_name  = "jz-asoc-dmic-dma",
		.cpu_dai_name   = "jz-asoc-dmic",
		.init           = dmic_dai_link_init,
		.codec_dai_name = "dmic-codec-hifi",
		.codec_name     = "jz-asoc-dmic",
	},

};

static struct snd_soc_card board_card = {
	.name = "d0085",
	.owner = THIS_MODULE,
	.dai_link = soc_dais,
	.num_links = ARRAY_SIZE(soc_dais),
};

static int snd_d0085_probe(struct platform_device *pdev)
{
	int ret;

	board_card.dev = &pdev->dev;
	snd_pdata = (struct snd_codec_data*)pdev->dev.platform_data;

#ifndef USE_JACK_FOR_HP_DETECT
	/* lineout detect register */
	if (gpio_is_valid(GPIO_HP.gpio)) {
		lineout_switch.sdev.name = "lineout";
		lineout_switch.sdev.print_state = switch_lineout_print_state;
		lineout_switch.sdev.print_name  = switch_lineout_print_name;
		lineout_switch.state = -1;
		lineout_switch.gpio = GPIO_HP.gpio;
		lineout_switch.valid_level = GPIO_HP.active_level;

		/* register switch device */
		ret = switch_dev_register(&lineout_switch.sdev);
		if (ret < 0) {
			printk("lineout switch dev register fail.\n");
			return ret;
		}

		/* request lineout detect gpio */
		ret = gpio_request(lineout_switch.gpio, "lineout_detect");
		if (ret < 0)
			goto err_request_lineout_gpio;

		/* request lineout detect irq */
		lineout_switch.irq = gpio_to_irq(lineout_switch.gpio);
		if (lineout_switch.irq < 0) {
			printk("get lineout_irq error.\n");
			ret = lineout_switch.irq;
			goto err_lineout_irq_num_failed;
		}
		ret = request_irq(lineout_switch.irq, lineout_irq_handler,
						  IRQF_TRIGGER_FALLING, "lineout_detect", &lineout_switch);
		if (ret < 0) {
			printk("request lineout detect irq fail.\n");
			goto err_request_lineout_irq;
		}
		disable_irq(lineout_switch.irq);

		INIT_WORK(&lineout_switch.work, lineout_switch_work);
		lineout_switch_work(&lineout_switch.work);
	} else {
		lineout_switch.irq = -1;
	}
#endif

	if (gpio_is_valid(GPIO_SPEAKER.gpio)) {
		ret = gpio_request(GPIO_SPEAKER.gpio, "Speaker_en");
		if (ret)
			goto err_request_gpio;
	}

	if (gpio_is_valid(GPIO_DMIC.gpio)) {
		ret = gpio_request(GPIO_DMIC.gpio, "Mic_en");
		if (ret)
			goto err_request_gpio;
	}

	ret = snd_soc_register_card(&board_card);
	if (ret < 0) {
		goto err_register_card;
	}

	dev_info(&pdev->dev, "Alsa sound card init ok!!!\n");
	return ret;

err_register_card:
err_request_gpio:
	if (gpio_is_valid(GPIO_SPEAKER.gpio))
		gpio_free(GPIO_SPEAKER.gpio);

	if (gpio_is_valid(GPIO_DMIC.gpio))
		gpio_free(GPIO_DMIC.gpio);
err_request_lineout_irq:
err_lineout_irq_num_failed:
	if (gpio_is_valid(GPIO_HP.gpio))
		gpio_free(GPIO_HP.gpio);
err_request_lineout_gpio:
	switch_dev_unregister(&lineout_switch.sdev);
	return ret;
}

static int snd_d0085_remove(struct platform_device *pdev)
{
	if (gpio_is_valid(GPIO_SPEAKER.gpio))
		gpio_free(GPIO_SPEAKER.gpio);

	if (gpio_is_valid(GPIO_DMIC.gpio))
		gpio_free(GPIO_DMIC.gpio);

	if (gpio_is_valid(GPIO_HP.gpio)) {
#ifndef USE_JACK_FOR_HP_DETECT
		cancel_work_sync(&lineout_switch.work);
#endif
		gpio_free(GPIO_HP.gpio);
	}

	switch_dev_unregister(&lineout_switch.sdev);
	snd_soc_unregister_card(&board_card);
	return 0;
}

static struct platform_driver snd_d0085_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = "ingenic-alsa",
		.pm    = &snd_soc_pm_ops,
	},
	.probe  = snd_d0085_probe,
	.remove = snd_d0085_remove,
};

module_platform_driver(snd_d0085_driver);

MODULE_AUTHOR("bo.liu<bo.liu@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC Board Snd Card");
MODULE_LICENSE("GPL");
