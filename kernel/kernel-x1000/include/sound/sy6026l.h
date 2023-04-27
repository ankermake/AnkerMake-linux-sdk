/*
 * sy6026l.h  --  SY6026L i2c client data
 *
 * Copyright 2017 Ingenic Semiconductor Co.,Ltd
 *
 * Author: Cli <chen.li@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __SY6026_H__
#define __SY6026_H__

/**
 * struct sy6026l_platform_data - platform specific SY6026L configuration
 * @gpio_rst_b:	GPIO to reset SY6026L
 * @gpio_rst_b_level: RST_B pin active level
 * @gpio_fault_b: GPIO connected to SY6026L FAULT_B pin
 * @gpio_fault_b_level: FAULT_B pin active level
 *
 * Both GPIO parameters are optional. -1 is unused
 */
struct sy6026l_platform_data {
	int gpio_rst_b;
        int gpio_rst_b_level;
	int gpio_fault_b;
	int gpio_fault_b_level;
};
#endif  /*__I2C_SY6026_H__*/
