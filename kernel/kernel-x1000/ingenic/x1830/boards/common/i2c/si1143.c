#include <si1143.h>
#include <board.h>

struct si1143_platform_device si1143_dev = {
    .platdev = {
        .name = SI1143_PLATFORM_NAME,
    },
    .gpio = SI1143_INT_GPIO,
    .i2c = SI1143_I2C_BUS_NUM,
};

static int __init si1143_platform_device_register(void) {
    return platform_device_register(&si1143_dev.platdev);
}

arch_initcall(si1143_platform_device_register);
