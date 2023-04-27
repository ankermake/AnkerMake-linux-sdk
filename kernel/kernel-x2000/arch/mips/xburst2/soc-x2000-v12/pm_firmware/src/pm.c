#include <common.h>
#include <barrier.h>
#include <uart.h>
#include <interface.h>
#include <printf.h>
#include "pm.h"
#include "pm_sleep.h"
#include "pm_fastboot.h"
#if defined CONFIG_KP_AXP
#include <linux/regulator/machine.h>
#endif


#define USE_MCU

#ifndef USE_MCU
#define firmware_uart_send(uart_idx, baud)
#endif

extern int pm_valid(int  v);
extern int pm_begin(int  v);
extern int pm_prepare(int d);
extern int pm_enter(int d);
extern int pm_finish(int d);
extern int pm_end(int d);
extern int pm_poweroff(int d);


struct pm_param p_slp_param ={
	.valid		= pm_valid,
	.begin		= pm_begin,
	.prepare	= pm_prepare,
	.enter		= pm_enter,
	.finish		= pm_finish,
	.end		= pm_end,
	.poweroff	= pm_poweroff,
};





#define SLEEP_LEVEL		(*(volatile unsigned int *)0xb0000034 & 0xf)
#define SLEEP_LEVEL_MAGIC	((*(volatile unsigned int *)0xb0000034 >> 16) & 0xffff)
#define SLEEP_MAGIC		(0x0915)



struct sleep_param sleep_param;



#ifdef DEBUG_PM
static void x2000_pm_gate_check(void)
{
	unsigned int gate0 = cpm_inl(CPM_CLKGR);
	unsigned int gate1 = cpm_inl(CPM_CLKGR1);
	int i;
	int x;

	printf("gate0 = 0x%x\n", gate0);
	printf("gate1 = 0x%x\n", gate1);
	for (i = 0; i < 32; i++) {
		x = (gate0 >> i) & 1;
		if (x == 0)
			printf("warning : bit[%d] in clk gate0 is enabled\n", i);
	}
	for (i = 0; i < 32; i++) {
		x = (gate1 >> i) & 1;
		if (x == 0)
			printf("warning : bit[%d] in clk gate1 is enabled\n", i);
	}

}
#endif



static int soc_pm_idle(void)
{
	printf("soc pm idle \n");

	soc_pm_idle_config();

	return 0;
}

static int soc_pm_idle_pd(void)
{
	printf("soc pm idle pd \n");

	soc_pm_idle_pd_config();

	soc_set_reset_entry();

	return 0;
}

static void soc_pm_sleep(void)
{
	printf("soc pm sleep \n");

	soc_pm_sleep_config();

	soc_set_reset_entry();
}


static void soc_pm_fastboot(void)
{
	printf("soc pm fastboot \n");

	soc_pm_fastboot_config();

	load_func_to_rtc_ram();

	sys_save();
}


static void goto_sleep(unsigned int sleep_addr)
{
	mb();
	save_goto(sleep_addr);
	mb();
}


int enter(int argv)
{
	unsigned int sleep_addr = 0;
	unsigned int sleep_level = 0;
	struct pm_param *p = (struct pm_param*)argv;


	suspend_state_t state = p->state;

	sleep_param.state = state;
	sleep_param.uart_idx = p->uart_index;
	sleep_param.uart_base = (UART0_IOBASE | 0xa0000000) + UART_OFF * p->uart_index;
	sleep_param.reload_pc = p->reload_pc;
	sleep_param.load_addr = p->load_addr;
	sleep_param.load_space = p->load_space;

	pm_serial_init();

	printf("x2000 pm enter!!\n");

	printf("sleep_param.uart_idx : 0x%x\n", sleep_param.uart_idx);
	printf("sleep_param.uart_base : 0x%x\n", sleep_param.uart_base);
	printf("sleep_param.reload_pc : 0x%x\n", sleep_param.reload_pc);
	printf("sleep_param.load_addr : 0x%x\n", sleep_param.load_addr);
	printf("sleep_param.load_space : 0x%x\n", sleep_param.load_space);


#ifdef USE_MCU
	unsigned int mcu_uart = p->mcu_uart_idx;
	unsigned int mcu_baud = p->mcu_uart_baud;

	printf("mcu_uart_idx : %d\n", mcu_uart);
	printf("mcu_uart_baud : %d\n", mcu_baud);

	firmware_uart_init(mcu_uart);
	firmware_uart_set_baud(mcu_uart, mcu_baud);

#endif


	if (SLEEP_LEVEL_MAGIC == SLEEP_MAGIC) {
		if ((state == PM_SUSPEND_STANDBY) && (SLEEP_LEVEL == IDLE)) {
			firmware_uart_send(mcu_uart, "idle\n");
			soc_pm_idle();
			sleep_addr = (unsigned int)cpu_sleep;
		} else if ((state == PM_SUSPEND_STANDBY) && (SLEEP_LEVEL == IDLE_PD)) {
			firmware_uart_send(mcu_uart, "idle_pd\n");
			soc_pm_idle_pd();
			sleep_addr = (unsigned int)cpu_sleep;
		} else if ((state == PM_SUSPEND_MEM) && (SLEEP_LEVEL == SLEEP)) {
			firmware_uart_send(mcu_uart, "sleep\n");
			soc_pm_sleep();
			sleep_addr = (unsigned int)cpu_sleep;
		} else if ((state == PM_SUSPEND_MEM) && (SLEEP_LEVEL == FASTBOOT)) {
			firmware_uart_send(mcu_uart, "fastboot\n");
			soc_pm_fastboot();
			sleep_addr = (unsigned int)fastboot_cpu_sleep;
			sleep_level = FASTBOOT;
		} else {
			printf("error suspend state or suspend level ! \n");
			return -1;
		}
	} else {
		if (state == PM_SUSPEND_STANDBY) {
			firmware_uart_send(mcu_uart, "idle\n");
			soc_pm_idle();
			sleep_addr = (unsigned int)cpu_sleep;
		} else if (state == PM_SUSPEND_MEM) {
			firmware_uart_send(mcu_uart, "sleep\n");
			soc_pm_sleep();
			sleep_addr = (unsigned int)cpu_sleep;
		} else {
			printf("error suspend state ! \n");
			return -1;
		}
	}


#ifdef DEBUG_PM
	printf("LCR: 0x%x\n", cpm_inl(CPM_LCR));
	printf("OPCR: 0x%x\n", cpm_inl(CPM_OPCR));
	x2000_pm_gate_check();
#endif

	goto_sleep(sleep_addr);


	if (sleep_level == FASTBOOT) {
		soc_pm_wakeup_fastboot();
	} else {
		soc_pm_wakeup_idle_sleep();
	}

	return 0;
}

int valid(int argv)
{
	struct pm_param *p = (struct pm_param*)argv;
	suspend_state_t state = p->state;

	switch (state) {
		case PM_SUSPEND_ON:
		case PM_SUSPEND_STANDBY:
		case PM_SUSPEND_MEM:
			return 1;

		default:
			return 0;
	}
}

int begin(int argv)
{
	return 0;
}
int prepare(int argv)
{
	return 0;
}
int finish(int argv)
{
	printf("x2000 pm finish!\n");
}

int end(int argv)
{
	printf("x2000 pm end!\n");
}

int poweroff(int argv)
{
#ifdef USE_MCU
	struct pm_param *p = (struct pm_param*)argv;

	unsigned int mcu_uart = p->mcu_uart_idx;
	unsigned int mcu_baud = p->mcu_uart_baud;

	firmware_uart_init(mcu_uart);
	firmware_uart_set_baud(mcu_uart, mcu_baud);

	firmware_uart_send(mcu_uart, "poweroff\n");

#endif

	return 0;
}

#if defined CONFIG_KP_AXP
static int suspend_prepare(void)
{
	return regulator_suspend_prepare(PM_SUSPEND_MEM);
}
static void suspend_finish(void)
{
	if (regulator_suspend_finish())
		pr_err("%s: Suspend finish failed\n", __func__);
}
#endif







