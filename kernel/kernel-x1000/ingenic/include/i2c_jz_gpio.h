#ifndef __I2C_JZ_GPIO_H__
#define __I2C_JZ_GPIO_H__
#include <linux/platform_device.h>
#include <linux/i2c-gpio.h>

#define GPIO_I2C_BUS_REGISTER(NO)	\
({	\
	int ret;	\
	static struct i2c_gpio_platform_data __initdata i2c##NO##_gpio_data = {	\
		.sda_pin	= GPIO_I2C##NO##_SDA,			\
		.scl_pin	= GPIO_I2C##NO##_SCK,			\
	};	\
	ret = platform_device_register_data(NULL, "i2c-gpio", (NO),		\
			&i2c##NO##_gpio_data, sizeof(struct i2c_gpio_platform_data));	\
	ret;	\
})
#endif /*__I2C_JZ_GPIO_H__*/
