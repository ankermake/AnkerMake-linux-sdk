#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/tsc.h>
#include <linux/leds.h>
#include <linux/jz4780-adc.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/android_pmem.h>
#include <mach/platform.h>
#include <mach/jzsnd.h>
#include <mach/jzmmc.h>
#include <mach/jzssi.h>
#include <mach/jz_efuse.h>
#include <soc/gpio.h>
#include <linux/jz_dwc.h>
#include <mach/jzeth_phy.h>
#include <linux/power/jz4780-battery.h>
#include <linux/i2c/ft6x06_ts.h>
#include <linux/interrupt.h>
#include <linux/platform_data/xburst_nand.h>
#include "board_base.h"

#ifdef CONFIG_JZ_EFUSE_V11
struct jz_efuse_platform_data jz_efuse_pdata = {
	    /* supply 2.5V to VDDQ */
	    .gpio_vddq_en_n = GPIO_EFUSE_VDDQ,
};
#endif

#if defined(CONFIG_JZ_MAC)
#ifndef CONFIG_MDIO_GPIO
static struct jz_ethphy_feature ethphy_feature = {
#ifdef CONFIG_JZGPIO_PHY_RESET
	.phy_hwreset = {
		.gpio = GMAC_PHY_PORT_GPIO,
		.active_level = GMAC_PHY_ACTIVE_HIGH,
		.crtl_port = GMAC_CRLT_PORT,
		.crtl_pins = GMAC_CRLT_PORT_PINS,
		.set_func = GMAC_CRTL_PORT_SET_FUNC,
		.delaytime_msec = GMAC_PHY_DELAYTIME,
	},
#endif
	.mdc_mincycle = 160, /* 160ns */
};

struct platform_device jz_mii_bus = {
        .name = "jz_mii_bus",
#ifdef CONFIG_JZGPIO_PHY_RESET
	.dev.platform_data = &ethphy_feature,
#endif
};
#else
static struct mdio_gpio_platform_data mdio_gpio_data = {
        .mdc = GPF(13),
        .mdio = GPF(14),
        .phy_mask = 0,
        .irqs = { 0 },
};
struct platform_device jz_mii_bus = {
        .name = "mdio-gpio",
        .dev.platform_data = &mdio_gpio_data,
};
#endif

struct platform_device jz_mac_device = {
        .name = "jz_mac",
        .dev.platform_data = &jz_mii_bus,
};
#endif

#ifdef CONFIG_ANDROID_PMEM
/* arch/mips/kernel/setup.c */
extern unsigned long set_reserved_pmem_total_size(unsigned long size);
void board_pmem_setup(void)
{
	/* reserve memory for pmem. */
	unsigned long pmem_total_size=0;
#if defined(JZ_PMEM_ADSP_SIZE) && (JZ_PMEM_ADSP_SIZE>0)
	pmem_total_size += JZ_PMEM_ADSP_SIZE;
#endif
#if defined(JZ_PMEM_CAMERA_SIZE) && (JZ_PMEM_CAMERA_SIZE>0)
	pmem_total_size += JZ_PMEM_CAMERA_SIZE;
#endif
	set_reserved_pmem_total_size(pmem_total_size);
}

static struct android_pmem_platform_data pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.no_allocator = 0,
	.cached = 1,
	.start = JZ_PMEM_ADSP_BASE,
	.size = JZ_PMEM_ADSP_SIZE,
};

static struct platform_device pmem_adsp_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &pmem_adsp_pdata },
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


#if (defined(CONFIG_USB_JZ_DWC2) || defined(CONFIG_USB_DWC_OTG)) && defined(GPIO_USB_DETE)
struct jzdwc_pin dwc2_dete_pin = {
	.num                            = GPIO_USB_DETE,
	.enable_level                   = HIGH_ENABLE,
};
struct jzdwc_pin dwc2_id_pin = {
	.num          = GPIO_USB_ID,
};
#endif

#if defined(GPIO_USB_DRVVBUS) && defined(GPIO_USB_DRVVBUS_LEVEL) && !defined(USB_DWC2_DRVVBUS_FUNCTION_PIN)
struct jzdwc_pin  dwc2_drvvbus_pin = {
	.num = GPIO_USB_DRVVBUS,
	.enable_level = GPIO_USB_DRVVBUS_LEVEL,
};
#endif
