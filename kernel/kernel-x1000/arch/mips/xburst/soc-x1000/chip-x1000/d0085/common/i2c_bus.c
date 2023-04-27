#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/interrupt.h>
#include "board_base.h"
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/jzsnd.h>
#include "board_base.h"


#if defined(CONFIG_AKM4951_EXTERNAL_CODEC) || defined(CONFIG_SND_ASOC_INGENIC_EXTCODEC_AK4951)
static struct snd_board_gpio power_down = {
	.gpio = GPIO_AK4951_PDN,
	.active_level = LOW_ENABLE,
};

static struct extcodec_platform_data ak4951_data = {
	.power = &power_down,
};
#endif

#if (defined(CONFIG_SOFT_I2C0_GPIO_V12_JZ) || defined(CONFIG_I2C0_V12_JZ))
struct i2c_board_info jz_i2c0_devs[] __initdata = {
#if defined(CONFIG_AKM4951_EXTERNAL_CODEC) || defined(CONFIG_SND_ASOC_INGENIC_EXTCODEC_AK4951)
	{
		I2C_BOARD_INFO("ak4951", 0x12),
		.platform_data  = &ak4951_data,
	},
#endif
};
int jz_i2c0_devs_size = ARRAY_SIZE(jz_i2c0_devs);
#endif

#if (defined(CONFIG_SOFT_I2C1_GPIO_V12_JZ) || defined(CONFIG_I2C1_V12_JZ))
struct i2c_board_info jz_i2c1_devs[] __initdata = {
};
int jz_i2c1_devs_size = ARRAY_SIZE(jz_i2c1_devs);
#endif
#if (defined(CONFIG_SOFT_I2C2_GPIO_V12_JZ) || defined(CONFIG_I2C2_V12_JZ))
struct i2c_board_info jz_i2c2_devs[] __initdata = {
};
int jz_i2c2_devs_size = ARRAY_SIZE(jz_i2c2_devs);
#endif

#ifdef CONFIG_I2C_GPIO
#define DEF_GPIO_I2C(NO)											\
    static struct i2c_gpio_platform_data i2c##NO##_gpio_data = {    \
        .sda_pin    = GPIO_I2C##NO##_SDA,							\
        .scl_pin    = GPIO_I2C##NO##_SCK,							\
	};																\
    struct platform_device i2c##NO##_gpio_device = {				\
        .name   = "i2c-gpio",										\
        .id = NO,													\
        .dev    = { .platform_data = &i2c##NO##_gpio_data,},		\
    };

#ifdef CONFIG_SOFT_I2C2_GPIO_V12_JZ
DEF_GPIO_I2C(2);
#endif
#ifdef CONFIG_SOFT_I2C1_GPIO_V12_JZ
DEF_GPIO_I2C(1);
#endif
#ifdef CONFIG_SOFT_I2C0_GPIO_V12_JZ
DEF_GPIO_I2C(0);
#endif
#endif /*CONFIG_I2C_GPIO*/
