#include <i2c_jz_gpio.h>
#include <board.h>
#include <linux/err.h>

static int __init i2c_gpio_device_init(void) {
	struct platform_device *ret;
#ifdef CONFIG_SOFT_I2C0_GPIO_V12_JZ
	ret = GPIO_I2C_BUS_REGISTER(0);
	if (IS_ERR(ret))
		printk("error: failed to register i2c gpio bus 0\n");
#endif

#ifdef CONFIG_SOFT_I2C1_GPIO_V12_JZ
	ret = GPIO_I2C_BUS_REGISTER(1);
	if (IS_ERR(ret))
		printk("error: failed to register i2c gpio bus 0\n");
#endif

#ifdef CONFIG_SOFT_I2C2_GPIO_V12_JZ
	ret = GPIO_I2C_BUS_REGISTER(2);
	if (IS_ERR(ret))
		printk("error: failed to register i2c gpio bus 0\n");
#endif
	return 0;
}
arch_initcall(i2c_gpio_device_init);
