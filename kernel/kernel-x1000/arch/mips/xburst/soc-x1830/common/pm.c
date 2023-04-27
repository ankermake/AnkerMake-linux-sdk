/*
 * linux/arch/mips/xburst/soc-m200/common/pm_p0.c
 *
 *  M200 Power Management Routines
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
	register unsigned int ddr_ctrl;
#ifdef DOWNVOLTAGE
	register unsigned int val;
#endif
	blast_dcache32();

	cache_prefetch(SLEEP_FUNCTION,(12*1024));
	asm volatile(
		".set push \t\n"
		".set noreorder \t\n"
		);
SLEEP_FUNCTION:
    //enter selfrefresh.
	ddr_ctrl = ddr_ahb_readl(DDRC_CTRL);
	ddr_ctrl &=  ~(1 << 11);
	ddr_ahb_writel(ddr_ctrl, DDRC_CTRL); //disable ddr auto power down.
	ddr_ctrl |= (1 << 17) | (1 << 5);
    //ddr_ctrl |=  (1 << 5);
	ddr_ahb_writel(ddr_ctrl, DDRC_CTRL); //enter selrefresh.
	while(!(ddr_ahb_readl(DDRC_STATUS) & (1 << 2))); // wait finish.

	ddr_apb_writel(1,DDRC_APB_PHY_INIT); // dfi_init_start to high.

	ddr_phy_writel(0x1a, INNO_PLL_CTRL); // power down ddr pll.
	ddr_phy_writel(0x1a, INNO_PLL_CTRL); //ensure write finish.
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
    /*gpio set*/
    REG32(0xb0010000+ 0x1000*(DOWNVOLTAGE/32) + 0x48) = 0x1 << (DOWNVOLTAGE%32);
#endif
    asm volatile (
    	"nop \n\t"
    		"wait \n\t"
    	"nop \n\t"
    	".set pop"
    	);
#ifdef DOWNVOLTAGE
    REG32(0xb0010000 + 0x1000*(DOWNVOLTAGE/32)+0x44) = 0x1 << (DOWNVOLTAGE%32);
    REG32(C_CPCCR) =
        (REG32(C_CPCCR) & (~0xfffff)) | (val & 0xfffff) | (0x7 << 20);
    while((REG32(C_CPCSR)) & 0x7);
    REG32(C_CPCCR) = val;
    while((REG32(C_CPCSR) & (0xf << 28)) != (0xf << 28));
#endif
	ddr_phy_writel(0x18, INNO_PLL_CTRL); // power up ddr pll.

	while(!(ddr_phy_readl(INNO_PLL_LOCK) & (1 << 3))); // wait locked.

	ddr_apb_writel(0,DDRC_APB_PHY_INIT); // dfi_init_start to low.
	while(!(ddr_apb_readl(DDRC_APB_PHY_INIT) & 2)); // wait finish.

	ddr_ctrl &= ~((1 << 5) | (1 << 17));
    //ddr_ctrl &= ~(1 << 5);
	ddr_ahb_writel(ddr_ctrl,DDRC_CTRL); //exit selrefresh.
	while(ddr_ahb_readl(DDRC_STATUS) & (1 << 2)); // wait finish.

	ddr_ctrl |= (1 << 11);
	ddr_ahb_writel(ddr_ctrl,DDRC_CTRL); //enable ddr auto power down.

#if 0

	phy_writel(DDR_MR1_VALUE & 0xff, INNO_WL_MODE1);
	phy_writel(0x40, INNO_WL_MODE2);


	/* phy_writel(0xa4, INNO_TRAINING_CTRL); */
	/* while (0x3 != phy_readl(INNO_WL_DONE)) */
	/* 	serial_putc('e'); */

	phy_writel(0xa1,INNO_TRAINING_CTRL);

	while (0x3 != phy_readl(INNO_CALIB_DONE))
		serial_putc('y');
	phy_writel(0xa0,INNO_TRAINING_CTRL);
	phy_writel(0xa0,INNO_TRAINING_CTRL);
#endif

}

static int soc_pm_enter(suspend_state_t state)
{
	unsigned int lcr,opcr;
#if 0
    //boot interrupt
    // int
    REG32(0xb0012000 + 0x14) = 0x1 << 1;
    //mask
    REG32(0xb0012000 + 0x28) = 0x1 << 1;
    //pad1
    REG32(0xb0012000 + 0x34) = 0x1 << 1;
    //pad0
    REG32(0xb0012000 + 0x44) = 0x1 << 1;
    //flag
    REG32(0xb0012000 + 0x58) = 0xffffffff;
#endif


	s_reg.opcr = cpm_inl(CPM_OPCR);
	s_reg.lcr = cpm_inl(CPM_LCR);

	/*
	 *   set OPCR.
	 */

	opcr = s_reg.opcr;
	lcr = s_reg.lcr;

    /*
     * Disable otg, uhc, extoscillator
     */
    opcr &= ~((0xfff << 8) | (1 << 7) | (1 << 6) | (1 << 4) | (1 << 22));

    /*
     * extoscillator state wait time,
     * mask interrupt,
     * L2 power down
     * Gate usb phy clk
     * Core power down
     */
    opcr |= (0xf << 8) | (1 << 30) | (1 << 23);

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
	printk("#####cpccr:%08x\n", cpm_inl(CPM_CPCCR));

	printk("#####lcr:%08x\n", cpm_inl(CPM_LCR));
	printk("#####gate:%08x\n", cpm_inl(CPM_CLKGR));
	printk("#####opcr:%08x\n", cpm_inl(CPM_OPCR));
	printk("#####INT_MASK0:%08x\n", *(volatile unsigned int*)(0xB0001004));
	printk("#####INT_MASK1:%08x\n", *(volatile unsigned int*)(0xB0001024));

	/* cpm_outl(0x5fff3ffe,CPM_CLKGR); */

	/* cpm_outl(0x12ff,CPM_CLKGR1); */

	cpu_sleep();
	cpm_outl(s_reg.lcr,CPM_LCR);
	cpm_outl(s_reg.opcr,CPM_OPCR);

	printk("-----lcr:%08x\n", cpm_inl(CPM_LCR));
	printk("-----gate:%08x\n", cpm_inl(CPM_CLKGR));
	printk("-----opcr:%08x\n", cpm_inl(CPM_OPCR));
	printk("-----INT_MASK0:%08x\n", *(volatile unsigned int*)(0xB0001004));
	printk("-----INT_MASK1:%08x\n", *(volatile unsigned int*)(0xB0001024));

	return 0;
}

static void soc_finish(void)
{
	s_reg.pm_state = 0;
}
static int soc_valid(suspend_state_t state)
{
	s_reg.pm_state = state;
	return s_reg.pm_state;
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
	lcr &= ~(0x3);		/* LCR.SLEEP.DS=1'b0,LCR.LPM=2'b00*/
	lcr |= 0x15 << 8;	/* power stable time */
	cpm_outl(lcr,CPM_LCR);

	opcr = cpm_inl(CPM_OPCR);
	opcr |= 0xff << 8;	/* EXCLK stable time */
	opcr &= ~(1 << 4);	/* EXCLK oscillator is disabled in Sleep mode */
	cpm_outl(opcr,CPM_OPCR);

	/* sysfs */
	return 0;
}

arch_initcall(soc_pm_init);
