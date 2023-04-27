#include <linux/platform_device.h>
#include <utils/gpio.h>
#include "wlan_plat.h"

static int wlan_wake_host = -1;

module_param_gpio_named(gpio_wlan_wake_host, wlan_wake_host, 0644);

static struct resource wlan_resources[] = {
    [0] = {
        .name = "md_bcmdhd_wlan_irq",
        .flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
    },
};

extern int bcm_wlan_get_mac_addr_from_sys(unsigned char *buf);

struct wifi_platform_data dhd_wlan_control = {
    .get_mac_addr = bcm_wlan_get_mac_addr_from_sys,
};

static struct platform_device wlan_device = {
    .name   = "md_bcmdhd_wlan",
    .id     = 1,
    .dev    = {
        .platform_data = &dhd_wlan_control,
    },
    .resource    = wlan_resources,
    .num_resources    = ARRAY_SIZE(wlan_resources),
};

int wlan_device_init(void)
{
    int ret;

    if (wlan_wake_host < 0) {
        printk(KERN_ERR "gpio_wlan_wake_host must be define!\n");
        return -EINVAL;
    }

    wlan_resources[0].start = wlan_wake_host;
    wlan_resources[0].end = wlan_wake_host;

    ret = platform_device_register(&wlan_device);

    return ret;
}

void wlan_device_exit(void)
{
    platform_device_unregister(&wlan_device);
}

int md_ingenic_sdio_wlan_get_irq(unsigned long *flag)
{
    *flag = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE;
    return wlan_wake_host;
}

EXPORT_SYMBOL(md_ingenic_sdio_wlan_get_irq);
