#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <soc/cpm.h>
#include <soc/base.h>

#define RTC_RTCCR    0x00
#define RTC_RTCSR    0x04
#define RTC_RTCSAR    0x08
#define RTC_RTCGR    0x0C

#define RTC_HCR        0x20
#define RTC_HWFCR    0x24
#define RTC_HRCR    0x28
#define RTC_HWCR    0x2C
#define RTC_HWRSR    0x30
#define RTC_HSPR    0x34
#define RTC_WENR    0x3C
#define RTC_CKPCR    0x40
#define RTC_PWRONCR    0x48

/* RTC Control Register */
#define RTC_RTCCR_WRDY        (1 << 7)
#define RTC_RTCCR_1HZ        (1 << 6)
#define RTC_RTCCR_1HZIE        (1 << 5)
#define RTC_RTCCR_AF        (1 << 4)
#define RTC_RTCCR_AIE        (1 << 3)
#define RTC_RTCCR_AE        (1 << 2)
#define RTC_RTCCR_SELEXC    (1 << 1)
#define RTC_RTCCR_RTCE        (1 << 0)

/* Write Enable Pattern Register */
#define RTC_WENR_WEN        (1 << 31)
#define RTC_WENR_WENPAT_BIT    0
#define RTC_WENR_WENPAT_MASK    (0xffff << RTC_WENR_WENPAT_BIT)

/* HIBERNATE Wakeup Status Register */
#define RTC_HWRSR_APD        (1 << 8)
#define RTC_HWRSR_HR        (1 << 5)
#define RTC_HWRSR_PPR        (1 << 4)
#define RTC_HWRSR_PIN        (1 << 1)
#define RTC_HWRSR_ALM        (1 << 0)

#define RTC_HCR_PD    1

#define rtc_inl(off)            inl(RTC_IOBASE + (off))
#define rtc_outl(val,off)       outl(val, RTC_IOBASE + (off))

#ifndef CONFIG_RTC_DRV_JZ

static inline void wait_write_ready(void) {
    int timeout = 0x100000;

    while (!(rtc_inl(RTC_RTCCR) & RTC_RTCCR_WRDY) && timeout--);
    if (timeout <= 0)
        printk(KERN_ERR "RTC wait_write_ready timeout!\n");
}

int rtc_clk_src_to_ext(void)
{
    unsigned int opcr, clkgr, rtccr;
    /*
     * Set OPCR.ERCS of CPM to 1
     */
    opcr = cpm_inl(CPM_OPCR);
    opcr |= OPCR_ERCS;
    cpm_outl(opcr, CPM_OPCR);

    /*
     * Set CLKGR.RTC of CPM to 0
     */
    clkgr = cpm_inl(CPM_CLKGR0);
    clkgr &= ~CLKGR_RTC;
    cpm_outl(clkgr, CPM_CLKGR0);

    /*
     * Set RTCCR.SELEXC to 1
     */
    wait_write_ready();
    rtccr = rtc_inl(RTC_RTCCR);
    rtccr |= RTC_RTCCR_SELEXC;
    rtc_outl(rtccr, RTC_RTCCR);

    /*
     * Wait two clock period of clock
     */
    udelay(10);

    opcr &= ~OPCR_ERCS;
    cpm_outl(opcr, CPM_OPCR);

    udelay(10);

    /*
     * Check RTCCR.SELEXC == 1
     */
    rtccr = rtc_inl(RTC_RTCCR);
    return !(rtccr & RTC_RTCCR_SELEXC);
}

static int __init jz_rtc_clk_ext_init(void)
{
    while(rtc_clk_src_to_ext())
        printk(KERN_ERR "RTC clock sel failed, try again !!!\n");

    rtc_outl(0xa55a, RTC_WENR);

    return 0;
}

subsys_initcall(jz_rtc_clk_ext_init);

#endif
