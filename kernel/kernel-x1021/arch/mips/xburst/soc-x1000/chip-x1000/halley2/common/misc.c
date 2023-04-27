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
#ifdef CONFIG_JZ_FACTORY_TEST
struct platform_device factory_test_device={ \
	        .name = "factory-test",                  \
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


#if defined(CONFIG_USB_ANDROID_HID) || defined(CONFIG_USB_G_HID)
#include <linux/platform_device.h>
#include <linux/usb/g_hid.h>

/* hid descriptor for a keyboard */
static struct hidg_func_descriptor hid_data = {
	.subclass		= 0, /* No subclass */
	.protocol		= 1, /* Keyboard */
	.report_length		= 8,
	.report_desc_length	= 63,
	.report_desc		= {
		0x05, 0x01,	/* USAGE_PAGE (Generic Desktop)	          */
		0x09, 0x06,	/* USAGE (Keyboard)                       */
		0xa1, 0x01,	/* COLLECTION (Application)               */
		0x05, 0x07,	/*   USAGE_PAGE (Keyboard)                */
		0x19, 0xe0,	/*   USAGE_MINIMUM (Keyboard LeftControl) */
		0x29, 0xe7,	/*   USAGE_MAXIMUM (Keyboard Right GUI)   */
		0x15, 0x00,	/*   LOGICAL_MINIMUM (0)                  */
		0x25, 0x01,	/*   LOGICAL_MAXIMUM (1)                  */
		0x75, 0x01,	/*   REPORT_SIZE (1)                      */
		0x95, 0x08,	/*   REPORT_COUNT (8)                     */
		0x81, 0x02,	/*   INPUT (Data,Var,Abs)                 */
		0x95, 0x01,	/*   REPORT_COUNT (1)                     */
		0x75, 0x08,	/*   REPORT_SIZE (8)                      */
		0x81, 0x03,	/*   INPUT (Cnst,Var,Abs)                 */
		0x95, 0x05,	/*   REPORT_COUNT (5)                     */
		0x75, 0x01,	/*   REPORT_SIZE (1)                      */
		0x05, 0x08,	/*   USAGE_PAGE (LEDs)                    */
		0x19, 0x01,	/*   USAGE_MINIMUM (Num Lock)             */
		0x29, 0x05,	/*   USAGE_MAXIMUM (Kana)                 */
		0x91, 0x02,	/*   OUTPUT (Data,Var,Abs)                */
		0x95, 0x01,	/*   REPORT_COUNT (1)                     */
		0x75, 0x03,	/*   REPORT_SIZE (3)                      */
		0x91, 0x03,	/*   OUTPUT (Cnst,Var,Abs)                */
		0x95, 0x06,	/*   REPORT_COUNT (6)                     */
		0x75, 0x08,	/*   REPORT_SIZE (8)                      */
		0x15, 0x00,	/*   LOGICAL_MINIMUM (0)                  */
		0x25, 0x65,	/*   LOGICAL_MAXIMUM (101)                */
		0x05, 0x07,	/*   USAGE_PAGE (Keyboard)                */
		0x19, 0x00,	/*   USAGE_MINIMUM (Reserved)             */
		0x29, 0x65,	/*   USAGE_MAXIMUM (Keyboard Application) */
		0x81, 0x00,	/*   INPUT (Data,Ary,Abs)                 */
		0xc0		/* END_COLLECTION                         */
	}
};

struct platform_device jz_hidg = {
	.name			= "hidg",
	.id			= -1,
	.num_resources		= 0,
	.resource		= 0,
	.dev.platform_data	= &hid_data,
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
