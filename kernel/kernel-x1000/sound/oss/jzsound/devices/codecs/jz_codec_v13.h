/*
 * sound/oss/jzsound/devices/codec/jz_codec_v13.h
 * Oss Audio driver -- ingenic internal codec driver

 * Copyright 2015 Ingenic Semiconductor Co.,Ltd
 *	cli <chen.li@ingenic.com>
 *
 * Note: icdc_d3 is an internal codec for jz SOC
 *	 used for jz4775 m150 and so on
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __X1000_CODEC_H__
#define __X1000_CODEC_H__

#include <mach/jzsnd.h>
#include "../xb47xx_i2s.h"
#include <linux/bitops.h>

enum {
	SCODA_REG_SR = 0x0,
	SCODA_REG_SR2,
	SCODA_REG_SIGR,
	SCODA_REG_SIGR2,
	SCODA_REG_SIGR3,
	SCODA_REG_SIGR5,
	SCODA_REG_SIGR7,
	SCODA_REG_MR,
	SCODA_REG_AICR_DAC,
	SCODA_REG_AICR_ADC,
	SCODA_REG_CR_DMIC,
	SCODA_REG_CR_MIC1,
	SCODA_REG_CR_MIC2,
	SCODA_REG_CR_DAC,
	SCODA_REG_CR_DAC2,
	SCODA_REG_CR_ADC,
	SCODA_REG_CR_MIX,
	SCODA_REG_DR_MIX,
	SCODA_REG_CR_VIC,
	SCODA_REG_CR_CK,
	SCODA_REG_FCR_DAC,
	SCODA_REG_SFCCR_DAC,
	SCODA_REG_SFFCR_DAC,
	SCODA_REG_FCR_ADC,
	SCODA_REG_CR_TIMER_MSB,
	SCODA_REG_CR_TIMER_LSB,
	SCODA_REG_ICR,
	SCODA_REG_IMR,
	SCODA_REG_IFR,
	SCODA_REG_IMR2,
	SCODA_REG_IFR2,
	SCODA_REG_GCR_DACL,
	SCODA_REG_GCR_DACR,
	SCODA_REG_GCR_DACL2,
	SCODA_REG_GCR_DACR2,
	SCODA_REG_GCR_MIC1,
	SCODA_REG_GCR_MIC2,
	SCODA_REG_GCR_ADCL,
	SCODA_REG_GCR_ADCR,
	SCODA_REG_GCR_MIXDACL,
	SCODA_REG_GCR_MIXDACR,
	SCODA_REG_GCR_MIXADCL,
	SCODA_REG_GCR_MIXADCR,
	SCODA_REG_CR_DAC_AGC,
	SCODA_REG_DR_DAC_AGC,
	SCODA_REG_CR_DAC2_AGC,
	SCODA_REG_DR_DAC2_AGC,
	SCODA_REG_CR_ADC_AGC,
	SCODA_REG_DR_ADC_AGC,
	SCODA_REG_SR_ADC_AGCDGL,
	SCODA_REG_SR_ADC_AGCDGR,
	SCODA_REG_SR_ADC_AGCAGL,
	SCODA_REG_SR_ADC_AGCAGR,
	SCODA_REG_CR_TR,
	SCODA_REG_DR_TR,
	SCODA_REG_SR_TR1,
	SCODA_REG_SR_TR2,
	SCODA_REG_SR_TR_SRCDAC,

/*  icdc_d3 internal register extend space */
	SCODA_MIX_0,
	SCODA_MIX_1,
	SCODA_MIX_2,
	SCODA_MIX_3,
	SCODA_MIX_4,

	SCODA_DAC_AGC0,
	SCODA_DAC_AGC1,
	SCODA_DAC_AGC2,
	SCODA_DAC_AGC3,

	SCODA_DAC2_AGC0,
	SCODA_DAC2_AGC1,
	SCODA_DAC2_AGC2,
	SCODA_DAC2_AGC3,

	SCODA_ADC_AGC0,
	SCODA_ADC_AGC1,
	SCODA_ADC_AGC2,
	SCODA_ADC_AGC3,
	SCODA_ADC_AGC4,
	SCODA_MAX_REG_NUM,
};

/*aicr dac*/
#define SCODA_AICR_DAC_ADWL_SHIFT (6)
#define SCODA_AICR_DAC_ADWL_MASK (0x3 << SCODA_AICR_DAC_ADWL_SHIFT)
#define SCODA_AICR_DAC_SLAVE_SHIFT (5)
#define SCODA_AICR_DAC_SLAVE_MASK (0x1 << SCODA_AICR_DAC_SLAVE_SHIFT)
#define SCODA_AICR_DAC_SLAVE (1 << 5)
#define SCODA_AICR_DAC_SB_SHIFT (4)
#define SCODA_AICR_DAC_SB_MASK (0x1 << SCODA_AICR_DAC_SB_SHIFT)
#define SCODA_AICR_DAC_AUDIOIF_SHIFT (0)
#define SCODA_AICR_DAC_AUDIO_MASK (0x3 << SCODA_AICR_DAC_AUDIOIF_SHIFT)
#define SCODA_AICR_DAC_AUDIOIF_I2S (0x3)

/* aicr adc */
#define SCODA_AICR_ADC_ADWL_SHIFT (6)
#define SCODA_AICR_ADC_ADWL_MASK (0x3 << SCODA_AICR_ADC_ADWL_SHIFT)
#define SCODA_AICR_ADC_SB_SHIFT (4)
#define SCODA_AICR_ADC_SB_MASK (0x1 << SCODA_AICR_ADC_SB_SHIFT)
#define SCODA_AICR_ADC_AUDIOIF_SHIFT (0)
#define SCODA_AICR_ADC_AUDIO_MASK (0x3 << SCODA_AICR_ADC_AUDIOIF_SHIFT)
#define SCODA_AICR_ADC_AUDIOIF_I2S (0x3)

/* cr vic */
#define SCODA_CR_VIC_SB_SHIFT (0)
#define SCODA_CR_VIC_SB_MASK (1 << SCODA_CR_VIC_SB_SHIFT)
#define SCODA_CR_VIC_SB_SLEEP_SHIFT (1)
#define SCODA_CR_VIC_SB_SLEEP_MASK (1 << SCODA_CR_VIC_SB_SLEEP_SHIFT)

/* fcr adc/dac */
#define SCODA_FCR_FREQ_SHIFT (0)
#define SCODA_FCR_FREQ_MASK (0xf << SCODA_FCR_FREQ_SHIFT)

/* cr dac */
#define SCODA_CR_DAC_SMUTE_SHIFT (7)
#define SCODA_CR_DAC_SMUTE_MASK (0x1 << SCODA_CR_DAC_SMUTE_SHIFT)
#define SCODA_CR_DAC_SB_SHIFT (4)
#define SCODA_CR_DAC_SB_MASK (0x1 << SCODA_CR_DAC_SB_SHIFT)
#define SCODA_CR_DAC_ZERO_SHIFT (0)
#define SCODA_CR_DAC_ZERO_MASK (0x1 << SCODA_CR_DAC_ZERO_SHIFT)

/* cr dac */
#define SCODA_CR_ADC_SMUTE_SHIFT (7)
#define SCODA_CR_ADC_SMUTE_MASK (0x1 << SCODA_CR_ADC_SMUTE_SHIFT)
#define SCODA_CR_ADC_MIC_SEL_SHIFT (6)
#define SCODA_CR_ADC_MIC_SEL_MASK (0x1 << SCODA_CR_ADC_MIC_SEL_SHIFT)
#define SCODA_CR_ADC_SB_SHIFT (4)
#define SCODA_CR_ADC_SB_MASK (0x1 << SCODA_CR_ADC_SB_SHIFT)
#define SCODA_CR_ADC_ZERO_SHIFT (0)
#define SCODA_CR_ADC_ZERO_MASK (0x1 << SCODA_CR_ADC_ZERO_SHIFT)

/* ifr */
#define SCODA_IFR_DAC_MUTE_SHIFT (0)
#define SCODA_IFR_DAC_MUTE_MASK (0x1 << SCODA_IFR_DAC_MUTE_SHIFT)
#define SCODA_IFR_ADC_MUTE_SHIFT (2)
#define SCODA_IFR_ADC_MUTE_MASK (0x1 << SCODA_IFR_ADC_MUTE_SHIFT)
#define SCODA_IFR_ADAS_LOCK_SHIFT (7)
#define SCODA_IFR_ADAS_LOCK_MASK (0x1 << SCODA_IFR_ADAS_LOCK_SHIFT)

/* cr ck */
#define SCODA_CR_CK_MCLK_DIV_SHIFT (6)
#define SCODA_CR_CK_MCLK_DIV_MASK (0x1 << SCODA_CR_CK_MCLK_DIV_SHIFT)
#define SCODA_CR_CK_SDCLK_SHIFT (4)
#define SCODA_CR_CK_SDCLK_MASK (0x1 << SCODA_CR_CK_SDCLK_SHIFT)
#define SCODA_CR_CRYSTAL_SHIFT (0)
#define SCODA_CR_CRYSTAL_MASK (0xf << SCODA_CR_CRYSTAL_SHIFT)

/* icr */
#define SCODA_ICR_INT_FORM_SHIFT (6)
#define SCODA_ICR_INT_FORM_MASK (0x3 << SCODA_ICR_INT_FORM_SHIFT)
#define SCODA_ICR_INT_FORM_HIGH (0)

/* imr */
#define SCODA_IMR_COMMON_MASK ((1)|(1<<2)|(1<<7))
#define SCODA_IMR2_COMMON_MASK (1<<4)


#define CODEC_MIX_FUNC_NORMAL		0x00
#define CODEC_MIX_FUNC_CROSS		0x01
#define CODEC_MIX_FUNC_MIXED		0x02
#define CODEC_MIX_FUNC_NOINPUT		0x03
/*For Codec*/
#define AIC_RGADW		(0xa4)
#define AIC_RGDATA		(0xa8)

static inline void icdc_d3_mapped_reg_set(void __iomem * xreg, int xmask, int xval)
{
	int val = readl(xreg);
	val &= ~(xmask);
	val |= xval;
	writel(val, xreg);
}

static inline int icdc_d3_mapped_test_bits(void __iomem * xreg, int xmask, int xval)
{
	int val = readl(xreg);
	val &= xmask;
	return (val == xval);
}

/*
 * AIC_RGADW
 */
#define SCODA_RGDIN_BIT		(0)
#define SCODA_RGDIN_MASK		(0xff << SCODA_RGDIN_BIT)
#define SCODA_RGADDR_BIT		(8)
#define SCODA_RGADDR_MASK		(0x7f << SCODA_RGADDR_BIT)
#define SCODA_RGWR_BIT		(16)
#define SCODA_RGWR_MASK		(0x1  << SCODA_RGWR_BIT)

#define icdc_d3_test_rw_inval(i2s_dev)      \
	icdc_d3_mapped_test_bits((i2s_dev->i2s_iomem + RGADW), SCODA_RGWR_MASK, (1 << SCODA_RGWR_BIT))
/*
 * AIC_RGDATA
 */
#define SCODA_RGDOUT_BIT		(0)
#define SCODA_RGDOUT_MASK		(0xff << SCODA_RGDOUT_BIT)
#define SCODA_IRQ_BIT		(8)
#define SCODA_IRQ_MASK		(0x1  << SCODA_IRQ_BIT)

#define icdc_d3_test_irq(icdc_d3)	\
	icdc_d3_mapped_test_bits((icdc_d3->mapped_base + AIC_RGDATA),	\
			SCODA_IRQ_MASK, (1 << SCODA_IRQ_BIT))

static inline u8 icdc_d3_hw_read_normal(struct codec_info *icdc_d3, int reg)
{
	struct i2s_device * i2s_dev = icdc_d3->codec_parent;
	void __iomem * mapped_base = i2s_dev->i2s_iomem;
	int reval;
	int timeout = 0xfffff;
	unsigned long flags;

//	printk ("%s, %s----->mapped_base = 0x%x, AIC_RGADW = 0x%x\n", __FILE__,  __func__, mapped_base, AIC_RGADW);
	spin_lock_irqsave(&i2s_dev->i2s_lock, flags);

	while(icdc_d3_test_rw_inval(i2s_dev)) {
		timeout--;
		if (!timeout) pr_err("i2s_dev test_rw_inval timeout\n");
	}

	icdc_d3_mapped_reg_set((mapped_base + AIC_RGADW), SCODA_RGWR_MASK,(0 << SCODA_RGWR_BIT));

	icdc_d3_mapped_reg_set((mapped_base + AIC_RGADW), SCODA_RGADDR_MASK,(reg << SCODA_RGADDR_BIT));

	reval = readl((mapped_base + AIC_RGDATA));
	reval = readl((mapped_base + AIC_RGDATA));
	reval = readl((mapped_base + AIC_RGDATA));
	reval = readl((mapped_base + AIC_RGDATA));
	reval = readl((mapped_base + AIC_RGDATA));
	reval = ((reval & SCODA_RGDOUT_MASK) >> SCODA_RGDOUT_BIT);
//	printk("reg %x = %x\n", reg, reval);
	spin_unlock_irqrestore(&i2s_dev->i2s_lock, flags);
	return (u8) reval;
}

static inline int icdc_d3_hw_write_normal(struct codec_info *icdc_d3, int reg, int data)
{
	struct i2s_device * i2s_dev = icdc_d3->codec_parent;
	void __iomem * mapped_base = i2s_dev->i2s_iomem;

//	printk ("%s, %s----->mapped_base = 0x%x, AIC_RGADW = 0x%x\n", __FILE__,  __func__, mapped_base, AIC_RGADW);

	int ret = 0;
	int timeout = 0xfffff;
	unsigned long flags;

	spin_lock_irqsave(&i2s_dev->i2s_lock, flags);

	while(icdc_d3_test_rw_inval(i2s_dev)) {
		timeout--;
		if (!timeout) pr_err("i2s_dev test_rw_inval timeout\n");
	}
	icdc_d3_mapped_reg_set((mapped_base + AIC_RGADW),SCODA_RGDIN_MASK|SCODA_RGADDR_MASK,(data << SCODA_RGDIN_BIT)|(reg << SCODA_RGADDR_BIT));
	icdc_d3_mapped_reg_set((mapped_base + AIC_RGADW), SCODA_RGWR_MASK , 1 << SCODA_RGWR_BIT);
	spin_unlock_irqrestore(&i2s_dev->i2s_lock, flags);
	ret = icdc_d3_hw_read_normal(icdc_d3, reg);
	if ((data != ret) && (reg != 0x11)){     //0x11 register is special, cannot follow this rule.
		printk("codec write reg %x err, exp %x now is %x\n",reg,data,ret);
		ret = -1;
	}
	return ret;
}

static int icdc_d3_hw_write_extend(struct codec_info *icdc_d3, u8 sreg, u8 sdata){

//	printk ("%s, %s----->\n", __FILE__, __func__);
	int creg, cdata, dreg;
	switch (sreg) {
		case SCODA_MIX_0 ... SCODA_MIX_4:
			creg = SCODA_REG_CR_MIX;
			dreg = SCODA_REG_DR_MIX;
			sreg -= (SCODA_REG_SR_TR_SRCDAC + 1);
			break;
		case SCODA_DAC_AGC0 ... SCODA_DAC_AGC3:
			creg = SCODA_REG_CR_DAC_AGC;
			dreg = SCODA_REG_DR_DAC_AGC;
			sreg -= (SCODA_MIX_4 +1);
			break;
		case SCODA_DAC2_AGC0 ... SCODA_DAC2_AGC3:
			creg = SCODA_REG_CR_DAC2;
			dreg = SCODA_REG_DR_DAC2_AGC;
			sreg -= (SCODA_DAC_AGC3 + 1);
			break;
		case SCODA_ADC_AGC0 ... SCODA_ADC_AGC4:
			creg = SCODA_REG_CR_ADC_AGC;
			dreg = SCODA_REG_DR_ADC_AGC;
			sreg -= (SCODA_ADC_AGC4 + 1);
			break;
		default:
			return 0;
	}
	cdata = (icdc_d3_hw_read_normal(icdc_d3,creg)&(~0x3f))|(sreg&0x3f);
	icdc_d3_hw_write_normal(icdc_d3, dreg, sdata);
	icdc_d3_hw_write_normal(icdc_d3, creg, cdata|0x40);
	icdc_d3_hw_write_normal(icdc_d3, creg, cdata);
	if(sdata!=icdc_d3_hw_read_normal(icdc_d3,dreg))
		return -1;
	return 0;
}


static u8 icdc_d3_hw_read_extend(struct codec_info *icdc_d3, u8 sreg)
{
	int creg, cdata, dreg, ddata;
//	printk ("%s, %s----->\n", __FILE__, __func__);
	switch (sreg) {

		case SCODA_MIX_0 ... SCODA_MIX_4:
			creg = SCODA_REG_CR_MIX;
			dreg = SCODA_REG_DR_MIX;
			sreg -= (SCODA_REG_SR_TR_SRCDAC + 1);
			break;
		case SCODA_DAC_AGC0 ... SCODA_DAC_AGC3:
			creg = SCODA_REG_CR_DAC_AGC;
			dreg = SCODA_REG_DR_DAC_AGC;
			sreg -= (SCODA_MIX_4 +1);
			break;
		case SCODA_DAC2_AGC0 ... SCODA_DAC2_AGC3:
			creg = SCODA_REG_CR_DAC2;
			dreg = SCODA_REG_DR_DAC2_AGC;
			sreg -= (SCODA_DAC_AGC3 + 1);
			break;
		case SCODA_ADC_AGC0 ... SCODA_ADC_AGC4:
			creg = SCODA_REG_CR_ADC_AGC;
			dreg = SCODA_REG_DR_ADC_AGC;
			sreg -= (SCODA_ADC_AGC4 + 1);
			break;
		default:
			return 0;
	}
	cdata = (icdc_d3_hw_read_normal(icdc_d3,creg)&(~0x7f))|(sreg&0x3f);
	icdc_d3_hw_write_normal(icdc_d3, creg, cdata);
	ddata = icdc_d3_hw_read_normal(icdc_d3, dreg);
	return (u8) ddata;
}


static inline u8 icdc_d3_hw_read(struct codec_info *icdc_d3, int reg)
{
	if (reg > SCODA_REG_SR_TR_SRCDAC)
		return icdc_d3_hw_read_extend(icdc_d3, reg);
	else
		return icdc_d3_hw_read_normal(icdc_d3, reg);
}
static inline int icdc_d3_hw_write(struct codec_info *icdc_d3, int reg, int data)
{
	if (reg > SCODA_REG_SR_TR_SRCDAC){
		return icdc_d3_hw_write_extend(icdc_d3, reg, data);
	} else {
		return icdc_d3_hw_write_normal(icdc_d3, reg, data);
	}
}


#endif	/* __ICDC_D3_REG_H__ */
