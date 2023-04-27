/*
 * ak4951.c  --  AK4951 ALSA Soc Audio driver
 *
 * Copyright 2016 Ingenic Semiconductor Co.,Ltd
 *
 * Author: tjin <tao.jin@ingenic.com>
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
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <mach/jzsnd.h>
#include <linux/gpio.h>

#include "ak4951.h"

#define HPL_HPR_REPLAY
/* #define LOUT_ROUT_REPLAY */
#define SPP_SPN_REPLAY

//#define USE_GAIN_FULL_RANGE

/* codec private data */
struct ak4951_priv {
	unsigned int sysclk;
	struct i2c_client * i2c_client;
	struct mutex i2c_access_lock;
	enum snd_soc_control_type control_type;
	struct extcodec_platform_data *control_pin;
}*ak4951;

static unsigned char power_reg0;
static unsigned char power_reg1;
static unsigned char power_reg2;
static unsigned long user_replay_rate = 0;

//static const u8 ak4951_reg[] = { };

static int ak4951_i2c_write_regs(unsigned char reg, unsigned char* data, unsigned int len)
{
	int ret = -1;
	int i = 0;
	unsigned char buf[80] = {0};
	struct i2c_client *client = ak4951->i2c_client;

	if (client == NULL) {
		printk("===>NOTE: %s error\n", __func__);
		return ret;
	}

	if(!data || len <= 0){
		printk("===>ERROR: %s\n", __func__);
		return ret;
	}

	mutex_lock(&ak4951->i2c_access_lock);
	buf[0] = reg;
	for(; i < len; i++) {
		buf[i+1] = *data;
		data++;
	}

	ret = i2c_master_send(client, buf, len+1);
	if (ret < len+1)
		printk("%s 0x%02x err %d!\n", __func__, reg, ret);
	mutex_unlock(&ak4951->i2c_access_lock);

	return ret < len+1 ? ret : 0;
}

static int ak4951_i2c_read_reg(unsigned char reg, unsigned char* data, unsigned int len)
{
	int ret = -1;
	struct i2c_client *client = ak4951->i2c_client;

	if (client == NULL) {
		printk("===>NOTE: %s error\n", __func__);
		return ret;
	}

	if(!data || len <= 0){
		printk("===>ERROR: %s\n", __func__);
		return ret;
	}

	mutex_lock(&ak4951->i2c_access_lock);
	ret = i2c_master_send(client, &reg, 1);
	if (ret < 1) {
		printk("%s 0x%02x err\n", __func__, reg);
		mutex_unlock(&ak4951->i2c_access_lock);
		return -1;
	}

	ret = i2c_master_recv(client, data, len);
	if (ret < 0)
		printk("%s 0x%02x err\n", __func__, reg);
	mutex_unlock(&ak4951->i2c_access_lock);

	return ret < len ? ret : 0;
}

/*
 * read ak4951 register cache
 */
static inline unsigned int ak4951_read_reg_cache(struct snd_soc_codec *codec,
												 unsigned int reg)
{
	u8 *cache = codec->reg_cache;

	if (reg >= AK4951_REGNUM)
		return 0;

	return cache[reg];
}

/*
 * write ak4951 register cache
 */
static inline void ak4951_write_reg_cache(struct snd_soc_codec *codec,
										  unsigned int reg, unsigned char value)
{
	u8 *cache = codec->reg_cache;

	if (reg >= AK4951_REGNUM)
		return;

	cache[reg] = value;
	return;
}

/*
 * read ak4951 real register
 */
static inline unsigned int ak4951_i2c_read(struct snd_soc_codec *codec,
										   unsigned int reg)
{
	int ret = -1;
	unsigned char data;
	unsigned char reg_addr = (unsigned char)reg;

	if (reg >= AK4951_REGNUM) {
		printk("===>NOTE: %s reg error\n", __func__);
		return 0;
	}

	ret = ak4951_i2c_read_reg(reg_addr, &data, 1);

	return ret != 0 ? 0 : data;
}

/*
 * write ak4951 register
 */
static int ak4951_i2c_write(struct snd_soc_codec *codec, unsigned int reg,
							unsigned int value)
{
	int ret =0;
	unsigned char data;
	data = (unsigned char)value;

	ret = ak4951_i2c_write_regs((unsigned char)reg, &data, 1);
	if (ret != 0)
		printk("%s i2c error \n", __func__);
	else
		ak4951_write_reg_cache(codec, reg, data);

	return 0;
}

static int dump_codec_regs(void)
{
	int i;
	int ret = 0;
	unsigned char reg_val[80] = {0};

	printk("ak4951 registers:\n");

	ret = ak4951_i2c_read_reg(0x0, reg_val, 80);
	for (i = 0; i < 80; i++){
		printk("reg 0x%02x = 0x%02x\n", i, reg_val[i]);
	}
	return ret;
}

static int codec_reset(void)
{
	int ret = 0;

	if (ak4951->control_pin->power->gpio != -1) {
		gpio_set_value(ak4951->control_pin->power->gpio, ak4951->control_pin->power->active_level);
		mdelay(20);
		gpio_set_value(ak4951->control_pin->power->gpio,!ak4951->control_pin->power->active_level);
		mdelay(1);
	}

	return ret;
}

static int ingenic_snd_soc_info_volsw_2r(struct snd_kcontrol *kcontrol,
										 struct snd_ctl_elem_info *uinfo)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	int platform_max;
	int min = mc->min;
	int max = mc->max;

	if (!mc->platform_max)
		mc->platform_max = max;
	platform_max = mc->platform_max;

	if (platform_max == 1 && !strstr(kcontrol->id.name, " Volume"))
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	else
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;

	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = max - min;
	return 0;
}

static int ingenic_snd_soc_get_volsw_2r(struct snd_kcontrol *kcontrol,
										struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int reg = mc->reg;
	unsigned int reg2 = mc->rreg;
	unsigned int shift = mc->shift;
	int max = mc->max;
	int min = mc->min;
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int invert = mc->invert;

	ucontrol->value.integer.value[0] =
		((snd_soc_read(codec, reg) >> shift) & mask) - min;
	ucontrol->value.integer.value[1] =
		((snd_soc_read(codec, reg2) >> shift) & mask) - min;
	if (invert) {
		ucontrol->value.integer.value[0] =
			max - ucontrol->value.integer.value[0] - min;
		ucontrol->value.integer.value[1] =
			max - ucontrol->value.integer.value[1] - min;
	}

	return 0;
}

static int ingenic_snd_soc_put_volsw_2r(struct snd_kcontrol *kcontrol,
										struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int reg = mc->reg;
	unsigned int reg2 = mc->rreg;
	unsigned int shift = mc->shift;
	int max = mc->max;
	int min = mc->min;
	/*
	 * Platform_max is stand for the register bit mask.
	 * Here we just use some bits of the register.
	 * The other bits of the function, we should clear it.
	 */
	int platform_max = mc->platform_max;
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int mask_1 = (1 << fls(platform_max)) - 1;
	unsigned int invert = mc->invert;
	int err;
	unsigned int val, val2, val_mask;

	val_mask = mask_1 << shift;
	val = ((ucontrol->value.integer.value[0] + min) & mask);
	val2 = ((ucontrol->value.integer.value[1] + min) & mask);

	if (invert) {
		val = max - (val - min);
		val2 = max - (val2 - min);
	}

	val = val << shift;
	val2 = val2 << shift;

	err = snd_soc_update_bits_locked(codec, reg, val_mask, val);
	if (err < 0)
		return err;

	err = snd_soc_update_bits_locked(codec, reg2, val_mask, val2);
	return err;
}

#define INGENIC_SOC_DOUBLE_R_TLV(xname, reg_left, reg_right, xshift, xmin, xmax, xmask, xinvert, tlv_array) \
	{       .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),		\
			.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |					\
			SNDRV_CTL_ELEM_ACCESS_READWRITE,							\
			.tlv.p = (tlv_array),										\
			.info = ingenic_snd_soc_info_volsw_2r,						\
			.get = ingenic_snd_soc_get_volsw_2r, .put = ingenic_snd_soc_put_volsw_2r, \
			.private_value = (unsigned long)&(struct soc_mixer_control) \
			{.reg = reg_left, .rreg = reg_right, .shift = xshift,		\
			 .min = xmin, .max = xmax, .platform_max = xmask, .invert = xinvert} }

static const char *ak4951_dac_input_select[] = {"I2S DIN", "PFVOL IN", "(I2S + PFVOL)/2"};
static const char *ak4951_alc_switch[] = {"ALC DISABLE", "ALC ENABLE"};
static const char *ak4951_eq1_switch[] = {"DISABLE", "ENABLE"};
static const char *ak4951_eq2_switch[] = {"DISABLE", "ENABLE"};
static const char *ak4951_eq3_switch[] = {"DISABLE", "ENABLE"};
static const char *ak4951_eq4_switch[] = {"DISABLE", "ENABLE"};
static const char *ak4951_eq5_switch[] = {"DISABLE", "ENABLE"};
static const char *ak4951_wind_noise_reduct_switch[] = {"DISABLE", "ENABLE"};
static const char *ak4951_i2s_dout_signal_select[] = {"ADC OUT", "PROGRAM FILTER OUT"};
static const char *ak4951_dac_out_switch[] = {"LINEOUT", "SPKEROUT"};
static const char *ak4951_microphone_power_switch[] = {"HI-Z", "LIN1", "HI-Z", "LIN2"};

static const struct soc_enum ak4951_enum[] = {
	SOC_ENUM_SINGLE(ALC_MODE_CONTROL, 5, 2, ak4951_alc_switch),
	SOC_ENUM_SINGLE(DIGITAL_FILTER_MODE, 2, 3, ak4951_dac_input_select),
	SOC_ENUM_SINGLE(DIGITAL_FILTER_SELECT, 0, 2, ak4951_eq1_switch),
	SOC_ENUM_SINGLE(DIGITAL_FILTER_SELECT, 1, 2, ak4951_eq2_switch),
	SOC_ENUM_SINGLE(DIGITAL_FILTER_SELECT, 2, 2, ak4951_eq3_switch),
	SOC_ENUM_SINGLE(DIGITAL_FILTER_SELECT, 3, 2, ak4951_eq4_switch),
	SOC_ENUM_SINGLE(DIGITAL_FILTER_SELECT, 4, 2, ak4951_eq5_switch),
	SOC_ENUM_SINGLE(AUTO_HPF_CONTROL, 5, 2, ak4951_wind_noise_reduct_switch),
	SOC_ENUM_SINGLE(DIGITAL_FILTER_MODE, 0, 2, ak4951_i2s_dout_signal_select),
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(ak4951_dac_out_switch), ak4951_dac_out_switch),
	SOC_ENUM_SINGLE(SIGNAL_SELECT_1, 3, ARRAY_SIZE(ak4951_microphone_power_switch), ak4951_microphone_power_switch),
};

static int ak4951_dac_out_get (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	unsigned int value = 0;

	ucontrol->value.enumerated.item[0] = value;

	return 0;
}

static int ak4951_dac_out_put (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	int ret = 0;
	unsigned int value = ucontrol->value.enumerated.item[0];
	unsigned char data;

	if(!strcmp(ak4951_dac_out_switch[value], "LINEOUT")) {
		spk_mute(2);
		msleep(1);

		ak4951_i2c_read_reg(SIGNAL_SELECT_1, &data, 1);
		data &= 0x7f;
		ret |= ak4951_i2c_write_regs(0x02, &data, 1);

		ak4951_i2c_read_reg(POWER_MANAGE2, &data, 1);
		data &= 0xfd;
		data |= 0x30;
		ak4951_i2c_write_regs(POWER_MANAGE2, &data, 1);

	} else if(!strcmp(ak4951_dac_out_switch[value], "SPKEROUT")) {
		ak4951_i2c_read_reg(POWER_MANAGE2, &data, 1);
		data &= 0xcf;
		data |= 0x02;
		ak4951_i2c_write_regs(POWER_MANAGE2, &data, 1);
		msleep(2); /* necessary */
		ak4951_i2c_read_reg(SIGNAL_SELECT_1, &data, 1);
		data |= 0xcf;
		ak4951_i2c_write_regs(SIGNAL_SELECT_1, &data, 1);
		msleep(1);
		spk_unmute(1);
	}

	if (ret < 0) {
		printk("%s i2c write err\n", __func__);
		return ret;
	}

	return 0;
}

static const DECLARE_TLV_DB_SCALE(mic_tlv, 0, 300, 0);

/* unit: 0.01dB */
#ifdef USE_GAIN_FULL_RANGE
/* If you want to use full gain range:-90dB ~ +12dB, you can enable the code */
static const DECLARE_TLV_DB_SCALE(dac_tlv, -9000, 50, 1);
#else
/* Here we just use -30dB ~ 0dB */
static const DECLARE_TLV_DB_SCALE(dac_tlv, -3000, 50, 0);
#endif

/* We use full gain range:-52.5dB ~ +36dB, but the real step is 0.375dB */
static const DECLARE_TLV_DB_SCALE(adc_tlv, -5250, 37, 1);

static const struct snd_kcontrol_new ak4951_snd_controls[] = {
#ifdef USE_GAIN_FULL_RANGE
	/* If you want to use full gain range:-90dB ~ +12dB, you can enable the code */
	SOC_DOUBLE_R_TLV("Master Playback Volume", LCH_DIGITAL_VOLUME, RCH_DIGITAL_VOLUME, 0, 0xcc, 1, dac_tlv),
#else
	INGENIC_SOC_DOUBLE_R_TLV("Master Playback Volume", LCH_DIGITAL_VOLUME, RCH_DIGITAL_VOLUME, 0, 0x18, 0x54, 0xff, 1, dac_tlv),
#endif
	INGENIC_SOC_DOUBLE_R_TLV("Adc Volume", LCH_INPUT_VOLUME, RCH_INPUT_VOLUME, 0, 0x04, 0xf1, 0xff, 0, adc_tlv),
	SOC_SINGLE("Digital Playback mute", MODE_CONTROL_3, 5, 1, 0),
	SOC_SINGLE("Aux In Switch", DIGITAL_FILTER_MODE, 1, 1, 0),
	SOC_ENUM("I2S Sdto Output Select", ak4951_enum[8]),
	SOC_ENUM("Alc Switch", ak4951_enum[0]),
	SOC_ENUM("Eq1 Switch", ak4951_enum[2]),
	SOC_ENUM("Eq2 Switch", ak4951_enum[3]),
	SOC_ENUM("Eq3 Switch", ak4951_enum[4]),
	SOC_ENUM("Eq4 Switch", ak4951_enum[5]),
	SOC_ENUM("Eq5 Switch", ak4951_enum[6]),
	SOC_ENUM("Wind Noise Filter Switch", ak4951_enum[7]),
	SOC_ENUM_EXT("DAC OUT Switch", ak4951_enum[9], ak4951_dac_out_get, ak4951_dac_out_put),
	//SOC_ENUM("Dac Input Signal Select", ak4951_enum[1]),
	SOC_ENUM("Microphone Power Switch", ak4951_enum[10]),
	SOC_SINGLE_TLV("Microphone Gain", SIGNAL_SELECT_1, 0, 0x7, 0, mic_tlv),
	SOC_DOUBLE("ADC POWER Switch", POWER_MANAGE1, 0, 1, 0x1, 0),
};

/* ak4951 dapm widgets */
static const struct snd_soc_dapm_widget ak4951_dapm_widgets[] = {
	/* ak4951 dapm route */
	SND_SOC_DAPM_DAC("DAC", "Playback", SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC", "Capture", SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_INPUT("ADC IN"),
	SND_SOC_DAPM_OUTPUT("SPK OUT"),
	SND_SOC_DAPM_OUTPUT("HP OUT"),
};

static const struct snd_soc_dapm_route ak4951_audio_map[] = {
	/* ADC */
	{"ADC", NULL, "ADC IN"},

	/* DAC */
	{"HP OUT", "DAC OUT Switch", "DAC"},
	{"SPK OUT", "DAC OUT Switch", "DAC"},
};

static int ak4951_set_dai_sysclk(struct snd_soc_dai *codec_dai,
								 int clk_id, unsigned int freq, int dir)
{
	/*
	 * AK4951 only need a 24MHZ mclk as I2S master.
	 * It is set in asoc-i2s-v**.c.
	 */
	ak4951->sysclk = freq;
	return 0;
}

static int ak4951_hw_params(struct snd_pcm_substream *substream,
							struct snd_pcm_hw_params *params,
							struct snd_soc_dai *dai)
{
	/*
	 * Params_format and params_channels are not set here.
	 * Because if you set register DIF to 0x3, ak4951 support all the I2S format.
	 * Such as 16bit,20bit,24bit.
	 * And ak4951 support stereo channels.
	 * If the replay channel is mono, we should transfer it to stereo by AIC register.
	 */
	int rate = params_rate(params);
	int i;
	unsigned char data = 0x0;
#ifndef CONFIG_SND_ASOC_AK4951_AEC_MODE
	unsigned long mrate[9] = {
		8000, 12000, 16000, 24000, 11025,
		22050, 32000, 48000, 44100
	};
#endif
	unsigned char reg[9] = {
		0, 1, 2, 9, 5,
		7, 10,11,15
	};
#ifdef CONFIG_SND_ASOC_AK4951_AEC_MODE
	rate = 48000;
	i = 7;
#else
	/* set rate */
	for (i = 0; i < 9; i++)
		if (rate == mrate[i])
			break;

	if (i == 9){
		printk("%d is not support, fix it to 48000\n", rate);
		rate = 48000;
		i = 7;
	}
#endif
	if (user_replay_rate == rate)
		return 0;
	user_replay_rate = rate;
	ak4951_i2c_read_reg(0x06, &data, 1);
	data &= 0xf0;
	data |= reg[i];
	ak4951_i2c_write_regs(0x06, &data, 1);
	msleep(50);            // wait for ak4951 i2s clk stable.

	return 0;
}

static int ak4951_mute(struct snd_soc_dai *dai, int mute)
{
	if (!mute){

	} else {

	}
	return 0;
}

static int ak4951_set_bias_level(struct snd_soc_codec *codec,
								 enum snd_soc_bias_level level)
{
	/* There is no bias level function in ak4951 */
	switch (level) {
	case SND_SOC_BIAS_ON:
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		break;
	case SND_SOC_BIAS_OFF:
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

#ifdef  CONFIG_SND_ASOC_AK4951_AEC_MODE
#define AK4951_RATES   (SNDRV_PCM_RATE_48000)
#else
#define AK4951_RATES   (SNDRV_PCM_RATE_8000_48000)
#endif
#define AK4951_FORMATS_CAP (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)
//#define AK4951_FORMATS (SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_U24_LE)
#define AK4951_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE)

static int ak4951_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	/* snd_pcm_open will call it */
	return 0;
}
static void ak4951_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	/* snd_pcm_close will call it */
	return;
}

static struct snd_soc_dai_ops ak4951_dai_ops = {
	.hw_params	= ak4951_hw_params,
	.digital_mute	= ak4951_mute,
	.set_sysclk	= ak4951_set_dai_sysclk,
	.startup	= ak4951_startup,
	.shutdown	= ak4951_shutdown,
};

static struct snd_soc_dai_driver ak4951_dai = {
	.name = "ak4951-dai",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = AK4951_RATES,
		.formats = AK4951_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = AK4951_RATES,
		.formats = AK4951_FORMATS_CAP,
	},
	.ops = &ak4951_dai_ops,
};

static int ak4951_suspend(struct snd_soc_codec *codec)
{
	int ret = 0;
	unsigned char data;
	spk_mute(0);
	msleep(5);

	ak4951_i2c_read_reg(0x02, &data, 1);
	power_reg2 = data;
	data &= 0x7f;
	ret |= ak4951_i2c_write_regs(0x02, &data, 1);
	msleep(5);
	ak4951_i2c_read_reg(0x01, &data, 1);
	power_reg1 = data;
	data &= 0xcd;
	ret |= ak4951_i2c_write_regs(0x01, &data, 1);
	msleep(5);
	ak4951_i2c_read_reg(0x00, &data, 1);
	power_reg0 = data;
	data &= 0x40;
	ret |= ak4951_i2c_write_regs(0x00, &data, 1);
	return 0;
}

static int ak4951_resume(struct snd_soc_codec *codec)
{
	int ret = 0;
	unsigned char data;

	data = power_reg0;
	ret |= ak4951_i2c_write_regs(0x00, &data, 1);
	msleep(5);
	data = power_reg1;
	ret |= ak4951_i2c_write_regs(0x01, &data, 1);
	msleep(5);
	data = power_reg2;
	ret |= ak4951_i2c_write_regs(0x02, &data, 1);
	msleep(50);

	spk_unmute(0);
	return 0;
}

static int codec_dac_setup(void)
{
	int i;
	int ret = 0;
	char data = 0x0;

	/* Init 0x02 ~ 0x4F registers */
	for (i = 0; i < sizeof(ak4951_registers) / sizeof(ak4951_registers[0]); i++){
		ret |= ak4951_i2c_write_regs(ak4951_registers[i][0], &ak4951_registers[i][1], 1);
	}

	/* Power on DAC ADC DSP */
	data = 0xc7;
	ret |= ak4951_i2c_write_regs(0x00, &data, 1);
#ifdef LOUT_ROUT_REPLAY
	data = 0x24;
	ret |= ak4951_i2c_write_regs(0x04, &data, 1);
	data = 0x8d;
	ret |= ak4951_i2c_write_regs(0x01, &data, 1);
	data = 0x8f;
	ret |= ak4951_i2c_write_regs(0x01, &data, 1);
	msleep(5);
	data = 0x80;
	ret |= ak4951_i2c_write_regs(0x02, &data, 1);
#elif defined(SPP_SPN_REPLAY)
	data = 0x8c;
	ret |= ak4951_i2c_write_regs(0x01, &data, 1);
	data = 0x8e;
	ret |= ak4951_i2c_write_regs(0x01, &data, 1);
	data = 0x20;
	ret |= ak4951_i2c_write_regs(0x02, &data, 1);
	msleep(5);
	data = 0xa0;
	ret |= ak4951_i2c_write_regs(0x02, &data, 1);
#else
	data = 0xbc;
	ret |= ak4951_i2c_write_regs(0x01, &data, 1);
#endif

	if (ret)
		printk("===>ERROR: ak4951 init error!\n");
	return ret;
}

static int codec_clk_setup(void)
{
	int ret = 0;
	unsigned char data = 0x0;
	/* I2S clk setup */
	data = 0x08;
	ret |= ak4951_i2c_write_regs(0x01, &data, 1);
	data = 0x7b;
	ret |= ak4951_i2c_write_regs(0x05, &data, 1);
	data = 0x0b;
	ret |= ak4951_i2c_write_regs(0x06, &data, 1);
	data = 0x40;
	ret |= ak4951_i2c_write_regs(0x00, &data, 1);
	mdelay(2);
	data = 0x0c;
	ret |= ak4951_i2c_write_regs(0x01, &data, 1);
	mdelay(5);

	return ret;
}

static ssize_t ak4951_regs_show(struct device *dev,
								struct device_attribute *attr, char *buf)
{
	dump_codec_regs();
	return 0;
}

/* The codec register write interface is just for debug. */
#define AK4951_REG_SUM         0x4f
static ssize_t ak4951_regs_write(struct device *dev, struct device_attribute *attr,
								 const char *buf, size_t n)
{
	int ret;
	char * buf_2;
	unsigned char reg_addr, reg_val;

	reg_addr  = (unsigned char)simple_strtoul(buf, &buf_2, 0);
	buf_2 = skip_spaces(buf_2);
	reg_val = (unsigned char)simple_strtoul(buf_2, NULL, 0);

	if (reg_addr > AK4951_REG_SUM)
		return -EINVAL;

	printk("\nwrite reg: 0x%x 0x%x\n", reg_addr, reg_val);

	ret = ak4951_i2c_write_regs(reg_addr, &reg_val, 1);
	if (ret)
		printk("write reg fail\n");
	return n;
}

static struct device_attribute ak4951_sysfs_attrs =
	__ATTR(ak4951_regs, S_IRUGO | S_IWUSR, ak4951_regs_show, ak4951_regs_write);

static int ak4951_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	/* ak4951 pdn pin reset */
	ret = codec_reset();
	if (ret < 0) {
		dev_err(codec->dev, "Failed to issue reset\n");
		return ret;
	}

	/* clk setup */
	ret |= codec_clk_setup();

	/* DAC outputs setup */
	ret |= codec_dac_setup();

	/* Creat dump codec register file interface */
	ret = device_create_file(codec->dev, &ak4951_sysfs_attrs);
	if (ret)
		dev_warn(codec->dev,"attribute %s create failed %x",
				 attr_name(ak4951_sysfs_attrs), ret);
	return ret;
}

/* power down chip */
static int ak4951_remove(struct snd_soc_codec *codec)
{
	int ret = 0;
	unsigned char data;
	/* Mute AMP pin */
	spk_mute(0);
	msleep(5);

	/* Power down ak4951 */
	ak4951_i2c_read_reg(0x02, &data, 1);
	data &= 0x7f;
	ret |= ak4951_i2c_write_regs(0x02, &data, 1);

	ak4951_i2c_read_reg(0x01, &data, 1);
	data &= 0xcd;
	ret |= ak4951_i2c_write_regs(0x01, &data, 1);

	ak4951_i2c_read_reg(0x00, &data, 1);
	data &= 0x40;
	ret |= ak4951_i2c_write_regs(0x00, &data, 1);

	ak4951_i2c_read_reg(0x01, &data, 1);
	data &= 0xfb;
	ret |= ak4951_i2c_write_regs(0x01, &data, 1);

	if (ak4951->control_pin->power->gpio != -1){
		gpio_set_value(ak4951->control_pin->power->gpio, ak4951->control_pin->power->active_level);
	}

	if (ret < 0) {
		printk("%s i2c write err\n", __func__);
		return ret;
	}

	/* Remove the dump register file interface */
	device_remove_file(codec->dev, &ak4951_sysfs_attrs);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_ak4951 = {
	.probe =	ak4951_probe,
	.remove =	ak4951_remove,
	.suspend =	ak4951_suspend,
	.resume =	ak4951_resume,
	//.read = ak4951_read_reg_cache,
	.read =  ak4951_i2c_read,
	.write = ak4951_i2c_write,
	.set_bias_level = ak4951_set_bias_level,
	.reg_cache_size = AK4951_REGNUM,
	.reg_word_size = sizeof(u8),
	//.reg_cache_default = ak4951_reg,
	.controls =     ak4951_snd_controls,
	.num_controls = ARRAY_SIZE(ak4951_snd_controls),
	.dapm_widgets = ak4951_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(ak4951_dapm_widgets),
	.dapm_routes = ak4951_audio_map,
	.num_dapm_routes = ARRAY_SIZE(ak4951_audio_map),
};

static int ak4951_i2c_probe(struct i2c_client *i2c,
							const struct i2c_device_id *id)
{
	int ret = 0;
	struct device *dev = &i2c->dev;
	struct extcodec_platform_data *ak4951_data = dev->platform_data;

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C)) {
		return -ENODEV;
	}

	if (ak4951_data->power->gpio != -1) {
		ret = gpio_request_one(ak4951_data->power->gpio,
							   GPIOF_DIR_OUT, "ak4951_pdn");
		if (ret != 0) {
			dev_err(dev, "Can't request pdn pin = %d\n",
					ak4951_data->power->gpio);
			return ret;
		} else
			gpio_set_value(ak4951_data->power->gpio, ak4951_data->power->active_level);
	}

	ak4951 = kzalloc(sizeof(struct ak4951_priv), GFP_KERNEL);
	if (ak4951 == NULL)
		return -ENOMEM;

	ak4951->i2c_client   = i2c;
	ak4951->control_type = SND_SOC_I2C;
	ak4951->control_pin  = ak4951_data;

	mutex_init(&ak4951->i2c_access_lock);

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_ak4951, &ak4951_dai, 1);
	if (ret < 0) {
		kfree(ak4951);
		dev_err(&i2c->dev, "Faild to register codec\n");
	}
	return ret;
}

static int ak4951_i2c_remove(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct extcodec_platform_data *ak4951_data = dev->platform_data;

	if (ak4951_data->power->gpio != -1)
		gpio_free(ak4951_data->power->gpio);

	snd_soc_unregister_codec(&client->dev);

	kfree(ak4951);
	return 0;
}

static const struct i2c_device_id ak4951_i2c_id[] = {
	{ "ak4951", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ak4951_i2c_id);

static struct i2c_driver ak4951_i2c_driver = {
	.driver = {
		.name = "ak4951",
		.owner = THIS_MODULE,
	},
	.probe =    ak4951_i2c_probe,
	.remove =   ak4951_i2c_remove,
	.id_table = ak4951_i2c_id,
};

static int ak4951_modinit(void)
{
	int ret = 0;

	ret = i2c_add_driver(&ak4951_i2c_driver);
	if (ret != 0) {
		printk(KERN_ERR "Failed to register AK4951 I2C driver: %d\n",
		       ret);
	}
	return ret;
}
module_init(ak4951_modinit);

static void ak4951_exit(void)
{
	i2c_del_driver(&ak4951_i2c_driver);
}
module_exit(ak4951_exit);

MODULE_DESCRIPTION("Soc AK4951 driver");
MODULE_AUTHOR("tjin<tao.jin@ingenic.com>");
MODULE_LICENSE("GPL");
