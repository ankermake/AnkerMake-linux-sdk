#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/leds.h>
//#include <linux/tsc.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
//#include <linux/android_pmem.h>
#include <mach/platform.h>
#include <mach/jzsnd.h>
#include <mach/jzmmc.h>
#include <mach/jzssi.h>
#include <mach/jz_efuse.h>
#include <gpio.h>
#include <linux/jz_dwc.h>
#include <linux/interrupt.h>
//#include <sound/jz-aic.h>
#include "board_base.h"

#ifdef CONFIG_JZ_MAC
#ifndef CONFIG_MDIO_GPIO
#ifdef CONFIG_JZGPIO_PHY_RESET
static struct jz_gpio_phy_reset gpio_phy_reset = {
	.gpio = GMAC_PHY_PORT_GPIO,
	.active_level = GMAC_PHY_ACTIVE_HIGH,
	.crtl_port = GMAC_CRLT_PORT,
	.crtl_pins = GMAC_CRLT_PORT_PINS,
	.set_func = GMAC_CRTL_PORT_SET_FUNC,
	.delaytime_msec = GMAC_PHY_DELAYTIME,
};
#endif
struct platform_device jz_mii_bus = {
	.name = "jz_mii_bus",
#ifdef CONFIG_JZGPIO_PHY_RESET
	.dev.platform_data = &gpio_phy_reset,
#endif
};
#else /* CONFIG_MDIO_GPIO */
static struct mdio_gpio_platform_data mdio_gpio_data = {
	.mdc = MDIO_MDIO_MDC_GPIO,
	.mdio = MDIO_MDIO_GPIO,
	.phy_mask = 0,
	.irqs = { 0 },
};

struct platform_device jz_mii_bus = {
	.name = "mdio-gpio",
	.dev.platform_data = &mdio_gpio_data,
};
#endif /* CONFIG_MDIO_GPIO */
struct platform_device jz_mac_device = {
	.name = "jz_mac",
	.dev.platform_data = &jz_mii_bus,
};
#endif /* CONFIG_JZ_MAC */


#ifdef CONFIG_JZ_EFUSE_V13
struct jz_efuse_platform_data jz_efuse_pdata = {
	    /* supply 2.5V to VDDQ */
	    .gpio_vddq_en_n = GPIO_EFUSE_VDDQ,
};
#endif


#ifdef CONFIG_LEDS_GPIO
struct gpio_led jz_leds[] = {
	[0]={
		.name = "wl_led_r",
		.gpio = WL_LED_R,
		.active_low = 0,
	},
	[1]={
		.name = "wl_led_g",
		.gpio = WL_LED_G,
		.active_low = 0,
	},

	[2]={
		.name = "wl_led_b",
		.gpio = WL_LED_B,
		.active_low = 0,
	},

};

struct gpio_led_platform_data  jz_led_pdata = {
	.num_leds = 3,
	.leds = jz_leds,
};

struct platform_device jz_led_rgb = {
	.name       = "leds-gpio",
	.id     = -1,
	.dev        = {
		.platform_data  = &jz_led_pdata,
	}
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

#ifdef CONFIG_LEDS_PWM
#include <linux/pwm.h>
#include <linux/leds_pwm.h>

/* led pwm config:
	CONFIG_NEW_LEDS=y
	CONFIG_LEDS_CLASS=y
	CONFIG_LEDS_PWM=y
	CONFIG_PWM=y
	CONFIG_JZ_PWM=y
	CONFIG_JZ_PWM_BIT0=y
	CONFIG_JZ_PWM_BIT4=y
 */

struct pwm_lookup fir_pwm_lookup[] = {
	PWM_LOOKUP("jz-pwm", 0, "leds_pwm", "led0"),
	PWM_LOOKUP("jz-pwm", 1, "leds_pwm", "led1"),
	PWM_LOOKUP("jz-pwm", 2, "leds_pwm", "led2"),
	PWM_LOOKUP("jz-pwm", 3, "leds_pwm", "led3"),
	PWM_LOOKUP("jz-pwm", 4, "leds_pwm", "led4"),
};

static struct led_pwm leds_pwm[] = {
	{
		.name = "led0",
		.default_trigger = "default_on",
		//.pwm_id     = 0, // __deprecated
		.active_low = true,
		.max_brightness = 255,
		.pwm_period_ns  = 30000,
	},
	{
		.name = "led1",
		.default_trigger = "default_on",
		//.pwm_id     = 0, // __deprecated
		.active_low = true,
		.max_brightness = 255,
		.pwm_period_ns  = 30000,
	},
	{
		.name = "led2",
		.default_trigger = "default_on",
		//.pwm_id     = 0, // __deprecated
		.active_low = true,
		.max_brightness = 255,
		.pwm_period_ns  = 30000,
	},
	{
		.name = "led3",
		.default_trigger = "default_on",
		//.pwm_id     = 0, // __deprecated
		.active_low = true,
		.max_brightness = 255,
		.pwm_period_ns  = 30000,
	},
	{
		.name = "led4",
		.default_trigger = "default_on",
		//.pwm_id     = 0, // __deprecated
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
	pwm_add_table(fir_pwm_lookup, ARRAY_SIZE(fir_pwm_lookup));
}

#endif	/* CONFIG_LEDS_PWM */
