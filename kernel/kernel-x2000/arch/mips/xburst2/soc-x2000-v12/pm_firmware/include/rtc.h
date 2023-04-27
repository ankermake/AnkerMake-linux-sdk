/*
 * JZ RTC register definition
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *
 */
#ifndef __RTC_H__
#define __RTC_H__

/*
 * RTC registers offset address definition
 */
#define RTC_RTCCR		(0x00)	/* rw, 32, 0x00000081 */
#define RTC_RTCSR		(0x04)	/* rw, 32, 0x???????? */
#define RTC_RTCSAR		(0x08)	/* rw, 32, 0x???????? */
#define RTC_RTCGR		(0x0c)	/* rw, 32, 0x0??????? */

#define RTC_HCR			(0x20)  /* rw, 32, 0x00000000 */
#define RTC_HWFCR		(0x24)  /* rw, 32, 0x0000???0 */
#define RTC_HRCR		(0x28)  /* rw, 32, 0x00000??0 */
#define RTC_HWCR		(0x2c)  /* rw, 32, 0x00000008 */
#define RTC_HWRSR		(0x30)  /* rw, 32, 0x00000000 */
#define RTC_HSPR		(0x34)  /* rw, 32, 0x???????? */
#define RTC_WENR		(0x3c)  /* rw, 32, 0x00000000 */
#define RTC_WKUPPINCR		(0x48)	/* rw, 32, 0x00af0064 */
/*
 * RTC registers common define
 */

/* RTC control register(RTCCR) */
#define RTCCR_WRDY		(1 << 7)
#define RTCCR_1HZ		(1 << 6)
#define RTCCR_1HZIE		(1 << 5)
#define RTCCR_AF		(1 << 4)
#define RTCCR_AIE		(1 << 3)
#define RTCCR_AE		(1 << 2)
#define RTCCR_SELEXC		(1 << 1)
#define RTCCR_RTCE		(1 << 0)


/* Generate the bit field mask from msb to lsb */
#define BITS_H2L(msb, lsb)  ((0xFFFFFFFF >> (32-((msb)-(lsb)+1))) << (lsb))

/* RTC regulator register(RTCGR) */
#define RTCGR_LOCK		(1 << 31)

#define RTCGR_ADJC_LSB		16
#define RTCGR_ADJC_MASK		BITS_H2L(25, RTCGR_ADJC_LSB)

#define RTCGR_NC1HZ_LSB		0
#define RTCGR_NC1HZ_MASK	BITS_H2L(15, RTCGR_NC1HZ_LSB)

/* Hibernate control register(HCR) */
#define HCR_PD			(1 << 0)

/* Hibernate wakeup filter counter register(HWFCR) */
#define HWFCR_LSB		5
#define HWFCR_MASK		BITS_H2L(15, HWFCR_LSB)
#define HWFCR_WAIT_TIME(ms)	(((ms) << HWFCR_LSB) > HWFCR_MASK ? HWFCR_MASK : ((ms) << HWFCR_LSB))

/* Hibernate reset counter register(HRCR) */
#define HRCR_LSB		11
#define HRCR_MASK		BITS_H2L(14, HRCR_LSB)
#define HRCR_WAIT_TIME(ms)     (ms < 62 ? 0 : (((ms / 62 - 1) << HRCR_LSB) > HRCR_MASK ? HRCR_MASK : ((ms / 62 - 1) << HRCR_LSB)))


/* Hibernate wakeup control register(HWCR) */

/* Power detect default value; this value means enable */
#define EPDET_DEFAULT           (0x5aa5a5a)
#define EPDET_ENABLE		(0x5aa5a5a)
#define EPDET_DISABLE		(0x1a55a5a5)
#define HWCR_EPDET		(1 << 3)
#define HWCR_WKUPVL		(1 << 2)
#define HWCR_EALM		(1 << 0)


/* Hibernate wakeup status register(HWRSR) */
#define HWRSR_APD		(1 << 8)
#define HWRSR_HR		(1 << 5)
#define HWRSR_PPR		(1 << 4)
#define HWRSR_PIN		(1 << 1)
#define HWRSR_ALM		(1 << 0)

/* write enable pattern register(WENR) */
#define WENR_WEN		(1 << 31)

#define WENR_WENPAT_LSB		0
#define WENR_WENPAT_MASK	BITS_H2L(15, WENR_WENPAT_LSB)
#define WENR_WENPAT_WRITABLE	(0xa55a)

/* Hibernate scratch pattern register(HSPR) */
#define HSPR_RTCV               (0x52544356)      /* The value is 'RTCV', means rtc is valid */

/*WKUP_PIN_RST control register (WKUPPINCR)*/
#define WKUPPINCR_DEFAULT	(0x00af0064)

#define WKUPPINCR_P_RST_EN_DEF	(0x4)
#define WKUPPINCR_P_RST_EN	(0xb)
#define WKUPPINCR_P_RST_CLEAR	(0xf)
#define WKUPPINCR_OSC_EN	(1 << 16)
#define WKUPPINCR_FASTBOOT	(1 << 20)
#define WKUPPINCR_BUFFEREN	(1 << 21)
#define WKUPPINCR_RAMSLEEP	(1 << 22)
#define WKUPPINCR_RAMSHUT	(1 << 23)


#if 1
#define rtc_readl(reg) ({ \
	while (!(readl(RTC_IOBASE + RTC_RTCCR)  & RTCCR_WRDY) ); \
	readl(RTC_IOBASE + reg);\
	})

#define rtc_writel(val, reg) do{ \
	while (!(readl(RTC_IOBASE + RTC_RTCCR)  & RTCCR_WRDY) ); \
	writel(0xa55a, RTC_IOBASE + RTC_WENR); \
	while (!(readl(RTC_IOBASE + RTC_WENR) & WENR_WEN) ); \
	while (!(readl(RTC_IOBASE + RTC_RTCCR)  & RTCCR_WRDY) ); \
	writel(val, RTC_IOBASE + reg); \
	while (!(readl(RTC_IOBASE + RTC_RTCCR)  & RTCCR_WRDY) ); \
}while(0)
#else
#define rtc_readl(reg) ({ \
		        while (!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1) ); \
		        *(volatile unsigned int *)(0xb0003000 + reg);\
		})

#define rtc_writel(val, reg) do{ \
	        while (!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1) ); \
	        *(volatile unsigned int *)0xb000303c = 0xa55a; \
	        while (!((*(volatile unsigned int *)0xb000303c >> 31) & 0x1) ); \
	        while (!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1) ); \
	        *(volatile unsigned int *)(0xb0003000 + reg) = val; \
	        while (!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1) ); \
}while(0)
#endif



#endif /* __RTC_H__ */
