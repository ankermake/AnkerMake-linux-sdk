/*
 * JZSOC GPIO port, usually used in arch code.
 *
 * Copyright (C) 2010 Ingenic Semiconductor Co., Ltd.
 */

#ifndef __JZSOC_JZ_GPIO_H__
#define __JZSOC_JZ_GPIO_H__

enum gpio_function {
	GPIO_FUNC_0	 = 0x10,  //0000, GPIO as function 0 / device 0
	GPIO_FUNC_1	 = 0x11,  //0001, GPIO as function 1 / device 1
	GPIO_FUNC_2	 = 0x12,  //0010, GPIO as function 2 / device 2
	GPIO_FUNC_3	 = 0x13,  //0011, GPIO as function 3 / device 3
	GPIO_OUTPUT0 = 0x14,  //0100, GPIO output low  level
	GPIO_OUTPUT1 = 0x15,  //0101, GPIO output high level
	GPIO_INPUT	 = 0x16,  //0110, GPIO as input
	GPIO_INT_LO	 = 0x18,  //1000, Low  Level trigger interrupt
	GPIO_INT_HI	 = 0x19,  //1001, High Level trigger interrupt
	GPIO_INT_FE	 = 0x1a,  //1010, Fall Edge trigger interrupt
	GPIO_INT_RE	 = 0x1b,  //1011, Rise Edge trigger interrupt
	GPIO_INT_MASK_LO   = 0x1c,  //1100, Port is low level triggered interrupt input. Interrupt is masked.
	GPIO_INT_MASK_HI   = 0x1d,  //1101, Port is high level triggered interrupt input. Interrupt is masked.
	GPIO_INT_MASK_FE   = 0x1e,  //1110, Port is fall edge triggered interrupt input. Interrupt is masked.
	GPIO_INT_MASK_RE   = 0x1f,  //1111, Port is rise edge triggered interrupt input. Interrupt is masked.
	GPIO_PULL_FUNC = 0x80,
	GPIO_PULL_HIZ  = 0x80,  // no pull 
	GPIO_PULL_ENABLE = 0xa0,  // enable pull
	GPIO_INPUT_PULL = GPIO_INPUT | GPIO_PULL_ENABLE, // gpio input and pull
};
#define GPIO_AS_FUNC(func)  (((func) & 0x10) && (! ((func) & 0xc)))

enum gpio_port {
	GPIO_PORT_A = 0, GPIO_PORT_B,
	GPIO_PORT_C, GPIO_PORT_D,
	GPIO_PORT_E, GPIO_PORT_F,
	/* this must be last */
	GPIO_NR_PORTS,
};

struct jz_gpio_func_def {
	char *name;
	int port;
	int func;
	unsigned long pins;
};

#ifndef GPIO_PG
#define GPIO_PG(n)      (5*32 + 23 + n)
#endif

/* PHY hard reset */
struct jz_gpio_phy_reset {
	enum gpio_port		port;
	unsigned short		pin;
	enum gpio_function	start_func;
	enum gpio_function	end_func;
	unsigned int		delaytime_usec;
};

/*
 * must define this array in board special file.
 * define the gpio pins in this array, use GPIO_DEF_END
 * as end of array. it will inited in function
 * setup_gpio_pins()
 */

extern struct jz_gpio_func_def platform_devio_array[];
extern int platform_devio_array_size;

struct gpio_reg_func {
	unsigned int save[5];
};

/*
 * This functions are used in special driver which need
 * operate the device IO pin.
 */
int jzgpio_set_func(enum gpio_port port,
		    enum gpio_function func,unsigned long pins);

int jz_gpio_set_func(int gpio, enum gpio_function func);

int jzgpio_ctrl_pull(enum gpio_port port, int enable_pull,
		     unsigned long pins);

int jz_gpio_set_glitch_filter(int gpio, unsigned char period);

int jz_gpio_get_glitch_filter(int gpio);

int jz_gpio_save_reset_func(enum gpio_port port, enum gpio_function dst_func,
			    unsigned long pins, struct gpio_reg_func *rfunc);
int jz_gpio_restore_func(enum gpio_port port,
			 unsigned long pins, struct gpio_reg_func *rfunc);
int mcu_gpio_register(unsigned int reg);
int jzgpio_phy_reset(struct jz_gpio_phy_reset *gpio_phy_reset);

#endif
