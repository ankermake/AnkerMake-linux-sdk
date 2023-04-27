#include <board.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>

#include "board.h"

#define AW87519_AUDIO       "aw87519_audio"

static struct platform_device aw87519_dev = {
    .name = AW87519_AUDIO,
    .dev  = {
        .platform_data = GPIO_AW87519_RESET,
    },
};

static int __init aw87519_init(void)
{
    return platform_device_register(&aw87519_dev);
}

device_initcall(aw87519_init);