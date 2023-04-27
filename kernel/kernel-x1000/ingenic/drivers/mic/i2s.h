/*
 *  ingenic/drivers/sound/i2s.h	(x1830)
 *  Copyright 2018 Ingenic Semiconductor Co.,Ltd
 *	cli <chen.li@ingenic.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#ifndef __I2S_H__
#define __I2S_H__

#include <linux/bitops.h>

#define I2S_TX_FIFO_DEPTH       64
#define I2S_RX_FIFO_DEPTH       32

/* I2S */
#define AICFR		(0x00)
#define AICCR		(0x04)
#define I2SCR		(0x10)
#define AICSR		(0x14)
#define I2SSR		(0x1c)
#define I2SDIV		(0x30)
#define AICDR		(0x34)

/* For AICFR */
#define AICFR_ENB	BIT(0)
#define AICFR_SYNCD	BIT(1)
#define AICFR_BCKD	BIT(2)
#define AICFR_RST	BIT(3)
#define AICFR_AUSEL	BIT(4)
#define AICFR_ICDC	BIT(5)
#define AICFR_LSMP	BIT(6)
#define AICFR_CDC_MASTER	BIT(7)
#define AICFR_SYSCLKD	BIT(11)
#define AICFR_MSB	BIT(12)
#define AICFR_TFTH_SFT	(16)
#define AICFR_TFTH_MSK	(0x1f << AICFR_TFTH_SFT)
#define AICFR_TFTH(v)	(((v) << AICFR_TFTH_SFT) & AICFR_TFTH_MSK)
#define AICFR_RFTH_SFT	(24)
#define AICFR_RFTH_MSK	(0xf << AICFR_RFTH_SFT)
#define AICFR_RFTH(v)	(((v) << AICFR_RFTH_SFT) & AICFR_RFTH_MSK)
#define AICFR_ICDC_RST	BIT(31)

/* For AICCR */
#define AICCR_EREC	BIT(0)
#define AICCR_ERPL	BIT(1)
#define AICCR_ENLBF	BIT(2)
#define	AICCR_ETFS	BIT(3)
#define AICCR_ERFS	BIT(4)
#define AICCR_ETUR	BIT(5)
#define AICCR_EROR	BIT(6)
#define AICCR_RFLUSH	BIT(7)
#define AICCR_TFLUSH	BIT(8)
#define AICCR_ASVTSU	BIT(9)
#define AICCR_ENDSW	BIT(10)
#define AICCR_M2S	BIT(11)
#define AICCR_TDMS	BIT(14)
#define AICCR_RDMS	BIT(15)
#define AICCR_ISS_SFT		(16)
#define AICCR_ISS_MSK		(0x7 << AICCR_ISS_SFT)
#define AICCR_ISS_16BIT		(0x1 << AICCR_ISS_SFT)
#define AICCR_OSS_SFT		(19)
#define AICCR_OSS_MSK		(0x7 << AICCR_OSS_SFT)
#define AICCR_OSS_16BIT		(0x1 << AICCR_OSS_SFT)
#define AICCR_CHANNEL_SFT	(24)
#define AICCR_CHANNEL_MSK	(0x7 << AICCR_CHANNEL_SFT)
#define AICCR_CHANNEL_STEREO	(0x1 << AICCR_CHANNEL_SFT)
#define AICCR_PACK16	BIT(28)

/* For I2SCR */
#define	I2SCR_AMSL	BIT(0)
#define	I2SCR_ESCLK	BIT(4)
#define	I2SCR_STPBK	BIT(12)
#define	I2SCR_SWLH	BIT(16)
#define	I2SCR_RFIRST	BIT(17)

/* For AICSR */
#define AICSR_TFS	BIT(3)
#define AICSR_RFS	BIT(4)
#define AICSR_TUR	BIT(5)
#define AICSR_ROR	BIT(6)
#define AICSR_TFL_SFT		8
#define AICSR_TFL_MSK		(0x3f << AICSR_TFL_SFT)
#define AICSR_TFL(regval)	(((regval) & AICSR_TFL_MSK) >> AICSR_TFL_SFT)
#define AICSR_RFL_SFT		(24)
#define AICSR_RFL_MSK		(0x3f << AICSR_RFL_SFT)
#define AICSR_RFL(regval)	(((regval) & AICSR_RFL_MSK) >> AICSR_RFL_SFT)

/* For I2SSR */
#define I2SSR_BSY	BIT(2)
#define I2SSR_RBSY	BIT(3)

#define I2SSR_TBSY	BIT(4)
#define I2SSR_CHBSY	BIT(5)

/* For I2SDIV */
#define I2SDIV_DV_SFT		(0)
#define I2SDIV_DV_MSK		(0x1ff << I2SDIV_DV_SFT)
#define I2SDIV_DV(v)		(((v) << I2SDIV_DV_SFT) & I2SDIV_DV_MSK)
#define I2SDIV_IDV_SFT		(16)
#define I2SDIV_IDV_MSK		(0x1ff << I2SDIV_IDV_SFT)
#define I2SDIV_IDV(v)		(((v) << I2SDIV_IDV_SFT) & I2SDIV_IDV_MSK)
#endif  /*__I2S_H__*/
