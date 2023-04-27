/*
 *  sound/soc/ingenic/asoc-aic.h
 *  ALSA Soc Audio Layer -- ingenic aic platform driver
 *
 *  Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *	cli <chen.li@ingenic.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#ifndef __ASOC_AIC_H__
#define __ASOC_AIC_H__

#include <linux/dmaengine.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#ifdef CONFIG_SND_ASOC_INGENIC_AIC_SPDIF
#include "asoc-spdif.h"
#endif

#define INGENIC_I2S_INNER_CODEC		(1 << 0)
#define INGENIC_I2S_EX_CODEC		(1 << 1)
#define INGENIC_I2S_CODEC_MASK		(0xF)

#define INGENIC_I2S_PLAYBACK		(1 << 4)
#define INGENIC_I2S_CAPTURE		(1 << 5)
#define INGENIC_I2S_PLAY_MASK		(0xF0)


#define STREAM_PLAY	0
#define STREAM_RECORD	1
#define STREAM_COMMON	2

struct ingenic_aic_subdev_pdata {
	dma_addr_t dma_base;
};

enum aic_mode {
	AIC_NO_MODE = 0,
	AIC_I2S_MODE,
	AIC_SPDIF_MODE,
	AIC_AC97_MODE
};

enum aic_stream_mode {
	AIC_STREAM_PLAYBACK,
	AIC_STREAM_RECORD,
	AIC_STREAM_COMMON,
};

enum aic_clk_mode {
	AIC_CLOCK_GATE,
	AIC_CLOCK_CE,
	AIC_CLOCK_DIV,
	AIC_CLOCK_MUX,
};

struct ingenic_aic_priv {
	enum aic_clk_mode clk_mode;
	const char *clk_name;
	const char *mux_select;
	enum aic_stream_mode stream_mode;
	bool is_bus_clk;
};

struct ingenic_aic {
	struct device	*dev;
	void __iomem	*vaddr_base;
	/* for clock */
	const struct ingenic_aic_priv *priv;
	struct clk *clk;
	struct clk *clk_t;
	struct clk *clk_r;
	int rate;
	int rate_t;
	int rate_r;
	/*for interrupt*/
	int irqno;
	unsigned int ror;		/*counter for debug*/
	unsigned int tur;
	int mask;
	/*for aic work mode protect*/
	spinlock_t	mode_lock;
	enum aic_mode aic_working_mode;
	/*for subdev*/
	int subdevs;
	struct platform_device* psubdev[];
};

const char* aic_work_mode_str(enum aic_mode mode);
int aic_set_rate(struct device *aic, unsigned long freq, int stream);
int aic_clk_ctrl(struct device *aic, bool enable);
enum aic_mode aic_set_work_mode(struct device *aic, enum aic_mode module_mode, bool enable);

static void inline ingenic_aic_write_reg(struct device *dev, unsigned int reg,
		unsigned int val)
{
	struct ingenic_aic *ingenic_aic = dev_get_drvdata(dev);
	writel(val, ingenic_aic->vaddr_base + reg);
}

static unsigned int inline ingenic_aic_read_reg(struct device *dev, unsigned int reg)
{
	struct ingenic_aic *ingenic_aic = dev_get_drvdata(dev);
	return readl(ingenic_aic->vaddr_base + reg);
}

/* For AC97 and I2S */
#define AICFR		(0x00)
#define AICCR		(0x04)
#define I2SCR		(0x10)
#define AICSR		(0x14)
#define I2SSR		(0x1c)
#define I2SDIV		(0x30)
#define AICDR		(0x34)
#define AICLR           (0x38)
#define AICTFLR		(0x3c)

/* For AICFR */
#define AICFR_ENB_BIT		(0)
#define AICFR_ENB_MASK		(1 << AICFR_ENB_BIT)
#define AICFR_SYNCD_BIT		(1)	/* x1600 : RMASTER */
#define AICFR_SYNCD_MASK	(1 << AICFR_SYNCD_BIT)
#define AICFR_BCKD_BIT		(2)	/* x1600 : TMASTER */
#define AICFR_BCKD_MASK		(1 << AICFR_BCKD_BIT)
#define AICFR_RST_BIT		(3)
#define AICFR_RST_MASK		(1 << AICFR_RST_BIT)
#define AICFR_AUSEL_BIT		(4)
#define AICFR_AUSEL_MASK	(1 << AICFR_AUSEL_BIT)
#define AICFR_LSMP_BIT		(6)
#define AICFR_LSMP_MASK		(1 << AICFR_LSMP_BIT)
#define AICFR_DMODE_BIT		(8)
#define AICFR_DMODE_MASK	(1 << AICFR_DMODE_BIT)
#define AICFR_MSB_BIT		(12)
#define AICFR_MSB_MASK		(1 << AICFR_MSB_BIT)
#define AICFR_TFTH_BIT		(16)
#define AICFR_TFTH_MASK		(0x1f << AICFR_TFTH_BIT)
#define AICFR_RFTH_BIT		(24)
#define AICFR_RFTH_MASK		(0x1f << AICFR_RFTH_BIT)

/* For AICCR */
#define AICCR_EREC_BIT		(0)
#define AICCR_EREC_MASK		(1 << AICCR_EREC_BIT)
#define AICCR_ERPL_BIT		(1)
#define AICCR_ERPL_MASK		(1 << AICCR_ERPL_BIT)
#define AICCR_ENLBF_BIT		(2)
#define AICCR_ENLBF_MASK	(1 << AICCR_ENLBF_BIT)
#define	AICCR_ETFS_BIT		(3)
#define	AICCR_ETFS_MASK		(1 << AICCR_ETFS_BIT)
#define AICCR_ERFS_BIT		(4)
#define AICCR_ERFS_MASK		(1 << AICCR_ERFS_BIT)
#define AICCR_ETUR_BIT		(5)
#define AICCR_ETUR_MASK		(1 << AICCR_ETUR_BIT)
#define AICCR_EROR_BIT		(6)
#define AICCR_EROR_MASK		(1 << AICCR_EROR_BIT)
#define AICCR_EALL_INT_MASK	(AICCR_EROR_MASK|AICCR_ETUR_MASK|AICCR_ERFS_MASK|AICCR_ETFS_MASK)
#define AICCR_RFLUSH_BIT	(7)
#define AICCR_RFLUSH_MASK	(1 << AICCR_RFLUSH_BIT)
#define AICCR_TFLUSH_BIT	(8)
#define AICCR_TFLUSH_MASK	(1 << AICCR_TFLUSH_BIT)
#define AICCR_ENDSW_BIT		(10)
#define AICCR_ENDSW_MASK	(1 << AICCR_ENDSW_BIT)
#define AICCR_MONOCTR_RIGHT_BIT	(12)
#define AICCR_MONOCTR_RIGHT_MASK	(0x1 << AICCR_MONOCTR_RIGHT_BIT)
#define AICCR_STEREOCTR_MASK	(0x3 << AICCR_MONOCTR_RIGHT_BIT)
#define AICCR_MONOCTR_OFFSET	(13)
#define AICCR_MONOCTR_MASK	(0x1 << AICCR_MONOCTR_OFFSET)
#define AICCR_TDMS_BIT		(14)
#define AICCR_TDMS_MASK		(1 << AICCR_TDMS_BIT)
#define AICCR_RDMS_BIT		(15)
#define AICCR_RDMS_MASK		(1 << AICCR_RDMS_BIT)
#define AICCR_ISS_BIT		(16)
#define AICCR_ISS_MASK		(0x7 << AICCR_ISS_BIT)
#define AICCR_OSS_BIT		(19)
#define AICCR_OSS_MASK		(0x7 << AICCR_OSS_BIT)
#define AICCR_TLDMS_BIT		(22)
#define AICCR_TLDMS_MASK        (0x1 << AICCR_TLDMS_BIT)
#define AICCR_ETFL_BIT		(23)
#define AICCR_ETFL_MASK         (0x1 << AICCR_ETFL_BIT)
#define AICCR_CHANNEL_BIT	(24)
#define AICCR_CHANNEL_MASK	(0x7 << AICCR_CHANNEL_BIT)
#define AICCR_PACK16_BIT	(28)
#define AICCR_PACK16_MASK	(1 << AICCR_PACK16_BIT)
#define AICCR_ETFLS_BIT		(30)
#define AICCR_ETFLS_MASK	(0x1 << AICCR_ETFLS_BIT)
#define AICCR_ETFLOR_BIT	(31)
#define AICCR_ETFLOR_MASK	(0x1 << AICCR_ETFLOR_BIT)

/* For I2SCR */
#define	I2SCR_AMSL_BIT		(0)
#define	I2SCR_AMSL_MASK		(1 << I2SCR_AMSL_BIT)
#define	I2SCR_SWLH_BIT		(16)
#define	I2SCR_SWLH_MASK		(1 << I2SCR_SWLH_BIT)
#define	I2SCR_RFIRST_BIT	(17)
#define	I2SCR_RFIRST_MASK	(1 << I2SCR_RFIRST_BIT)

/* For AICSR */
#define AICSR_TFS_BIT		(3)
#define AICSR_TFS_MASK		(1 << AICSR_TFS_BIT)
#define AICSR_RFS_BIT		(4)
#define AICSR_RFS_MASK		(1 << AICSR_RFS_BIT)
#define AICSR_TUR_BIT		(5)
#define AICSR_TUR_MASK		(1 << AICSR_TUR_BIT)
#define AICSR_ROR_BIT		(6)
#define AICSR_ROR_MASK		(1 << AICSR_ROR_BIT)
#define AICSR_ALL_INT_MASK	(AICSR_TFS_MASK|AICSR_RFS_MASK|AICSR_TUR_MASK|AICSR_ROR_MASK)
#define AICSR_TFL_BIT		(8)
#define AICSR_TFL_MASK		(0x3f << AICSR_TFL_BIT)
#define AICSR_TFLL_BIT		(15)
#define AICSR_TFLL_MASK		(0x3f << AICSR_TFLL_BIT)
#define AICSR_RFL_BIT		(24)
#define AICSR_RFL_MASK		(0x3f << AICSR_RFL_BIT)

/* For I2SSR */
#define I2SSR_BSY_BIT		(2)
#define I2SSR_BSY_MASK		(1 << I2SSR_BSY_BIT)
#define I2SSR_RBSY_BIT		(3)
#define I2SSR_RBSY_MASK		(1 << I2SSR_RBSY_BIT)
#define I2SSR_TBSY_BIT		(4)
#define I2SSR_TBSY_MASK		(1 << I2SSR_TBSY_BIT)
#define I2SSR_CHBSY_BIT		(5)
#define I2SSR_CHBSY_MASK	(1 << I2SSR_CHBSY_BIT)

/* For I2SDIV */
#define I2SDIV_DV_BIT		(0)	/* x1600 : I2SDIV_TDIV_BIT */
#define I2SDIV_DV_MASK		(0x1ff << I2SDIV_DV_BIT)
#define I2SDIV_IDV_BIT		(16)	/* x1600 : I2SDIV_RDIV_BIT */
#define I2SDIV_IDV_MASK		(0x1ff << I2SDIV_IDV_BIT)

/* For AICDR */
#define AICDR_DATA_BIT		(0)
#define AICDR_DATA_MASK		(0xffffff << AICDR_DATA_BIT)

/* For AICLR*/
#define AICLR_LOOP_DATA_BIT	(0)
#define AICLR_LOOP_DATA_MASK	(0xffffff << AICDR_LOOP_DATA_BIT)

/* For AICTFLR */
#define AICTFLR_TFLTH_BIT	(0)
#define AICTFLR_TFLTH_MASK	(0xf << AICTFLR_TFLTH_BIT)


#define ingenic_aic_set_reg(parent, addr, val, mask, offset)		\
	do {							\
		volatile unsigned int reg_tmp;				\
		reg_tmp = ingenic_aic_read_reg(parent, addr);	\
		reg_tmp &= ~(mask);				\
		reg_tmp |= (val << offset) & mask;		\
		ingenic_aic_write_reg(parent, addr, reg_tmp);	\
	} while(0)

/*
 * Fix: FIFO status is not updated in time, And
 * needs to be read twice to obtain the real value.
 */
#define ingenic_aic_get_reg(parent, addr, mask, offset)			\
({									\
	u32 reg = 0;							\
	if (addr == AICSR) {						\
		ingenic_aic_read_reg(parent, addr);			\
	}								\
	reg = (ingenic_aic_read_reg(parent, addr) & mask) >> offset;	\
	reg;								\
})

#include "asoc-aic-x2500.h"

/*For ALL*/
/*aic fr*/
#define __aic_enable_msb(parent)		\
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_MSB_MASK, AICFR_MSB_BIT)
#define __aic_disable_msb(parent)		\
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_MSB_MASK, AICFR_MSB_BIT)
#define __aic_reset(parent)	\
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_RST_MASK, AICFR_RST_BIT)
/*aic cr*/
#define __aic_flush_rxfifo(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_RFLUSH_MASK, AICCR_RFLUSH_BIT)
#define __aic_flush_txfifo(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_TFLUSH_MASK, AICCR_TFLUSH_BIT)
#define __aic_en_ror_int(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_EROR_MASK, AICCR_EROR_BIT)
#define __aic_dis_ror_int(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_EROR_MASK, AICCR_EROR_BIT)
#define __aic_en_tur_int(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_ETUR_MASK, AICCR_ETUR_BIT)
#define __aic_dis_tur_int(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_ETUR_MASK, AICCR_ETUR_BIT)
#define __aic_en_rfs_int(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_ERFS_MASK, AICCR_ERFS_BIT)
#define __aic_dis_rfs_int(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_ERFS_MASK, AICCR_ERFS_BIT)
#define __aic_en_tfs_int(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_ETFS_MASK, AICCR_ETFS_BIT)
#define __aic_dis_tfs_int(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_ETFS_MASK, AICCR_ETFS_BIT)
#define __aic_get_irq_enmask(parent)	\
	ingenic_aic_get_reg(parent, AICCR, AICCR_EALL_INT_MASK, AICCR_ETFS_BIT)
#define __aic_set_irq_enmask(parent, mask)	\
	ingenic_aic_set_reg(parent, AICCR, mask, AICCR_EALL_INT_MASK, AICCR_ETFS_BIT)
/*aic sr*/
#define __aic_read_rfl(parent)		\
	ingenic_aic_get_reg(parent, AICSR ,AICSR_RFL_MASK, AICSR_RFL_BIT)
#define __aic_read_tfl(parent)		\
	ingenic_aic_get_reg(parent, AICSR, AICSR_TFL_MASK, AICSR_TFL_BIT)
#define __aic_clear_ror(parent)		\
	ingenic_aic_set_reg(parent, AICSR, 0, AICSR_ROR_MASK, AICSR_ROR_BIT)
#define __aic_test_ror(parent)		\
	ingenic_aic_get_reg(parent, AICSR, AICSR_ROR_MASK, AICSR_ROR_BIT)
#define __aic_clear_tur(parent)		\
	ingenic_aic_set_reg(parent, AICSR, 0, AICSR_TUR_MASK, AICSR_TUR_BIT)
#define __aic_test_tur(parent)		\
	ingenic_aic_get_reg(parent, AICSR, AICSR_TUR_MASK, AICSR_TUR_BIT)
#define __aic_clear_rfs(parent)		\
	ingenic_aic_set_reg(parent, AICSR, 0, AICSR_RFS_MASK, AICSR_RFS_BIT)
#define __aic_test_rfs(parent)		\
	ingenic_aic_get_reg(parent, AICSR, AICSR_RFS_MASK, AICSR_RFS_BIT)
#define __aic_clear_tfs(parent)		\
	ingenic_aic_set_reg(parent, AICSR, 0, AICSR_TFS_MASK, AICSR_TFS_BIT)
#define __aic_test_tfs(parent)		\
	ingenic_aic_get_reg(parent, AICSR, AICSR_TFS_MASK, AICSR_TFS_BIT)
#define __aic_get_irq_flag(parent)	\
	ingenic_aic_get_reg(parent, AICSR, AICSR_ALL_INT_MASK, AICSR_TFS_BIT)
#define __aic_clear_all_irq_flag(parent)	\
	ingenic_aic_set_reg(parent, AICSR, AICSR_ALL_INT_MASK, AICSR_ALL_INT_MASK, AICSR_TFS_BIT)
/* aic dr*/
#define __aic_write_txfifo(parent, n)	\
	ingenic_aic_write_reg(parent, AICDR, (n))
#define __aic_read_rxfifo(parent)	\
	ingenic_aic_read_reg(parent, AICDR)

/* For I2S */
/*aic fr*/
#define __i2s_is_enable(parent)	\
	ingenic_aic_get_reg(parent, AICFR, AICFR_ENB_MASK, AICFR_ENB_BIT)
#define __aic_enable(parent)			\
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_ENB_MASK, AICFR_ENB_BIT)

#define __aic_disable(parent)			\
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_ENB_MASK, AICFR_ENB_BIT)

#define __i2s_sync_output(parent)            \
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_SYNCD_MASK, AICFR_SYNCD_BIT)

#define __i2s_sync_input(parent)	\
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_SYNCD_MASK, AICFR_SYNCD_BIT)

#define __i2s_bclk_output(parent)            \
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_BCKD_MASK, AICFR_BCKD_BIT)

#define __i2s_bclk_input(parent)            \
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_BCKD_MASK, AICFR_BCKD_BIT)

#define __aic_select_i2s(parent)             \
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_AUSEL_MASK, AICFR_AUSEL_BIT)

#define __i2s_play_zero(parent)              \
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_LSMP_MASK, AICFR_LSMP_BIT)

#define __i2s_play_lastsample(parent)        \
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_LSMP_MASK, AICFR_LSMP_BIT)

#define __i2s_set_transmit_trigger(parent, n)  \
	ingenic_aic_set_reg(parent, AICFR, n, AICFR_TFTH_MASK, AICFR_TFTH_BIT)

#define __i2s_set_receive_trigger(parent, n)   \
	ingenic_aic_set_reg(parent, AICFR, n, AICFR_RFTH_MASK, AICFR_RFTH_BIT)

#define __i2s_set_tfifo_loop_trigger(parent, n)   \
	ingenic_aic_set_reg(parent, AICTFLR, n, AICTFLR_TFLTH_MASK, AICTFLR_TFLTH_BIT)
/*aic cr*/
#define I2S_SS2REG(n)   (((n) > 18 ? (n)/6 : (n)/9))	/* n = 8, 16, 18, 20, 24 */
#define __i2s_aic_packet16(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_PACK16_MASK, AICCR_PACK16_BIT)
#define __i2s_aic_unpacket16(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_PACK16_MASK, AICCR_PACK16_BIT)
#define __i2s_channel(parent, n)	\
	ingenic_aic_set_reg(parent, AICCR, ((n) - 1), AICCR_CHANNEL_MASK, AICCR_CHANNEL_BIT)
#define __i2s_set_oss(parent, n)	\
	ingenic_aic_set_reg(parent, AICCR, I2S_SS2REG(n) , AICCR_OSS_MASK, AICCR_OSS_BIT)
#define __i2s_set_iss(parent, n)	\
	ingenic_aic_set_reg(parent, AICCR, I2S_SS2REG(n) , AICCR_ISS_MASK, AICCR_ISS_BIT)
#define __i2s_transmit_dma_is_enable(parent)	\
	ingenic_aic_get_reg(parent, AICCR, AICCR_TDMS_MASK,AICCR_TDMS_BIT)
#define __i2s_disable_transmit_dma(parent)			\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_TDMS_MASK, AICCR_TDMS_BIT)
#define __i2s_enable_transmit_dma(parent)			\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_TDMS_MASK, AICCR_TDMS_BIT)
#define __i2s_receive_dma_is_enable(parent)	\
	ingenic_aic_get_reg(parent, AICCR, AICCR_RDMS_MASK,AICCR_RDMS_BIT)
#define __i2s_disable_receive_dma(parent)			\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_RDMS_MASK, AICCR_RDMS_BIT)
#define __i2s_enable_receive_dma(parent)			\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_RDMS_MASK, AICCR_RDMS_BIT)
#define __i2s_endsw_enable(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_ENDSW_MASK, AICCR_ENDSW_BIT)
#define __i2s_endsw_disable(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_ENDSW_MASK, AICCR_ENDSW_BIT)
#define __i2s_enable_replay(parent)				\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_ERPL_MASK, AICCR_ERPL_BIT)
#define __i2s_enable_record(parent)				\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_EREC_MASK, AICCR_EREC_BIT)
#define __i2s_enable_loopback(parent)				\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_ENLBF_MASK, AICCR_ENLBF_BIT)
#define __i2s_disable_replay(parent)				\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_ERPL_MASK, AICCR_ERPL_BIT)
#define __i2s_disable_record(parent)				\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_EREC_MASK, AICCR_EREC_BIT)
#define __i2s_disable_loopback(parent)				\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_ENLBF_MASK, AICCR_ENLBF_BIT)
#define __i2s_enable_monoctr_left(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 2, AICCR_STEREOCTR_MASK, AICCR_MONOCTR_RIGHT_BIT)
#define __i2s_disable_monoctr_left(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_STEREOCTR_MASK, AICCR_MONOCTR_RIGHT_BIT)
#define __i2s_enable_monoctr_right(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_STEREOCTR_MASK, AICCR_MONOCTR_RIGHT_BIT)
#define __i2s_disable_monoctr_right(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_STEREOCTR_MASK, AICCR_MONOCTR_RIGHT_BIT)
#define __i2s_enable_stereo(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_STEREOCTR_MASK, AICCR_MONOCTR_RIGHT_BIT)
#define __i2s_enable_tfifo_loop(parent)				\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_ETFL_MASK, AICCR_ETFL_BIT)
#define __i2s_disable_tfifo_loop(parent)				\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_ETFL_MASK, AICCR_ETFL_BIT)
#define __i2s_enable_tfifo_loop_dma(parent)				\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_TLDMS_MASK, AICCR_TLDMS_BIT)
#define __i2s_disable_tfifo_loop_dma(parent)				\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_TLDMS_MASK, AICCR_TLDMS_BIT)
/*i2s cr*/
#define __i2s_select_i2s_fmt(parent)	\
	ingenic_aic_set_reg(parent, I2SCR, 0, I2SCR_AMSL_MASK, I2SCR_AMSL_BIT)
#define __i2s_select_msb_fmt(parent)	\
	ingenic_aic_set_reg(parent, I2SCR, 1, I2SCR_AMSL_MASK, I2SCR_AMSL_BIT)
#define __i2s_send_rfirst(parent)            \
	ingenic_aic_set_reg(parent, I2SCR, 1, I2SCR_RFIRST_MASK, I2SCR_RFIRST_BIT)
#define __i2s_select_packed_lrswap(parent)	\
	ingenic_aic_set_reg(parent, I2SCR, 1, I2SCR_SWLH_MASK, I2SCR_SWLH_BIT)
#define __i2s_select_packed_lrnorm(parent)	\
	ingenic_aic_set_reg(parent, I2SCR, 0, I2SCR_SWLH_MASK, I2SCR_SWLH_BIT)
#define __i2s_send_rfirst(parent)	\
	ingenic_aic_set_reg(parent, I2SCR, 1, I2SCR_RFIRST_MASK, I2SCR_RFIRST_BIT)
#define __i2s_send_lfirst(parent)	\
	ingenic_aic_set_reg(parent, I2SCR, 0, I2SCR_RFIRST_MASK, I2SCR_RFIRST_BIT)
/*i2s sr*/
#define __i2s_transmiter_is_busy(parent)	\
	(!!(ingenic_aic_read_reg(parent, I2SSR) & I2SSR_TBSY_MASK))
#define __i2s_receiver_is_busy(parent)	\
	(!!(ingenic_aic_read_reg(parent, I2SSR) & I2SSR_TBSY_MASK))

#define __i2s_slave_clkset(parent)        \
	do {                                            \
		__i2s_bclk_input(parent);                    \
		__i2s_sync_input(parent);                    \
		__i2s_isync_input(parent);                   \
		__i2s_ibclk_input(parent);                   \
	}while(0)

#define __i2s_master_clkset(parent)        \
	do {                                            \
		__i2s_bclk_output(parent);                    \
		__i2s_sync_output(parent);                    \
	}while(0)

/*i2s div*/
#define __i2s_set_idv(parent, div)	\
	ingenic_aic_set_reg(parent, I2SDIV, div, I2SDIV_IDV_MASK, I2SDIV_IDV_BIT)
#define __i2s_set_dv(parent, div)	\
	ingenic_aic_set_reg(parent, I2SDIV, div, I2SDIV_DV_MASK, I2SDIV_DV_BIT)

/*AICTFLR*/
#define AICFR_TFIFO_LOOP_OFFSET (0)
#define AICFR_LOOP_DATA_OFFSET	(0)
#define AICFR_TFIFO_LOOP_MASK   (0xf << AICFR_TFIFO_LOOP_OFFSET)

#define __i2s_read_tfifo_loop(parent)		\
	ingenic_aic_set_reg_get_reg(parent, AICTFLR, AICFR_TFIFO_LOOP_MASK, AICFR_LOOP_DATA_OFFSET)
#define __i2s_write_tfifo_loop(parent, v)	\
	ingenic_aic_set_reg(parent, AICTFLR, v, AICFR_TFIFO_LOOP_MASK, AICFR_LOOP_DATA_OFFSET)

/* xzhang add */
#define AICFR_RMASTER_BIT	(1)
#define AICFR_RMASTER_MASK	(1 << AICFR_RMASTER_BIT)
#define AICFR_TMASTER_BIT	(2)
#define AICFR_TMASTER_MASK	(1 << AICFR_TMASTER_BIT)
#define AICCR_MONOCTRL_BIT	(12)
#define AICCR_MONOCTRL_MASK	(0x3 << AICCR_MONOCTRL_BIT)

#define __i2s_codec_record_slave(parent)            \
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_RMASTER_MASK, AICFR_RMASTER_BIT)

#define __i2s_codec_record_master(parent)	\
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_RMASTER_MASK, AICFR_RMASTER_BIT)

#define __i2s_codec_playback_slave(parent)            \
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_TMASTER_MASK, AICFR_TMASTER_BIT)

#define __i2s_codec_playback_master(parent)            \
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_TMASTER_MASK, AICFR_TMASTER_BIT)

#define __i2s_independent_clk(parent)	\
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_DMODE_MASK, AICFR_DMODE_BIT)
#define __i2s_share_clk(parent)	\
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_DMODE_MASK, AICFR_DMODE_BIT)

#define __i2s_record_left_right_channel_work(parent)			\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_MONOCTRL_MASK, AICCR_MONOCTRL_BIT)
#define __i2s_record_right_channel_work(parent)			\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_MONOCTRL_MASK, AICCR_MONOCTRL_BIT)
#define __i2s_record_left_channel_work(parent)			\
	ingenic_aic_set_reg(parent, AICCR, 2, AICCR_MONOCTRL_MASK, AICCR_MONOCTRL_BIT)

/* xzhang add */

#endif  /*__ASOC_AIC_H__*/
