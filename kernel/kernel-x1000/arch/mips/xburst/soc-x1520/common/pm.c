/*
 * linux/arch/mips/xburst/soc-x1520/common/pm.c
 *
 *  x1520 Power Management Routines
 *  Copyright (C) 2006 - 2012 Ingenic Semiconductor Inc.
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */

#include <linux/init.h>
#include <linux/pm.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/sysctl.h>
#include <linux/delay.h>
#include <linux/syscore_ops.h>
#include <linux/clk.h>
#include <asm/fpu.h>
#include <linux/notifier.h>
#include <asm/cacheops.h>
#include <soc/cache.h>
#include <asm/r4kcache.h>
#include <soc/base.h>
#include <soc/cpm.h>
#include <soc/gpio.h>
#include <soc/ddr.h>
#include <gpio.h>
#include <board.h>

int gpio_show(void);
struct sleep_save_register
{
    unsigned int lcr;
    unsigned int opcr;
    unsigned int ddr_ctrl;
    unsigned int ddr_autosr;
    unsigned int ddr_dlp;
    suspend_state_t pm_state;
    unsigned int ddr_training_space[20];
};
static struct sleep_save_register s_reg;

#ifndef REG32
#define REG32(val) (*(volatile unsigned int *)(val))
#endif

#define TCSM_DELAY(x)                    \
    do{                        \
        register unsigned int i= x;        \
    while(i--)                    \
        __asm__ volatile(".set push\n\t"   \
                 ".set mips32\n\t"    \
                 "nop\n\t"        \
                 ".set pop\n\t"); \
    }while(0)

#if 0
#define OFF_TDR         (0x00)
#define OFF_LCR         (0x0C)
#define OFF_LSR         (0x14)

#define LSR_TDRQ        (1 << 5)
#define LSR_TEMT        (1 << 6)

static __attribute__ ((always_inline)) void  inline serial_putc(char x)
{
#define U_ADDR (0x10031000 + 0xa0000000)
    REG32(U_ADDR + OFF_TDR) = x;
    while ((REG32(U_ADDR + OFF_LSR) & (LSR_TDRQ | LSR_TEMT)) != (LSR_TDRQ | LSR_TEMT));
}
static __attribute__( (always_inline) ) void inline serial_put_hex(unsigned int x)
{
    register int i;
    register unsigned int d;
    for(i = 7;i >= 0;i--) {
        d = (x  >> (i * 4)) & 0xf;
        if(d < 10) d += '0';
        else d += 'A' - 10;
        serial_putc(d);
    }
}
#endif

static void noinline cpu_sleep(void)
{
    register unsigned int val;
    blast_icache32();
    blast_dcache32();
    blast_scache32();
    cache_prefetch(SLEEP_FUNCTION,(2*1024));


SLEEP_FUNCTION:
    val = ddr_readl(DDRC_CTRL);
    val &= ~(0x1f << 11);
    val |= (1 << 17) | (1 << 5);
    ddr_writel(val, DDRC_CTRL); //enter selrefresh.
    while(!(ddr_readl(DDRC_STATUS) & (1 << 2))); // wait finish.
#ifdef DOWNVOLTAGE
#define C_CPCCR (0xb0000000)
#define C_CPCSR (0xb00000d4)

    val = REG32(C_CPCCR);

    REG32(C_CPCCR) =
        (REG32(C_CPCCR) &
        (~((0x3 << 30) | (0x3 << 28) | (0x3 << 26) | (0x3 << 24)))) |
        ((0x1 << 30) | (0x1 << 28) | (0x1 << 26) | (0x1 << 24));
    while((REG32(C_CPCSR) & (0xf << 28)) != (0xf << 28));

    REG32(C_CPCCR) = REG32(C_CPCCR) & (~0xff);
    while(REG32(C_CPCSR) & 0x1);

    REG32(C_CPCCR) = REG32(C_CPCCR) & (~(0xfff << 8));
    while(REG32(C_CPCSR) & (0x3 << 1));

    //serial_putc('\n');
    //serial_put_hex(REG32(C_CPCCR));
    //serial_putc('\n');

    /*gpio set*/
    REG32(0xb0010000+ 0x100*(DOWNVOLTAGE/32) + 0x48) = 0x1 << (DOWNVOLTAGE%32);
#endif

    __asm__ volatile(".set push\n\t"
                     ".set mips32\n\t"
                     "nop\n\t"
                     "wait\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     ".set mips32 \n\t"
                     ".set pop\n\t");
#ifdef DOWNVOLTAGE
    /*gpio set*/
    REG32(0xb0010000 + 0x100*(DOWNVOLTAGE/32)+0x44) = 0x1 << (DOWNVOLTAGE%32);

    REG32(C_CPCCR) =
        (REG32(C_CPCCR) & (~0xfffff)) | (val & 0xfffff) | (0x7 << 20);
    while((REG32(C_CPCSR)) & 0x7);

    REG32(C_CPCCR) = val;
    while((REG32(C_CPCSR) & (0xf << 28)) != (0xf << 28));
#endif
    if(!(ddr_readl(DDRP_PIR) & DDRP_PIR_DLLBYP)) {
        /**
         * reset dll of ddr.
         * WARNING: 2015-01-08
         *     DDR CLK GATE(CPM_DRCG 0xB00000D0), BIT6 must set to 1 (or 0x40).
         *     If clear BIT6, chip memory will not stable, gpu hang occur.
         */
        /* { */
        /*     val = ddr_readl(DDRP_DSGCR); */
        /*     val &= ~(1 << 4); */
        /*     ddr_writel(val,DDRP_DSGCR); */
        /* } */
#define CPM_DRCG (0xB00000D0)

        *(volatile unsigned int *)CPM_DRCG |= (1<<1);
        TCSM_DELAY(0x1ff);
        *(volatile unsigned int *)CPM_DRCG &= ~(1<<1);
        TCSM_DELAY(0x1ff);
        /*
         * for disabled ddr enter power down.
         */
        *(volatile unsigned int *)0xb301102c &= ~(1 << 4);
        TCSM_DELAY(0xf);

        /*
         * reset dll of ddr too.
         */
        *(volatile unsigned int *)CPM_DRCG |= (1<<1);
        TCSM_DELAY(0x1ff);
        *(volatile unsigned int *)CPM_DRCG &= ~(1<<1);
        TCSM_DELAY(0x1ff);

        val = DDRP_PIR_INIT | DDRP_PIR_ITMSRST  | DDRP_PIR_DLLSRST | DDRP_PIR_DLLLOCK;// | DDRP_PIR_ZCAL  ;
        ddr_writel(val, DDRP_PIR);
        val = DDRP_PGSR_IDONE | DDRP_PGSR_DLDONE | DDRP_PGSR_DIDONE;// | DDRP_PGSR_ZCDONE;
        while ((ddr_readl(DDRP_PGSR) & val) != val) {
            if(ddr_readl(DDRP_PGSR) & (DDRP_PGSR_DTERR | DDRP_PGSR_DTIERR)) {
                break;
            }
        }
    }

    val = ddr_readl(DDRC_CTRL);
    val &= ~((1 << 5) | (1 << 17));
    ddr_writel(val,DDRC_CTRL); //exit selrefresh.
    while(ddr_readl(DDRC_STATUS) & (1 << 2)); // wait finish.

    if(!(ddr_readl(DDRP_PIR) & DDRP_PIR_DLLBYP)){
        val = DDRP_PIR_INIT | DDRP_PIR_QSTRN;
    }else
        val = DDRP_PIR_INIT | DDRP_PIR_QSTRN | DDRP_PIR_DLLBYP;
    ddr_writel(val, DDRP_PIR);
    val = (DDRP_PGSR_IDONE | DDRP_PGSR_DLDONE | DDRP_PGSR_DIDONE | DDRP_PGSR_DTDONE);
    while ((ddr_readl(DDRP_PGSR) & val) != val) {
        if(ddr_readl(DDRP_PGSR) & (DDRP_PGSR_DTERR | DDRP_PGSR_DTIERR)) {
            break;
        }
    }
    if(!(ddr_readl(DDRP_PIR) & DDRP_PIR_DLLBYP))
    {
        *(volatile unsigned int *)0xb301102c |= (1 << 4);
        TCSM_DELAY(0xf);
    }

}

static int soc_pm_enter(suspend_state_t state)
{
    unsigned int lcr,opcr;
    unsigned int bypassmode;
   //boot 按键唤醒
#if 0
    //gpio interrupt
    // int
    REG32(0xb0010200 + 0x14) = 0x1 << 1;
    //mask
    REG32(0xb0010200 + 0x28) = 0x1 << 1;
    //pad1
    REG32(0xb0010200 + 0x34) = 0x1 << 1;
    //pad0
    REG32(0xb0010200 + 0x44) = 0x1 << 1;
    //flag
    REG32(0Xb0010200 + 0x58) = 0xffffffff;
#endif
    s_reg.opcr = cpm_inl(CPM_OPCR);
    s_reg.lcr = cpm_inl(CPM_LCR);
    opcr = s_reg.opcr;
    lcr = s_reg.lcr;
    opcr &= ~((1 << 24) | (0xfff << 8) | (1 << 4) | (1 << 2));
    opcr |= (1 << 30) | (1 << 24) | (0x15 << 8);

#ifndef CONFIG_RTC_DRV_JZ
    /*
     * cpm clk select to extclk / 512
     */
    opcr |= (1 << 4);
    opcr &= ~(1 << 2);
#else
    /*
     * cpm clk select rtc clk
     */
    opcr &= ~(1 << 4);
    opcr |= (1 << 2);
#endif

    lcr &= ~3;
    if(s_reg.pm_state == PM_SUSPEND_STANDBY) {
        opcr &= ~((1 << 31) | (1 << 30));
    } else {
        lcr |= LCR_LPM_SLEEP;
    }
    cpm_outl(opcr,CPM_OPCR);
    cpm_outl(lcr,CPM_LCR);

    ddr_writel(0, DDRP_DTAR);
    memcpy(&s_reg.ddr_training_space,(void *)0x80000000,sizeof(s_reg.ddr_training_space));

    s_reg.ddr_dlp = ddr_readl(DDRC_DLP);
    s_reg.ddr_ctrl = ddr_readl(DDRC_CTRL);
    s_reg.ddr_autosr = ddr_readl(DDRC_AUTOSR_EN);
    ddr_writel(0,DDRC_AUTOSR_EN); // exit auto sel-refresh

    bypassmode = ddr_readl(DDRP_PIR) & DDRP_PIR_DLLBYP;
    if(!bypassmode)
    {
        ddr_writel(0xf003 , DDRC_DLP);
        /* val = ddr_readl(DDRP_DSGCR); */
        /* val |= (1 << 4); */
        /* ddr_writel(val,DDRP_DSGCR); */
    }

    printk("#####cpccr:%08x\n", cpm_inl(CPM_CPCCR));
    printk("#####cpapcr:%08x\n", cpm_inl(CPM_CPAPCR));
    printk("#####cpmpcr:%08x\n", cpm_inl(CPM_CPMPCR));
    printk("#####lcr:%08x\n", cpm_inl(CPM_LCR));
    printk("#####gate:%08x\n", cpm_inl(CPM_CLKGR));
    printk("#####opcr:%08x\n", cpm_inl(CPM_OPCR));
    printk("#####INT_MASK0:%08x\n", *(volatile unsigned int*)(0xB0001004));
    printk("#####INT_MASK1:%08x\n", *(volatile unsigned int*)(0xB0001024));

    cpu_sleep();

    if(!s_reg.ddr_dlp && !bypassmode)
    {
        ddr_writel(0x0 , DDRC_DLP);
    }
    if(s_reg.ddr_autosr) {
        ddr_writel(1,DDRC_AUTOSR_EN);   // enter auto sel-refresh
    }
    ddr_writel(s_reg.ddr_ctrl, DDRC_CTRL);
    memcpy((void*)0x80000000,&s_reg.ddr_training_space,sizeof(s_reg.ddr_training_space));
    dma_cache_wback_inv(0x80000000,sizeof(s_reg.ddr_training_space));
    cpm_outl(s_reg.lcr,CPM_LCR);
    cpm_outl(s_reg.opcr,CPM_OPCR);

    return 0;
}

static void soc_finish(void)
{
    s_reg.pm_state = 0;
}
static int soc_valid(suspend_state_t state)
{
    s_reg.pm_state = PM_SUSPEND_MEM;
    return 1;
}

/*
 * Initialize power interface
 */
struct platform_suspend_ops pm_ops = {
    .valid = soc_valid,
    .enter = soc_pm_enter,
    .end = soc_finish,
};

int __init soc_pm_init(void)
{
    volatile unsigned int lcr,opcr;
    suspend_set_ops(&pm_ops);

    /* init opcr and lcr for idle */
    lcr = cpm_inl(CPM_LCR);
    lcr &= ~(0x3);        /* LCR.SLEEP.DS=1'b0,LCR.LPM=2'b00*/
    cpm_outl(lcr,CPM_LCR);

    opcr = cpm_inl(CPM_OPCR);
    opcr |= 0xff << 8;    /* EXCLK stable time */
    opcr &= ~(1 << 4);    /* EXCLK oscillator is disabled in Sleep mode */
    cpm_outl(opcr,CPM_OPCR);

    /* sysfs */
    return 0;
}

arch_initcall(soc_pm_init);
