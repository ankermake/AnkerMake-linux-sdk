/*
 * sound/soc/ingenic/icodec/icdc_d4.c
 * ALSA SoC Audio driver -- ingenic internal codec (icdc_d4) driver

 * Copyright 2017 Ingenic Semiconductor Co.,Ltd
 *	cli <chen.li@ingenic.com>
 *
 * Note: icdc_d4 is an internal codec for jz SOC
 *	 used for X1630 X1830 and so on
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #define DEBUG*/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <asm/div64.h>
#include <sound/soc-dai.h>
#include <linux/platform_data/asoc-ingenic.h>
#include <soc/irq.h>
#include "icdc_d4.h"

static int icdc_d4_debug = 1;
module_param(icdc_d4_debug, int, 0644);
#define DEBUG_MSG(msg...)			\
	do {					\
		if (icdc_d4_debug)		\
		printk(KERN_DEBUG"ICDC: " msg);	\
	} while(0)

static int adc_hpf_en = 1;
module_param(adc_hpf_en, int, 0644);

static u8 icdc_d4_reg_defcache[ICDC_REG_NUM] = {
	0x03, 0x00, 0x50, 0x0e, 0x50, 0x0e, 0x00, 0x05,
	0x00, 0x00, 0x0c, 0x00, 0x00, 0x03, 0x03, 0xff,
	0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x46, 0x41, 0x2c, 0x00, 0x26, 0x40, 0x36,
	0x20, 0x38, 0x00, 0x00, 0x0c,
};

static int icdc_d4_vaild_register(struct snd_soc_codec *codec, unsigned int reg)
{
	return icdc_register_is_vaild(reg);
}

static void dump_registers_hazard(struct icdc_d4 *icdc_d4)
{
	int reg = 0;
	dev_info(icdc_d4->dev, "-------------------register:");
	for ( ; reg < ICDC_REG_NUM; reg++) {
		if (reg % 8 == 0)
			printk("\n");
		if (icdc_register_is_vaild(reg))
			printk(" [0x%04x:0x%02x],", reg*sizeof(u32), readw(icdc_d4->mapped_base + reg * sizeof(u32)));
		else
			printk("  0x%04x:0x%02x ,", 0x0, 0x0);
	}
	printk("\n");
	dev_info(icdc_d4->dev, "----------------------------\n");
	return;
}

static int icdc_d4_write(struct snd_soc_codec *codec, unsigned int reg,
		unsigned int value)
{
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);

	snd_soc_cache_write(codec, reg, value);
	writel(value, icdc_d4->mapped_base + reg * sizeof(u32));
#ifdef DEBUG
	{
recheck:
		int timeout = 0x5;
		int val1 = readl(icdc_d4->mapped_base + reg * sizeof(u32));
		if (value != val1 && (--timeout))
			goto recheck;
		if (!timeout) {
			printk("icdc_d4 write 0x%p:(0x%02x->0x%02x) failed\n",
					(icdc_d4->mapped_base + reg * sizeof(u32)),
					value, val1);
			return 0;
		}
		printk("icdc_d4 write 0x%p:(0x%02x->0x%02x) ok\n",
				(icdc_d4->mapped_base + reg * sizeof(u32)), value, val1);
	}
#endif
	return 0;
}

static unsigned int icdc_d4_read(struct snd_soc_codec *codec, unsigned int reg)
{
	unsigned int val;
#ifdef DEBUG
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);
	val = readl(icdc_d4->mapped_base + reg * sizeof(u32));
#else
	snd_soc_cache_read(codec, reg, &val);
#endif
	return val;
}

static int icdc_d4_dac_disable(struct snd_soc_codec*);
static int icdc_d4_adc_disable(struct snd_soc_codec*);

static int icdc_d4_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	int i;
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);

	printk("%s enter set level %d\n", __func__, level);
	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		if (!icdc_d4->power_on) {
			snd_soc_update_bits(codec, ICDC_CGR, ICDC_CGR_DCRST_N|ICDC_CGR_SRST_N, 0);
			msleep(10);
			snd_soc_update_bits(codec, ICDC_CGR, ICDC_CGR_DCRST_N|ICDC_CGR_SRST_N,
					ICDC_CGR_DCRST_N|ICDC_CGR_SRST_N);
			msleep(10);
			/*i2s 32bit master*/
			snd_soc_write(codec, ICDC_CACR, ICDC_CACR_ADCVALDALEN_24BIT|ICDC_CACR_ADCI2SMODE_I2S|ICDC_CACR_ADCINTF_MONO);
			snd_soc_write(codec, ICDC_CMCR, ICDC_CMCR_MASTEREN|ICDC_CMCR_ADCDAC_MASTER|ICDC_CMCR_ADCDATALEN_32BIT|ICDC_CMCR_ADCI2SRST_N);
			snd_soc_write(codec, ICDC_CDC1R, ICDC_CDCR1_DACVALDALEN_24BIT|ICDC_CDCR1_DACI2SMODE_I2S);
			snd_soc_write(codec, ICDC_CDC2R, ICDC_CDCR2_DACDATALEN_32BIT|ICDC_CDCR2_DACI2SRST_N);
			msleep(10);
			/* codec precharge */
			snd_soc_update_bits(codec, ICDC_CHCR, ICDC_CHCR_HPOUTPOP_MSK, ICDC_CHCR_HPOUTPOP_PERCHARGE);
			msleep(10);
			/*datasheet said 0 is precharge, but the actual situation is the opposite*/
			for (i = 0; i <= 6; i++) {
				snd_soc_write(codec, ICDC_CCR, ICDC_CCR_DACCHAGESEL((0x3f >> (6 - i))) | ICDC_CCR_DACPERCHARGE_N);
				usleep_range(20000, 30000);
			}
			snd_soc_update_bits(codec, ICDC_CCR, ICDC_CCR_DACCHAGESEL_MSK, ICDC_CCR_DACCHAGESEL(0x1));
			msleep(20);
			icdc_d4->power_on = 1;
		}
		break;
	case SND_SOC_BIAS_OFF:
		if (icdc_d4->power_on) {
			icdc_d4_adc_disable(codec);
			icdc_d4_dac_disable(codec);
			snd_soc_update_bits(codec, ICDC_CHCR, ICDC_CHCR_HPOUTPOP_MSK, ICDC_CHCR_HPOUTPOP_PERCHARGE);
			msleep(10);
			snd_soc_write(codec, ICDC_CCR, ICDC_CCR_DACCHAGESEL_MSK);
			msleep(10);
			snd_soc_update_bits(codec, ICDC_CCR, ICDC_CCR_DACPERCHARGE_N, 0);
			msleep(10);
			for (i = 0; i < 6; i++) {
				usleep_range(20000, 30000);
				snd_soc_write(codec, ICDC_CCR, ((ICDC_CCR_DACCHAGESEL_MSK >> (6 - i)) & ICDC_CCR_DACCHAGESEL_MSK));
			}
			msleep(10);
			icdc_d4->power_on = 0;
		}
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

static int icdc_d4_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *params,
                struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	int sample_attr[] = {	96000, 48000,  44100, 36000, 24000,
		16000, 12000,  8000};
	int speed = params_rate(params);
	int speed_sel = 0;

	DEBUG_MSG("%s enter sample rate %d\n", __func__, speed);

	/*sample rate*/
	for (speed_sel = 0; speed < sample_attr[speed_sel] &&
			speed_sel < ARRAY_SIZE(sample_attr); speed_sel++)
		;
	if (speed_sel == ARRAY_SIZE(sample_attr))
		return -EINVAL;
	snd_soc_update_bits(codec, ICDC_CSRR, ICDC_CSRR_SAMPLERATE_MSK,
			ICDC_CSRR_SAMPLERATE(speed_sel));
	msleep(10);
	return 0;
}

static int icdc_d4_digital_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);

	DEBUG_MSG("%s enter mute %d\n", __func__, mute);
	if (mute) {
		snd_soc_update_bits(codec, ICDC_CASR, ICDC_CASR_DACLECHSRC_MSK, ICDC_CASR_DACLECHSRC_ZERO);
		snd_soc_update_bits(codec, ICDC_CHR, ICDC_CHR_HPOUTMUTE_N, 0);
		msleep(10);
	} else {
		if (!icdc_d4->dac_vol_zero) {
			snd_soc_update_bits(codec, ICDC_CASR, ICDC_CASR_DACLECHSRC_MSK, ICDC_CASR_DACLECHSRC_I2S);
			snd_soc_update_bits(codec, ICDC_CHR, ICDC_CHR_HPOUTMUTE_N, ICDC_CHR_HPOUTMUTE_N);
		}
		msleep(10);
	}

	icdc_d4->dac_mute = !!mute;

	return 0;
}

static int icdc_d4_trigger(struct snd_pcm_substream * stream, int cmd,
		struct snd_soc_dai *dai)
{
#ifdef DEBUG
	struct snd_soc_codec *codec = dai->codec;
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			dump_registers_hazard(icdc_d4);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			dump_registers_hazard(icdc_d4);
			break;
	}
#endif
	return 0;
}

#define ICDC_D4_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)
#define ICDC_D4_RATE    (SNDRV_PCM_RATE_8000|SNDRV_PCM_RATE_16000|SNDRV_PCM_RATE_44100| \
                        SNDRV_PCM_RATE_48000|SNDRV_PCM_RATE_96000)

static struct snd_soc_dai_ops icdc_d4_dai_ops = {
	.hw_params	= icdc_d4_hw_params,
	.digital_mute	= icdc_d4_digital_mute,
	.trigger = icdc_d4_trigger,
};

static struct snd_soc_dai_driver  icdc_d4_codec_dai = {
	.name = "icdc-d4-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 1,
		.rates = ICDC_D4_RATE,
		.formats = ICDC_D4_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = ICDC_D4_RATE,
		.formats = ICDC_D4_FORMATS,
	},
	.symmetric_rates = 1,
	.ops = &icdc_d4_dai_ops,
};

/* unit: 1.5dB */
static const DECLARE_TLV_DB_SCALE(hpout_tlv, -3900, 150, 0);

static int hpout_info_volsw(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = icdc_d4->hpout_maxvol;
	return 0;
}

static int hpout_volsw_put(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);
	struct snd_soc_card *card = codec->card;

	mutex_lock_nested(&card->dapm_mutex, SND_SOC_DAPM_CLASS_RUNTIME);
	if (ucontrol->value.integer.value[0] > icdc_d4->hpout_maxvol)
		ucontrol->value.integer.value[0] = icdc_d4->hpout_maxvol;
	if (ucontrol->value.integer.value[0] < 0)
		ucontrol->value.integer.value[0] = 0;
	icdc_d4->hpout_vol = ucontrol->value.integer.value[0];

	if (!icdc_d4->dac_enabled) {
		mutex_unlock(&card->dapm_mutex);
		return 0;
	}
	snd_soc_update_bits(codec, ICDC_CHCR, ICDC_CHCR_HPOUTGAIN_MSK, ICDC_CHCR_HPOUTGAIN(icdc_d4->hpout_vol));

	if (!icdc_d4->hpout_vol && !icdc_d4->dac_vol_zero) {
		if (!icdc_d4->dac_mute) {
			snd_soc_update_bits(codec, ICDC_CASR, ICDC_CASR_DACLECHSRC_MSK, ICDC_CASR_DACLECHSRC_ZERO);
			snd_soc_update_bits(codec, ICDC_CHR, ICDC_CHR_HPOUTMUTE_N, 0);
		}
		icdc_d4->dac_vol_zero = 1;
	}

	if (icdc_d4->hpout_vol && icdc_d4->dac_vol_zero) {
		if (!icdc_d4->dac_mute) {
			snd_soc_update_bits(codec, ICDC_CASR, ICDC_CASR_DACLECHSRC_MSK, ICDC_CASR_DACLECHSRC_I2S);
			snd_soc_update_bits(codec, ICDC_CHR, ICDC_CHR_HPOUTMUTE_N, ICDC_CHR_HPOUTMUTE_N);
		}
		icdc_d4->dac_vol_zero = 0;
	}

	msleep(10);
	mutex_unlock(&card->dapm_mutex);
	return 0;
}

static int hpout_volsw_get(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = icdc_d4->hpout_vol;
	return 0;
}

static const DECLARE_TLV_DB_SCALE(alc_tlv, -1800, 150, 0);

static int alc_info_volsw(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = icdc_d4->mic_maxvol;
	return 0;
}

static int alc_volsw_put(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);
	struct snd_soc_card *card = codec->card;

	mutex_lock_nested(&card->dapm_mutex, SND_SOC_DAPM_CLASS_RUNTIME);
	if (ucontrol->value.integer.value[0] > icdc_d4->mic_maxvol)
		ucontrol->value.integer.value[0] = icdc_d4->mic_maxvol;
	if (ucontrol->value.integer.value[0] < 0)
		ucontrol->value.integer.value[0] = 0;
	icdc_d4->mic_vol = ucontrol->value.integer.value[0];
	if (!icdc_d4->adc_enabled) {
		mutex_unlock(&card->dapm_mutex);
		return 0;
	}
	snd_soc_update_bits(codec, ICDC_CACR2, ICDC_CACR2_ALCGAIN_MSK, ICDC_CACR2_ALCGAIN(icdc_d4->mic_vol));
	msleep(10);
	mutex_unlock(&card->dapm_mutex);
	return 0;
}

static int alc_volsw_get(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = icdc_d4->mic_vol;
	return 0;
}

#define MICBIAS_USED    BIT(0)  /* No microphone bias is required on the product */
#define MICBIAS_ENABLE  BIT(1)  /* Manually open microphone bias, for some applications, want to open the microphone in advance.*/
static const char *micbias_mode[] = {"UNUSED", "DISABLE", "UNUSED" ,"ENABLE"};
static const struct soc_enum micbias_enum[] = {
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(micbias_mode), micbias_mode),
};

static int micbias_put(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);
	struct snd_soc_card *card = codec->card;

	mutex_lock_nested(&card->dapm_mutex, SND_SOC_DAPM_CLASS_RUNTIME);
	icdc_d4->micbias_enable = ucontrol->value.enumerated.item[0] & 0x3;

	if (!(icdc_d4->micbias_enable & MICBIAS_USED) ||
			!(icdc_d4->micbias_enable & MICBIAS_ENABLE)) {
		snd_soc_update_bits(codec, ICDC_CAACR, ICDC_CACCR_MICBAISCTR_MSK, 0);
		msleep(10);
		snd_soc_update_bits(codec, ICDC_CAACR, ICDC_CACCR_MICBIASEN, 0);
		msleep(10);
		icdc_d4->micbias_jiffies_vaild = 0;
	} else {
		bool changed = false;
		if (snd_soc_update_bits(codec, ICDC_CAACR, ICDC_CACCR_MICBIASEN, ICDC_CACCR_MICBIASEN)) {
			msleep(10);
			changed = true;
		}
		if (snd_soc_update_bits(codec, ICDC_CAACR, ICDC_CACCR_MICBAISCTR_MSK, ICDC_CACCR_MICBAISCTR(7))) {
			msleep(10);
			changed = true;
		}
		if (changed) {
			icdc_d4->micbias_jiffies = jiffies;
			icdc_d4->micbias_jiffies_vaild = 1;
		}
	}
	mutex_unlock(&card->dapm_mutex);
	return 0;
	return 0;
}

static int micbias_get(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.enumerated.item[0] = icdc_d4->micbias_enable;
	return 0;
}


static int micbias_sw_time_put(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	int max = mc->max;
	struct snd_soc_card *card = codec->card;

	mutex_lock_nested(&card->dapm_mutex, SND_SOC_DAPM_CLASS_RUNTIME);
	icdc_d4->micbias_time_val = ucontrol->value.integer.value[0] > max ?
		max : ucontrol->value.integer.value[0];
	mutex_unlock(&card->dapm_mutex);
	return 0;
}

static int micbias_sw_time_get(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = icdc_d4->micbias_time_val;
	return 0;
}

#define SOC_SINGLE_EXT_ITLV(xname, xhandler_info, xhandler_get, xhandler_put, tlv_array) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
	.name = xname, \
	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | SNDRV_CTL_ELEM_ACCESS_READWRITE,\
	.tlv.p = (tlv_array), \
	.info = xhandler_info, \
	.get = xhandler_get,	\
	.put = xhandler_put	\
}

static const struct snd_kcontrol_new icdc_d4_snd_controls[] = {
	SOC_SINGLE_EXT_ITLV("Master Playback Volume",
			hpout_info_volsw,
			hpout_volsw_get,
			hpout_volsw_put,
			hpout_tlv),
	SOC_SINGLE_EXT_ITLV("Mic Volume", alc_info_volsw,
			alc_volsw_get,
			alc_volsw_put,
			alc_tlv),
	SOC_ENUM_EXT("Micbias Switch", micbias_enum[0], micbias_get, micbias_put),
	/*
	 * Microphone bias voltage will have a large signal after power up,
	 * the purpose of this control is to skip this period of time.
	 * Note: Need To Check, develop board does not have buidlin-mic
	 */
	SOC_SINGLE_EXT("Micbias Switch Time(unit:jiffies(10ms))", SND_SOC_NOPM, 0, 1000, 0,
			micbias_sw_time_get, micbias_sw_time_put),
};

static int icdc_d4_adc_enable(struct snd_soc_codec *codec)
{
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);
	bool changed = false;

	snd_soc_update_bits(codec, ICDC_CACR, ICDC_CACR_ADCEN, ICDC_CACR_ADCEN);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAACR, ICDC_CACCR_ADCSRCEN, ICDC_CACCR_ADCSRCEN);
	msleep(10);
	if (icdc_d4->micbias_enable & MICBIAS_USED) {
		if (snd_soc_update_bits(codec, ICDC_CAACR, ICDC_CACCR_MICBIASEN, ICDC_CACCR_MICBIASEN)) {
			changed = true;
			msleep(10);
		}
		if (snd_soc_update_bits(codec, ICDC_CAACR, ICDC_CACCR_MICBAISCTR_MSK, ICDC_CACCR_MICBAISCTR(7))) {
			msleep(10);
			changed = true;
		}
		if (icdc_d4->micbias_jiffies_vaild)
			changed = true;
	}
	snd_soc_update_bits(codec, ICDC_CAMPCR, ICDC_CAMPCR_ADCREFVOLEN, ICDC_CAMPCR_ADCREFVOLEN); /* datasheet said set 0 enable, but the source code set 1*/
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAACR, BIT(4), BIT(4));
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CMICCR, ICDC_CMICCR_ALCMUTE_N, ICDC_CMICCR_ALCMUTE_N);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAMPCR, ICDC_CAMPCR_ADCLCKEN, ICDC_CAMPCR_ADCLCKEN);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAACR, BIT(3), BIT(3));
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAMPCR, BIT(4), BIT(4));
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CMICCR, ICDC_CMICCR_MICEN, ICDC_CMICCR_MICEN);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CACR2, ICDC_CACR2_ALCSEL_SIGNAL_ENDED, 0);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CMICCR, BIT(3), BIT(3));
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CMICCR, (0x3 << 1), (0x1 << 1));
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CACR2, ICDC_CACR2_ALCGAIN_MSK, ICDC_CACR2_ALCGAIN(icdc_d4->mic_vol));
	msleep(10);
	if (adc_hpf_en) {
		/*adc high pass filter enable*/
		snd_soc_update_bits(codec, ICDC_CGAINR, ICDC_CGAINR_ADC_HPF_EN_N, 0);
		msleep(10);
	}

	//	if (icdc_d4->alc_mode) {	 // ALC CODE from t30
	//		snd_soc_update_bits(codec, ICDC_CAACR, BIT(5), BIT(5), 1);
	//		msleep(10);
	//	}

	if ((icdc_d4->micbias_enable & MICBIAS_USED) && changed) { /*micbias percharge*/
		unsigned long cur_jiffies, delta_jiffies = 0;
		if (icdc_d4->micbias_jiffies_vaild) {
			cur_jiffies = jiffies;
			delta_jiffies = cur_jiffies > icdc_d4->micbias_jiffies ?
				icdc_d4->micbias_time_val : cur_jiffies - icdc_d4->micbias_jiffies;
		}
		if (icdc_d4->micbias_time_val > delta_jiffies) {
			msleep((icdc_d4->micbias_time_val - delta_jiffies) * 10);
		}
		icdc_d4->micbias_jiffies_vaild = 0;
	}

	return 0;
}

static int icdc_d4_adc_disable(struct snd_soc_codec *codec)
{
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);

	snd_soc_update_bits(codec, ICDC_CACR2, ICDC_CACR2_ALCGAIN_MSK, ICDC_CACR2_ALCGAIN(0xc));
	msleep(10);
	if (!(icdc_d4->micbias_enable & MICBIAS_ENABLE)) {
		snd_soc_update_bits(codec, ICDC_CAACR, ICDC_CACCR_MICBAISCTR_MSK, 0);
		msleep(10);
		snd_soc_update_bits(codec, ICDC_CAACR, ICDC_CACCR_MICBIASEN, 0);
		msleep(10);
		icdc_d4->micbias_jiffies_vaild = 0;
	}
	snd_soc_update_bits(codec, ICDC_CMICCR, ICDC_CMICCR_MICEN, 0);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAMPCR, ICDC_CAMPCR_ADCLCKEN, 0);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAMPCR, ICDC_CAMPCR_ADCREFVOLEN, 0);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAMPCR, BIT(4), 0);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CACR, ICDC_CACR_ADCEN, 0);
	msleep(10);
	return 0;
}

static int icdc_d4_adc_switch(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event) {

	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(w->codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		icdc_d4_adc_enable(w->codec);
		icdc_d4->adc_enabled = 1;
		break;
	case SND_SOC_DAPM_POST_PMD:
		icdc_d4_adc_disable(w->codec);
		icdc_d4->adc_enabled = 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int icdc_d4_dac_enable(struct snd_soc_codec *codec)
{
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);
	snd_soc_update_bits(codec, ICDC_CAACR, ICDC_CACCR_ADCSRCEN, ICDC_CACCR_ADCSRCEN);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAR, ICDC_CAR_DACREFVOLBUIFEN, ICDC_CAR_DACREFVOLBUIFEN);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAR, ICDC_CAR_DACREFVOLEN, ICDC_CAR_DACREFVOLEN);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAR, ICDC_CAR_DACCLKEN, ICDC_CAR_DACCLKEN);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAR, ICDC_CAR_DACEN, ICDC_CAR_DACEN);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAR, ICDC_CAR_DACINIT_N, ICDC_CAR_DACINIT_N);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CHCR, ICDC_CHCR_HPOUTPOP_MSK, ICDC_CHCR_HPOUTPOP_WORK);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CHR, ICDC_CHR_HPOUTEN, ICDC_CHR_HPOUTEN);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CHR, ICDC_CHR_HPOUTINIT_N, ICDC_CHR_HPOUTINIT_N);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CMICCR, ICDC_CMICCR_MICMUTE_N, ICDC_CMICCR_MICMUTE_N);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CHCR, ICDC_CHCR_HPOUTGAIN_MSK, ICDC_CHCR_HPOUTGAIN(icdc_d4->hpout_vol));
	msleep(10);
	return 0;
}

static int icdc_d4_dac_disable(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, ICDC_CAR, ICDC_CAR_DACEN, 0);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAR, ICDC_CAR_DACCLKEN, 0);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAR, ICDC_CAR_DACREFVOLEN, 0);
	msleep(10);
	snd_soc_update_bits(codec, ICDC_CAR, ICDC_CAR_DACINIT_N, 0);
	msleep(10);
	return 0;
}

static int icdc_d4_dac_switch(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(w->codec);

	switch (event) {
		case SND_SOC_DAPM_PRE_PMU:
			icdc_d4_dac_enable(w->codec);
			icdc_d4->dac_enabled = 1;
			break;
		case SND_SOC_DAPM_POST_PMD:
			icdc_d4_dac_disable(w->codec);
			icdc_d4->dac_enabled = 0;
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static const struct snd_soc_dapm_widget icdc_d4_dapm_widgets[] = {
	SND_SOC_DAPM_ADC_E("ADC", "Capture" , SND_SOC_NOPM, 0, 0, icdc_d4_adc_switch, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_DAC_E("DAC", "Playback", SND_SOC_NOPM, 0, 0, icdc_d4_dac_switch, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_OUTPUT("HPOUT"),
	SND_SOC_DAPM_INPUT("MICP"),
	SND_SOC_DAPM_INPUT("MICN"),
};

static const struct snd_soc_dapm_route intercon[] = {
	/*input*/
	{ "ADC", NULL, "MICP" },
	{ "ADC", NULL, "MICN" },
	/*output*/
	{ "HPOUT", NULL, "DAC"},
};

static int icdc_d4_probe(struct snd_soc_codec *codec)
{
	struct icdc_d4 *icdc_d4 = snd_soc_codec_get_drvdata(codec);
	dev_info(codec->dev, "codec icdc-d4 probe enter\n");
        icdc_d4_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	icdc_d4->codec = codec;
	dev_info(codec->dev, "codec icdc-d4 probe success!!!\n");
	return 0;
}

static int icdc_d4_remove(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "codec icdc_d4 remove enter\n");
	icdc_d4_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_icdc_d4_codec = {
	.probe = 	icdc_d4_probe,
	.remove = 	icdc_d4_remove,
	.read = 	icdc_d4_read,
	.write = 	icdc_d4_write,
	.writable_register = icdc_d4_vaild_register,
	.reg_cache_default = icdc_d4_reg_defcache,
	.reg_word_size = sizeof(u8),
	.reg_cache_step = 1,
	.reg_cache_size = ICDC_REG_NUM,
	.set_bias_level = icdc_d4_set_bias_level,

	.controls = 	icdc_d4_snd_controls,
	.num_controls = ARRAY_SIZE(icdc_d4_snd_controls),
	.dapm_widgets = icdc_d4_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(icdc_d4_dapm_widgets),
	.dapm_routes = intercon,
	.num_dapm_routes = ARRAY_SIZE(intercon),
};

/*Just for debug*/
static ssize_t icdc_d4_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct icdc_d4 *icdc_d4 = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = icdc_d4->codec;
	if (!codec) {
		dev_info(dev, "icdc_d4 is not probe, can not use %s function\n", __func__);
		return 0;
	}
	mutex_lock(&codec->mutex);
	dump_registers_hazard(icdc_d4);
	mutex_unlock(&codec->mutex);
	return 0;
}

static ssize_t icdc_d4_regs_store(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t count)
{
	struct icdc_d4 *icdc_d4 = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = icdc_d4->codec;
	const char *start = buf;
	unsigned int reg, val;
	int ret_count = 0;

	if (!codec) {
		dev_info(dev, "icdc_d4 is not probe, can not use %s function\n", __func__);
		return count;
	}

	while(!isxdigit(*start)) {
		start++;
		if (++ret_count >= count)
			return count;
	}
	reg = simple_strtoul(start, (char **)&start, 16);
	while(!isxdigit(*start)) {
		start++;
		if (++ret_count >= count)
			return count;
	}
	val = simple_strtoul(start, (char **)&start, 16);
	mutex_lock(&codec->mutex);
	writew(val, icdc_d4->mapped_base + reg);
	mutex_unlock(&codec->mutex);
	return count;
}

static struct device_attribute icdc_d4_sysfs_attrs =
	__ATTR(hw_regs, S_IRUGO|S_IWUGO, icdc_d4_regs_show, icdc_d4_regs_store);

static int icdc_d4_platform_probe(struct platform_device *pdev)
{
	struct icdc_d4 *icdc_d4 = NULL;
	struct resource *res = NULL;
	struct snd_codec_pdata *codec_vol = pdev->dev.platform_data;
	int ret = 0;

	icdc_d4 = (struct icdc_d4*)devm_kzalloc(&pdev->dev,
			sizeof(struct icdc_d4), GFP_KERNEL);
	if (!icdc_d4)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Faild to get ioresource mem\n");
		return -ENOENT;
	}

	if (!devm_request_mem_region(&pdev->dev, res->start,
				resource_size(res), pdev->name)) {
		dev_err(&pdev->dev, "Failed to request mmio memory region\n");
		return -EBUSY;
	}
	icdc_d4->mapped_resstart = res->start;
	icdc_d4->mapped_ressize = resource_size(res);
	icdc_d4->mapped_base = devm_ioremap_nocache(&pdev->dev,
			icdc_d4->mapped_resstart,
			icdc_d4->mapped_ressize);
	if (!icdc_d4->mapped_base) {
		dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
		return -ENOMEM;
	}

	icdc_d4->dev = &pdev->dev;
	platform_set_drvdata(pdev, (void *)icdc_d4);

	/*Default value*/
	icdc_d4->mic_vol = 0x10;
	icdc_d4->hpout_vol = 0x18;
	icdc_d4->dac_mute = 0;
	icdc_d4->dac_vol_zero = 0;
	icdc_d4->mic_maxvol = ICDC_CACR2_ALCGAIN_MSK;
	icdc_d4->hpout_maxvol = ICDC_CHCR_HPOUTGAIN_MSK;
	if (codec_vol) {
		if (codec_vol->mic_dftvol > ICDC_CACR2_ALCGAIN_MSK)
			icdc_d4->mic_vol = ICDC_CACR2_ALCGAIN_MSK;
		else if (codec_vol->mic_dftvol >= 0)
			icdc_d4->mic_vol = codec_vol->mic_dftvol;

		if (codec_vol->hp_dftvol > ICDC_CHCR_HPOUTGAIN_MSK)
			icdc_d4->hpout_vol = ICDC_CHCR_HPOUTGAIN_MSK;
		else if (codec_vol->hp_dftvol >= 0)
			icdc_d4->hpout_vol = codec_vol->hp_dftvol;

		if (codec_vol->mic_maxvol > 0 ||
				codec_vol->mic_maxvol < ICDC_CACR2_ALCGAIN_MSK)
			icdc_d4->mic_maxvol = codec_vol->mic_maxvol;

		if (codec_vol->hp_maxvol > 0 ||
				codec_vol->hp_maxvol < ICDC_CHCR_HPOUTGAIN_MSK)
			icdc_d4->hpout_maxvol = codec_vol->hp_maxvol;
	}
	icdc_d4->micbias_jiffies_vaild = 0;
	icdc_d4->micbias_time_val = 250;
	if (IS_ENABLED(CONFIG_JZ_ASOC_ICDC_USE_SOC_MICBIAS))
		icdc_d4->micbias_enable = MICBIAS_USED;
	else
		icdc_d4->micbias_enable = 0;
	ret = snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_icdc_d4_codec, &icdc_d4_codec_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Faild to register codec\n");
		platform_set_drvdata(pdev, NULL);
		return ret;
	}

	ret = device_create_file(&pdev->dev, &icdc_d4_sysfs_attrs);
	if (ret)
		dev_warn(&pdev->dev,"attribute %s create failed %x",
				attr_name(icdc_d4_sysfs_attrs), ret);
	dev_info(&pdev->dev, "codec icdc-d4 platfrom probe success\n");
	return 0;
}

static int icdc_d4_platform_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "codec icdc-d4 platform remove\n");
	snd_soc_unregister_codec(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver icdc_d4_codec_driver = {
	.driver = {
		.name = "icdc-d4",
		.owner = THIS_MODULE,
	},
	.probe = icdc_d4_platform_probe,
	.remove = icdc_d4_platform_remove,
};

static int icdc_d4_modinit(void)
{
	return platform_driver_register(&icdc_d4_codec_driver);
}
module_init(icdc_d4_modinit);

static void icdc_d4_exit(void)
{
	platform_driver_unregister(&icdc_d4_codec_driver);
}
module_exit(icdc_d4_exit);

MODULE_DESCRIPTION("icdc D4 Codec Driver");
MODULE_AUTHOR("cli<chen.li@ingenic.com>");
MODULE_LICENSE("GPL");
