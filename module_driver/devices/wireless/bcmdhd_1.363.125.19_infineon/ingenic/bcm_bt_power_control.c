#include <linux/platform_device.h>
#include <linux/module.h>
#include "bt-rfkill.h"
#include <utils/gpio.h>
/**
 * Bluetooth gpio
 **/
static struct bt_rfkill_platform_data  gpio_data = {
    .gpio = {
        .bt_rst_n = -1,
        .bt_reg_on = -1,
        .bt_wake = -1,
        .bt_int = -1,
        .bt_uart_rts = -1,
    },
};

module_param_gpio_named(gpio_bt_rst_n, gpio_data.gpio.bt_rst_n, 0644);
module_param_gpio_named(gpio_bt_reg_on, gpio_data.gpio.bt_reg_on, 0644);
module_param_gpio_named(gpio_host_wake_bt, gpio_data.gpio.bt_wake, 0644);
module_param_gpio_named(gpio_bt_wake_host, gpio_data.gpio.bt_int, 0644);
module_param_gpio_named(gpio_bt_uart_rts, gpio_data.gpio.bt_uart_rts, 0644);

static void m_release(struct device *dev)
{

}

static struct platform_device bt_power_device = {
    .name = "md_bcmdhd_bt_power",
    .id = -1,
    .dev = {
        .platform_data = &gpio_data,
        .release = m_release,
    },
};

int bt_power_dev_init(void)
{
    return platform_device_register(&bt_power_device);
}

int bt_power_dev_exit(void)
{
    platform_device_unregister(&bt_power_device);
    return 0;
}

EXPORT_SYMBOL(bt_power_dev_init);
EXPORT_SYMBOL(bt_power_dev_exit);