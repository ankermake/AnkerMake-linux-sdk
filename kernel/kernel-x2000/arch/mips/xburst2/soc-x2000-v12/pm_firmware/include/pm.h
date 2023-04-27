#ifndef __PM_H__
#define __PM_H__

#include <cpm.h>
#include <suspend.h>
#include <compiler-gcc.h>

#define SRAM_MEMORY_START	0xb2400000
#define SRAM_MEMORY_END		0xb2407ff8
#define RTC_MEMORY_START	0xb0004000
#define RTC_MEMORY_END          0xb0005000


#define FASTBOOT_RESUME_SP		(FASTBOOT_DATA_ADDR - 4)
#define FASTBOOT_RESUME_CODE1_ADDR	RTC_MEMORY_START
#define FASTBOOT_RESUME_CODE1_LEN	64
#define FASTBOOT_RESUME_CODE2_ADDR	(FASTBOOT_RESUME_CODE1_ADDR + FASTBOOT_RESUME_CODE1_LEN)
#define FASTBOOT_RESUME_CODE_LEN	0xb00
#define FASTBOOT_DATA_ADDR		0xb0004c00
#define FASTBOOT_DATA_LEN		1024



struct sleep_param {
	suspend_state_t state;

	unsigned int pdt;
	unsigned int dpd;
	unsigned int dlp;
	unsigned int autorefresh;
	unsigned int cpu_div;

	unsigned int uart_base;
	unsigned int uart_idx;
	unsigned int sleep_level;

	unsigned int reload_pc;
	unsigned int resume_pc;

	unsigned int load_addr;
	unsigned int load_space;
};

extern struct sleep_param sleep_param;

enum {
	IDLE,
	IDLE_PD,
	SLEEP,
	FASTBOOT,
};


void load_func_to_tcsm(unsigned int *tcsm_addr,unsigned int *f_addr,unsigned int size);



long long save_goto(unsigned int func);
int restore_goto(unsigned int func);


static inline void rtc_write_reg(unsigned int reg, unsigned int val)
{
	while (!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1) );
	*(volatile unsigned int *)0xb000303c = 0xa55a;
	while (!((*(volatile unsigned int *)0xb000303c >> 31) & 0x1) );
	while (!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1) );

	*(volatile unsigned int *)reg = val;

	while (!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1) );
}

static inline unsigned int  rtc_read_reg(unsigned int reg)
{
	while (!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1) );

	return *(volatile unsigned int *)reg;
}



#define reg_ddr_phy(x)   (*(volatile unsigned int *)(0xb3011000 + ((x) << 2)))




#endif
