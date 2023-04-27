#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/leds.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <mach/platform.h>
#include <mach/jzsnd.h>
#include <mach/jzmmc.h>
#include <mach/jzssi.h>
#include <mach/jz_efuse.h>
#include <gpio.h>
#include <linux/jz_dwc.h>
#include <linux/interrupt.h>
#include "board_base.h"

#ifdef CONFIG_LEDS_GPIO
#include <linux/leds.h>
#endif

#ifdef CONFIG_LEDS_GPIO
static struct gpio_led gpio_leds[] = {

};

static struct gpio_led_platform_data gpio_led_info = {
	.leds = gpio_leds,
	.num_leds = ARRAY_SIZE(gpio_leds),
};

struct platform_device jz_leds_gpio = {
	.name = "leds-gpio",
	.id = -1,
	.dev = {
		.platform_data  = &gpio_led_info,
	},
};
#endif

/* led pwm config:
	CONFIG_NEW_LEDS=y
	CONFIG_LEDS_CLASS=y
	CONFIG_LEDS_PWM=y
	CONFIG_PWM=y
	CONFIG_JZ_PWM=y
	CONFIG_JZ_PWM_BIT0=y
	CONFIG_JZ_PWM_BIT4=y
 */

#ifdef CONFIG_LEDS_PWM
#include <linux/pwm.h>
#include <linux/leds_pwm.h>

struct pwm_lookup jz_pwm_lookup[] = {
	PWM_LOOKUP("jz-pwm", 0, "leds_pwm", "led_red"),
	PWM_LOOKUP("jz-pwm", 4, "leds_pwm", "led_blue"),
};

static struct led_pwm leds_pwm[] = {
	{
		.name = "led_red",
		.default_trigger = "default_on",
		//.pwm_id     = 0, // __deprecated
		.active_low = true,
		.max_brightness = 255,
		.pwm_period_ns  = 30000,
	},
	{
		.name = "led_blue",
		.default_trigger = "default_on",
		//.pwm_id     = 4, // __deprecated
		.active_low = true,
		.max_brightness = 255,
		.pwm_period_ns  = 30000,
	},
};

static struct led_pwm_platform_data led_pwm_info = {
	.leds = leds_pwm,
	.num_leds = ARRAY_SIZE(leds_pwm),
};

struct platform_device jz_leds_pwm = {
	.name = "leds_pwm",
	.id = -1,
	.dev = {
		.platform_data  = &led_pwm_info,
	},
};

void pwm_table_init(void)
{
	pwm_add_table(jz_pwm_lookup, ARRAY_SIZE(jz_pwm_lookup));
}

#endif	/* CONFIG_LEDS_PWM */

#ifdef CONFIG_JZ_EFUSE_V13
struct jz_efuse_platform_data jz_efuse_pdata = {
	/* supply 2.5V to VDDQ */
	.gpio_vddq_en_n = GPIO_EFUSE_VDDQ,
};
#endif

#if defined(GPIO_USB_ID) && defined(GPIO_USB_ID_LEVEL)
struct jzdwc_pin dwc2_id_pin = {
	.num = GPIO_USB_ID,
	.enable_level = GPIO_USB_ID_LEVEL,
};
#endif


#if defined(GPIO_USB_DETE) && defined(GPIO_USB_DETE_LEVEL)
struct jzdwc_pin dwc2_dete_pin = {
	.num = GPIO_USB_DETE,
	.enable_level = GPIO_USB_DETE_LEVEL,
};
#endif

#if defined(GPIO_USB_DRVVBUS) && defined(GPIO_USB_DRVVBUS_LEVEL) && !defined(USB_DWC2_DRVVBUS_FUNCTION_PIN)
struct jzdwc_pin dwc2_drvvbus_pin = {
	.num = GPIO_USB_DRVVBUS,
	.enable_level = GPIO_USB_DRVVBUS_LEVEL,
};
#endif
