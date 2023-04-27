#ifndef _LINUX_TCU_H
#define _LINUX_TCU_H

#include <linux/interrupt.h>
#define NR_TCU_CH	(4)
#define NR_PWM_CH	NR_TCU_CH

enum tcu_mode {
	TCU1_MODE = 1,
	TCU2_MODE = 2,
};

enum tcu_clk_mode {
	PCLK_EN = 0, /*timer input clock is PCLK*/
	RTC_EN = 1,	/*timer input clock is RTC*/
	EXT_EN = 2,	/*timer input clock is EXT*/
	CLK_MASK = 3,
};

enum {
	TCU_FFLAG0 = IRQ_TCU_BASE,
	TCU_FFLAG1,
	TCU_FFLAG2,
	TCU_FFLAG3,
	TCU_FFLAG4,
	TCU_FFLAG5,
	TCU_FFLAG6,
	TCU_FFLAG7,
	TCU_FIFOFLAG0,
	TCU_FIFOFLAG1,
	TCU_FIFOFLAG2,
	TCU_FIFOFLAG3,
	TCU_OSTFLAG = 15 + IRQ_TCU_BASE,
	TCU_HFLAG0,
	TCU_HFLAG1,
	TCU_HFLAG2,
	TCU_HFLAG3,
	TCU_HFLAG4,
	TCU_HFLAG5,
	TCU_HFLAG6,
	TCU_HFLAG7,
	TCU_FIFO_EMPTY_FLAG0,
	TCU_FIFO_EMPTY_FLAG1,
	TCU_FIFO_EMPTY_FLAG2,
	TCU_FIFO_EMPTY_FLAG3,
	TCU_FIFO_EMPTY_FLAG4,
	TCU_FIFO_EMPTY_FLAG5,
};

struct tcu_device {
	int id;
	struct mutex    tcu_lock;
	short pwm_flag;
	int io_func;
	struct jz_pwm_gpio *gpio_def;

	enum tcu_clk_mode clock;
	enum tcu_mode tcumode;
	int half_num;
	int full_num;
	int count_value;
	unsigned int init_level; /*used in pwm output mode*/
	unsigned int divi_ratio;  /*  0/1/2/3/4/5/something else------>1/4/16/64/256/1024/mask  */
	unsigned int pwm_shutdown; /*0-->graceful shutdown   1-->abrupt shutdown only use in TCU1_MODE*/
	unsigned int rtc_bypass_mode; /* 1: bypass rtcclk to pwm */
};

struct jz_pwm_gpio {
	char *name;
	int id;
	int gpio;
	int func;
};

extern struct jz_pwm_gpio pwm_gpio_array[];
extern int pwm_gpio_array_size;

struct tcu_device *tcu_request(int channel_num);
void tcu_free(struct tcu_device *tcu);
int tcu_as_timer_init(struct tcu_device* tcu);
int tcu_as_timer_config(struct tcu_device *tcu);
int tcu_as_counter_config(struct tcu_device *tcu);
int tcu_enable(struct tcu_device *tcu);
void tcu_disable(struct tcu_device *tcu);
#endif
