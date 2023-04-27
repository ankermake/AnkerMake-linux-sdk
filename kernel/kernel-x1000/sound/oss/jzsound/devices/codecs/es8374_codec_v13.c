/*
 * Linux/sound/oss/jzsound/devices/codecs/es8374_codec_v13.c
 *
 * CODEC driver for es8374 i2s external codec
 *
 * 2017-2-xx   dlzhang <daolin.zhang@ingenic.com>
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
#include "es8374_codec_v13.h"

#define ES8374_EXTERNAL_CODEC_CLOCK 24000000
#define CODEC_MODE  CODEC_SLAVE
//#define CODEC_MODE  CODEC_MASTER
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
#endif

#ifdef USE_48000_SAMPLE_RATE
unsigned long DEFAULT_REPLAY_SAMPLE_RATE = 48000;
#else
unsigned long DEFAULT_REPLAY_SAMPLE_RATE = 44100;
#endif

static int user_replay_volume = 50;
static unsigned long user_replay_rate = 0;
static int user_linein_state = 0;
static int user_replay_state = 0;

static struct i2c_client *es8374_client = NULL;
static struct snd_codec_data *codec_platform_data = NULL;
static struct mutex i2c_access_lock;
static struct mutex switch_dev_lock;

extern int i2s_register_codec(char*, void *,unsigned long,enum codec_mode);

static void codec_get_format_cap(unsigned long *format)
{
	*format = AFMT_S24_LE | AFMT_U24_LE;
}

static int es8374_i2c_write(unsigned char* data, unsigned int len)
{
	int ret = -1;
	struct i2c_client *client = es8374_client;

	if (client == NULL) {
		printk("===>NOTE: %s error\n", __func__);
		return ret;
	}

	if(!data || len <= 0){
		printk("===>ERROR: %s\n", __func__);
		return ret;
	}
	mutex_lock(&i2c_access_lock);
	ret = i2c_master_send(client, data, len);
	if (ret < len)
		printk("%s err %d!\n", __func__, ret);
	mutex_unlock(&i2c_access_lock);

	return ret < len ? ret : 0;
}

static int es8374_i2c_read(unsigned char* data, unsigned int len)
{
	int ret = -1;
	struct i2c_client *client = es8374_client;

	if (client == NULL) {
		printk("===>NOTE: %s error\n", __func__);
		return ret;
	}

	if(!data || len <= 0){
		printk("===>ERROR: %s\n", __func__);
		return ret;
	}

	mutex_lock(&i2c_access_lock);

	ret = i2c_master_recv(client, data, len);
	if (ret < 0) {
		printk("%s err\n", __func__);
	}
	mutex_unlock(&i2c_access_lock);

	return ret < len ? ret : 0;
}
static void gpio_enable_spk_en(void)
{
	if(codec_platform_data && (codec_platform_data->gpio_spk_en.gpio != -1)) {
		if (codec_platform_data->gpio_spk_en.active_level) {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 1);
		} else {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 0);
		}
		msleep(5);
	}
}

static void gpio_disable_spk_en(void)
{
	if(codec_platform_data && (codec_platform_data->gpio_spk_en.gpio != -1)) {
		if (codec_platform_data->gpio_spk_en.active_level) {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 0);
		} else {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 1);
		}
		msleep(5);
	}
}

static int codec_set_device(enum snd_device_t device)
{
	int i;
	int ret = 0;
	unsigned char data;
	mutex_lock(&switch_dev_lock);
	switch (device) {
		case SND_DEVICE_HEADSET:
		case SND_DEVICE_SPEAKER:
		case SND_DEVICE_LINEIN_RECORD:
			break;
		default:
			printk("JZ CODEC: Unkown ioctl argument %d in SND_SET_DEVICE\n",device);
			ret = -1;
	};
	mutex_unlock(&switch_dev_lock);
	return ret;
}

static int codec_set_replay_channel(int* channel)
{
#ifdef CONFIG_BOARD_X1000_HL01_V10
	*channel = 1;
#else
	*channel = (*channel >= 2) + 1;
#endif
	return 0;
}

static int codec_set_record_channel(int* channel)
{
	if (*channel == 2)
		return 0;
	else {
		*channel = (*channel >= 2) + 1;
		return 0;
	}
}

static int codec_init(void)
{
	int ret = 0;
	int i;
	struct device *dev;
	struct extcodec_platform_data *es8374;
	if (es8374_client == NULL){
		printk("The i2c is not register, es8374 can't init\n");
		return -1;
	}
	dev = &es8374_client->dev;
	es8374 = dev->platform_data;


	/* es8374 init */
	for (i = 0;i < sizeof(RegCommandsSetUpCodec)/2 ; i++) {
	if(i == 1) {
		mdelay(1);
	} else if(i == 20) {
		mdelay(50);
	}
		ret = es8374_i2c_write(RegCommandsSetUpCodec + 2*i,2);
		mdelay(1);
		if (ret < 0) {
			printk("%s i2c write err\n",__func__);
			printk("***************************************\n");
			return ret;
		}
	}
	return ret;
}

static int es8374_write(unsigned int reg, unsigned int value)
{
	int ret = 0;
	int i;
	unsigned char data[2];
	data[0] = reg;
	data[1] = value;
	ret |= es8374_i2c_write(data, 2);
	if (ret < 0)
		printk("%s i2c error \n", __func__);

	return 0;
}

static int set_adc_volume (int *val)
{
	int ret = 0;
	int i;
	int value;

	if (*val < 0) {
		*val = user_replay_volume;
		return *val;
	} else if (*val > 100)
		*val = 100;
	switch (*val) {
		case 10:
		value = 0x4b; break;
		case 20:
		value = 0x43; break;
		case 30:
		value = 0x3a; break;
		case 40:
		value = 0x32; break;
		case 50:
		value = 0x2a; break;
		case 60:
		value = 0x21; break;
		case 70:
		value = 0x19; break;
		case 80:
		value = 0x10; break;
		case 90:
		value = 0x08; break;
		case 100:
		value = 0x00; break;
	}
	return *val;
}

static int codec_set_replay_volume(int *val)
{
	int ret =0;
	int i;
	int value;

	if (*val < 0) {
		*val = user_replay_volume;
		return *val;
	} else if (*val > 100)
		*val = 100;
	switch (*val) {
		case 0:
	/* Keep this, depending on the needs of the DAC can be set to mute or power amplifier mute*/
			break;
		case 10:
			value = 0xc0; break;
		case 20:
			value = 0xb4; break;
		case 30:
			value = 0xa0; break;
		case 40:
			value = 0x8c; break;
		case 50:
			value = 0x78; break;
		case 60:
			value = 0x64; break;
		case 70:
			value = 0x50; break;
		case 80:
			value = 0x28; break;
		case 90:
			value = 0x14; break;
		case 100:
			value = 0x00; break;
		default :
			value = 0x2a; break;
	}
	es8374_write(0x38,value);
	return *val;
}

static int codec_suspend(void)
{
	return 0;
}

static int codec_resume(void)
{
	return 0;
}

static int codec_shutdown(void)
{
	return 0;
}

static int jzcodec_ctl(unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	{
		switch (cmd) {
		case CODEC_INIT:
	//	while(1)
			codec_init();
			break;

		case CODEC_TURN_OFF:
			if (arg & CODEC_RMODE)
			break;
			if (user_linein_state == 0) {
				gpio_disable_spk_en();
			}
			user_replay_state = 0;
			break;

		case CODEC_SHUTDOWN:
			ret = codec_shutdown();
			break;

		case CODEC_RESET:
			break;

		case CODEC_SUSPEND:
			ret = codec_suspend();
			break;

		case CODEC_RESUME:
			ret = codec_resume();
			break;

		case CODEC_ANTI_POP:
			if (arg & CODEC_RMODE)
			break;

			if (user_replay_volume) {
				gpio_enable_spk_en();
			}
			user_replay_state = 1;
			break;

		case CODEC_SET_DEFROUTE:
			break;

		case CODEC_SET_DEVICE:
			ret = codec_set_device(*(enum snd_device_t *)arg);
			break;

		case CODEC_SET_STANDBY:
			break;

		case CODEC_SET_REPLAY_RATE:
			*(unsigned long *)arg = DEFAULT_REPLAY_SAMPLE_RATE;
			break;

		case CODEC_SET_RECORD_RATE:
			/*
			 * Record sample rate is follow the replay sample rate. Here set is invalid.
			 * Because record and replay i2s use the same BCLK and SYNC pin.
			*/
			*(unsigned long*)arg = user_replay_rate;
			break;

		case CODEC_SET_REPLAY_DATA_WIDTH:
			break;

		case CODEC_SET_RECORD_DATA_WIDTH:
			break;

		case CODEC_SET_REPLAY_VOLUME:
			ret = codec_set_replay_volume((int*)arg);
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

		case CODEC_DUMP_REG:
			break;
		default:
			printk("JZ CODEC:%s:%d: Unkown IOC commond\n", __FUNCTION__, __LINE__);
			ret = -1;
		}
	}

	return ret;
}

static int __devinit es8374_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        int ret = 0;
        struct device *dev = &client->dev;
        struct extcodec_platform_data *es8374_data = dev->platform_data;
		if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
                return -ENODEV;
		}
/*
 The current state i2c address gpio is low, i2c address is 0x20, if i2c address gpio is high must modify i2c address 0x22 in i2c_bus.c
 */
	if (es8374_data->i2c_address->gpio != -1) {
		ret = gpio_request_one(es8374_data->i2c_address->gpio,
				GPIOF_DIR_OUT, "es8374_data_address");
		if (ret != 0) {
			dev_err(dev, "Can't request pdn pin = %d\n",
					es8374_data->i2c_address->gpio);
			return ret;
		} else
			gpio_set_value(es8374_data->i2c_address->gpio, es8374_data->i2c_address->active_level);
	}
        es8374_client = client;

        return 0;
}

static int es8374_i2c_remove(struct i2c_client *client)
{
        struct device *dev = &client->dev;
        struct extcodec_platform_data *es8374_data = dev->platform_data;

	if (es8374_data->i2c_address->gpio != -1)
		gpio_free(es8374_data->i2c_address->gpio);

        es8374_client = NULL;

        return 0;
}

static int jz_codec_probe(struct platform_device *pdev)
{
	codec_platform_data = pdev->dev.platform_data;

	codec_platform_data->codec_sys_clk = ES8374_EXTERNAL_CODEC_CLOCK;

	jz_set_hp_detect_type(SND_SWITCH_TYPE_GPIO,
			&codec_platform_data->gpio_hp_detect,
			NULL,
			NULL,
			NULL,
			&codec_platform_data->gpio_linein_detect,
			-1);

	if (codec_platform_data->gpio_spk_en.gpio != -1) {
		if (gpio_request(codec_platform_data->gpio_spk_en.gpio, "gpio_spk_en") < 0) {
			gpio_free(codec_platform_data->gpio_spk_en.gpio);
			gpio_request(codec_platform_data->gpio_spk_en.gpio,"gpio_spk_en");
		}
	}
}

static int jz_codec_remove(struct platform_device *pdev)
{
	if (codec_platform_data->gpio_spk_en.gpio != -1)
		gpio_free(codec_platform_data->gpio_spk_en.gpio);
	codec_platform_data = NULL;

	return 0;
}

static const struct i2c_device_id es8374_i2c_id[] = {
        { "es8374", 0 },
        { }
};
MODULE_DEVICE_TABLE(i2c, es8374_i2c_id);

static struct i2c_driver es8374_i2c_driver = {
        .driver.name = "es8374",
        .probe = es8374_i2c_probe,
        .remove = es8374_i2c_remove,
        .id_table = es8374_i2c_id,
};

static struct platform_driver jz_codec_driver = {
        .probe          = jz_codec_probe,
        .remove         = jz_codec_remove,
        .driver         = {
                .name   = "es8374_codec",
                .owner  = THIS_MODULE,
        },
};

/**
 * Module init
 */
static int __init init_es8374_codec(void)
{
	int ret = 0;

	mutex_init(&i2c_access_lock);
	mutex_init(&switch_dev_lock);

	ret = i2s_register_codec("i2s_external_codec", (void *)jzcodec_ctl, ES8374_EXTERNAL_CODEC_CLOCK, CODEC_MODE);
	if (ret < 0){
		printk("i2s audio is not support\n");
		return ret;
	}

	ret = platform_driver_register(&jz_codec_driver);
	if (ret) {
		printk("JZ CODEC: Could not register jz_codec_driver\n");
		return ret;
	}

	ret = i2c_add_driver(&es8374_i2c_driver);
	if (ret) {
		printk("JZ CODEC: Could not register es8374 i2c driver\n");
		platform_driver_unregister(&jz_codec_driver);
		return ret;
	} else
		printk("audio codec es8374 register success\n");
	return 0;
}

/**
 * Module exit
 */
static void __exit cleanup_es8374_codec(void)
{
	i2c_del_driver(&es8374_i2c_driver);
	platform_driver_unregister(&jz_codec_driver);
}
module_init(init_es8374_codec);
module_exit(cleanup_es8374_codec);
MODULE_LICENSE("GPL");
