/*
 * Linux/sound/oss/jzsound/devices/codecs/i2s_virual_codec.c
 *
 * CODEC driver for virual i2s external codec
 *
 * 2015-06-xx   tjin <tjin@ingenic.cn>
 *
 * Copyright (c) Ingenic Semiconductor Co., Ltd.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/sound.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <linux/proc_fs.h>
#include <linux/soundcard.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <mach/jzsnd.h>
#include "../xb47xx_i2s_v13.h"

/* It is the codec LRCLK rate. You can change it refer to the board design.*/
#define VIRUAL_I2S_CODEC_SAMPLE_RATE 48000

#if 1
#define CODEC_MODE  CODEC_SLAVE
#define VIRUAL_EXTERNAL_CODEC_CLOCK  (VIRUAL_I2S_CODEC_SAMPLE_RATE * 256)

#else
/* Some external codec need a sysclk to work.
 * You can change the VIRUAL_EXTERNAL_CODEC_CLOCK value refer to the codec need.
 * But if it's not 24000000, the x1000's DMIC record will be error,
 * such as the record speed maybe fast or slow.
*/
#define CODEC_MODE  CODEC_MASTER
#define VIRUAL_EXTERNAL_CODEC_CLOCK    24000000

#endif

extern int i2s_register_codec(char*, void *,unsigned long,enum codec_mode);

static struct snd_codec_data *codec_platform_data = NULL;
static void codec_get_format_cap(unsigned long *format)
{
	*format = AFMT_S24_LE | AFMT_U24_LE;
}

static int codec_set_device(enum snd_device_t device)
{
	int ret = 0;
	switch (device) {
		case SND_DEVICE_SPEAKER:
		case SND_DEVICE_HEADSET:
		case SND_DEVICE_BUILDIN_MIC:
		case SND_DEVICE_LINEIN_RECORD:
			break;
		default:
			printk("JZ CODEC: Unkown ioctl argument %d in SND_SET_DEVICE\n",device);
	};

	return ret;
}

static int codec_set_replay_channel(int* channel)
{
	*channel = (*channel >= 2) + 1;

	return 0;
}

static int codec_set_record_channel(int* channel)
{
#ifdef CONFIG_I2S_VIRUAL_AEC_MODE
        *channel = 2;
#else
        *channel = (*channel >= 2) + 1;
#endif
	return 0;
}

static int jzcodec_ctl(unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	{
		switch (cmd) {

		case CODEC_INIT:
			break;

		case CODEC_TURN_OFF:
			break;

		case CODEC_SHUTDOWN:
			break;

		case CODEC_RESET:
			break;

		case CODEC_SUSPEND:
			break;

		case CODEC_RESUME:
			break;

		case CODEC_ANTI_POP:
			break;

		case CODEC_SET_DEFROUTE:
			break;

		case CODEC_SET_DEVICE:
			ret = codec_set_device(*(enum snd_device_t *)arg);
			break;

		case CODEC_SET_STANDBY:
			break;

		case CODEC_SET_REPLAY_RATE:
			*(unsigned long*)arg = VIRUAL_I2S_CODEC_SAMPLE_RATE;
			break;

		case CODEC_SET_RECORD_RATE:
			*(unsigned long*)arg = VIRUAL_I2S_CODEC_SAMPLE_RATE;
			break;

		case CODEC_SET_REPLAY_DATA_WIDTH:
			break;

		case CODEC_SET_RECORD_DATA_WIDTH:
			break;

		case CODEC_SET_REPLAY_VOLUME:
			ret = *(int*)arg;
			break;

		case CODEC_SET_RECORD_VOLUME:
			ret = *(int*)arg;
			break;

		case CODEC_SET_MIC_VOLUME:
			ret = *(int*)arg;
			break;

		case CODEC_SET_REPLAY_CHANNEL:
			ret = codec_set_replay_channel((int*)arg);
			break;

		case CODEC_SET_RECORD_CHANNEL:
			ret = codec_set_record_channel((int*)arg);
			break;

		case CODEC_GET_REPLAY_FMT_CAP:
			codec_get_format_cap((unsigned long *)arg);
			break;

		case CODEC_GET_RECORD_FMT_CAP:
			*(unsigned long *)arg = 0;
			break;

		case CODEC_DAC_MUTE:
			break;

		case CODEC_ADC_MUTE:
			break;

		case CODEC_DEBUG_ROUTINE:
			break;

		case CODEC_CLR_ROUTE:
			break;

		case CODEC_DEBUG:
			break;
		default:
			printk("JZ CODEC:%s:%d: Unkown IOC commond\n", __FUNCTION__, __LINE__);
			ret = -1;
		}
	}

	return ret;
}

/**
 * Module init
 */

static void gpio_enable_spk_en(void) {
	if(codec_platform_data && (codec_platform_data->gpio_spk_en.gpio != -1)) {
		if (codec_platform_data->gpio_spk_en.active_level) {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 1);
		} else {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 0);
		}
	}
}


static void gpio_disable_spk_en(void) {
	if(codec_platform_data && (codec_platform_data->gpio_spk_en.gpio != -1)) {
		if (codec_platform_data->gpio_spk_en.active_level) {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 0);
		} else {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 1);
		}
		msleep(5);      // Wait for amp mute.
	}
}

static int jz_codec_probe(struct platform_device *pdev){
	codec_platform_data = pdev->dev.platform_data;
	if (codec_platform_data->gpio_spk_en.gpio != -1) {
		if (gpio_request(codec_platform_data->gpio_spk_en.gpio, "gpio_spk_en") < 0) {
			gpio_free(codec_platform_data->gpio_spk_en.gpio);
			gpio_request(codec_platform_data->gpio_spk_en.gpio,"gpio_spk_en");
		}
	}
	gpio_enable_spk_en();
}

static int jz_codec_remove(struct platform_device *pdev) {
	if (codec_platform_data->gpio_spk_en.gpio != -1 )
		gpio_free(codec_platform_data->gpio_spk_en.gpio);
		codec_platform_data = NULL;
		return 0;
}


static struct platform_driver jz_codec_driver = {
	.probe          = jz_codec_probe,
	.remove     = jz_codec_remove,
	.driver         = {
		.name   = "tas5010_codec",
		.owner  = THIS_MODULE,
    },
};

static int __init init_codec(void)
{
	int ret = 0;
	ret = i2s_register_codec("i2s_external_codec", (void *)jzcodec_ctl, VIRUAL_EXTERNAL_CODEC_CLOCK, CODEC_MODE);
	if (ret < 0){
		printk("i2s audio is not support\n");
		return ret;
	}
	ret = platform_driver_register(&jz_codec_driver);
	if (ret) {
		printk("JZ CODEC: Could not register jz_codec_driver\n");
		return ret;
	}
	printk("i2s audio codec register success\n");
	return 0;
}

/**
 * Module exit
 */
static void __exit cleanup_codec(void)
{
}
module_init(init_codec);
module_exit(cleanup_codec);
MODULE_LICENSE("GPL");
