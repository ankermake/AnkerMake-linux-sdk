#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sizes.h>
#include <linux/resource.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>

#include <linux/regulator/fixed.h>
#include <linux/regulator/machine.h>
#include <linux/usb/gpio_vbus.h>

#include <linux/usb/phy-ingenic.h>

#include <soc/base.h>
#include <soc/irq.h>

#define DWC2_DRIVER_NAME	"dwc2"
#define DWC2_VBUS_REG_NAME	"vbus"

#define usb_device_dma_mask (*((u64[]) { DMA_BIT_MASK(32) }))

static struct resource usb_hsotg_resources[] = {
	[0] = DEFINE_RES_MEM(OTG_IOBASE, SZ_128K),
	[1] = DEFINE_RES_IRQ(IRQ_OTG),
};

struct platform_device dwc2_new_usb_hsotg = {
	.name		= DWC2_DRIVER_NAME,
	.id		= -1,
	.num_resources	= ARRAY_SIZE(usb_hsotg_resources),
	.resource	= usb_hsotg_resources,
	.dev		= {
		.dma_mask		= &usb_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

struct ingenic_phy_data ingenic_phy_data_config = {

#ifdef CONFIG_INGENIC_USB_PHY_VBUS_DETECT_GPIO_WAKEUP_ENABLE
	.gpio_vbus_wakeup_enable = CONFIG_INGENIC_USB_PHY_VBUS_DETECT_GPIO_WAKEUP_ENABLE,
#endif

#ifdef CONFIG_INGENIC_USB_PHY_VBUS_DETECT_GPIO
	.gpio_vbus = CONFIG_INGENIC_USB_PHY_VBUS_DETECT_GPIO_NUM,
#ifdef CONFIG_INGENIC_USB_PHY_VBUS_DETECT_GPIO_INVERT
	.gpio_vbus_inverted = ture,
#else
	.gpio_vbus_inverted = false,
#endif
#endif

#ifdef CONFIG_INGENIC_USB_PHY_VBUS_SUPPLY_GPIO
	.gpio_drvvbus = CONFIG_INGENIC_USB_PHY_VBUS_SUPPLY_GPIO_NUM,
#ifdef CONFIG_INGENIC_USB_PHY_VBUS_SUPPLY_GPIO_INVERT
	.gpio_drvvbus_inverted = ture,
#else
	.gpio_drvvbus_inverted = false,
#endif
#endif


#ifdef CONFIG_INGENIC_USB_PHY_SWITCH_GPIO
	.gpio_switch = CONFIG_INGENIC_USB_PHY_SWITCH_GPIO_NUM,
#ifdef CONFIG_INGENIC_USB_PHY_SWITCH_GPIO_INVERT
	.gpio_switch_inverted = ture,
#else
	.gpio_switch_inverted = false,
#endif
#endif

#ifdef CONFIG_INGENIC_USB_PHY_WAKEUP_GPIO
	.gpio_wakeup = CONFIG_INGENIC_USB_PHY_WAKEUP_GPIO_NUM,
#ifdef CONFIG_INGENIC_USB_PHY_WAKEUP_GPIO_INVERT
	.gpio_wakeup_inverted = ture,
#else
	.gpio_wakeup_inverted = false,
#endif
#endif
};

struct platform_device dwc2_new_usb_phy = {
	.name		= "ingenic_usb_phy",
	.id		= -1,
	.dev		= {
		.platform_data	= &ingenic_phy_data_config,
	},
};

static int __init dwc2_new_platform_device_init(void)
{
	platform_device_register(&dwc2_new_usb_phy);

	platform_device_register(&dwc2_new_usb_hsotg);

	return 0;
}

subsys_initcall(dwc2_new_platform_device_init);
