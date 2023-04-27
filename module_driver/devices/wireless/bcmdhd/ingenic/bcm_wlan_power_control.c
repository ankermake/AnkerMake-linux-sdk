#include <linux/mmc/host.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <utils/gpio.h>
#include "../../module_wakeup_lock.h"

extern int jzmmc_manual_detect(int index, int on);
extern int jzmmc_clk_ctrl(int index, int on);

static int is_inited = 0;
static int wlan_reg_on = -1;
static int bt_wifi_power = -1;
static int wlan_mmc_num = -1;
static int wlan_wake_host = -1;
static struct wake_lock wifi_wake_lock;

extern void ingenic_rtc32k_enable(void);
extern void ingenic_rtc32k_disable(void);

module_param_gpio_named(gpio_wlan_reg_on, wlan_reg_on, 0644);
module_param_gpio_named(gpio_bt_wifi_power, bt_wifi_power, 0644);
module_param_gpio_named(gpio_wlan_wake_host, wlan_wake_host, 0644);
module_param(wlan_mmc_num, int, 0644);

int bcm_wlan_init(void)
{
    is_inited = -1;

    if (wlan_mmc_num < 0) {
        printk(KERN_ERR "wlan_mmc_num must be define!\n");
        return -EINVAL;
    }

    if (wlan_reg_on < 0) {
        printk(KERN_ERR "wlan_reg_on must be define!\n");
        return -EINVAL;
    }

    if (gpio_request_one(wlan_reg_on, GPIOF_DIR_OUT|GPIOF_INIT_LOW, "wlan_reg_on") < 0) {
        char gpio[10];
        gpio_to_str(wlan_reg_on, gpio);
        printk(KERN_ERR "gpio_wlan_reg_on:%s request failed!\n", gpio);
        return -EINVAL;
    }

    if (bt_wifi_power >= 0) {
        if (gpio_request_one(bt_wifi_power, GPIOF_DIR_OUT|GPIOF_INIT_LOW, "bt_wifi_power") < 0) {
            char gpio[10];
            gpio_to_str(bt_wifi_power, gpio);
            printk(KERN_ERR "bt_wifi_power:%s request failed!\n", gpio);
            return -EINVAL;
        }
    }
    is_inited = 1;

    wake_lock_init(&wifi_wake_lock, WAKE_LOCK_SUSPEND, "wifi_wake_lock");
    return 0;
}

int bcm_wlan_deinit(void)
{
    gpio_set_value(wlan_reg_on, 0);
    gpio_free(wlan_reg_on);

    if (bt_wifi_power >= 0) {
        gpio_set_value(bt_wifi_power, 1);
        gpio_free(bt_wifi_power);
    }

    jzmmc_clk_ctrl(wlan_mmc_num, 0);
    wake_lock_destroy(&wifi_wake_lock);
    return 0;
}

void wlan_pw_en_enable(void)
{
}

void wlan_pw_en_disable(void)
{
}

#define RESET                       0
#define NORMAL                      1

int bcm_wlan_power_on(int flag)
{
    if (is_inited < 0)
        return -EINVAL;

    if (!is_inited && bcm_wlan_init())
        return -ENODEV;

    pr_err("wlan power on: %s\n", flag ? "init" : "poweron");

    ingenic_rtc32k_enable();

    switch(flag) {
    case RESET:
        printk("bcm_wlan_power_on, RESET");
        jzmmc_clk_ctrl(wlan_mmc_num, 1);
        gpio_set_value(wlan_reg_on, 0);
        msleep(100);
        gpio_set_value(wlan_reg_on, 1);
        break;
    case NORMAL:
        printk("bcm_wlan_power_on  NORMAL\n");
        gpio_set_value(wlan_reg_on, 0);
        msleep(100);
        gpio_set_value(wlan_reg_on, 1);
        jzmmc_manual_detect(wlan_mmc_num, 1);
        break;
    }
    wake_lock(&wifi_wake_lock);
    return 0;
}


int bcm_wlan_power_off(int flag)
{
    if (wlan_reg_on < 0 && bcm_wlan_init())
        return -ENODEV;

    pr_err("wlan power off: %s\n", flag ? "initok" : "poweroff");

    switch(flag) {
    case RESET:
    case NORMAL:
        gpio_set_value(wlan_reg_on, 0);
        break;
    }

    ingenic_rtc32k_disable();

    wake_unlock(&wifi_wake_lock);
    return 0;
}

int md_ingenic_sdio_wlan_get_irq(unsigned long *flag)
{
	*flag = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE;
	return wlan_wake_host;
}

EXPORT_SYMBOL(bcm_wlan_init);
EXPORT_SYMBOL(bcm_wlan_deinit);
EXPORT_SYMBOL(bcm_wlan_power_on);
EXPORT_SYMBOL(bcm_wlan_power_off);
EXPORT_SYMBOL(md_ingenic_sdio_wlan_get_irq);