
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/interrupt.h>
#include "board_base.h"
#include <mach/jz_efuse.h>
#include <soc/base.h>
#include <soc/irq.h>


#ifdef CONFIG_JZ_MAC
#ifndef CONFIG_MDIO_GPIO
#ifdef CONFIG_JZGPIO_PHY_RESET
static struct jz_gpio_phy_reset gpio_phy_reset = {
	.port = GMAC_PHY_PORT_GPIO / 32,
	.pin = GMAC_PHY_PORT_GPIO % 32,
	.start_func = GMAC_PHY_PORT_START_FUNC,
	.end_func = GMAC_PHY_PORT_END_FUNC,
	.delaytime_usec = GMAC_PHY_DELAYTIME,
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

#ifdef CONFIG_JZ_EFUSE_V14
struct jz_efuse_platform_data jz_efuse_pdata = {
	/* supply 2.5V to VDDQ */
	.gpio_vddq_en_n = -1,
};
#endif

#ifdef CONFIG_CHARGER_LI_ION
/* li-ion charger */
static struct li_ion_charger_platform_data jz_li_ion_charger_pdata = {
	.gpio_charge = GPIO_LI_ION_CHARGE,
	.gpio_ac = GPIO_LI_ION_AC,
	.gpio_active_low = GPIO_ACTIVE_LOW,
};

static struct platform_device jz_li_ion_charger_device = {
	.name = "li-ion-charger",
	.dev = {
		.platform_data = &jz_li_ion_charger_pdata,
	},
};
#endif

#ifdef CONFIG_JZ_BATTERY
static struct jz_battery_info  dorado_battery_info = {
	.max_vol        = 4050,
	.min_vol        = 3600,
	.usb_max_vol    = 4100,
	.usb_min_vol    = 3760,
	.ac_max_vol     = 4100,
	.ac_min_vol     = 3760,
	.battery_max_cpt = 3000,
	.ac_chg_current = 800,
	.usb_chg_current = 400,
};
struct jz_adc_platform_data adc_platform_data = {
	battery_info = dorado_battery_info;
}
#endif

#if defined(CONFIG_USB_JZ_DWC2) || defined(CONFIG_USB_DWC_OTG)
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
#endif /*CONFIG_USB_JZ_DWC2 || CONFIG_USB_DWC_OTG*/

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
	.id			= 0,
	.num_resources		= 0,
	.resource		= 0,
	.dev.platform_data	= &hid_data,
};
#endif

#ifdef CONFIG_PWM_SDK
struct pwm_lookup jz_pwm_lookup[] = {
	PWM_LOOKUP("jz-pwm", 2, "pwm-sdk", "pwm-sdk.2"),
	PWM_LOOKUP("jz-pwm", 3, "pwm-sdk", "pwm-sdk.3"),
};
int jz_pwm_lookup_size = ARRAY_SIZE(jz_pwm_lookup);
#endif


static struct resource jz_isp_resources[] =
{
    [0] = {
            /*.name = "isp_device",*/
		.start = ISP_VIC_IOBASE,
		.end = ISP_VIC_IOBASE + 0x21000 - 1,
		.flags = IORESOURCE_MEM,
    },
    [1] = {
            /*.name = "isp_irq_id",*/
            .start = IRQ_VIC,
            .end = IRQ_VIC,
            .flags = IORESOURCE_IRQ,
    },

};

struct platform_device jz_isp_device =
{
    .name = "tx-isp",
    .resource = jz_isp_resources,
    .num_resources = ARRAY_SIZE(jz_isp_resources),
};
