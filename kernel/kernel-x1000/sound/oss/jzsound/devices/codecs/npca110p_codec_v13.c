/*
 * Linux/sound/oss/jzsound/devices/codecs/npca110p_codec_v13.c
 *
 * CODEC driver for npca110p i2s external codec
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
#include "npca110p_codec_v13.h"

#define NPCA110P_EXTERNAL_CODEC_CLOCK 24000000
//#define CODEC_MODE  CODEC_SLAVE
#define CODEC_MODE  CODEC_MASTER
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

static struct i2c_client *npca110p_client = NULL;
static struct snd_codec_data *codec_platform_data = NULL;
static struct mutex i2c_access_lock;
static struct mutex switch_dev_lock;

extern int i2s_register_codec(char*, void *,unsigned long,enum codec_mode);

static void codec_get_format_cap(unsigned long *format)
{
	*format = AFMT_S24_LE | AFMT_U24_LE;
}

static int npca110p_i2c_write(unsigned char* data, unsigned int len)
{
	int ret = -1;
	struct i2c_client *client = npca110p_client;

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

static int npca110p_i2c_read(unsigned char* data, unsigned int len)
{
	int ret = -1;
	struct i2c_client *client = npca110p_client;

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
	struct extcodec_platform_data *npca110p;

	if (npca110p_client == NULL){
		printk("The i2c is not register, npca110p can't init\n");
		return -1;
	}
	dev = &npca110p_client->dev;
	npca110p = dev->platform_data;

	/* reset PDN pin */
	if (npca110p->power->gpio != -1){
		gpio_set_value(npca110p->reset->gpio, npca110p->reset->active_level);
		mdelay(50);
		gpio_set_value(npca110p->reset->gpio, !npca110p->reset->active_level);
		msleep(50);
	}

	/* npca110p init */
	for (i = 0;i < sizeof(g_abMaxxDSPCommands)/3 ; i++) {
		ret = npca110p_i2c_write(g_abMaxxDSPCommands + 3*i,3);
		if (ret < 0) {
			printk("%s i2c write err\n",__func__);
			printk("***************************************\n");
			return ret;
		}
	}
	return ret;
}

static int npca110p_codec_power_ctl(unsigned int reg,unsigned int value)
{
	/*value is 16 bit*/
	int ret = 0;
	int i;
	unsigned char data[3];
	if (value > 0xffff) {
		printk("power ctl faild,value must less than 16 bit\n");
		return 0;
	}

	data[0] = 0x00;
	data[1] = value >> 8;
	data[2] = value & 0x00ff;
	if (reg <= CODEC_REGNUM) {
		ret |= npca110p_i2c_write((npca110p_codec_command) + (reg*3), 3);
		if (ret < 0)
			printk("%s i2c error \n",__func__);
	}

	ret |= npca110p_i2c_write(data, 3);
	if (ret < 0)
		printk("%s i2c error \n",__func__);

	if (reg <= CODEC_REGNUM) {
		for (i = 0; i < 4; i++)
			ret |= npca110p_i2c_write((npca110p_codec_latch + i*3), 3);
	}

	if (ret != 0)
		printk("%s i2c error \n",__func__);
	return 0;
}

static int npca110p_write(unsigned int reg, unsigned int value)
{
	int ret = 0;
	int i;
	unsigned char data[3];
	data[0] = 0x00;
	data[1] = value;
	data[2] = value;
	/* If it's a codec register, the first command send */
	if (value == 0) {
		data[0] = 0x03;
	}
	if (reg <= CODEC_REGNUM) {
		ret |= npca110p_i2c_write((npca110p_codec_command) + (reg*3), 3);
		if (ret < 0)
			printk("%s i2c error \n", __func__);
	}

	ret |= npca110p_i2c_write(data, 3);
	if (ret < 0)
		printk("%s i2c error \n", __func__);

	/* If it's a codec register, latch the command */
	if (reg <= CODEC_REGNUM) {
		for (i = 0; i < 4; i++)
			ret |= npca110p_i2c_write((npca110p_codec_latch + i*3), 3);
	}

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
	npca110p_write(ADC_VOL,value);
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
			value = 0x08; break;
		case 20:
			value = 0x10; break;
		case 30:
			value = 0x19; break;
		case 40:
			value = 0x21; break;
		case 50:
			value = 0x2a; break;
		case 60:
			value = 0x32; break;
		case 70:
			value = 0x3a; break;
		case 80:
			value = 0x43; break;
		case 90:
			value = 0x4b; break;
		case 100:
			value = 0x54; break;
		default :
			value = 0x2a; break;
	}
		npca110p_write(DAC2_VOL,value);
	return *val;
}

static int codec_suspend(void)
{
	int ret = 0;
	unsigned int value;
	ret |= npca110p_codec_power_ctl(CODEC_POWER_CTL,0x1ffd);
	if (ret != 0){
		printk("%s i2c err\n",__func__);
	}
	return 0;
}

static int codec_resume(void)
{
	int ret = 0;
	unsigned int value;
	ret |= npca110p_codec_power_ctl(CODEC_POWER_CTL,0x1fff);
	if (ret != 0){
		printk("%s i2c err\n",__func__);
	}
	return 0;
}

static int codec_shutdown(void)
{
	int ret = 0;
	unsigned char data;
	struct device *dev;
	struct extcodec_platform_data *npca110p;

	if (npca110p_client == NULL){
		printk("The i2c is not register, npca110p can't init\n");
		return -1;
	}
	dev = &npca110p_client->dev;
	npca110p = dev->platform_data;

	if (npca110p->power->gpio != -1){
		gpio_set_value(npca110p->power->gpio, npca110p->power->active_level);
	}

	return 0;
}

static int jzcodec_ctl(unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	{
		switch (cmd) {
		case CODEC_INIT:
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

static int __devinit npca110p_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        int ret = 0;
        struct device *dev = &client->dev;
        struct extcodec_platform_data *npca110p_data = dev->platform_data;
		if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
                return -ENODEV;
		}

	if (npca110p_data->power->gpio != -1) {
		ret = gpio_request_one(npca110p_data->power->gpio,
				GPIOF_DIR_OUT, "npca110p_data-power");
		if (ret != 0) {
			dev_err(dev, "Can't request pdn pin = %d\n",
					npca110p_data->power->gpio);
			return ret;
		} else
			gpio_set_value(npca110p_data->power->gpio, npca110p_data->power->active_level);
	}
	if (npca110p_data->reset->gpio != -1) {
		ret = gpio_request_one(npca110p_data->reset->gpio,GPIOF_DIR_OUT, "npca110p_reset");
		if (ret != 0) {
			dev_err(dev,"Can't request pin = %d\n",npca110p_data->reset->gpio);
			return ret;
		}
	}
        npca110p_client = client;

        return 0;
}

static int npca110p_i2c_remove(struct i2c_client *client)
{
        struct device *dev = &client->dev;
        struct extcodec_platform_data *npca110p_data = dev->platform_data;

	if (npca110p_data->power->gpio != -1)
		gpio_free(npca110p_data->power->gpio);
	if (npca110p_data->reset->gpio != -1)
		gpio_free(npca110p_data->reset->gpio);

        npca110p_client = NULL;

        return 0;
}

static int jz_codec_probe(struct platform_device *pdev)
{
	codec_platform_data = pdev->dev.platform_data;

	codec_platform_data->codec_sys_clk = NPCA110P_EXTERNAL_CODEC_CLOCK;

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

static const struct i2c_device_id npca110p_i2c_id[] = {
        { "npca110p", 2 },
        { }
};
MODULE_DEVICE_TABLE(i2c, npca110p_i2c_id);

static struct i2c_driver npca110p_i2c_driver = {
        .driver.name = "npca110p",
        .probe = npca110p_i2c_probe,
        .remove = npca110p_i2c_remove,
        .id_table = npca110p_i2c_id,
};

static struct platform_driver jz_codec_driver = {
        .probe          = jz_codec_probe,
        .remove         = jz_codec_remove,
        .driver         = {
                .name   = "npca110p_codec",
                .owner  = THIS_MODULE,
        },
};

/**
 * Module init
 */
static int __init init_npca110p_codec(void)
{
	int ret = 0;

	mutex_init(&i2c_access_lock);
	mutex_init(&switch_dev_lock);

	ret = i2s_register_codec("i2s_external_codec", (void *)jzcodec_ctl, NPCA110P_EXTERNAL_CODEC_CLOCK, CODEC_MODE);
	if (ret < 0){
		printk("i2s audio is not support\n");
		return ret;
	}

	ret = platform_driver_register(&jz_codec_driver);
	if (ret) {
		printk("JZ CODEC: Could not register jz_codec_driver\n");
		return ret;
	}

	ret = i2c_add_driver(&npca110p_i2c_driver);
	if (ret) {
		printk("JZ CODEC: Could not register npca110p i2c driver\n");
		platform_driver_unregister(&jz_codec_driver);
		return ret;
	} else
		printk("audio codec npca110p register success\n");
	return 0;
}

/**
 * Module exit
 */
static void __exit cleanup_npca110p_codec(void)
{
	i2c_del_driver(&npca110p_i2c_driver);
	platform_driver_unregister(&jz_codec_driver);
}
module_init(init_npca110p_codec);
module_exit(cleanup_npca110p_codec);
MODULE_LICENSE("GPL");
