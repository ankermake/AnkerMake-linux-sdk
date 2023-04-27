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

#include <board.h>

extern int rtk_wlan_bt_power_ctrl(int is_on);

#define WLAN_SDIO_INDEX    1
#define RESET 0
#define NORMAL 1

#ifdef CONFIG_RTL8723DS
    #define DEVNAME "rtl8723ds_wifi"
#endif

#ifdef CONFIG_RTL8822CS
    #define DEVNAME "rtl88x2cs_wifi"
#endif

static struct wifi_data {
    struct wake_lock wifi_wake_lock;
    struct regulator *wifi_vbat;
    struct regulator *wifi_vddio;
    int wifi_reset;
} rtl_8723_data;

struct rtl_power_data {
    int (*rtk_power_init)(void);
    int (*rtk_power_on)(int);
    int (*rtk_power_off)(int);
};

extern int jzmmc_manual_detect(int index, int on);

int rtk_wlan_init(void)
{
    static struct wake_lock    *wifi_wake_lock = &rtl_8723_data.wifi_wake_lock;
    pr_info("INFO: %s fun runing\n", __func__);
    rtk_wlan_bt_power_ctrl(1);
    rtl_8723_data.wifi_reset = GPIO_WL_DIS_N;
    wake_lock_init(wifi_wake_lock, WAKE_LOCK_SUSPEND, "wifi_wake_lock");
    return 0;
}

static int rtk_wlan_power_on(int flag)
{
    static struct wake_lock    *wifi_wake_lock = &rtl_8723_data.wifi_wake_lock;
    int reset = rtl_8723_data.wifi_reset;

    pr_info("INFO: %s fun runing\n", __func__);
    if (NULL == wifi_wake_lock)
        pr_warn("%s: invalid wifi_wake_lock\n", __func__);
    else if (!gpio_is_valid(reset))
        pr_warn("%s: invalid reset\n", __func__);
    else
        goto start;
    return -ENODEV;

start:
    might_sleep();
    msleep(10);
    switch(flag) {
    case RESET:
        jzmmc_clk_ctrl(WL_MMC_NUM, 1);
        gpio_set_value(reset, 0);
        msleep(10);
        gpio_set_value(reset, 1);
        break;
    case NORMAL:
        gpio_set_value(reset, 1);
        pr_info("gpio_pb wifi enable %d high\n", reset);
        jzmmc_manual_detect(WL_MMC_NUM, 1);
        break;
    }
    wake_lock(wifi_wake_lock);
    return 0;
}

static int rtk_wlan_power_off(int flag)
{
    static struct wake_lock    *wifi_wake_lock = &rtl_8723_data.wifi_wake_lock;
    int reset = rtl_8723_data.wifi_reset;
    pr_info("INFO: %s fun runing\n", __func__);

    if (wifi_wake_lock == NULL)
        pr_warn("%s: invalid wifi_wake_lock\n", __func__);
    else if (!gpio_is_valid(reset))
        pr_warn("%s: invalid reset\n", __func__);
    else
        goto start;
    return -ENODEV;

start:
    switch(flag) {
    case RESET:
        gpio_set_value(reset, 0);
        break;

    case NORMAL:
        gpio_set_value(reset, 0);
        break;
    }
    msleep(100);
    wake_unlock(wifi_wake_lock);
    return 0;
}

static struct rtl_power_data fun_data = {
    .rtk_power_init = rtk_wlan_init,
    .rtk_power_on = rtk_wlan_power_on,
    .rtk_power_off = rtk_wlan_power_off,
};

struct platform_device rtk_dev = {
    .name = DEVNAME,
    .id = -1,
    .dev = {
        .platform_data = &fun_data,
    },
};
static int __init rtk_init(void)
{
    return platform_device_register(&rtk_dev);
}
arch_initcall(rtk_init);

int sdio_private_init(void)
{
    return rtk_wlan_init();
}
EXPORT_SYMBOL(sdio_private_init);
