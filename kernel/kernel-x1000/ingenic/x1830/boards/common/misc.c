
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/interrupt.h>
#include "board_base.h"
#include <mach/jz_efuse.h>
#include <mach/jzssi.h>
#include <linux/jz_dwc.h>

/*
 * EFUSE
 */
#ifdef CONFIG_JZ_EFUSE
struct jz_efuse_platform_data jz_efuse_pdata = {
    /* supply 2.5V to VDDQ */
    .gpio_vddq_en_n                     = GPIO_EFUSE_VDDQ,
};
#endif /* end of EFUSE */

/*
 * DWC2
 */
#if defined(CONFIG_USB_JZ_DWC2) || defined(CONFIG_USB_DWC_OTG)

#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY

#if defined(GPIO_USB_ID) && defined(GPIO_USB_ID_LEVEL)
struct jzdwc_pin dwc2_id_pin = {
    .num                                = GPIO_USB_ID,
    .enable_level                       = GPIO_USB_ID_LEVEL,
};
#endif

#if defined(GPIO_USB_DETE) && defined(GPIO_USB_DETE_LEVEL)
struct jzdwc_pin dwc2_dete_pin = {
    .num                                = GPIO_USB_DETE,
    .enable_level                       = GPIO_USB_DETE_LEVEL,
};
#endif

#endif /* end of CONFIG_BOARD_HAS_NO_DETE_FACILITY */


#if defined(GPIO_USB_DRVVBUS) && defined(GPIO_USB_DRVVBUS_LEVEL) && !defined(USB_DWC2_DRVVBUS_FUNCTION_PIN)
struct jzdwc_pin dwc2_drvvbus_pin = {
    .num                                = GPIO_USB_DRVVBUS,
    .enable_level                       = GPIO_USB_DRVVBUS_LEVEL,
};
#endif


#if defined(CONFIG_SND_ASOC_JZ_ICDC_D4)
struct snd_codec_pdata snd_codec_pdata = {
#ifdef MIC_DEF_VOL_REGVAL
    .mic_dftvol                         = MIC_DEF_VOL_REGVAL,
#else
    .mic_dftvol                         = -1,
#endif
#ifdef HP_DEF_VOL_REGVAL
    .hp_dftvol                          = HP_DEF_VOL_REGVAL,
#else
    .hp_dftvol                          = -1,
#endif
#ifdef MIC_MAX_VOL_REGVAL
    .mic_maxvol                         = MIC_MAX_VOL_REGVAL,
#else
    .mic_maxvol                         = -1,
#endif
#ifdef HP_MAX_VOL_REGVAL
    .hp_maxvol                          = HP_MAX_VOL_REGVAL,
#else
    .hp_maxvol                          = -1,
#endif
};
#endif /* CONFIG_SND_ASOC_JZ_ICDC_D4 */

#endif /* CONFIG_USB_JZ_DWC2 || CONFIG_USB_DWC_OTG */


/*
 * JZ-MAC
 */
#ifdef CONFIG_JZ_MAC

#ifndef CONFIG_MDIO_GPIO

/* !defined(CONFIG_MDIO_GPIO) */
#ifdef CONFIG_JZGPIO_PHY_RESET
static struct jz_gpio_phy_reset gpio_phy_reset = {
    .port                               = GMAC_PHY_PORT_GPIO / 32,
    .pin                                = GMAC_PHY_PORT_GPIO % 32,
    .start_func                         = GMAC_PHY_PORT_START_FUNC,
    .end_func                           = GMAC_PHY_PORT_END_FUNC,
    .delaytime_usec                     = GMAC_PHY_DELAYTIME,
};
#endif /* end of CONFIG_JZGPIO_PHY_RESET */

struct platform_device jz_mii_bus = {
    .name   = "jz_mii_bus",
#ifdef CONFIG_JZGPIO_PHY_RESET
    .dev.platform_data                  = &gpio_phy_reset,
#endif
};

#else /* CONFIG_MDIO_GPIO */

/* defined(CONFIG_MDIO_GPIO) */
static struct mdio_gpio_platform_data mdio_gpio_data = {
    .mdc                                = MDIO_MDIO_MDC_GPIO,
    .mdio                               = MDIO_MDIO_GPIO,
    .phy_mask                           = 0,
    .irqs = { 0 },
};

struct platform_device jz_mii_bus = {
    .name   = "mdio-gpio",
    .dev.platform_data                  = &mdio_gpio_data,
};
#endif /* end of CONFIG_MDIO_GPIO */



struct platform_device jz_mac_device = {
    .name   = "jz_mac",
    .dev.platform_data                  = &jz_mii_bus,
};
#endif /* end of CONFIG_JZ_MAC */


#if defined(CONFIG_USB_ANDROID_HID)
#include <linux/platform_device.h>
#include <linux/usb/g_hid.h>
/* hid descriptor for a sykean  */
static struct hidg_func_descriptor hid_data = {
       .subclass               = 0, /* No subclass */
       .protocol               = 0, /* none */
       .report_length          = 64,
       .report_desc_length     = 34,
       .report_desc            = {
               0x06, 0x00, 0xff,       /* USAGE_PAGE (Vendor Defined paage 1)       */
               0x09, 0x01,                     /* USAGE (Vendor Usage)                      */
               0xa1, 0x01,                     /* COLLECTION (Application)                  */
               0x15, 0x00,                     /* LOGICAL_MINIMUM(0)                        */
               0x26, 0xff, 0x00,       /* LOGICAL_MAXIMUM(0XFF)                     */
               0x95, 0x40,                     /* REPORT_COIUNT(64)                         */
               0x75, 0x08,                     /* REPORT_COUNT (8)                          */
               0x09, 0x01,                     /* USAGE (Vendor usage)                      */
               0x81, 0x02,                     /* INPUT(DATA, VAR, ABS) 32 bytes for input  */
               0x15, 0x00,                     /* LOGICAL_MINIMUM(0)                        */
               0x26, 0xff, 0x00,       /* LOGICAL_MAXIMUM(0XFF)                     */
               0x95, 0x40,                     /* REPORT_COUNT (64)                         */
               0x75, 0x08,                     /* REPORT_SIZE (8)                           */
               0x09, 0x01,                     /* USAGE (Vendor usage)                      */
               0x91, 0x02,                     /* OUTPUT (Data,Var,Abs)                     */
               0xc0                            /* END_COLLECTION                            */
       }
};

struct platform_device jz_hidg = {
       .name                   = "hidg",
       .id                     = 0,
       .num_resources          = 0,
       .resource               = 0,
       .dev.platform_data      = &hid_data,
};

#endif
