/*
 * sy6026l.c  --  SY6026L ALSA SoC Audio driver
 *
 * Copyright 2017 Ingenic Semiconductor Co.,Ltd
 *
 * Author: Cli <chen.li@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/gpio.h>
#include <sound/sy6026l.h>
#include "sy6026l.h"

/* codec private data */
struct sy6026l_priv {
	struct i2c_client *i2c_client;
#define REGVAL_MAX_WIDTH   (20)
	u8 buf[REGVAL_MAX_WIDTH + 1];
	struct mutex reg_lock;

	struct mutex soft_lock;
	unsigned int master_vol;
	unsigned int power_on:1;
	unsigned int mute:1;
	u8 i2s_ctrl;
};

static int sy6026l_read_hw_reg(struct sy6026l_priv *s, u8 reg, u8 *buf, int len)
{
	int ret;

	BUG_ON(!(len > 0 && len <= REGVAL_MAX_WIDTH));

	mutex_lock(&s->reg_lock);
	ret = i2c_master_send(s->i2c_client, &reg, 1);
	if (ret != 1) {
		ret = -EIO;
		dev_err(&s->i2c_client->dev, "send read reg(0x%02x) failed(err = %d)\n", reg, ret);
		goto out;
	}
	ret = i2c_master_recv(s->i2c_client, buf, len);
	if (ret != len) {
		ret = -EIO;
		dev_err(&s->i2c_client->dev, "read reg(0x%02x) failed(err = %d)\n", reg, ret);
		goto out;
	}
out:
	mutex_unlock(&s->reg_lock);
	return ret;
}

static int sy6026l_write_hw_reg(struct sy6026l_priv *s, u8 reg, u8 *buf, int len)
{
	int ret;

	BUG_ON(!(len > 0 && len <= REGVAL_MAX_WIDTH));

	mutex_lock(&s->reg_lock);

	s->buf[0] = reg;
	memcpy(s->buf + 1, buf, len);

	ret = i2c_master_send(s->i2c_client, s->buf, len + 1);
	if (ret != len + 1) {
		dev_err(&s->i2c_client->dev, "write reg(0x%02x) failed(err = %d)\n", reg, ret);
		ret = -EIO;
		goto out;
	}
out:
	mutex_unlock(&s->reg_lock);
	return ret;
}

static int to_hex(char ch)
{
	switch (ch) {
		case 'A': case 'B': case 'C':
		case 'D': case 'E': case 'F':
			return ch - 'A' + 10;
		case 'a': case 'b': case 'c':
		case 'd': case 'e': case 'f':
			return ch - 'a' + 10;
		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			return ch - '0';
		default:
			return -EINVAL;
	}
}

static int get_one_byte(const char *src, uint8_t *dst)
{
	int h, l;
	h = to_hex(*src++);
	if (h < 0)
		return h;

	l = to_hex(*src);
	if (l < 0)
		return l;
	*dst = h << 4 | l;
	return 0;
}

int sy6026_write_reg_str(struct sy6026l_priv *s, const char *str)
{
	int len = strlen(str);
	uint8_t *dst;
	int ret = 0;

	if (len < 5 || str[2] != '='
			|| (len - 3) % 2
			|| ((len - 1) / 2) > REGVAL_MAX_WIDTH + 1)
		return -EINVAL;

	mutex_lock(&s->reg_lock);
	if (get_one_byte(str, &s->buf[0]) < 0) {
		ret = -EINVAL;
		goto out;
	}

	printk("--> %02x=", s->buf[0]);
	for (dst = &s->buf[1], str += 3; *str != '\0'; ) {
		if (get_one_byte(str, dst) < 0) {
			ret = -EINVAL;
			goto out;
		}
		printk("%02x", (int)*dst);
		str += 2;
		dst += 1;
	}
	printk("len %d\n", dst - s->buf);

	ret = i2c_master_send(s->i2c_client, s->buf, dst - s->buf);
	if (ret != (dst - s->buf)) {
		dev_err(&s->i2c_client->dev, "write reg(0x%02x) failed(err = %d)\n", s->buf[0], ret);
		ret = -EIO;
		goto out;
	}
out:
	mutex_unlock(&s->reg_lock);
	return ret;
}

enum {
	SY6026L_PLAYBACK_VOL = 0,
	SY6026L_REG_NUM,
};

static unsigned int sy6026l_read(struct snd_soc_codec *codec, unsigned int reg)
{
	struct sy6026l_priv *s = snd_soc_codec_get_drvdata(codec);

	unsigned int value = (~0U);
	switch (reg) {
		case SY6026L_PLAYBACK_VOL:
			value = s->master_vol;
			break;
		default:
			break;
	}
	return value;
}

static int sy6026l_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	struct sy6026l_priv *s = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	switch (reg) {
		case SY6026L_PLAYBACK_VOL:
			dev_info(codec->dev, "set volume [%d]\n", value);
			s->master_vol = value;
			if (s->power_on)
				ret = sy6026l_write_hw_reg(s, MASTER_VOLUME, (u8*)&s->master_vol, 1);
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static const DECLARE_TLV_DB_SCALE(out_tlv, -12700, 50, 0);
static const struct snd_kcontrol_new sy6026l_snd_controls[] = {
	SOC_SINGLE_TLV("Master Playback Volume", SY6026L_PLAYBACK_VOL, 0, 255, 0, out_tlv),
};

static const struct snd_soc_dapm_widget sy6026l_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "Playback", SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_OUTPUT("HPOUT"),
};

static const struct snd_soc_dapm_route sy6026l_intercon[] = {
	{ "HPOUT", NULL, "DAC"},
};

static int sy6026l_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	return 0;
}

static int sy6026l_digital_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	struct sy6026l_priv *s = snd_soc_codec_get_drvdata(codec);

	dev_info(codec->dev, "set mute [%d] %d %d\n", mute, s->power_on, s->mute);
	if (s->mute == !!mute)
		return 0;
	s->mute = !!mute;

	mutex_lock(&s->soft_lock);
	if (s->power_on) {
		u8  soft_mute = s->mute ? 0x1b : 0x10;
		u8  master_vol = s->mute ? 0x0 : s->master_vol;
		sy6026l_write_hw_reg(s, MASTER_VOLUME, &master_vol, 1);
		sy6026l_write_hw_reg(s, SOFT_MUTE, &soft_mute, 1);
		if (s->mute) {
			/*TEMP CODE: keep the i2s clk continue, before mute worked, avoid pop AIC_FR enable*/
			msleep(100);
		}
	}
	mutex_unlock(&s->soft_lock);
	return 0;
}

static int sy6026l_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct sy6026l_priv *s = snd_soc_codec_get_drvdata(codec);
	u8 i2s_ctrl = I2S_CTRL_ENABLE | I2S_CTRL_VBITS_24;

	dev_info(codec->dev, "set fmt [%x]\n", fmt);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:
			i2s_ctrl |= I2S_CTRL_I2S_MODE;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:
			i2s_ctrl |= I2S_CTRL_RJ_MODE;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			i2s_ctrl |= I2S_CTRL_LJ_MODE;
			break;
		default:
			return -EINVAL;
	}

	switch(fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			break;
		case SND_SOC_DAIFMT_NB_IF:
			i2s_ctrl |= I2S_CTRL_LR_POL;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			i2s_ctrl |= I2S_CTRL_SCLK_INV;
			break;
		case SND_SOC_DAIFMT_IB_IF:
			i2s_ctrl |= I2S_CTRL_LR_POL | I2S_CTRL_SCLK_INV;
			break;
	}

	if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) != SND_SOC_DAIFMT_CBS_CFS)
		return -EINVAL;

	s->i2s_ctrl = i2s_ctrl;
	mutex_lock(&s->soft_lock);
	if (s->power_on)
		sy6026l_write_hw_reg(s, I2S_CONTROL, &(s->i2s_ctrl) ,1);
	mutex_unlock(&s->soft_lock);
	return 0;
}

static int sy6026l_trigger(struct snd_pcm_substream *substream, int cmd,
		struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			dev_info(codec->dev, "set trigger [start]\n");
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			dev_info(codec->dev, "set trigger [stop]\n");
			break;
	}
	return 0;
}

static int sy6026l_power_on(struct snd_soc_codec *codec)
{
	struct sy6026l_priv *s = snd_soc_codec_get_drvdata(codec);
	u8 master_vol = 0x0;
	u8 soft_mute = 0x1b;

	sy6026l_write_hw_reg(s, MASTER_VOLUME, &master_vol, 1);
	sy6026l_write_hw_reg(s, SOFT_MUTE, &soft_mute, 1);
	sy6026_write_reg_str(s, "08=b6");
	sy6026_write_reg_str(s, "09=b6");
	sy6026_write_reg_str(s, "0a=79");
	sy6026l_write_hw_reg(s, I2S_CONTROL, &(s->i2s_ctrl) ,1);
	sy6026_write_reg_str(s, "18=5f");
	sy6026_write_reg_str(s, "1b=5d");
	sy6026_write_reg_str(s, "23=00");
	sy6026_write_reg_str(s, "36=00000000");
	sy6026_write_reg_str(s, "4e=010000");
	sy6026_write_reg_str(s, "4f=7ff800");
	sy6026_write_reg_str(s, "50=010000");
	sy6026_write_reg_str(s, "51=7ff000");
	sy6026_write_reg_str(s, "52=010000");
	sy6026_write_reg_str(s, "53=7f0000");
	sy6026_write_reg_str(s, "55=40203210");
	sy6026_write_reg_str(s, "56=0000002e");
	sy6026_write_reg_str(s, "58=00f403");
	sy6026_write_reg_str(s, "59=441ff0");

	sy6026_write_reg_str(s, "5b=0000001e");
	sy6026_write_reg_str(s, "60=0100000f");
	sy6026_write_reg_str(s, "61=3cc30c");
	sy6026_write_reg_str(s, "62=7fbbcd");
	sy6026_write_reg_str(s, "65=7fbbcd");
	sy6026_write_reg_str(s, "68=7fbbcd");
	sy6026_write_reg_str(s, "6b=7fbbcd");
	sy6026_write_reg_str(s, "63=7ffc96");
	sy6026_write_reg_str(s, "66=7ffc96");
	sy6026_write_reg_str(s, "69=7ffc96");
	sy6026_write_reg_str(s, "6c=7ffc96");
	sy6026_write_reg_str(s, "6d=010000");
	sy6026_write_reg_str(s, "6e=7ff000");

	sy6026_write_reg_str(s, "76=0f");

	sy6026_write_reg_str(s, "85=00000003");
	sy6026_write_reg_str(s, "86=00001003");

	sy6026_write_reg_str(s, "39=00000000000000000000000000000000008001ff");
	sy6026_write_reg_str(s, "47=00000000007b11d3000000000002771700027717");
	sy6026_write_reg_str(s, "49=00000000007b11d3000000001f827717007d88e9");
	sy6026_write_reg_str(s, "4a=00000000005586e10000000000153c9000153c90");

	sy6026_write_reg_str(s, "21=10");
	sy6026_write_reg_str(s, "22=03");

	mutex_lock(&s->soft_lock);
	s->power_on = 1;
	soft_mute = s->mute ? 0x1b : 0x10;
	sy6026l_write_hw_reg(s, MASTER_VOLUME, (u8 *)(&s->master_vol), 1);
	sy6026l_write_hw_reg(s, SOFT_MUTE, &soft_mute, 1);
	mutex_unlock(&s->soft_lock);
	return 0;
}

static int sy6026l_power_off(struct snd_soc_codec *codec)
{
	struct sy6026l_priv *s = snd_soc_codec_get_drvdata(codec);
	u8 soft_mute = 0x1b;
	u8 master_vol = 0x0;
	u8 tmp = 0x13;

	mutex_lock(&s->soft_lock);
	s->power_on = 0;
	/*mute*/
	sy6026l_write_hw_reg(s, MASTER_VOLUME, &master_vol, 1);
	sy6026l_write_hw_reg(s, SOFT_MUTE, &soft_mute, 1);
	/* enter standby */
	sy6026l_write_hw_reg(s, 0x22, &tmp, 1);
	msleep(10);
	/* enter shutdown */
	tmp = 0x33;
	sy6026l_write_hw_reg(s, 0x22, &tmp, 1);
	msleep(20);
	mutex_unlock(&s->soft_lock);
	return 0;
}

static int sy6026l_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	char* bias_str[] = {
		"OFF", "STANDBY", "PREPARE", "ON",
	};

	dev_info(codec->dev, "set bias level [%s]\n", bias_str[level]);

	switch (level) {
		case SND_SOC_BIAS_ON:
			sy6026l_power_on(codec);
			break;
		case SND_SOC_BIAS_PREPARE:
			break;
		case SND_SOC_BIAS_STANDBY:
			break;
		case SND_SOC_BIAS_OFF:
			sy6026l_power_off(codec);
			break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

#define SY6026L_RATES SNDRV_PCM_RATE_8000_96000

#define SY6026L_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
		SNDRV_PCM_FMTBIT_S24_LE)

static const struct snd_soc_dai_ops sy6026l_dai_ops = {
	.hw_params	= sy6026l_hw_params,
	.digital_mute	= sy6026l_digital_mute,
	.set_fmt	= sy6026l_set_dai_fmt,
	.trigger        = sy6026l_trigger,
};

static struct snd_soc_dai_driver sy6026l_dai = {
	.name = "sy6026l-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SY6026L_RATES,
		.formats = SY6026L_FORMATS,
	},
	.ops = &sy6026l_dai_ops,
};

static int sy6026l_probe(struct snd_soc_codec *codec)
{
	struct sy6026l_priv *s = snd_soc_codec_get_drvdata(codec);
	struct sy6026l_platform_data *pdata = s->i2c_client->dev.platform_data;

	u8 dev_id = 0;
	if (gpio_is_valid(pdata->gpio_rst_b)) { /*reset codec*/
		gpio_direction_output(pdata->gpio_rst_b, pdata->gpio_rst_b_level ? 1 : 0);
		udelay(200); /*us*/
		gpio_direction_output(pdata->gpio_rst_b, pdata->gpio_rst_b_level ? 0 : 1);
		mdelay(14); /*ms*/
	}

	sy6026l_read_hw_reg(s, DEV_ID, &dev_id ,1);

	if (dev_id != 0x25 && dev_id != 0x35) {
		dev_err(&s->i2c_client->dev, "codec check dev id(0x%02x) failed\n", dev_id);
		return -ENODEV;
	}

	s->master_vol = 205;
	s->i2s_ctrl = I2S_CTRL_ENABLE | I2S_CTRL_VBITS_24 | I2S_CTRL_I2S_MODE;
	s->mute = 1;
	dev_info(&s->i2c_client->dev, "codec dev id is 0x%02x\n", dev_id);
	return 0;
}

/* power down chip */
static int sy6026l_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_sy6026l = {
	.probe =	sy6026l_probe,
	.remove =	sy6026l_remove,
	.read = sy6026l_read,
	.write = sy6026l_write,
	.set_bias_level = sy6026l_set_bias_level,
	.dapm_widgets = sy6026l_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(sy6026l_dapm_widgets),
	.dapm_routes = sy6026l_intercon,
	.num_dapm_routes = ARRAY_SIZE(sy6026l_intercon),
	.controls =	sy6026l_snd_controls,
	.num_controls = ARRAY_SIZE(sy6026l_snd_controls),
};


/*Just for debug*/
static ssize_t debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sy6026l_priv *s =  dev_get_drvdata(dev);
	u8 dev_id;
	gpio_direction_output(GPIO_PB(5), 0);
	mdelay(10);
	gpio_direction_output(GPIO_PB(5), 1);
	mdelay(13);
	sy6026l_read_hw_reg(s, DEV_ID, &dev_id ,1);
	dev_info(&s->i2c_client->dev, "codec dev id is 0x%02x\n", dev_id);
	return 0;
}

static struct device_attribute sy6026l_debug_sysfs_attrs =
__ATTR_RO(debug);

static int sy6026l_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *id)
{
	struct sy6026l_platform_data *pdata = i2c->dev.platform_data;
	struct sy6026l_priv *sy6026l;
	int ret;

	sy6026l = devm_kzalloc(&i2c->dev, sizeof(struct sy6026l_priv),
			GFP_KERNEL);
	if (sy6026l == NULL)
		return -ENOMEM;

	sy6026l->i2c_client = i2c;
	i2c_set_clientdata(i2c, sy6026l);
	mutex_init(&sy6026l->reg_lock);
	mutex_init(&sy6026l->soft_lock);

	if (gpio_is_valid(pdata->gpio_fault_b)) {
		ret = gpio_request_one(pdata->gpio_fault_b,
				pdata->gpio_fault_b_level ? GPIOF_OUT_INIT_LOW: GPIOF_OUT_INIT_HIGH,
				"sy6026l_fault_b");
		if (ret < 0) {
			dev_err(&i2c->dev, "fault b request failed (err %d)\n", ret);
			return ret;
		}
		udelay(100);
	}
	if (gpio_is_valid(pdata->gpio_rst_b)) {
		ret = gpio_request_one(pdata->gpio_rst_b,
				pdata->gpio_rst_b_level ? GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW,
				"sy6026l_rst_b");
		if (ret < 0) {
			dev_err(&i2c->dev, "rst b request failed (err %d)\n", ret);
			goto err_rst_b;
		}
	}

	ret = snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_sy6026l, &sy6026l_dai, 1);
	if (ret != 0) {
		dev_err(&i2c->dev, "Failed to register CODEC: %d\n", ret);
		goto err_register;
	}

	ret = device_create_file(&i2c->dev, &sy6026l_debug_sysfs_attrs);
	if (ret)
		dev_warn(&i2c->dev,"attribute %s create failed %x",
				attr_name(sy6026l_debug_sysfs_attrs), ret);
	dev_info(&i2c->dev, "CODEC register success\n");
	return 0;

err_register:
	if (gpio_is_valid(pdata->gpio_rst_b))
		gpio_free(pdata->gpio_rst_b);
err_rst_b:
	if (gpio_is_valid(pdata->gpio_fault_b))
		gpio_free(pdata->gpio_fault_b);
	return ret;
}

static int sy6026l_i2c_remove(struct i2c_client *i2c)
{
	struct sy6026l_platform_data *pdata = i2c->dev.platform_data;

	snd_soc_unregister_codec(&i2c->dev);

	if (gpio_is_valid(pdata->gpio_fault_b))
		gpio_free(pdata->gpio_fault_b);

	if (gpio_is_valid(pdata->gpio_rst_b))
		gpio_free(pdata->gpio_rst_b);
	return 0;
}

static const struct i2c_device_id sy6026l_i2c_id[] = {
	{ "sy6026l", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sy6026l_i2c_id);

static struct i2c_driver sy6026l_i2c_driver = {
	.driver = {
		.name = "sy6026l",
		.owner = THIS_MODULE,
	},
	.probe =    sy6026l_i2c_probe,
	.remove =   sy6026l_i2c_remove,
	.id_table = sy6026l_i2c_id,
};

static int __init sy6026l_modinit(void)
{
	int ret = 0;
	ret = i2c_add_driver(&sy6026l_i2c_driver);
	if (ret != 0) {
		printk(KERN_ERR "Failed to register SY6026L I2C driver: %d\n",
				ret);
	}
	return ret;
}
module_init(sy6026l_modinit);

static void __exit sy6026l_exit(void)
{
	i2c_del_driver(&sy6026l_i2c_driver);
}
module_exit(sy6026l_exit);

MODULE_DESCRIPTION("ASoC SY6026L driver");
MODULE_AUTHOR("chen li");
MODULE_LICENSE("GPL");
