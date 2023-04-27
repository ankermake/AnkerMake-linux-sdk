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

#include <linux/io.h>
#include <linux/seq_file.h>
#include <soc/base.h>
#include <jz_proc.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <sound/control.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <linux/gpio.h>
#include <linux/switch.h>
#include <linux/delay.h>
#include <mach/jzsnd.h>
#include "canna_icdc.h"


#include "../icodec/icdc_d3.h"

struct snd_switch_data {
	struct switch_dev sdev;
	struct work_struct work;
	int irq;
	int gpio;
	int valid_level;
	int state;
};
static struct snd_switch_data linein_switch;
struct ingenic_snd_codec_data *codec_pdata = NULL;

static char *current_mode_name = NULL;
unsigned long codec_sysclk = 24000000;

static int spk_en_power(struct snd_soc_dapm_widget *w,
						struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		if (codec_pdata && codec_pdata->gpio_spk_en.gpio >= 0) {
			gpio_direction_output(codec_pdata->gpio_spk_en.gpio, codec_pdata->gpio_spk_en.active_level);
			printk("gpio speaker enable %d\n", gpio_get_value(codec_pdata->gpio_spk_en.gpio));
		}
	} else {
		if (codec_pdata && codec_pdata->gpio_spk_en.gpio >= 0) {
			gpio_direction_output(codec_pdata->gpio_spk_en.gpio, !codec_pdata->gpio_spk_en.active_level);
			printk("gpio speaker disable %d\n", gpio_get_value(codec_pdata->gpio_spk_en.gpio));
		}
	}
	return 0;
}

static void spk_sdown(struct snd_pcm_substream *sps)
{
#if 0
	/* Because the operation of SPK_EN_GPIO will cause POP sound on CANNA_V10,we disable the code.*/
	if(sps->stream == SNDRV_PCM_STREAM_PLAYBACK){
		if (codec_pdata && codec_pdata->gpio_spk_en.gpio >= 0)
			gpio_direction_output(codec_pdata->gpio_spk_en.gpio, !codec_pdata->gpio_spk_en.active_level);
	}
#endif
}

static int spk_sup(struct snd_pcm_substream *sps)
{
#if 0
	if(sps->stream == SNDRV_PCM_STREAM_PLAYBACK){
		if (codec_pdata && codec_pdata->gpio_spk_en.gpio >= 0)
			gpio_direction_output(codec_pdata->gpio_spk_en.gpio, codec_pdata->gpio_spk_en.active_level);
	}
#endif
	return 0;
}


int spk_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM);
	if (ret)
		return ret;
	ret = snd_soc_dai_set_sysclk(cpu_dai, JZ_I2S_INNER_CODEC, 24000000, SND_SOC_CLOCK_OUT);
	if (ret)
		return ret;
	return 0;
};

int spk_hw_free(struct snd_pcm_substream *substream)
{
	return 0;
};


static struct snd_soc_ops i2s_machine_ops = {
	.startup = spk_sup,
	.shutdown = spk_sdown,
	.hw_params = spk_hw_params,
	.hw_free = spk_hw_free,
};

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
		delay = 300;
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

	if (state == switch_data->valid_level){
		state = 1;
	}else{
		state = 0;
	}
	if (switch_data->state != state) {
		switch_set_state(&switch_data->sdev, state);
		switch_data->state = state;
	}
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

static const struct snd_soc_dapm_widget i2s_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker", spk_en_power),
	SND_SOC_DAPM_MIC("Mic Buildin", NULL),
	SND_SOC_DAPM_MIC("DMic", NULL),
};

static struct snd_soc_jack icdc_d3_hp_jack;
static struct snd_soc_jack_pin icdc_d3_hp_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};

#ifdef HAVE_HEADPHONE
static struct snd_soc_jack_gpio icdc_d3_jack_gpio[] = {
	{
		.name = "Headphone detection",
		.report = SND_JACK_HEADPHONE,
		.debounce_time = 150,
	}
};
#endif

static int vfs_ioctl(struct file* fd, struct mixer_controls *mixer_c)
{
	int i = 0;
	int ret;

	while (mixer_c[i].mixer_type != END) {
		struct snd_ctl_elem_value control;
		struct snd_ctl_elem_info info;

		info.id.iface = SNDRV_CTL_ELEM_IFACE_MIXER;
		info.id.numid = mixer_c[i].mixer_id;
		ret=fd->f_op->unlocked_ioctl(fd, SNDRV_CTL_IOCTL_ELEM_INFO, (unsigned long)&info);

		control.id = info.id;
		ret=fd->f_op->unlocked_ioctl(fd, SNDRV_CTL_IOCTL_ELEM_READ, (unsigned long)&control);

		switch (mixer_c[i].mixer_type) {
			case ENUMERATED:
				control.value.enumerated.item[0] = mixer_c[i].mixer_value;
				break;
			case BOOLEAN:
			case INTEGER:
				control.value.integer.value[0] = mixer_c[i].mixer_value;
				break;
			default:
				break;
		}
		ret=fd->f_op->unlocked_ioctl(fd, SNDRV_CTL_IOCTL_ELEM_WRITE, (unsigned long)&control);
		i++;
	}
	return ret;
}

static ssize_t mixer_route_read(struct seq_file *file, void *p)
{
	if (current_mode_name)
		printk("%s\n", current_mode_name);
	else
		printk("Not set yet!\n");

	return 0;
}

static ssize_t mixer_route_write(struct file *file, const char __user *buffer,
        size_t count, loff_t *pos)
{
	int mode_num = -1;
	char str[20] = {0};
	int i;

	if (count > 0) {
		if (copy_from_user(str, buffer, count - 1))
			return -EFAULT;
	}

	for (i=0; i< ARRAY_SIZE(mixer_modes); i++) {
		if(!strcmp(str, mixer_modes[i].name)) {
			mode_num = i;
			break;
		}
	}

	if (mode_num != -1) {
		struct file *fd = NULL;
		mm_segment_t old_fs;

		fd=filp_open("/dev/snd/controlC0", O_RDWR, 0);
		if(IS_ERR(fd)) {
			printk("open error\n");
			return -EFAULT;
		}

		old_fs=get_fs();
		set_fs(get_ds());

		if (vfs_ioctl(fd, mixer_modes[mode_num].controls))
			printk("change to %s mode failed.\n", mixer_modes[mode_num].name);
		else {
			current_mode_name = mixer_modes[mode_num].name;
			printk("change to %s mode done.\n", mixer_modes[mode_num].name);
		}

		set_fs(old_fs);
		filp_close(fd,NULL);
	} else {
		for (i=0; i< ARRAY_SIZE(mixer_modes); i++)
			printk("%s ", mixer_modes[i].name);
		printk("\n");
	}

	return count;
}

static struct jz_single_file_ops mixer_route_fops = {
	.read		= mixer_route_read,
	.write		= mixer_route_write,
};

/* machine audio_map */
static const struct snd_soc_dapm_route i2s_audio_map[] = {
	/* ext speaker connected to DO_LO_PWM  */
	{"Speaker", NULL, "DO_LO_PWM"},
	/* mic is connected to AIP/N1 */
	{"MICBIAS", NULL, "Mic Buildin"},
	{"DMIC", NULL, "DMic"},
};

static int i2s_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int err;

	err = snd_soc_dapm_new_controls(dapm, i2s_dapm_widgets,
									ARRAY_SIZE(i2s_dapm_widgets));
	if (err < 0) {
		printk("add dapm controls err!!\n");
		return err;
	}


	/* Set up audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, i2s_audio_map,
								  ARRAY_SIZE(i2s_audio_map));
	if (err < 0) {
		printk("add dapm routes err!!\n");
		return err;
	}


	snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &icdc_d3_hp_jack);
	snd_soc_jack_add_pins(&icdc_d3_hp_jack, ARRAY_SIZE(icdc_d3_hp_jack_pins), icdc_d3_hp_jack_pins);
#ifdef HAVE_HEADPHONE
	if (gpio_is_valid(DORADO_HP_DET)) {
		icdc_d3_jack_gpio[jack].gpio = PHOENIX_HP_DET;
		icdc_d3_jack_gpio[jack].invert = !PHOENIX_HP_DET_LEVEL;
		snd_soc_jack_add_gpios(&icdc_d3_hp_jack, 1, icdc_d3_jack_gpio);
	}
#else
	snd_soc_dapm_disable_pin(dapm, "Headphone Jack");
#endif
	snd_soc_dapm_enable_pin(dapm, "Speaker");
	snd_soc_dapm_enable_pin(dapm, "Mic Buildin");
	snd_soc_dapm_enable_pin(dapm, "DMic");

	snd_soc_dapm_sync(dapm);
	return 0;
}

static struct snd_soc_dai_link soc_dais[] = {
	[0] = {
		.name = "canna ICDC",
		.stream_name = "canna ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = i2s_dai_link_init,
		.codec_dai_name = "icdc-d3-hifi",
		.codec_name = "icdc-d3",
		.ops = &i2s_machine_ops,
	},
	[1] = {
		.name = "canna PCMBT",
		.stream_name = "canna PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
	[2] = {
		.name = "canna DMIC",
		.stream_name = "canna DMIC",
		.platform_name = "jz-asoc-dmic-dma",
		.cpu_dai_name = "jz-asoc-dmic",
		.codec_dai_name = "dmic dump dai",
		.codec_name = "dmic dump",
	},
};

static struct snd_soc_card canna_card = {
	.name = "canna",
	.owner = THIS_MODULE,
	.dai_link = soc_dais,
	.num_links = ARRAY_SIZE(soc_dais),
};

static int linein_switch_init(struct platform_device *pdev, struct ingenic_snd_codec_data *pdata)
{
	int ret = 0;

	linein_switch.sdev.name = "linein";
	linein_switch.sdev.print_state = switch_linein_print_state;
	linein_switch.sdev.print_name  = switch_linein_print_name;
	linein_switch.state = -1;
	ret = switch_dev_register(&linein_switch.sdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "linein switch dev register  failed.\n");
		return ret;
	}

	/* linein detect register */
	if(pdata->gpio_linein_sw.gpio >= 0) {
		linein_switch.gpio = pdata->gpio_linein_sw.gpio;
		linein_switch.valid_level = pdata->gpio_linein_sw.active_level;
		if (!gpio_is_valid(linein_switch.gpio)){
			dev_err(&pdev->dev, "linein detect gpio error.\n");
			return -1;
		}

		ret = gpio_request(linein_switch.gpio, "linein_detect");
		if (ret < 0) {
			dev_err(&pdev->dev, "linein detect gpio request failed.\n");
			return ret;
		}
		linein_switch.irq = gpio_to_irq(linein_switch.gpio);
		if (linein_switch.irq < 0) {
			dev_err(&pdev->dev, "get linein_irq error.\n");
			ret = linein_switch.irq;
			gpio_free(linein_switch.gpio);
			return ret;
		}

		INIT_WORK(&linein_switch.work, linein_switch_work);
		ret = request_irq(linein_switch.irq, linein_irq_handler,
						  IRQF_TRIGGER_FALLING, "linein_detect", &linein_switch);
		if (ret < 0) {
			dev_err(&pdev->dev, "request linein detect irq failed.\n");
			gpio_free(linein_switch.gpio);
			return ret;
		}
		disable_irq(linein_switch.irq);
		linein_switch_work(&linein_switch.work);
	} else {
		linein_switch.gpio = -1;
		linein_switch.valid_level = -1;
		linein_switch.irq = -1;
	}

	return ret;
}
static int snd_canna_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct proc_dir_entry *p, *res;

	canna_card.dev = &pdev->dev;
	codec_pdata = (struct ingenic_snd_codec_data *)canna_card.dev->platform_data;

	ret = linein_switch_init(pdev, codec_pdata);
	if(ret < 0) {
		dev_err(&pdev->dev, "linein switch init failed!\n");
		return ret;
	}

	if (codec_pdata->gpio_spk_en.gpio >= 0) {
		ret = gpio_request(codec_pdata->gpio_spk_en.gpio, "Speaker_en");
		if (ret < 0) {
			dev_err(&pdev->dev, "Speaker enable gpio request failed!\n");
			return ret;
		}
	}

	ret = snd_soc_register_card(&canna_card);
	if (ret < 0) {
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
		return ret;
	}

	p = jz_proc_mkdir("codec");
	if (!p) {
		pr_warning("create_proc_entry for codec failed.\n");
		/* return -ENODEV; */
	}

	res = jz_proc_create_data("mixer_route", 0x444, p, &mixer_route_fops, NULL);
	if (!res) {
		pr_warning("proc_create_data for mixer route failed.\n");
		/* return -ENODEV; */
	}

	dev_info(&pdev->dev, "Alsa sound card: init ok!!!\n");
	return ret;
}

static int snd_canna_remove(struct platform_device *pdev)
{
	struct ingenic_snd_codec_data *codec_pdata = NULL;

	codec_pdata = (struct ingenic_snd_codec_data *)(&pdev->dev)->platform_data;

	if (codec_pdata->gpio_linein_sw.gpio >= 0)
		gpio_free(codec_pdata->gpio_linein_sw.gpio);

	if(codec_pdata->gpio_spk_en.gpio >= 0) {
		cancel_work_sync(&linein_switch.work);
		gpio_free(codec_pdata->gpio_spk_en.gpio);
	}

	switch_dev_unregister(&linein_switch.sdev);
	snd_soc_unregister_card(&canna_card);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver snd_canna_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-alsa",
		.pm = &snd_soc_pm_ops,
	},
	.probe = snd_canna_probe,
	.remove = snd_canna_remove,
};
module_platform_driver(snd_canna_driver);

MODULE_AUTHOR("sccheng<shicheng.cheng@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC canna Snd Card");
MODULE_LICENSE("GPL");
