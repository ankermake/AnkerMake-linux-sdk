#include <linux/mmc/host.h>
#include <linux/fs.h>
#include <linux/wlan_plat.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <mach/jzmmc.h>
#include <linux/bcm_pm_core.h>

#include <gpio.h>
#include <soc/gpio.h>
#include "board_base.h"

static int wlan_reset_pin = -1;
static struct wake_lock wifi_wake_lock;

extern int bcm_wlan_get_mac_addr_from_sys(unsigned char *buf);

struct wifi_platform_data wlan_pdata = {
    .get_mac_addr = bcm_wlan_get_mac_addr_from_sys,
};

void wlan_pw_en_enable(void)
{
}

void wlan_pw_en_disable(void)
{
}

extern void rtc32k_enable(void);
extern void rtc32k_disable(void);

int bcm_wlan_init(void)
{
    wlan_reset_pin = GPIO_WL_DIS_N;
    if (gpio_request_one(wlan_reset_pin, GPIOF_DIR_OUT|GPIOF_INIT_HIGH, "wifi_reset_n") < 0)
        goto error;

    if (GPIO_EN_BT_WIFI_VDD >=0) {
        if (gpio_request_one(GPIO_EN_BT_WIFI_VDD, GPIOF_DIR_OUT|GPIOF_INIT_LOW, "wifi_bt_power_en") < 0)
            goto error;
    }

    wake_lock_init(&wifi_wake_lock, WAKE_LOCK_SUSPEND, "wifi_wake_lock");
    return 0;
error:
    pr_err("[ERROR] %s failed\n", __func__);
    wlan_reset_pin = -ENODEV;
    return -ENODEV;
}

#define RESET                       0
#define NORMAL                      1

int bcm_wlan_power_on(int flag)
{
    if (wlan_reset_pin < 0 && bcm_wlan_init())
        return -ENODEV;

    pr_err("wlan power on: %s\n", flag ? "init" : "poweron");

    rtc32k_enable();

    switch(flag) {
    case RESET:
        jzmmc_clk_ctrl(WL_MMC_NUM, 1);
        gpio_set_value(wlan_reset_pin, 0);
        msleep(100);
        gpio_set_value(wlan_reset_pin, 1);
        break;
    case NORMAL:
        gpio_set_value(wlan_reset_pin, 0);
        msleep(100);
        gpio_set_value(wlan_reset_pin, 1);
        jzmmc_manual_detect(WL_MMC_NUM, 1);
        break;
    }
    wake_lock(&wifi_wake_lock);
    return 0;
}

int bcm_wlan_power_off(int flag)
{
    if (wlan_reset_pin < 0 && bcm_wlan_init())
        return -ENODEV;

    pr_err("wlan power off: %s\n", flag ? "initok" : "poweroff");

    switch(flag) {
    case RESET:
    case NORMAL:
        gpio_set_value(wlan_reset_pin, 0);
        break;
    }

    rtc32k_disable();

    wake_unlock(&wifi_wake_lock);
    return 0;
}

EXPORT_SYMBOL(bcm_wlan_init);
EXPORT_SYMBOL(bcm_wlan_power_on);
EXPORT_SYMBOL(bcm_wlan_power_off);
