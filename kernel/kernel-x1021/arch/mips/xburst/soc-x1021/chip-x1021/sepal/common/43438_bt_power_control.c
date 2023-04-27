#include <linux/platform_device.h>
#include <linux/bcm_pm_core.h>
#include <linux/bt-rfkill.h>
#include <gpio.h>
#include <soc/gpio.h>
#include "board_base.h"

/**
 * Bluetooth gpio
 **/
struct bt_rfkill_platform_data  gpio_data = {
	.gpio = {
		.bt_rst_n = -1,
		.bt_reg_on = GPIO_BT_DIS_N,
		.bt_wake = GPIO_BT_WAKE_HOST,
		.bt_int = GPIO_HOST_WAKE_BT,
		.bt_uart_rts = GPIO_BT_UART_RTS,
	},
};

struct platform_device bt_power_device = {
	.name = "bt_power",
	.id = -1,
	.dev = {
		.platform_data = &gpio_data,
	},
};
