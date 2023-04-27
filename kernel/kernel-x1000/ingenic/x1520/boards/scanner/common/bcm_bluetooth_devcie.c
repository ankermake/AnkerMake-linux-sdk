#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mach/jzmmc.h>
#include <linux/bcm_pm_core.h>
#include <linux/delay.h>

#include "board_base.h"

/*For BlueTooth*/

#include <linux/bt-rfkill.h>


static struct bt_rfkill_platform_data  bt_gpio_data = {
    .gpio = {
        .bt_rst_n       = -1,
        .bt_reg_on      = BT_REG_EN,
        .bt_wake        = HOST_WAKE_BT,
        .bt_int         = BT_WAKE_HOST,
        .bt_uart_rts    = BT_UART_RTS,
    },

    .restore_pin_status = NULL,
    .set_pin_status     = NULL,

};

struct platform_device bt_power_device  = {
    .name               = "bt_power" ,
    .id                 = -1 ,
    .dev   = {
        .platform_data = &bt_gpio_data,
    },
};

struct platform_device bluesleep_device = {
    .name               = "bluesleep" ,
    .id                 = -1 ,
    .dev   = {
        .platform_data = &bt_gpio_data,
    },

};

#ifdef CONFIG_BT_BLUEDROID_SUPPORT
int bluesleep_tty_strcmp(const char* name)
{
    if(!strcmp(name,BLUETOOTH_UPORT_NAME)){
        return 0;
    } else {
        return -1;
    }
}
EXPORT_SYMBOL(bluesleep_tty_strcmp);
#endif

