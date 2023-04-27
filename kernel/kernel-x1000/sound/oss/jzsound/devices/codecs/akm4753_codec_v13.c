/*
 * Linux/sound/oss/jzsound/devices/codecs/akm4753_codec_v13.c
 *
 * CODEC driver for akm4753 i2s external codec
 *
 * 2015-10-xx   tjin <tao.jin@ingenic.com>
 *
 * Copyright (c) Ingenic Semiconductor Co., Ltd.
 */
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
#include "akm4753_codec_v13.h"

#define DEFAULT_REPLAY_SAMPLE_RATE   48000
#define AKM4753_EXTERNAL_CODEC_CLOCK 24000000
//#define CODEC_MODE  CODEC_SLAVE
#define CODEC_MODE  CODEC_MASTER

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
#endif
static int user_replay_volume = 100;
static unsigned long user_replay_rate = 0;
static int user_linein_state = 0;
static int user_replay_state = 0;

static struct i2c_client *akm4753_client = NULL;
static struct snd_codec_data *codec_platform_data = NULL;
static struct mutex i2c_access_lock;
static struct mutex switch_dev_lock;

extern int i2s_register_codec(char*, void *,unsigned long,enum codec_mode);

static void codec_get_format_cap(unsigned long *format)
{
	*format = AFMT_S24_LE | AFMT_U24_LE;
}

static int akm4753_i2c_write_regs(unsigned char reg, unsigned char* data, unsigned int len)
{
	int ret = -1;
	int i = 0;
	unsigned char buf[127] = {0};
	struct i2c_client *client = akm4753_client;

	if (client == NULL) {
		printk("===>NOTE: %s error\n", __func__);
		return ret;
	}

	if(!data || len <= 0){
		printk("===>ERROR: %s\n", __func__);
		return ret;
	}

	mutex_lock(&i2c_access_lock);
	buf[0] = reg;
	for(; i < len; i++){
		buf[i+1] = *data;
		data++;
	}

	ret = i2c_master_send(client, buf, len+1);
	if (ret < len+1)
		printk("%s 0x%02x err %d!\n", __func__, reg, ret);
	mutex_unlock(&i2c_access_lock);

	return ret < len+1 ? ret : 0;
}

static int akm4753_i2c_read_reg(unsigned char reg, unsigned char* data, unsigned int len)
{
	int ret = -1;
	struct i2c_client *client = akm4753_client;

	if (client == NULL) {
		printk("===>NOTE: %s error\n", __func__);
		return ret;
	}

	if(!data || len <= 0){
		printk("===>ERROR: %s\n", __func__);
		return ret;
	}

	mutex_lock(&i2c_access_lock);
	ret = i2c_master_send(client, &reg, 1);
	if (ret < 1) {
		printk("%s 0x%02x err\n", __func__, reg);
		mutex_unlock(&i2c_access_lock);
		return -1;
	}

	ret = i2c_master_recv(client, data, len);
	if (ret < 0)
		printk("%s 0x%02x err\n", __func__, reg);
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

static void set_eq_param(unsigned long rate_value)
{
        int i;
	int reg_num;
        unsigned char *param;
        switch (rate_value)
	{
        case 8000:
                param = &eq_regs_8000hz[0][0];
		reg_num = sizeof(eq_regs_8000hz) / sizeof(eq_regs_8000hz[0]);
                break;
        case 12000:
                param = &eq_regs_12000hz[0][0];
		reg_num = sizeof(eq_regs_12000hz) / sizeof(eq_regs_12000hz[0]);
                break;
        case 16000:
                param = &eq_regs_16000hz[0][0];
		reg_num = sizeof(eq_regs_16000hz) / sizeof(eq_regs_16000hz[0]);
                break;
        case 24000:
                param = &eq_regs_24000hz[0][0];
		reg_num = sizeof(eq_regs_24000hz) / sizeof(eq_regs_24000hz[0]);
                break;
        case 7350:
                param = &eq_regs_7350hz[0][0];
		reg_num = sizeof(eq_regs_7350hz) / sizeof(eq_regs_7350hz[0]);
                break;
        case 11025:
		param = &eq_regs_11025hz[0][0];
		reg_num = sizeof(eq_regs_11025hz) / sizeof(eq_regs_11025hz[0]);
                break;
        case 14700:
                param = &eq_regs_14700hz[0][0];
		reg_num = sizeof(eq_regs_14700hz) / sizeof(eq_regs_14700hz[0]);
                break;
        case 22050:
                param = &eq_regs_22050hz[0][0];
		reg_num = sizeof(eq_regs_22050hz) / sizeof(eq_regs_22050hz[0]);
                break;
        case 32000:
                param = &eq_regs_32000hz[0][0];
		reg_num = sizeof(eq_regs_32000hz) / sizeof(eq_regs_32000hz[0]);
                break;
        case 48000:
                param = &eq_regs_48000hz[0][0];
		reg_num = sizeof(eq_regs_48000hz) / sizeof(eq_regs_48000hz[0]);
                break;
        case 29400:
                param = &eq_regs_29400hz[0][0];
		reg_num = sizeof(eq_regs_29400hz) / sizeof(eq_regs_29400hz[0]);
                break;
        case 44100:
                param = &eq_regs_44100hz[0][0];
		reg_num = sizeof(eq_regs_44100hz) / sizeof(eq_regs_44100hz[0]);
                break;
        default:
		printk("The sample rate %lu is not in the EQ range.", rate_value);
                return;
	}
	for(i = 0; i < reg_num; i++) {
		akm4753_i2c_write_regs(*(param+i*2), param+i*2+1, 1);
        }

	return;
}

static void check_replay_rate(unsigned long *rate)
{
	int i;
        unsigned long mrate[12] = {
                8000, 12000, 16000, 24000, 7350, 11025,
                14700,22050, 32000, 48000, 29400,44100
        };
#ifdef CONFIG_AKM4753_AEC_MODE
        /* Here we should only support integer times of 16KHZ sample rate. */
        *rate = DEFAULT_REPLAY_SAMPLE_RATE;
#endif
	for (i = 0; i < 12; i++) {
                if (*rate == mrate[i])
                        break;
        }
        if (i == 12){
                printk("Replay rate %ld is not support by akm4753, we fix it to 48000\n", *rate);
                *rate = 48000;
	}
	return;
}

static int codec_set_replay_rate(unsigned long *rate)
{
	int i = 0;
	unsigned char data = 0x0;
	unsigned long mrate[12] = {
		8000, 12000, 16000, 24000, 7350, 11025,
		14700,22050, 32000, 48000, 29400,44100
	};
	unsigned char reg[12] = {
		0, 1, 2, 3, 4, 5,
		6, 7, 10,11,14,15
	};

#ifdef CONFIG_AKM4753_AEC_MODE
        /* Here we should only support integer times of 16KHZ sample rate. */
        *rate = DEFAULT_REPLAY_SAMPLE_RATE;
#endif
	for (i = 0; i < 12; i++) {
		if (*rate == mrate[i])
			break;
	}
	if (i == 12){
		printk("Replay rate %ld is not support by akm4753, we fix it to 48000\n", *rate);
		*rate = 48000;
		i = 9;
	}

        user_replay_rate = *rate;

	gpio_disable_spk_en();
	akm4753_i2c_read_reg(0x02, &data, 1);
	data &= 0xf;
	data |= reg[i]<<4;
	akm4753_i2c_write_regs(0x02, &data, 1);

	/* Set EQ for different rate */
	set_eq_param(user_replay_rate);
	msleep(50);            // Wait for akm4753 i2s clk stable.

	if (user_replay_volume && user_replay_state)
		gpio_enable_spk_en();
	return 0;
}

static int codec_set_device(enum snd_device_t device)
{
	int i;
	int ret = 0;
	unsigned char data;
	mutex_lock(&switch_dev_lock);
	switch (device) {
		case SND_DEVICE_HEADSET:
			gpio_disable_spk_en();
			akm4753_i2c_read_reg(0x01, &data, 1);
			data &= 0xf0;
			data |= 0x1;
			akm4753_i2c_write_regs(0x01, &data, 1);

			/* Set other registers for the device, such as HPF, LPF, EQ. */
			codec_set_replay_rate(&user_replay_rate);
			user_linein_state = 0;
			break;
		case SND_DEVICE_SPEAKER:
			gpio_disable_spk_en();
			akm4753_i2c_read_reg(0x01, &data, 1);
			data &= 0xf0;
			data |= 0x1;
			akm4753_i2c_write_regs(0x01, &data, 1);

			/* Set other registers for the device, such as HPF, LPF, EQ. */
			codec_set_replay_rate(&user_replay_rate);
			user_linein_state = 0;
			if (user_replay_volume && user_replay_state)
				gpio_enable_spk_en();
			break;
		case SND_DEVICE_LINEIN_RECORD:
			user_linein_state = 1;
			gpio_disable_spk_en();
			akm4753_i2c_read_reg(0x01, &data, 1);
			data &= 0xf0;
			akm4753_i2c_write_regs(0x01, &data, 1);
			msleep(5);
			/* fix to 48000 sample rate */
			akm4753_i2c_read_reg(0x02, &data, 1);
			data &= 0x0f;
			data |= 0xb0;
			akm4753_i2c_write_regs(0x02, &data, 1);

			/* Set other registers for the device, such as HPF, LPF, EQ. */
			for (i = 0; i < sizeof(eq_regs_linein) / sizeof(eq_regs_linein[0]); i++){
				ret |= akm4753_i2c_write_regs(eq_regs_linein[i][0], &eq_regs_linein[i][1], 1);
			}
			msleep(5);
			if (user_replay_volume)
				gpio_enable_spk_en();
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
#ifdef CONFIG_AKM4753_AEC_MODE
        *channel = 2;
#else
        *channel = (*channel >= 2) + 1;
#endif
	return 0;
}

static int codec_dac_setup(void)
{
	int i;
	int ret = 0;
	char data = 0x0;
	/* Disable SAR and SAIN */
	data = 0x0;
	ret |= akm4753_i2c_write_regs(0x0, &data, 1);

#ifdef CONFIG_BOARD_X1000_HL01_V10
	/* Set up stereo mode(HPF, LPF individual mode) */
	data = 0x11;
	ret |= akm4753_i2c_write_regs(0x01, &data, 1);
#elif defined(CONFIG_BOARD_X1000_YOUTH_V10)
	/* Set up 2.1-channels mode */
	data = 0x21;
	ret |= akm4753_i2c_write_regs(0x01, &data, 1);
#elif defined(CHUANGXIN_CR_2122)
	/* Set up stereo mode(HPF, LPF individual mode) */
	data = 0xd1;
	ret |= akm4753_i2c_write_regs(0x01, &data, 1);
#endif
	/* Mute DATT volume */
	data = 0xff;
	ret |= akm4753_i2c_write_regs(0x05, &data, 1);
	ret |= akm4753_i2c_write_regs(0x06, &data, 1);

	/* Init 0x07 ~ 0x7d registers */
        for (i = 0; i < sizeof(akm4753_i2s_registers) / sizeof(akm4753_i2s_registers[0]); i++){
                ret |= akm4753_i2c_write_regs(akm4753_i2s_registers[i][0], &akm4753_i2s_registers[i][1], 1);
	}

	/* Power on DAC ADC DSP */
	data = 0xbd;
	ret |= akm4753_i2c_write_regs(0x04, &data, 1);

	if (ret)
		printk("===>ERROR: akm4753 init error!\n");
        return ret;
}

static int codec_clk_setup(void)
{
	int ret = 0;
	char data = 0x0;
	/* I2S clk setup */
	data = 0xb7;
	ret |= akm4753_i2c_write_regs(0x02, &data, 1);
	data = 0xc3;
	ret |= akm4753_i2c_write_regs(0x03, &data, 1);
	data = 0x84;
	ret |= akm4753_i2c_write_regs(0x04, &data, 1);
	msleep(5);

	return ret;
}

static int codec_init(void)
{
	int ret = 0;
	struct device *dev;
	struct extcodec_platform_data *akm4753;

	if (akm4753_client == NULL){
		printk("The i2c is not register, akm4753 can't init\n");
		return -1;
	}
	dev = &akm4753_client->dev;
	akm4753 = dev->platform_data;

	/* reset PDN pin */
	if (akm4753->power->gpio != -1){
		gpio_set_value(akm4753->power->gpio, akm4753->power->active_level);
		msleep(20);
		gpio_set_value(akm4753->power->gpio, !akm4753->power->active_level);
		msleep(1);
	}

	/* clk setup */
	ret |= codec_clk_setup();

	/* DAC outputs setup */
	ret |= codec_dac_setup();

	return ret;
}

static int dump_codec_regs(void)
{
	int i;
	int ret = 0;
	unsigned char reg_val[126] = {0};

	printk("akm4753 registers:\n");

	ret = akm4753_i2c_read_reg(0x0, reg_val, 126);
	for (i = 0; i < 126; i++){
		printk("reg 0x%02x = 0x%02x\n", i, reg_val[i]);
	}
	return ret;
}

static int codec_set_replay_volume(int *val)
{
	char data = 0x0;
	/* get current volume */
	if (*val < 0) {
		*val = user_replay_volume;
		return *val;
	} else if (*val > 100)
		*val = 100;
	user_replay_volume = *val;

	if (*val) {
#ifdef CONFIG_BOARD_X1000_HL01_V10
		/* use -16dB ~ -49dB for 3w speaker */
		data = (16 + (100-*val)/3) *2;
		akm4753_i2c_write_regs(0x05, &data, 1);
		akm4753_i2c_write_regs(0x06, &data, 1);
#else
		/* use 0dB ~ -50dB */
		data = 100 - *val;
		akm4753_i2c_write_regs(0x05, &data, 1);
		akm4753_i2c_write_regs(0x06, &data, 1);
#endif
		if (user_replay_state || user_linein_state)
			gpio_enable_spk_en();
	} else{
		/* Digital Volume mute */
		data = 0xff;
		akm4753_i2c_write_regs(0x05, &data, 1);
		akm4753_i2c_write_regs(0x06, &data, 1);
		gpio_disable_spk_en();
	}
	return *val;
}

static int codec_suspend(void)
{
	int ret = 0;
	unsigned char data;
	gpio_disable_spk_en();
	data = 0x84;
	ret |= akm4753_i2c_write_regs(0x04, &data, 1);
	return 0;
}

static int codec_resume(void)
{
	int ret = 0;
	unsigned char data;
	data = 0xbd;
	ret |= akm4753_i2c_write_regs(0x04, &data, 1);
	msleep(5);

	if (user_replay_volume && (user_replay_state || user_linein_state))
		gpio_enable_spk_en();
	return 0;
}

static int codec_shutdown(void)
{
	int ret = 0;
	unsigned char data;
	struct device *dev;
	struct extcodec_platform_data *akm4753;

	if (akm4753_client == NULL){
		printk("The i2c is not register, akm4753 can't init\n");
		return -1;
	}
	dev = &akm4753_client->dev;
	akm4753 = dev->platform_data;

	gpio_disable_spk_en();

	akm4753_i2c_read_reg(0x04, &data, 1);
	data = 0x0;
	ret |= akm4753_i2c_write_regs(0x04, &data, 1);

	if (akm4753->power->gpio != -1){
		gpio_set_value(akm4753->power->gpio, akm4753->power->active_level);
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
			if (user_linein_state) {
				check_replay_rate((unsigned long *)arg);
				user_replay_rate = *(unsigned long *)arg;
				break;
			}
			if (user_replay_rate == *(unsigned long *)arg)
				break;

			ret = codec_set_replay_rate((unsigned long*)arg);
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
			dump_codec_regs();
			break;
		default:
			printk("JZ CODEC:%s:%d: Unkown IOC commond\n", __FUNCTION__, __LINE__);
			ret = -1;
		}
	}

	return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void codec_early_suspend(struct early_suspend *handler)
{
}
static void codec_late_resume(struct early_suspend *handler)
{
}
#endif

static int __devinit akm4753_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        int ret = 0;
        struct device *dev = &client->dev;
        struct extcodec_platform_data *akm4753 = dev->platform_data;

        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
                ret = -ENODEV;
                return ret;
	}

	if (akm4753->power->gpio != -1) {
		ret = gpio_request_one(akm4753->power->gpio,
				GPIOF_DIR_OUT, "akm4753-pdn");
		if (ret != 0) {
			dev_err(dev, "Can't request pdn pin = %d\n",
					akm4753->power->gpio);
			return ret;
		} else
			gpio_set_value(akm4753->power->gpio, akm4753->power->active_level);
	}

        akm4753_client = client;

        return 0;
}

static int akm4753_i2c_remove(struct i2c_client *client)
{
        struct device *dev = &client->dev;
        struct extcodec_platform_data *akm4753 = dev->platform_data;

	if (akm4753->power->gpio != -1)
		gpio_free(akm4753->power->gpio);

        akm4753_client = NULL;

        return 0;
}

static int jz_codec_probe(struct platform_device *pdev)
{
	codec_platform_data = pdev->dev.platform_data;

	codec_platform_data->codec_sys_clk = AKM4753_EXTERNAL_CODEC_CLOCK;

	jz_set_hp_detect_type(SND_SWITCH_TYPE_GPIO,
			&codec_platform_data->gpio_hp_detect,
			NULL,
			NULL,
			NULL,
			&codec_platform_data->gpio_linein_detect,
			-1);

	if (codec_platform_data->gpio_amp_power.gpio != -1) {
		if (gpio_request(codec_platform_data->gpio_amp_power.gpio, "gpio_amp_power") < 0) {
			gpio_free(codec_platform_data->gpio_amp_power.gpio);
			gpio_request(codec_platform_data->gpio_amp_power.gpio,"gpio_amp_power");
		}
		/* power on amplifier */
		gpio_direction_output(codec_platform_data->gpio_amp_power.gpio, codec_platform_data->gpio_amp_power.active_level);
	}

	if (codec_platform_data->gpio_spk_en.gpio != -1) {
		if (gpio_request(codec_platform_data->gpio_spk_en.gpio, "gpio_spk_en") < 0) {
			gpio_free(codec_platform_data->gpio_spk_en.gpio);
			gpio_request(codec_platform_data->gpio_spk_en.gpio,"gpio_spk_en");
		}
		gpio_disable_spk_en();
	}

	return 0;
}

static int jz_codec_remove(struct platform_device *pdev)
{
	if (codec_platform_data->gpio_spk_en.gpio != -1)
		gpio_free(codec_platform_data->gpio_spk_en.gpio);
	codec_platform_data = NULL;

	return 0;
}

static const struct i2c_device_id akm4753_id[] = {
        { "akm4753", 0 },
        { }
};
MODULE_DEVICE_TABLE(i2c, akm4753_id);

static struct i2c_driver akm4753_i2c_driver = {
        .driver.name = "akm4753",
        .probe = akm4753_i2c_probe,
        .remove = akm4753_i2c_remove,
        .id_table = akm4753_id,
};

static struct platform_driver jz_codec_driver = {
        .probe          = jz_codec_probe,
        .remove         = jz_codec_remove,
        .driver         = {
                .name   = "akm4753_codec",
                .owner  = THIS_MODULE,
        },
};

/**
 * Module init
 */
static int __init init_akm4753_codec(void)
{
	int ret = 0;

	mutex_init(&i2c_access_lock);
	mutex_init(&switch_dev_lock);

	ret = i2s_register_codec("i2s_external_codec", (void *)jzcodec_ctl, AKM4753_EXTERNAL_CODEC_CLOCK, CODEC_MODE);
	if (ret < 0){
		printk("i2s audio is not support\n");
		return ret;
	}

        ret = platform_driver_register(&jz_codec_driver);
        if (ret) {
                printk("JZ CODEC: Could not register jz_codec_driver\n");
                return ret;
        }

        ret = i2c_add_driver(&akm4753_i2c_driver);
        if (ret) {
                printk("JZ CODEC: Could not register akm4753 i2c driver\n");
		platform_driver_unregister(&jz_codec_driver);
                return ret;
        }

#ifdef CONFIG_HAS_EARLYSUSPEND
        early_suspend.suspend = codec_early_suspend;
        early_suspend.resume = codec_late_resume;
        early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
        register_early_suspend(&early_suspend);
#endif
	printk("audio codec akm4753 register success\n");
	return 0;
}

/**
 * Module exit
 */
static void __exit cleanup_akm4753_codec(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
        unregister_early_suspend(&early_suspend);
#endif
	i2c_del_driver(&akm4753_i2c_driver);
	platform_driver_unregister(&jz_codec_driver);
}
module_init(init_akm4753_codec);
module_exit(cleanup_akm4753_codec);
MODULE_LICENSE("GPL");
