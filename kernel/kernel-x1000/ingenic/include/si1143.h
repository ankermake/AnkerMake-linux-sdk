#ifndef __SI1143_H__
#define __SI1143_H__

#include <linux/platform_device.h>

#define SI1143_PLATFORM_NAME "si1143plat"

struct si1143_platform_device {
    struct platform_device platdev;
    int gpio;
    int i2c;
};

#endif
