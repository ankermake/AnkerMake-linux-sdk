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
#include <linux/notifier.h>
#include <asm/cacheops.h>
#include <asm/r4kcache.h>
#include <asm/fpu.h>

#include <soc/cache.h>
#include <soc/tcsm_layout.h>
#include <soc/base.h>
#include <soc/cpm.h>


/*-----------------------------------------------------------------------------
 *  extern function declare
 *-----------------------------------------------------------------------------*/
extern long long save_goto(unsigned int);
extern int restore_goto(void);


/*-----------------------------------------------------------------------------
 *  Run in TCSM ??
 *-----------------------------------------------------------------------------*/
/*
 * CPU bootup entry in deep sleep mode
 *	cpu_resume_bootup --> cpu_resume
 *
 * after doing some preinitialize in cpu_resume,
 * jmp into kernel space and sys wakeup.
 * */
static noinline void cpu_resume_bootup(void)
{
	__asm__ volatile(
		".set push	\n\t"
		".set mips32r2	\n\t"
		"move $29, %0	\n\t"
		"jr.hb   %1	\n\t"
		"nop		\n\t"
		".set pop	\n\t"
		:
		:"r" (SLEEP_TCSM_CPU_RESMUE_SP), "r"(SLEEP_TCSM_RESUME_TEXT)
		:
		);
#if 0
	__asm__ volatile(".set mips32\n\t"
		"jr %0\n\t"
		"nop\n\t"
		".set mips32 \n\t"
		:: "r" (SLEEP_TCSM_RESUME_TEXT));
#endif

}

static noinline void cpu_resume(void)
{

	__asm__ volatile(
		".set push	\n\t"
		".set mips32r2	\n\t"
		"jr.hb %0	\n\t"
		"nop		\n\t"
		".set pop 	\n\t"
		:
		: "r" (restore_goto)
		:
		);
}





static void load_func_to_tcsm(unsigned int *tcsm_addr,unsigned int *f_addr,unsigned int size)
{
	unsigned int instr;
	int offset;
	int i;
	printk("tcsm addr = %p %p size = %d\n",tcsm_addr,f_addr,size);
	for(i = 0;i < size / 4;i++) {
		instr = f_addr[i];
		if((instr >> 26) == 2){
			offset = instr & 0x3ffffff;
			offset = (offset << 2) - ((unsigned int)f_addr & 0xfffffff);
			if(offset > 0) {
				offset = ((unsigned int)tcsm_addr & 0xfffffff) + offset;
				instr = (2 << 26) | (offset >> 2);
			}
		}
		tcsm_addr[i] = instr;
	}
}

static noinline void cpu_sleep(void)
{



	cache_prefetch(RUN_IN_CACHE, 200);
	blast_dcache32();
	__sync();
	__fast_iob();
RUN_IN_CACHE:
	/* exec in cache, make ddr into sleep mode */
	__asm__ volatile(
		".set push	\n\t"
		".set mips32r2	\n\t"
		"wait		\n\t"
		"nop		\n\t"
		"nop		\n\t"
		"nop		\n\t"
		".set pop	\n\t"
	);

}


struct sleep_save_register {
	unsigned int clkgr;
};

static struct sleep_save_register s_reg;

static int x2000_pm_enter(suspend_state_t state)
{
	unsigned int clkgr = 0;
	printk("x2000 pm enter!!\n");

	/* 1. Do Some CPM settings */

	s_reg.clkgr = clkgr = cpm_inl(CPM_CLKGR);
	clkgr &= ~(1 << 21);
	cpm_outl(clkgr, CPM_CLKGR);

	load_func_to_tcsm((unsigned int *)SLEEP_TCSM_BOOT_TEXT,(unsigned int *)cpu_resume_bootup,SLEEP_TCSM_BOOT_LEN);
	load_func_to_tcsm((unsigned int *)SLEEP_TCSM_RESUME_TEXT,(unsigned int *)cpu_resume,SLEEP_TCSM_RESUME_LEN);

	/* set sleep PC, use reset entry instead for fpga test */
//	*(volatile unsigned int *)0xbfbf0f00 = SLEEP_TCSM_BOOT_TEXT; /* RESET entry = SLEEP_TCSM_BOOT_TEXT */
	mb();
	save_goto((unsigned int)cpu_sleep);
	mb();
	/* set sleep PC, use reset entry instead for fpga test */
//	*(volatile unsigned int *)0xbfbf0f00 = 0xbfc00000; /* RESET entry = 0xbfc00000 ,reset value */

	cpm_outl(s_reg.clkgr, CPM_CLKGR);
	return 0;
}

static int x2000_pm_begin(suspend_state_t state)
{
	printk("x2000 suspend begin\n");
	return 0;
}

static void x2000_pm_end(void)
{
	printk("x2000 pm end!\n");
}

static const struct platform_suspend_ops x2000_pm_ops = {
	.valid		= suspend_valid_only_mem,
	.begin		= x2000_pm_begin,
	.enter		= x2000_pm_enter,
	.end		= x2000_pm_end,
};

/*
 * Initialize suspend interface
 */
static int __init pm_init(void)
{

	suspend_set_ops(&x2000_pm_ops);

	return 0;
	//return sysfs_create_group(power_kobj, &db1x_pmattr_group);
}

late_initcall(pm_init);
