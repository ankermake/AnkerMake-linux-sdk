/*
 * JZSOC CPM register definition.
 *
 * CPM (Clock reset and Power control Management)
 *
 * Copyright (C) 2018 Ingenic Semiconductor Co., Ltd.
 */

#ifndef __CPM_H__
#define __CPM_H__

#include <soc/base.h>

#define CPM_CPCCR	(0x00)
#define CPM_CPPCR   (0x0c)
#define CPM_CPAPCR	(0x10)
#define CPM_CPMPCR	(0x14)
#define CPM_CPVPCR  (0xe0)
#define CPM_DDRCDR	(0x2c)
#define CPM_HELIXCDR    (0x30)
#define CPM_CPPSR	(0x34)
#define CPM_CPSPPR	(0x38)
#define CPM_USBPCR  (0x3c)
#define CPM_USBRDT (0x40)
#define CPM_USBVBFIL (0x44)
#define CPM_USBPCR1 (0x48)
#define CPM_MACPHYP (0x50)
#define CPM_MACCDR  (0x54)
#define CPM_LPCDR	(0x64)
#define CPM_MSC0CDR	(0x68)
#define CPM_MSC1CDR	(0xa4)
#define CPM_I2SCDR	(0x60)
#define CPM_I2SCDR1 (0x70)
#define CPM_SSICDR	(0x74)
#define CPM_CIMCDR	(0x7c)
#define CPM_TIZIANOCDR	(0x80)
#define CPM_EXCLK_DS    (0x8c)
#define CPM_INTR	(0xb0)
#define CPM_INTRE	(0xb4)
#define CPM_DRCG    (0xd0)
#define CPM_CPCSR	(0xd4)
#define CPM_MACPHYC (0xe8)


#define CPM_LPCR		(0x04)
#define CPM_CLKGR0	(0x20)
#define CPM_OPCR	(0x24)
#define CPM_CLKGR1  (0x28)
#define CPM_SRBC0	(0xc4)
#define CPM_MESTSEL   (0xec)
#define CPM_MEMSCR0 (0xf0)
#define CPM_MEMSCR1 (0xf4)
#define CPM_MEMPDR0 (0xf8)

#define CPM_RSR		(0x08)


#define LCR_LPM_MASK		(0x3)
#define LCR_LPM_SLEEP		(0x1)

#define CPM_LCR_PD_X2D		(0x1<<31)
#define CPM_LCR_PD_VPU		(0x1<<30)
#define CPM_LCR_PD_MASK		(0x3<<30)
#define CPM_LCR_X2DS 		(0x1<<27)
#define CPM_LCR_VPUS		(0x1<<26)
#define CPM_LCR_STATUS_MASK 	(0x3<<26)

#define OPCR_ERCS		(0x1<<2)
#define OPCR_PD			(0x1<<3)
#define OPCR_IDLE		(0x1<<31)

//#define CLKGR1_VPU              (0x1<<2)
#define CLKGR_VPU              (0x1<<19)
#define CLKGR_RTC              (0x1 << 29)

#if 0
//#ifdef CONFIG_BOARD_T15_FPGA
#define cpm_inl(x)		0x3
#define cpm_outl(val,off)	do{int tmp = 0; tmp = val; tmp = off;}while(0)
//#define cpm_outl(v,x)		do{}while(0)
#define cpm_clear_bit(off,x)	do{int tmp = 0; tmp = x; tmp = off;}while(0)
#define cpm_set_bit(off,x)	do{int tmp = 0; tmp = x; tmp = off;}while(0)
#define cpm_test_bit(val,off)	1
#else
#define cpm_inl(off)		inl(CPM_IOBASE + (off))
#define cpm_outl(val,off)	outl(val,CPM_IOBASE + (off))
#define cpm_clear_bit(val,off)	do{cpm_outl((cpm_inl(off) & ~(1 << (val))),off);}while(0)
#define cpm_set_bit(val,off)	do{cpm_outl((cpm_inl(off) | (1 << (val))),off);}while(0)
#define cpm_test_bit(val,off)	(cpm_inl(off) & (0x1 << (val)))
#endif

#endif
/* __CPM_H__ */
