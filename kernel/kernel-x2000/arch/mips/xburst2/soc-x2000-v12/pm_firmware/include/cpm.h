

#ifndef __SOC_CPM_H__
#define __SOC_CPM_H__

#include <base.h>

#define CPM_CPCCR               (0x00)
#define CPM_CPCSR               (0xD4)

#define CPM_DDRCDR              (0x2c)
#define CPM_MACCDR              (0x54)
#define CPM_MACTXCDR            (0x58)
#define CPM_MACTXCDR1           (0xdc)
#define CPM_MACPTPCDR           (0x4c)
#define CPM_I2S0CDR             (0x60)
#define CPM_I2S1CDR             (0x7c)
#define CPM_I2S2CDR             (0x84)
#define CPM_I2S3CDR             (0x8c)
#define CPM_I2S0CDR1            (0x70)
#define CPM_I2S1CDR1            (0x80)
#define CPM_I2S2CDR1            (0x88)
#define CPM_I2S3CDR1            (0xa0)
#define CPM_AUDIOCR             (0xac)
#define CPM_LPCDR               (0x64)
#define CPM_MSC0CDR             (0x68)
#define CPM_MSC1CDR             (0xa4)
#define CPM_MSC2CDR             (0xa8)
#define CPM_SFCCDR              (0x74)
#define CPM_SSICDR              (0x5c)
#define CPM_CIMCDR              (0x78)
#define CPM_PWMCDR              (0x6c)
#define CPM_ISPCDR              (0x30)
#define CPM_RSACDR              (0x50)

#define CPM_INTR                (0xB0)
#define CPM_INTRE               (0xB4)
#define CPM_SFTINT              (0xBC)
#define CPM_DRCG                (0xD0)
#define CPM_CPPSR               (0x34)
#define CPM_CPSPPR              (0x38)
#define CPM_USBPCR              (0x3C)
#define CPM_USBRDT              (0x40)
#define CPM_USBVBFIL            (0x44)
#define CPM_USBPCR1             (0x48)

#define CPM_CPPCR               (0x0C)
#define CPM_CPAPCR              (0x10)
#define CPM_CPMPCR              (0x14)
#define CPM_CPEPCR              (0x18)

#define CPM_LCR                 (0x04)
#define CPM_PSWC0ST             (0x90)
#define CPM_PSWC1ST             (0x94)
#define CPM_PSWC2ST             (0x98)
#define CPM_PSWC3ST             (0x9C)
#define CPM_CLKGR               (0x20)
#define CPM_CLKGR1              (0x28)
#define CPM_MESTSEL             (0xEC)
#define CPM_SRBC                (0xC4)
#define CPM_EXCLK_DS            (0xE0)
#define CPM_MPDCR               (0xF8)
#define CPM_MPDCR1              (0xFC)
#define CPM_SLBC                (0xC8)
#define CPM_SLPC                (0xCC)
#define CPM_OPCR                (0x24)
#define CPM_RSR                 (0x08)



#define CPCCR_CE_CPU		(22)
#define CPCCR_L2CDIV		(4)
#define CPCCR_CDIV		(0)
#define CPCSR_CDIV_BUSY		(0)

#define LCR_LPM_MASK            (0x3)
#define LCR_LPM_IDLE		(0)
#define LCR_LPM_SLEEP           (0x1)
#define LCR_LPM_IDLE_PD         (0x2)

#define OPCR_IDLE_DIS		(1 << 31)
#define OPCR_MASK_INT		(1 << 30)
#define OPCR_L2C_PD		(1 << 26)
#define OPCR_NEMC_ROM_DS	(1 << 22)
#define OPCR_CPU_RAM_DS		(1 << 21)
#define OPCR_DDR_RAM_DS		(1 << 20)
/*#define OPCR_O1ST		()*/
#define OPCR_O1SE		(1 << 4)
#define OPCR_PD			(1 << 3)
#define OPCR_ERCS		(1 << 2)

#define GLKGR0_APB0		(1 << 28)
#define GLKGR0_RTC		(1 << 27)
#define GLKGR0_OST		(1 << 20)
#define GLKGR1_INTC		(1 << 26)


#define cpm_inl(off)		readl(CPM_IOBASE + off)
#define cpm_outl(val, off)	writel(val, CPM_IOBASE + off)



#endif

