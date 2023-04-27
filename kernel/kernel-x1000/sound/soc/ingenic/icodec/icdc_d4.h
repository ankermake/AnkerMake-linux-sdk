/*
 * sound/soc/ingenic/icodec/icdc_d4.h
 * ALSA SoC Audio driver -- ingenic internal codec (icdc_d4) driver

 * Copyright 2017 Ingenic Semiconductor Co.,Ltd
 *	cli <chen.li@ingenic.com>
 *
 * Note: icdc_d4 is an internal codec for jz SOC
 *	 used for x1630 x1830 and so on
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __ICDC_D4_REG_H__
#define __ICDC_D4_REG_H__

#include <linux/spinlock.h>
#include <sound/soc.h>
#include <soc/base.h>

struct icdc_d4 {
	struct device		*dev;		/*aic device used to access register*/
	struct snd_soc_codec	*codec;
	void * __iomem mapped_base;		/*vir addr*/
	resource_size_t mapped_resstart;	/*resource phy addr*/
        resource_size_t mapped_ressize;		/*resource phy addr size*/

        int hpout_vol;
        int mic_vol;
	int hpout_maxvol;
	int mic_maxvol;
        unsigned long micbias_jiffies;
	int power_on:1;
        int dac_enabled:1;
        int adc_enabled:1;
        int micbias_jiffies_vaild:1;
        int micbias_enable:2;
	int alc_mode:1;
        int micbias_time_val:10;
        int dac_mute:1;
        int dac_vol_zero:1;
};

enum {
        ICDC_CGR = 0,   /*0x00  0x03*/
        ICDC_CACR = 2,  /*0x08  0x50*/
        ICDC_CMCR,      /*0x0C  0x0e*/
        ICDC_CDC1R,     /*0x10  0x50*/
        ICDC_CDC2R,     /*0x14  0x0e*/
        ICDC_CADR = 7,  /*0x1C  0x00*/
        ICDC_CGAINR =10,/*0x28  0x00*/
        ICDC_CASR = 13, /*0x34  0x00*/
        ICDC_CDPR,      /*0x38  0x03*/
        ICDC_CDDPR2,    /*0x3C  0xff*/
        ICDC_CDDPR1,    /*0x40  0xe0*/
        ICDC_CDDPR0,    /*0x44  0x00*/
        ICDC_CAACR = 33,/*0x84  0x00*/
        ICDC_CMICCR,    /*0x88  0x00*/
        ICDC_CACR2,     /*0x8C  0x0c*/
        ICDC_CAMPCR,    /*0x90  0x00*/
        ICDC_CAR,       /*0x94  0x00*/
        ICDC_CHR,       /*0x98  0x00*/
        ICDC_CHCR,      /*0x9C  0x40*/
        ICDC_CCR,       /*0xA0  0x1e*/
        ICDC_CMR = 64,  /*0x100 0x00*/
        ICDC_CTR,       /*0x104 0x46*/
        ICDC_CAGCCR,    /*0x108 0x41*/
        ICDC_CPGR,      /*0x10C 0x2c*/
        ICDC_CSRR,      /*0x110 0x00*/
        ICDC_CALMR,     /*0x114 0x26*/
        ICDC_CAHMR,     /*0x118 0x40*/
        ICDC_CALMINR,   /*0x11c 0x36*/
        ICDC_CAHMINR,   /*0x120 0x20*/
        ICDC_CAFR,      /*0x124 0x38*/
        ICDC_CCAGVR =76,/*0x130 0x0c*/
        ICDC_REG_NUM,
};

static int inline icdc_register_is_vaild(int reg)
{
        if (reg < 0 || reg > ICDC_REG_NUM)
                return 0;
        if (reg == ICDC_CGR)
                return 1;
        if (reg >= ICDC_CACR && reg <= ICDC_CDC2R)
                return 1;
        if (reg == ICDC_CGAINR || reg == ICDC_CADR)
                return 1;
        if (reg >= ICDC_CASR && reg <= ICDC_CDDPR0)
                return 1;
        if (reg >= ICDC_CAACR && reg <= ICDC_CCR)
                return 1;
        if (reg >= ICDC_CMR && reg <= ICDC_CAFR)
                return 1;
        if (reg == ICDC_CCAGVR)
                return 1;
        return 0;
}

/*CGR*/
#define ICDC_CGR_BIST_EN            BIT(7)  /*Build-In-Self-Test enable*/
#define ICDC_CGR_DCRST_N            BIT(1)  /*Digtial-CORE reset*/
#define ICDC_CGR_SRST_N             BIT(0)  /*System reset*/

/*CACR*/
#define ICDC_CACR_ADCEN              BIT(7)
#define ICDC_CACR_ADCVALDALEN_SFT    5
#define ICDC_CACR_ADCVALDALEN_MSK    (0x3 << ICDC_CACR_ADCVALDALEN_SFT)
#define ICDC_CACR_ADCVALDALEN_16BIT  (0x0 << ICDC_CACR_ADCVALDALEN_SFT)
#define ICDC_CACR_ADCVALDALEN_20BIT  (0x1 << ICDC_CACR_ADCVALDALEN_SFT)
#define ICDC_CACR_ADCVALDALEN_24BIT  (0x2 << ICDC_CACR_ADCVALDALEN_SFT)
#define ICDC_CACR_ADCI2SMODE_SFT     3
#define ICDC_CACR_ADCI2SMODE_MSK     (0x3 << ICDC_CACR_ADCI2SMODE_SFT)
#define ICDC_CACR_ADCI2SMODE_LJM     (0x1 << ICDC_CACR_ADCI2SMODE_SFT)
#define ICDC_CACR_ADCI2SMODE_I2S     (0X2 << ICDC_CACR_ADCI2SMODE_SFT)
#define ICDC_CACR_ADCSWAP_EN         BIT(1)
#define ICDC_CACR_ADCINTF_MONO       BIT(0)

/*CMCR*/
#define ICDC_CMCR_MASTEREN           BIT(5)  /*relate to i2s mode*/
#define ICDC_CMCR_ADCDAC_MASTER      BIT(4)
#define ICDC_CMCR_ADCDATALEN_SFT     2
#define ICDC_CMCR_ADCDATALEN_MSK     (0x3 << ICDC_CMCR_ADCDATALEN_SFT)
#define ICDC_CMCR_ADCDATALEN_16BIT   (0x0 << ICDC_CMCR_ADCDATALEN_SFT)
#define ICDC_CMCR_ADCDATALEN_20BIT   (0x1 << ICDC_CMCR_ADCDATALEN_SFT)
#define ICDC_CMCR_ADCDATALEN_24BIT   (0x2 << ICDC_CMCR_ADCDATALEN_SFT)
#define ICDC_CMCR_ADCDATALEN_32BIT   (0x3 << ICDC_CMCR_ADCDATALEN_SFT)
#define ICDC_CMCR_ADCI2SRST_N        BIT(1)
#define ICDC_CMCR_RASING_ALIGN       BIT(0)

/*CDCR1*/
#define ICDC_CDCR1_DACEN              BIT(7)
#define ICDC_CDCR1_DACVALDALEN_SFT    5
#define ICDC_CDCR1_DACVALDALEN_MSK    (0x3 << ICDC_CDCR1_DACVALDALEN_SFT)
#define ICDC_CDCR1_DACVALDALEN_16BIT  (0x0 << ICDC_CDCR1_DACVALDALEN_SFT)
#define ICDC_CDCR1_DACVALDALEN_20BIT  (0x1 << ICDC_CDCR1_DACVALDALEN_SFT)
#define ICDC_CDCR1_DACVALDALEN_24BIT  (0x2 << ICDC_CDCR1_DACVALDALEN_SFT)
#define ICDC_CDCR1_DACI2SMODE_SFT     3
#define ICDC_CDCR1_DACI2SMODE_MSK     (0x3 << ICDC_CDCR1_DACI2SMODE_SFT)
#define ICDC_CDCR1_DACI2SMODE_LJM     (0x1 << ICDC_CDCR1_DACI2SMODE_SFT)
#define ICDC_CDCR1_DACI2SMODE_I2S     (0X2 << ICDC_CDCR1_DACI2SMODE_SFT)
#define ICDC_CDCR1_DACSWAP_EN         BIT(1)

/*CDCR2*/
#define ICDC_CDCR2_DACDATALEN_SFT     2
#define ICDC_CDCR2_DACDATALEN_MSK     (0x3 << ICDC_CDCR2_DACDATALEN_SFT)
#define ICDC_CDCR2_DACDATALEN_16BIT   (0x0 << ICDC_CDCR2_DACDATALEN_SFT)
#define ICDC_CDCR2_DACDATALEN_20BIT   (0x1 << ICDC_CDCR2_DACDATALEN_SFT)
#define ICDC_CDCR2_DACDATALEN_24BIT   (0x2 << ICDC_CDCR2_DACDATALEN_SFT)
#define ICDC_CDCR2_DACDATALEN_32BIT   (0x3 << ICDC_CDCR2_DACDATALEN_SFT)
#define ICDC_CDCR2_DACI2SRST_N        BIT(1)

/*CADR*/
#define ICDC_CADR_ADCLOOPDAC_EN      BIT(4)
#define ICDC_CADR_DACIN              BIT(3)

/*CGAINR*/
#define ICDC_CGAINR_GAINE       BIT(5)
#define ICDC_CGAINR_ADC_HPF_EN_N	BIT(2)


/*CASR*/
#define ICDC_CASR_DACLECHSRC_SFT        6
#define ICDC_CASR_DACLECHSRC_MSK        (0x3 << ICDC_CASR_DACLECHSRC_SFT)
#define ICDC_CASR_DACLECHSRC_I2S        (0x0 << ICDC_CASR_DACLECHSRC_SFT)
#define ICDC_CASR_DACLECHSRC_SINE       (0x1 << ICDC_CASR_DACLECHSRC_SFT)
#define ICDC_CASR_DACLECHSRC_SQUARE     (0x2 << ICDC_CASR_DACLECHSRC_SFT)
#define ICDC_CASR_DACLECHSRC_ZERO       (0x3 << ICDC_CASR_DACLECHSRC_SFT)
#define ICDC_CASR_ADCLECHSRC_SFT        2
#define ICDC_CASR_ADCLECHSRC_MSK        (0x3 << ICDC_CASR_ADCLECHSRC_SFT)
#define ICDC_CASR_ADCLECHSRC_I2S        (0x0 << ICDC_CASR_ADCLECHSRC_SFT)
#define ICDC_CASR_ADCLECHSRC_SINE       (0x1 << ICDC_CASR_ADCLECHSRC_SFT)
#define ICDC_CASR_ADCLECHSRC_SQUARE     (0x2 << ICDC_CASR_ADCLECHSRC_SFT)
#define ICDC_CASR_ADCLECHSRC_ZERO       (0x3 << ICDC_CASR_ADCLECHSRC_SFT)

/*CDPR*/
#define ICDC_CDPR_ADCDATAPATH_MSK       (0x3 << 6)      /*please keep the default value*/
#define ICDC_CDPR_ADCDATAIN             BIT(3)          /*please keep the default value*/
#define ICDC_CDPR_DAIINDEBUG3           (0x3 << 0)      /*Please reference the description of CADR. dacin*/

/*CDDPR1*/
/*CDDPR0*/

/*CAACR*/
#define ICDC_CACCR_ADCSRCEN                   BIT(7)
#define ICDC_CACCR_MICBIASEN                  BIT(6)
#define ICDC_CACCR_ADCZEROEN                  BIT(5)
#define ICDC_CACCR_MICBAISCTR_SFT             0
#define ICDC_CACCR_MICBAISCTR_MSK             (0x7 << ICDC_CACCR_MICBAISCTR_SFT)
#define ICDC_CACCR_MICBAISCTR(x)              (((x) & ICDC_CACCR_MICBAISCTR_MSK) << ICDC_CACCR_MICBAISCTR_SFT)

/*CMICCR*/
#define ICDC_CMICCR_MICEN               BIT(6)
#define ICDC_CMICCR_MICGAIN_20DB        BIT(5)
#define ICDC_CMICCR_MICMUTE_N           BIT(4)
#define ICDC_CMICCR_ALCEN               BIT(1)
#define ICDC_CMICCR_ALCMUTE_N           BIT(0)

/*CACR2*/
#define ICDC_CACR2_ALCSEL_SIGNAL_ENDED  BIT(5)
#define ICDC_CACR2_ALCGAIN_SFT          0
#define ICDC_CACR2_ALCGAIN_MSK          (0X1f << ICDC_CACR2_ALCGAIN_SFT)
#define ICDC_CACR2_ALCGAIN(x)           (((x) << ICDC_CACR2_ALCGAIN_SFT) & ICDC_CACR2_ALCGAIN_MSK)

/*CAMPCR*/
#define ICDC_CAMPCR_ADCREFVOLEN         BIT(7)
#define ICDC_CAMPCR_ADCLCKEN            BIT(6)
#define ICDC_CAMPCR_ADCLAMPEN           BIT(5)
#define ICDC_CAMPCR_ADCLRST             BIT(4)

/*CAR*/
#define ICDC_CAR_DETDACSHORT            BIT(7)
#define ICDC_CAR_AUDIODACEN             BIT(6)
#define ICDC_CAR_DACREFVOLBUIFEN        BIT(5)
#define ICDC_CAR_DACREFVOLEN            BIT(3)
#define ICDC_CAR_DACCLKEN               BIT(2)
#define ICDC_CAR_DACEN                  BIT(1)
#define ICDC_CAR_DACINIT_N              BIT(0)

/*CHR*/
#define ICDC_CHR_HPOUTEN                BIT(7)
#define ICDC_CHR_HPOUTINIT_N            BIT(6)
#define ICDC_CHR_HPOUTMUTE_N            BIT(5)

/*CHCR*/
#define ICDC_CHCR_HPOUTPOP_SFT          6
#define ICDC_CHCR_HPOUTPOP_MSK          (0x3 << ICDC_CHCR_HPOUTPOP_SFT)
#define ICDC_CHCR_HPOUTPOP_PERCHARGE    (0x1 << ICDC_CHCR_HPOUTPOP_SFT)
#define ICDC_CHCR_HPOUTPOP_WORK         (0x2 << ICDC_CHCR_HPOUTPOP_SFT)
#define ICDC_CHCR_HPOUTGAIN_SFT         0
#define ICDC_CHCR_HPOUTGAIN_MSK         (0x1f << ICDC_CHCR_HPOUTGAIN_SFT)
#define ICDC_CHCR_HPOUTGAIN(x)          (((x) << ICDC_CHCR_HPOUTGAIN_SFT) & ICDC_CHCR_HPOUTGAIN_MSK)


/*CCR*/
#define ICDC_CCR_DACPERCHARGE_N         BIT(7)
#define ICDC_CCR_DACCHAGESEL_SFT        0
#define ICDC_CCR_DACCHAGESEL_MSK        (0x3f << ICDC_CCR_DACCHAGESEL_SFT)
#define ICDC_CCR_DACCHAGESEL(x)		((x << ICDC_CCR_DACCHAGESEL_SFT) & ICDC_CCR_DACCHAGESEL_MSK)

/*CPGR*/
#define ICDC_CPGR_PGALEZEROEN           BIT(5)
#define ICDC_CPGR_PGALEGAIN_SFT         0
#define ICDC_CPGR_PGALEGAIN_MSK         (0x1f << ICDC_CPGR_PGALEGAIN_SFT)
#define ICDC_CPGR_PGALEGAIN(x)		(((x) << ICDC_CPGR_PGALEGAIN_SFT) & ICDC_CPGR_PGALEGAIN_MSK)

/*CSRR*/
#define ICDC_CSRR_SLOWCLKEN             BIT(3)
#define ICDC_CSRR_SAMPLERATE_SFT        0x0
#define ICDC_CSRR_SAMPLERATE_MSK        (0x7 << ICDC_CSRR_SAMPLERATE_SFT)
#define ICDC_CSRR_SAMPLERATE_96000      (0x0 << ICDC_CSRR_SAMPLERATE_SFT)
#define ICDC_CSRR_SAMPLERATE_48000      (0x1 << ICDC_CSRR_SAMPLERATE_SFT)
#define ICDC_CSRR_SAMPLERATE_44100      (0x2 << ICDC_CSRR_SAMPLERATE_SFT)
#define ICDC_CSRR_SAMPLERATE_36000      (0x3 << ICDC_CSRR_SAMPLERATE_SFT)
#define ICDC_CSRR_SAMPLERATE_24000      (0x4 << ICDC_CSRR_SAMPLERATE_SFT)
#define ICDC_CSRR_SAMPLERATE_16000      (0x5 << ICDC_CSRR_SAMPLERATE_SFT)
#define ICDC_CSRR_SAMPLERATE_12000      (0x6 << ICDC_CSRR_SAMPLERATE_SFT)
#define ICDC_CSRR_SAMPLERATE_8000       (0x7 << ICDC_CSRR_SAMPLERATE_SFT)
#define ICDC_CSRR_SAMPLERATE(sel)       (((sel) << ICDC_CSRR_SAMPLERATE_SFT) & ICDC_CSRR_SAMPLERATE_MSK)
#endif	/* __ICDC_D4_REG_H__ */
