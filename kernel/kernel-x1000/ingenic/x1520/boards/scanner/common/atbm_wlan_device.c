#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mach/jzmmc.h>
#include <linux/bcm_pm_core.h>
#include <linux/delay.h>

#include "board_base.h"

struct resource atbm_wlan_resources[] = {
    [0] = {
        .start          = WL_WAKE_HOST,
        .end            = WL_WAKE_HOST,
        .name           = "atbm_wlan_irq",
        .flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
    },
};


struct platform_device atbm_wlan_device = {
    .name               = "atbm_wlan",
    .id                 = 1,
    .dev = {
        .platform_data  = NULL,
    },
    .resource           = atbm_wlan_resources,
    .num_resources      = ARRAY_SIZE(atbm_wlan_resources),
};

