#include <board.h>
#include <linux/i2c.h>
#include <aw9163.h>

#define DEVICE_NAME		"aw9163_ts"

static struct aw9163_io_data aw9163_data = {
    .aw9163_pdn = AW9163_PDN_GPIO,
    .aw9163_int = AW9163_INT_GPIO,
    .bus_number = AW9163_I2C_BUS_NUM,
};

static struct platform_device aw9153_plat = {
    .name = "aw9163-ts",
    .dev = {
        .platform_data = &aw9163_data,
    },
};

static int __init aw9163_init(void) {
    return platform_device_register(&aw9153_plat);
}

device_initcall(aw9163_init);
