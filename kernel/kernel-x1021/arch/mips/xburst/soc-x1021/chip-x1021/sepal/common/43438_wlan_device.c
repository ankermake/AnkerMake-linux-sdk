#include <linux/platform_device.h>
#include <linux/gpio.h>
#include "board_base.h"


static struct resource wlan_resources[] = {
	[0] = {
		.start = GPIO_WL_DEV_WAKE_HOST,
		.end = GPIO_WL_DEV_WAKE_HOST,
		.name = "bcmdhd_wlan_irq",
		.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
	},
};

extern struct wifi_platform_data wlan_pdata;

static struct platform_device wlan_device = {
	.name   = "bcmdhd_wlan",
	.id     = 1,
	.dev    = {
		.platform_data = &wlan_pdata,
	},
	.resource	= wlan_resources,
	.num_resources	= ARRAY_SIZE(wlan_resources),
};

static int __init wlan_device_init(void)
{
	int ret;

	ret = platform_device_register(&wlan_device);

	return ret;
}

device_initcall(wlan_device_init);
