/*
 * ak4493.c  --  audio driver for AK4493
 *
 * Copyright (C) 2018 Asahi Kasei Microdevices Corporation
 *  Author             Date      Revision      kernel version
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                   18/02/07      1.0          3.18.XX
 *                     18/03/07      1.1          4.4.XX
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/pm_runtime.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <linux/version.h>

#include "ak4493.h"

#define MUTEPIN_ON     0
#define MUTEPIN_OFF    1
#define MUTE_WAIT_TIME         300  // msec

//#define AK4493_DEBUG   //used at debug mode
static int cd_sel_gpio   = -1;    //PB18
static int ubw_sel_gpio   = -1;   //PB23
static int pdn_gpio    = -1;      //PB16
static int mute_gpio    = -1;     //-1
static int i2c_bus_num  = -1;     //1

module_param_gpio(cd_sel_gpio, 0644);
module_param_gpio(ubw_sel_gpio, 0644);
module_param_gpio(pdn_gpio, 0644);
module_param_gpio(mute_gpio, 0644);
module_param(i2c_bus_num, int, 0644);

#define I2C_ADDR 0x12

static struct i2c_client *i2c_dev;

#ifdef AK4493_DEBUG
#define akdbgprt printk
#else
#define akdbgprt(format, arg...) do {} while (0)
#endif

/* AK4493 Codec Private Data */
struct ak4493_priv {
    struct i2c_client *i2c;
    struct regmap *regmap;
    int fs1;         // Sampling Frequency
    int nBickFreq;   //  0: 48fs for 24bit,  1: 64fs or more for 32bit
    int nDSDSel;
    int nTdmSds;
};

/* ak4493 register cache & default register settings */
static const struct reg_default ak4493_reg[] = {
    { 0x00, 0x0C},   /*    0x00    AK4493_00_CONTROL1            */
    { 0x01, 0x22},   /*    0x01    AK4493_01_CONTROL2            */
    { 0x02, 0x00},   /*    0x02    AK4493_02_CONTROL3            */
    { 0x03, 0xFF},   /*    0x03    AK4493_03_LCHATT            */
    { 0x04, 0xFF},   /*    0x04    AK4493_04_RCHATT            */
    { 0x05, 0x00},   /*    0x05    AK4493_05_CONTROL4            */
    { 0x06, 0x00},   /*    0x06    AK4493_06_DSD1                */
    { 0x07, 0x00},   /*    0x07    AK4493_07_CONTROL5            */
    { 0x08, 0x00},   /*    0x08    AK4493_08_SOUNDCONTROL        */
    { 0x09, 0x00},   /*    0x09    AK4493_09_DSD2                */
    { 0x0A, 0x04},   /*    0x0A    AK4493_0A_CONTROL7          */
    { 0x0B, 0x00},   /*    0x0B    AK4493_0B_CONTROL8          */
    { 0x0C, 0x00},   /*    0x0C    AK4493_0C_RESERVED          */
    { 0x0D, 0x00},   /*    0x0D    AK4493_0D_RESERVED          */
    { 0x0E, 0x00},   /*    0x0E    AK4493_0E_RESERVED          */
    { 0x0F, 0x00},   /*    0x0F    AK4493_0F_RESERVED          */
    { 0x10, 0x00},   /*    0x10    AK4493_10_RESERVED          */
    { 0x11, 0x00},   /*    0x11    AK4493_11_RESERVED          */
    { 0x12, 0x00},   /*    0x12    AK4493_12_RESERVED          */
    { 0x13, 0x00},   /*    0x13    AK4493_13_RESERVED          */
    { 0x14, 0x00},   /*    0x14    AK4493_14_RESERVED          */
    { 0x15, 0x00},   /*    0x15    AK4493_15_DFSREAD           */



};

/* Volume control:
 * from -127 to 0 dB in 0.5 dB steps (mute instead of -127.5 dB) */
static DECLARE_TLV_DB_SCALE(latt_tlv, -12750, 50, 0);
static DECLARE_TLV_DB_SCALE(ratt_tlv, -12750, 50, 0);

static const char *ak4493_ecs_select_texts[] = {"768kHz", "384kHz"};

static const char *ak4493_dem_select_texts[] = {"44.1kHz", "OFF", "48kHz", "32kHz"};
static const char *ak4493_dzfm_select_texts[] = {"Separated", "ANDed"};

static const char *ak4493_sellr_select_texts[] = {"ALL_Lch", "ALL_Rch"};
static const char *ak4493_dckb_select_texts[] = {"Falling", "Rising"};
static const char *ak4493_dcks_select_texts[] = {"512fs", "768fs"};

static const char *ak4493_dsdd_select_texts[] = {"Normal", "Volume Bypass"};

static const char *ak4493_dsdf_select_texts[] = {"39/78/156kHz", "76/152/304kHz"};
static const char *ak4493_dsd_input_path_select[] = {"A3_B1_B2pin", "J1_H1_G1pin"};
static const char *ak4493_ats_select_texts[] = {"4080/fs", "2040/fs", "510/fs", "255/fs"};

static const char *ak4493_dzf_select_texts[] = {"H", "L"};
static const char *ak4493_dsd_mute_texts[] = {"Hold", "Release"};
static const char *ak4493_dsd_detect_texts[] = {"Not FS", "FS"};


static const char *ak4493_ddmt_select_texts[] = {"256/264 DCLK", "512/520 DCLK", "1024/1032 DCLK", "128/136 DCLK"};
static const char *ak4493_adpt_select_texts[] = {"8192/fs", "4096/fs", "2048/fs", "1024/fs"};

static const char *ak4493_sc_select_texts[] = {
    "Sound Mode 1", "Sound Mode 2", "Setting 3",
    "Sound Mode 4", "Sound Mode 5"
};

static int ak4493_rstn_control(struct snd_soc_codec *codec, int bit);

static const struct soc_enum ak4493_dac_enum[] = {
    SOC_ENUM_SINGLE(AK4493_00_CONTROL1, 5,
            ARRAY_SIZE(ak4493_ecs_select_texts), ak4493_ecs_select_texts),
    SOC_ENUM_SINGLE(AK4493_01_CONTROL2, 1,
            ARRAY_SIZE(ak4493_dem_select_texts), ak4493_dem_select_texts),
    SOC_ENUM_SINGLE(AK4493_01_CONTROL2, 6,
            ARRAY_SIZE(ak4493_dzfm_select_texts), ak4493_dzfm_select_texts),
    SOC_ENUM_SINGLE(AK4493_02_CONTROL3, 1,
            ARRAY_SIZE(ak4493_sellr_select_texts), ak4493_sellr_select_texts),
    SOC_ENUM_SINGLE(AK4493_02_CONTROL3, 4,
            ARRAY_SIZE(ak4493_dckb_select_texts), ak4493_dckb_select_texts),

    SOC_ENUM_SINGLE(AK4493_02_CONTROL3, 5,
            ARRAY_SIZE(ak4493_dcks_select_texts), ak4493_dcks_select_texts),
    SOC_ENUM_SINGLE(AK4493_06_DSD1, 1,
            ARRAY_SIZE(ak4493_dsdd_select_texts), ak4493_dsdd_select_texts),
    SOC_ENUM_SINGLE(AK4493_09_DSD2, 1,
            ARRAY_SIZE(ak4493_dsdf_select_texts), ak4493_dsdf_select_texts),
    SOC_ENUM_SINGLE(AK4493_09_DSD2, 2,
            ARRAY_SIZE(ak4493_dsd_input_path_select), ak4493_dsd_input_path_select),
    SOC_ENUM_SINGLE(AK4493_0B_CONTROL8, 6,
            ARRAY_SIZE(ak4493_ats_select_texts), ak4493_ats_select_texts),

    SOC_ENUM_SINGLE(AK4493_02_CONTROL3, 2,
            ARRAY_SIZE(ak4493_dzf_select_texts), ak4493_dzf_select_texts),
    SOC_ENUM_SINGLE(AK4493_06_DSD1, 3,
            ARRAY_SIZE(ak4493_dsd_mute_texts), ak4493_dsd_mute_texts),
    SOC_ENUM_SINGLE(AK4493_06_DSD1, 5,
            ARRAY_SIZE(ak4493_dsd_detect_texts), ak4493_dsd_detect_texts),
    SOC_ENUM_SINGLE(AK4493_06_DSD1, 6,
            ARRAY_SIZE(ak4493_dsd_detect_texts), ak4493_dsd_detect_texts),

    SOC_ENUM_SINGLE(AK4493_06_DSD1, 2,
            ARRAY_SIZE(ak4493_ddmt_select_texts), ak4493_ddmt_select_texts),
    SOC_ENUM_SINGLE(AK4493_15_DFSREAD, 5,
            ARRAY_SIZE(ak4493_adpt_select_texts), ak4493_adpt_select_texts),
    SOC_ENUM_SINGLE(AK4493_08_SOUNDCONTROL, 0,
            ARRAY_SIZE(ak4493_sc_select_texts), ak4493_sc_select_texts),

};

static const char *ak4493_dsdsel_select_texts[] = {"64fs", "128fs", "256fs","512fs"};
static const char *ak4493_bickfreq_select[] = {"48fs", "64fs"};

static const char *ak4493_tdm_sds_select[] = {"L1R1", "TDM128_L1R1", "TDM128_L2R2",
            "TDM256_L1R1", "TDM256_L2R2",  "TDM256_L3R3", "TDM256_L4R4",
            "TDM512_L1R1", "TDM512_L2R2",  "TDM512_L3R3", "TDM512_L4R4",
            "TDM512_L5R5", "TDM512_L6R6",  "TDM512_L7R7", "TDM512_L8R8",
};


static const struct soc_enum ak4493_dac_enum2[] = {
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ak4493_dsdsel_select_texts), ak4493_dsdsel_select_texts),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ak4493_bickfreq_select), ak4493_bickfreq_select),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ak4493_tdm_sds_select), ak4493_tdm_sds_select),
};

static int ak4493_get_dsdsel(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
    struct ak4493_priv *ak4493 = snd_soc_codec_get_drvdata(codec);

    ucontrol->value.enumerated.item[0] = ak4493->nDSDSel;

    return 0;
}

static int ak4493_set_dsdsel(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
    struct ak4493_priv *ak4493 = snd_soc_codec_get_drvdata(codec);

    ak4493->nDSDSel = ucontrol->value.enumerated.item[0];

    if ( ak4493->nDSDSel == 0 ) {     //  2.8224MHz
        snd_soc_update_bits(codec, AK4493_06_DSD1, 0x01, 0x00);
        snd_soc_update_bits(codec, AK4493_09_DSD2, 0x01, 0x00);
    }
    else if( ak4493->nDSDSel == 1 ) {    // 5.6448MHz
        snd_soc_update_bits(codec, AK4493_06_DSD1, 0x01, 0x01);
        snd_soc_update_bits(codec, AK4493_09_DSD2, 0x01, 0x00);
    }
    else if( ak4493->nDSDSel == 2 ) {    //  11.2896MHz
        snd_soc_update_bits(codec, AK4493_06_DSD1, 0x01, 0x00);
        snd_soc_update_bits(codec, AK4493_09_DSD2, 0x01, 0x01);
    }
    else {                                //  22.5792MHz
        snd_soc_update_bits(codec, AK4493_06_DSD1, 0x01, 0x01);
        snd_soc_update_bits(codec, AK4493_09_DSD2, 0x01, 0x01);
    }

    return 0;
}

static int ak4493_get_bickfs(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
    struct ak4493_priv *ak4493 = snd_soc_codec_get_drvdata(codec);

    ucontrol->value.enumerated.item[0] = ak4493->nBickFreq;

    return 0;
}

static int ak4493_set_bickfs(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
    struct ak4493_priv *ak4493 = snd_soc_codec_get_drvdata(codec);

    ak4493->nBickFreq = ucontrol->value.enumerated.item[0];

    return 0;
}

static int ak4493_get_tdmsds(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
    struct ak4493_priv *ak4493 = snd_soc_codec_get_drvdata(codec);

    ucontrol->value.enumerated.item[0] = ak4493->nTdmSds;

    return 0;
}

static int ak4493_set_tdmsds(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
    struct ak4493_priv *ak4493 = snd_soc_codec_get_drvdata(codec);
    int    regA, regB;

    ak4493->nTdmSds = ucontrol->value.enumerated.item[0];

    if ( ak4493->nTdmSds == 0 ) regB = 0;      //  SDS0 bit = 0
    else regB = (1 & (ak4493->nTdmSds - 1));    //  SDS0 bit = 1

    switch(ak4493->nTdmSds) {
        case 0:
            regA = 0;  // Normal
            break;
        case 1:
        case 2:
            regA = 4;  // TDM128 TDM1-0bits = 1
            break;
        case 3:
        case 4:
            regA = 8;  // TDM128 TDM1-0bits = 2
            break;
        case 5:
        case 6:
            regA = 9;  // TDM128 TDM1-0bits = 2
            break;
        case 7:
        case 8:
            regA = 0xC;  // TDM128 TDM1-0bits = 3
            break;
        case 9:
        case 10:
            regA = 0xD;  // TDM128 TDM1-0bits = 3
            break;
        case 11:
        case 12:
            regA = 0xE;  // TDM128 TDM1-0bits = 3
            break;
        case 13:
        case 14:
            regA = 0xF;  // TDM128 TDM1-0bits = 3
            break;
        default:
            regA = 0;
            regB = 0;
            break;
    }

    regA <<= 4;
    regB <<= 4;

    snd_soc_update_bits(codec, AK4493_0A_CONTROL7, 0xF0, regA);
    snd_soc_update_bits(codec, AK4493_0B_CONTROL8, 0x10, regB);

    return 0;
}



static const char *gain_control_texts[] = {
    "2.8_2.8Vpp", "2.8_2.5Vpp", "2.5_2.5Vpp", "3.75_3.75Vpp", "3.75_2.5Vpp"
};

static const unsigned int gain_control_values[] = {
    0, 1, 2, 4, 5
};

static const struct soc_enum ak4493_gain_control_enum =
       SOC_VALUE_ENUM_SINGLE(AK4493_07_CONTROL5, 1, 7,
                             ARRAY_SIZE(gain_control_texts),
                             gain_control_texts,
                             gain_control_values);



#ifdef AK4493_DEBUG

static const char *test_reg_select[]   =
{
    "read AK4493 Reg 00:0B",
    "read AK4493 Reg 15"
};

static const struct soc_enum ak4493_enum[] =
{
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(test_reg_select), test_reg_select),
};

static int nTestRegNo = 0;

static int get_test_reg(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    /* Get the current output routing */
    ucontrol->value.enumerated.item[0] = nTestRegNo;

    return 0;
}

static int set_test_reg(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
    u32    currMode = ucontrol->value.enumerated.item[0];
    int    i, value;
    int       regs, rege;

    nTestRegNo = currMode;

    if ( nTestRegNo == 0 ) {
        regs = 0x00;
        rege = 0x0B;
    }
    else {
        regs = 0x15;
        rege = 0x15;
    }

    for ( i = regs ; i <= rege ; i++ ){
        value = snd_soc_read(codec, i);
        printk("***AK4493 Addr,Reg=(%x, %x)\n", i, value);
    }

    return 0;
}
#endif


static const struct snd_kcontrol_new ak4493_snd_controls[] = {
    SOC_SINGLE_TLV("AK4493 Lch Digital Volume",
            AK4493_03_LCHATT, 0, 0xFF, 0, latt_tlv),
    SOC_SINGLE_TLV("AK4493 Rch Digital Volume",
            AK4493_04_RCHATT, 0, 0xFF, 0, ratt_tlv),

    SOC_ENUM("AK4493 EX DF I/F clock", ak4493_dac_enum[0]),
    SOC_ENUM("AK4493 De-emphasis Response", ak4493_dac_enum[1]),
    SOC_ENUM("AK4493 Data Zero Detect Mode", ak4493_dac_enum[2]),
    SOC_ENUM("AK4493 Data Selection at Mono Mode", ak4493_dac_enum[3]),

    SOC_ENUM("AK4493 Polarity of DCLK", ak4493_dac_enum[4]),
    SOC_ENUM("AK4493 DCKL Frequency", ak4493_dac_enum[5]),

    SOC_ENUM("AK4493 DDSD Play Back Path", ak4493_dac_enum[6]),
    SOC_ENUM("AK4493 Cut Off of DSD Filter", ak4493_dac_enum[7]),

    SOC_ENUM_EXT("AK4493 DSD Data Stream", ak4493_dac_enum2[0], ak4493_get_dsdsel, ak4493_set_dsdsel), //akm dsd512
    SOC_ENUM_EXT("AK4493 BICK Frequency Select", ak4493_dac_enum2[1], ak4493_get_bickfs, ak4493_set_bickfs),
    SOC_ENUM_EXT("AK4493 TDM Data Select", ak4493_dac_enum2[2], ak4493_get_tdmsds, ak4493_set_tdmsds),

    SOC_SINGLE("AK4493 External Digital Filter", AK4493_00_CONTROL1, 6, 1, 0),
    SOC_SINGLE("AK4493 MCLK Frequncy Auto Setting", AK4493_00_CONTROL1, 7, 1, 0),

    SOC_SINGLE("AK4493 Soft Mute Control", AK4493_01_CONTROL2, 0, 1, 0),
    SOC_SINGLE("AK4493 Short delay filter", AK4493_01_CONTROL2, 5, 1, 0),
    SOC_SINGLE("AK4493 Data Zero Detect Enable", AK4493_01_CONTROL2, 7, 1, 0),
    SOC_SINGLE("AK4493 Slow Roll-off Filter", AK4493_02_CONTROL3, 0, 1, 0),

    SOC_ENUM("AK4493 Inverting Enable of DZF", ak4493_dac_enum[10]),

    SOC_SINGLE("AK4493 Mono Mode", AK4493_02_CONTROL3, 3, 1, 0), // 0:stereo 1:mono(SELLR bit)
    SOC_SINGLE("AK4493 Super Slow Roll-off Filter", AK4493_05_CONTROL4, 0, 1, 0),
    SOC_SINGLE("AK4493 AOUTR Phase Inverting", AK4493_05_CONTROL4, 6, 1, 0),
    SOC_SINGLE("AK4493 AOUTL Phase Inverting", AK4493_05_CONTROL4, 7, 1, 0),

    SOC_ENUM("AK4493 DSDR is detected", ak4493_dac_enum[12]),
    SOC_ENUM("AK4493 DSDL is detected", ak4493_dac_enum[13]),

    SOC_SINGLE("AK4493 DSD Data Mute", AK4493_06_DSD1, 7, 1, 0),
    SOC_SINGLE("AK4493 Synchronization Control", AK4493_07_CONTROL5, 0, 1, 0),

    SOC_ENUM("AK4493 Output Level", ak4493_gain_control_enum),
    SOC_ENUM("AK4493 ATT Transit Time", ak4493_dac_enum[9]),

    SOC_ENUM("AK4493 Sound control", ak4493_dac_enum[16]),
    SOC_SINGLE("AK4493 DDMODE DZFL/R Pin", AK4493_06_DSD1, 4, 1, 0),
    SOC_ENUM("AK4493 DSD Signal Dtime", ak4493_dac_enum[14]),

    SOC_SINGLE("AK4493 AIF Auto Mode Switch", AK4493_15_DFSREAD, 7, 1, 0),
    SOC_ENUM("AK4493 ADPT Time for Zero Data", ak4493_dac_enum[15]),

#ifdef AK4493_DEBUG
    SOC_ENUM_EXT("Reg Read", ak4493_enum[0], get_test_reg, set_test_reg),
#endif

};

static const char *ak4493_dac_enable_texts[] =
        {"Off", "On"};

static SOC_ENUM_SINGLE_VIRT_DECL(ak4493_dac_enable_enum, ak4493_dac_enable_texts);

static const struct snd_kcontrol_new ak4493_dac_enable_control =
    SOC_DAPM_ENUM("DAC Switch", ak4493_dac_enable_enum);

/* ak4493 dapm widgets */
static const struct snd_soc_dapm_widget ak4493_dapm_widgets[] = {

    SND_SOC_DAPM_AIF_IN("AK4493 SDTI", "Playback", 0, SND_SOC_NOPM, 0, 0),

    SND_SOC_DAPM_DAC("AK4493 DAC", NULL, AK4493_0A_CONTROL7, 2, 0),

    SND_SOC_DAPM_MUX("AK4493 DAC Enable", SND_SOC_NOPM, 0, 0, &ak4493_dac_enable_control),

    SND_SOC_DAPM_OUTPUT("AK4493 AOUT"),


};

static const struct snd_soc_dapm_route ak4493_intercon[] =
{
    {"AK4493 DAC", NULL, "AK4493 SDTI"},
    {"AK4493 DAC Enable", "On", "AK4493 DAC"},
    {"AK4493 AOUT", NULL, "AK4493 DAC Enable"},
};

static int ak4493_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params,
        struct snd_soc_dai *dai)
{
    struct snd_soc_codec *codec = dai->codec;
    struct ak4493_priv *ak4493 = snd_soc_codec_get_drvdata(codec);

    u8     dfs;
    u8  dfs2;
    int nfs1;

    nfs1 = params_rate(params);
    ak4493->fs1 = nfs1;

    dfs = snd_soc_read(codec, AK4493_01_CONTROL2);
    dfs &= ~AK4493_DFS;

    dfs2 = snd_soc_read(codec, AK4493_05_CONTROL4);
    dfs2 &= ~AK4493_DFS2;

    switch (nfs1) {
        case 32000:
        case 44100:
        case 48000:
            dfs |= AK4493_DFS_48KHZ;
            dfs2 |= AK4493_DFS2_48KHZ;
            break;
        case 88200:
        case 96000:
            dfs |= AK4493_DFS_96KHZ;
            dfs2 |= AK4493_DFS2_48KHZ;
            break;
        case 176400:
        case 192000:
            dfs |= AK4493_DFS_192KHZ;
            dfs2 |= AK4493_DFS2_48KHZ;
            break;
        case 384000:
            dfs |= AK4493_DFS_384KHZ;
            dfs2 |= AK4493_DFS2_384KHZ;
            break;
        case 768000:
            dfs |= AK4493_DFS_768KHZ;
            dfs2 |= AK4493_DFS2_384KHZ;
            break;
        default:
            return -EINVAL;
    }

    snd_soc_write(codec, AK4493_01_CONTROL2, dfs);
    snd_soc_write(codec, AK4493_05_CONTROL4, dfs2);

    return 0;
}

static int ak4493_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
        unsigned int freq, int dir)
{

    akdbgprt("\t[AK4493] %s(%d)\n",__FUNCTION__,__LINE__);

    return 0;
}

static int ak4493_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    struct snd_soc_codec *codec = dai->codec;
    struct ak4493_priv *ak4493 = snd_soc_codec_get_drvdata(codec);
    u8 format;
    u8 format2;

    akdbgprt("\t[AK4493] %s(%d)\n",__FUNCTION__,__LINE__);

    /* set master/slave audio interface */
    format = snd_soc_read(codec, AK4493_00_CONTROL1);
    format &= ~AK4493_DIF;

    format2 = snd_soc_read(codec, AK4493_02_CONTROL3);
    format2 &= ~AK4493_DIF_DSD;

    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
        case SND_SOC_DAIFMT_CBS_CFS:
            akdbgprt("\t[AK4493] %s(%d)\n",__FUNCTION__,__LINE__);
            break;
        case SND_SOC_DAIFMT_CBM_CFM:
        case SND_SOC_DAIFMT_CBS_CFM:
        case SND_SOC_DAIFMT_CBM_CFS:
        default:
            dev_err(codec->dev, "Clock mode unsupported");
           return -EINVAL;
    }

    switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
        case SND_SOC_DAIFMT_I2S:
            akdbgprt("\t[AK4493] %s(%d)\n",__FUNCTION__,__LINE__);
            format |= AK4493_DIF_I2S_MODE;
            if ( ak4493->nBickFreq == 1 )     format |= AK4493_DIF_32BIT_MODE;
            break;
        case SND_SOC_DAIFMT_LEFT_J:
            format |= AK4493_DIF_MSB_MODE;
            if ( ak4493->nBickFreq == 1 )     format |= AK4493_DIF_32BIT_MODE;
            break;
        case SND_SOC_DAIFMT_DSD:
            format2 |= AK4493_DIF_DSD_MODE;
            break;
        default:
            return -EINVAL;
    }

    /* set format */
    snd_soc_write(codec, AK4493_00_CONTROL1, format);
    snd_soc_write(codec, AK4493_02_CONTROL3, format2);

    return 0;
}

static bool ak4493_volatile(struct device *dev, unsigned int reg)
{
    int    ret;

#ifdef AK4493_DEBUG
    ret = 1;
#else
    switch (reg) {
        case AK4493_06_DSD1:
        case AK4493_15_DFSREAD:
            ret = 1;
            break;
        default:
            ret = 0;
            break;
    }
#endif
    return(ret);
}

static bool ak4493_readable(struct device *dev, unsigned int reg)
{

    if (reg <= AK4493_MAX_REGISTERS)
        return true;
    else
        return false;

}


static bool ak4493_writeable(struct device *dev, unsigned int reg)
{
    if (reg <= AK4493_MAX_REGISTERS)
        return true;
    else
        return false;
}

static int ak4493_set_bias_level(struct snd_soc_codec *codec,
        enum snd_soc_bias_level level)
{
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
    struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
#endif
    akdbgprt("\t[AK4493] %s(%d)\n",__FUNCTION__,__LINE__);

    switch (level) {
    case SND_SOC_BIAS_ON:
    case SND_SOC_BIAS_PREPARE:
    case SND_SOC_BIAS_STANDBY:
        snd_soc_update_bits(codec, AK4493_00_CONTROL1, 0x01, 0x01);  // RSTN bit = 1
        break;
    case SND_SOC_BIAS_OFF:
        snd_soc_update_bits(codec, AK4493_00_CONTROL1, 0x01, 0x00);  // RSTN bit = 0
        break;
    }
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
        dapm->bias_level = level;
#else
        codec->dapm.bias_level = level;
#endif

    return 0;
}

static int ak4493_set_dai_mute(struct snd_soc_dai *dai, int mute)
{
    struct snd_soc_codec *codec = dai->codec;
    struct ak4493_priv *ak4493 = snd_soc_codec_get_drvdata(codec);
    int nfs, ndt;

    akdbgprt("\t[AK4493] %s mute[%s]\n",__FUNCTION__, mute ? "ON":"OFF");

    nfs = ak4493->fs1;

    if (mute) {    //SMUTE: 1 , MUTE
        snd_soc_update_bits(codec, AK4493_01_CONTROL2, 0x01, 0x01);
        snd_soc_update_bits(codec, AK4493_0A_CONTROL7, 0x04, 0x00);
        ndt = 7424000 / nfs;
        mdelay(ndt);
        if(mute_gpio !=-1) {
            gpio_set_value(mute_gpio, MUTEPIN_ON); //External Mute ON
            akdbgprt("[AK4493] %s  MUTEPIN_ON\n",__FUNCTION__);
            mdelay(MUTE_WAIT_TIME);
        }
    }
    else {        // SMUTE: 0 ,NORMAL operation
        if(mute_gpio !=-1) {
            gpio_set_value(mute_gpio, MUTEPIN_OFF); //External Mute OFF
            akdbgprt("[AK4493] %s  MUTEPIN_OFF\n",__FUNCTION__);
            mdelay(MUTE_WAIT_TIME);
        }
        snd_soc_update_bits(codec, AK4493_01_CONTROL2, 0x01, 0x00);
        snd_soc_update_bits(codec, AK4493_0A_CONTROL7, 0x04, 0x04);
    }

    return 0;
}

#define AK4493_RATES        (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
                SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
                SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
                SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |\
                SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 |\
                SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_384000 | SNDRV_PCM_RATE_768000)

#define AK4493_FORMATS        SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE |\
    SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_U24_3BE |\
        SNDRV_PCM_FMTBIT_U24_3LE | SNDRV_PCM_FMTBIT_U24_3BE |\
        SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE |\
        SNDRV_PCM_FMTBIT_U20_3LE | SNDRV_PCM_FMTBIT_U20_3BE

static struct snd_soc_dai_ops ak4493_dai_ops = {
    .hw_params    = ak4493_hw_params,
    .set_sysclk    = ak4493_set_dai_sysclk,
    .set_fmt    = ak4493_set_dai_fmt,
    .digital_mute = ak4493_set_dai_mute,
};

struct snd_soc_dai_driver ak4493_dai[] = {
    {
        .name = "ak4493-aif",
        .playback = {
               .stream_name = "Playback",
               .channels_min = 1,
               .channels_max = 2,
               .rates = AK4493_RATES,
               .formats = AK4493_FORMATS,
        },
        .ops = &ak4493_dai_ops,
    },
};

static int ak4493_rstn_control(struct snd_soc_codec *codec, int bit)
{
    int ret;
    if (bit)
        ret = snd_soc_update_bits(codec,
                      AK4493_00_CONTROL1,
                      AK4493_RSTN_MASK,
                      0x1);
    else
        ret = snd_soc_update_bits(codec,
                      AK4493_00_CONTROL1,
                      AK4493_RSTN_MASK,
                      0x0);
    return ret;
}

static int ak4493_init_reg(struct snd_soc_codec *codec)
{
    snd_soc_update_bits(codec, AK4493_00_CONTROL1, 0x80, 0x80);      // ACKS bit = 1
    snd_soc_update_bits(codec, AK4493_07_CONTROL5, 0x01, 0x01);      // SYNCE bit = 1
    ak4493_rstn_control(codec, 1);
    return 0;
}

static int ak4493_probe(struct snd_soc_codec *codec)
{
    int ret = 0;
    struct ak4493_priv *ak4493 = snd_soc_codec_get_drvdata(codec);

    akdbgprt("\t[AK4493] %s(%d)\n",__FUNCTION__,__LINE__);

    if (cd_sel_gpio != -1) {
        ret = gpio_request(cd_sel_gpio, "ak4493 cd_sel");
        gpio_direction_output(cd_sel_gpio, 1);
    }
    if (ubw_sel_gpio != -1) {
        ret = gpio_request(ubw_sel_gpio, "ak4493 uwb_sel");
        gpio_direction_output(ubw_sel_gpio, 0);
    }

    if (pdn_gpio != -1) {
        ret = gpio_request(pdn_gpio, "ak4493 pdn");
        gpio_direction_output(pdn_gpio, 1);
        msleep(1);
        gpio_direction_output(pdn_gpio, 0);
        msleep(1);
        gpio_direction_output(pdn_gpio, 1);
        msleep(1);
    }
    if (mute_gpio != -1) {
        ret = gpio_request(mute_gpio, "ak4493 mute");
        gpio_direction_output(mute_gpio, MUTEPIN_ON);
    }

    ak4493->fs1 = 48000;
    ak4493->nBickFreq = 0;        //default 0:48fs  1:64fs
    ak4493->nDSDSel = 0;
    ak4493->nTdmSds = 0;

    ak4493_init_reg(codec);

    return ret;
}

static int ak4493_gpo_low(void)
{
    if (mute_gpio > 0 ) {
        gpio_set_value(mute_gpio, MUTEPIN_ON);
        mdelay(1);
    }
    if (pdn_gpio > 0 ) {
        gpio_set_value(pdn_gpio, 0);
    }
    return(0);
}

static int ak4493_remove(struct snd_soc_codec *codec)
{
    akdbgprt("\t[AK4493] %s(%d)\n",__FUNCTION__,__LINE__);

    ak4493_set_bias_level(codec, SND_SOC_BIAS_OFF);
    ak4493_gpo_low();

    if (pdn_gpio > 0) {
        gpio_free(pdn_gpio);
    }

    if (mute_gpio > 0) {
        gpio_free(mute_gpio);
    }

    return 0;
}

static int ak4493_suspend(struct snd_soc_codec *codec)
{
    ak4493_set_bias_level(codec, SND_SOC_BIAS_OFF);
    ak4493_gpo_low();
    snd_soc_cache_init(codec);
    return 0;
}

static int ak4493_resume(struct snd_soc_codec *codec)
{

    ak4493_init_reg(codec);

    return 0;
}


struct snd_soc_codec_driver soc_codec_dev_ak4493 = {
    .probe = ak4493_probe,
    .remove = ak4493_remove,
    .suspend =    ak4493_suspend,
    .resume =    ak4493_resume,

    .idle_bias_off = true,
    .set_bias_level = ak4493_set_bias_level,

    .controls = ak4493_snd_controls,
    .num_controls = ARRAY_SIZE(ak4493_snd_controls),
    .dapm_widgets = ak4493_dapm_widgets,
    .num_dapm_widgets = ARRAY_SIZE(ak4493_dapm_widgets),
    .dapm_routes = ak4493_intercon,
    .num_dapm_routes = ARRAY_SIZE(ak4493_intercon),
};


static const struct regmap_config ak4493_regmap = {
    .reg_bits = 8,
    .val_bits = 8,

    .max_register = AK4493_MAX_REGISTERS,
    .volatile_reg = ak4493_volatile,
    .writeable_reg = ak4493_writeable,
    .readable_reg = ak4493_readable,

    .reg_defaults = ak4493_reg,
    .num_reg_defaults = ARRAY_SIZE(ak4493_reg),
    .cache_type = REGCACHE_RBTREE,
};

static int ak4493_i2c_probe(struct i2c_client *i2c,
                            const struct i2c_device_id *id)
{
    struct ak4493_priv *ak4493;
    int ret=0;

    akdbgprt("\t[AK4493] %s(%d)\n",__FUNCTION__,__LINE__);

    ak4493 = kzalloc(sizeof(struct ak4493_priv), GFP_KERNEL);
    if (ak4493 == NULL) return -ENOMEM;

    ak4493->regmap = devm_regmap_init_i2c(i2c, &ak4493_regmap);
    if (IS_ERR(ak4493->regmap))
        return PTR_ERR(ak4493->regmap);

    i2c_set_clientdata(i2c, ak4493);
    ak4493->i2c = i2c;
    ret = snd_soc_register_codec(&i2c->dev,
            &soc_codec_dev_ak4493, ak4493_dai, ARRAY_SIZE(ak4493_dai));
    if (ret < 0){
        kfree(ak4493);
        akdbgprt("\t[AK4493 Error!] %s(%d)\n",__FUNCTION__,__LINE__);
    }

    printk("AK4493 probe ok, dev_name: %s\n", dev_name(&ak4493->i2c->dev));
    return ret;
}

static int ak4493_i2c_remove(struct i2c_client *client)
{
    snd_soc_unregister_codec(&client->dev);
    kfree(i2c_get_clientdata(client));
    return 0;
}

static const struct i2c_device_id ak4493_i2c_id[] = {
    { "ak4493", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ak4493_i2c_id);

static struct i2c_driver ak4493_i2c_driver = {
    .driver = {
        .name = "ak4493",
        //.of_match_table = of_match_ptr(ak4493_i2c_dt_ids),
    },
    .probe = ak4493_i2c_probe,
    .remove = ak4493_i2c_remove,
    .id_table = ak4493_i2c_id,
};

static struct i2c_board_info codec_ak4493_info = {
    .type = "ak4493",
    .addr = I2C_ADDR,
};

static int __init ak4493_codec_init(void)
{

    akdbgprt("\t[AK4493] %s(%d)\n", __FUNCTION__,__LINE__);
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "ak4493: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&ak4493_i2c_driver);
    if (ret) {
        printk(KERN_ERR "ak4493: failed to register i2c driver\n");
        return ret;
    }
    akdbgprt("\t[AK4493] %s(%d)\n", __FUNCTION__,__LINE__);

    i2c_dev = i2c_register_device(&codec_ak4493_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "ak4493: failed to register i2c device\n");
        i2c_del_driver(&ak4493_i2c_driver);
        return -EINVAL;
    }
    akdbgprt("\t[AK4493] %s(%d)\n", __FUNCTION__,__LINE__);
    return ret;
}

module_init(ak4493_codec_init);

static void __exit ak4493_codec_exit(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&ak4493_i2c_driver);
}
module_exit(ak4493_codec_exit);

MODULE_DESCRIPTION("ASoC ak4493 codec driver");
MODULE_LICENSE("GPL v2");
