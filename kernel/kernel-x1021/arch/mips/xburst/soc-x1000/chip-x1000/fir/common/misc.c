#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/leds.h>
//#include <linux/tsc.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/pwm.h>
#include <linux/leds_pwm.h>
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
#include <mach/jz_pwm_dev.h>

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

#ifdef CONFIG_LEDS_PWM
struct pwm_lookup fir_pwm_lookup[] = {
	PWM_LOOKUP("jz-pwm", 2, "leds_pwm", "yellow:zigbee"),
	PWM_LOOKUP("jz-pwm", 3, "leds_pwm", "blue:bt"),
	PWM_LOOKUP("jz-pwm", 4, "leds_pwm", "red:wifi"),
};

static struct led_pwm fir_pwm_leds[] = {
	{
		.name       = "yellow:zigbee",
		.max_brightness = 255,
		.pwm_period_ns  = 7812500,
	},
	{
		.name       = "blue:bt",
		.max_brightness = 255,
		.pwm_period_ns  = 7812500,
	},
	{
		.name       = "red:wifi",
		.max_brightness = 255,
		.pwm_period_ns  = 7812500,
	},
};

static struct led_pwm_platform_data fir_pwm_data = {
	.num_leds   = ARRAY_SIZE(fir_pwm_leds),
	.leds       = fir_pwm_leds,
};

struct platform_device fir_leds_pwm = {
	.name   = "leds_pwm",
	.id = -1,
	.dev    = {
		.platform_data = &fir_pwm_data,
	},
};

void pwm_table_init(void)
{
	pwm_add_table(fir_pwm_lookup, ARRAY_SIZE(fir_pwm_lookup));
}
#endif

#ifdef CONFIG_JZ_PWM_GENERIC
struct jz_pwm_dev wireless_leds[3] = {
	[0]={
		.name = "led_wifi",
		.pwm_id = 4,
		.active_low = 0,
		.max_duty_ratio = 255,
		.period_ns = 1000000000,
	},
	[1]={
		.name = "led_bt",
		.pwm_id = 3,
		.active_low = 0,
		.max_duty_ratio = 255,
		.period_ns = 1000000000,
	},
	[2]={
		.name = "led_zigbee",
		.pwm_id = 2,
		.active_low = 0,
		.max_duty_ratio = 255,
		.period_ns = 1000000000,
	},
};

struct jz_pwm_dev_platform_data pwm_leds = {
	.num_devs = ARRAY_SIZE(wireless_leds),
	.devs = wireless_leds,
};

struct platform_device jz_pwm_devs_platform_device = {
	.name           = "jz_pwm_dev",
	.id             = -1,
	.num_resources  = 0,
	.dev            = {
		.platform_data  = &pwm_leds,
	}
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

