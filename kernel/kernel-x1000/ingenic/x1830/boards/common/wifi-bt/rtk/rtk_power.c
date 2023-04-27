#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <board.h>
#include <linux/syscore_ops.h>
#include <linux/delay.h>

static DEFINE_MUTEX(power_ctrl_mutex);
static int power_ctrl_gpio = 0;
static void rtk_wlan_bt_power_shutdown(void)
{
    if (power_ctrl_gpio) {
#ifdef GPIO_EN_BT_WIFI_VDD
        gpio_direction_output(GPIO_EN_BT_WIFI_VDD, 0);
#endif
#ifdef GPIO_BT_DIS_N
        gpio_direction_output(GPIO_BT_DIS_N, 0);
#endif
#ifdef GPIO_WL_DIS_N
        gpio_direction_output(GPIO_WL_DIS_N, 0);
#endif
    }
    might_sleep();
    msleep(2000);
    return;
}

static struct syscore_ops syscore_ops = {
    .shutdown = rtk_wlan_bt_power_shutdown,
};

int rtk_wlan_bt_power_ctrl(int is_on)
{
    static int ref_count = 0;
    int  ret;

    might_sleep();

    mutex_lock(&power_ctrl_mutex);

    if (!power_ctrl_gpio) {
#ifdef GPIO_BT_DIS_N
        if (!gpio_is_valid(GPIO_BT_DIS_N)) {
            mutex_unlock(&power_ctrl_mutex);
            return -ENODEV;
        }
        ret = gpio_request_one(GPIO_BT_DIS_N, GPIOF_OUT_INIT_LOW,  "rtl_bt_en");
        if (ret < 0) {
            mutex_unlock(&power_ctrl_mutex);
            pr_err("ERROR: request rtl_bt_enable pin failed\n");
            return ret;
        }
#endif
#ifdef GPIO_WL_DIS_N
        if (!gpio_is_valid(GPIO_WL_DIS_N)) {
            mutex_unlock(&power_ctrl_mutex);
            return -ENODEV;
        }
        ret = gpio_request_one(GPIO_WL_DIS_N, GPIOF_OUT_INIT_LOW, "wifi_reset-rtl");
        if (ret < 0) {
            mutex_unlock(&power_ctrl_mutex);
            pr_err("ERROR: request rtl_bt_enable pin failed\n");
            return ret;
        }
#endif
#ifdef GPIO_EN_BT_WIFI_VDD
        if (!gpio_is_valid(GPIO_EN_BT_WIFI_VDD)) {
            mutex_unlock(&power_ctrl_mutex);
            return -ENODEV;
        }

        ret = gpio_request_one(GPIO_EN_BT_WIFI_VDD, GPIOF_OUT_INIT_LOW, "bt_wifi_en");
        if (ret < 0) {
            pr_err("gpio en_bt&wifi_3V3 request failed\n");
            mutex_unlock(&power_ctrl_mutex);
            return ret;
        }
#endif
        register_syscore_ops(&syscore_ops);
        power_ctrl_gpio = 1;
    }

    if (is_on) {
        if (!ref_count) {
#ifdef GPIO_BT_DIS_N
            gpio_direction_output(GPIO_BT_DIS_N, 0);
#endif
#ifdef GPIO_WL_DIS_N
            gpio_direction_output(GPIO_WL_DIS_N, 0);
#endif
#ifdef GPIO_EN_BT_WIFI_VDD
    #ifdef CONFIG_RTL8723DS
            gpio_direction_output(GPIO_EN_BT_WIFI_VDD, 0);
    #endif
    #ifdef CONFIG_RTL8822CS
            gpio_direction_output(GPIO_EN_BT_WIFI_VDD, 1);
    #endif
#endif
            msleep(200);
#ifdef GPIO_BT_DIS_N
            gpio_direction_output(GPIO_BT_DIS_N, 1);
#endif
#ifdef GPIO_WL_DIS_N
            gpio_direction_output(GPIO_WL_DIS_N, 1);
#endif
        }
        ref_count++;
    } else {
        ref_count--;
#ifdef GPIO_EN_BT_WIFI_VDD
        if (!ref_count) {
            #ifdef CONFIG_RTL8723DS
                gpio_direction_output(GPIO_EN_BT_WIFI_VDD, 1);
            #endif
            #ifdef CONFIG_RTL8822CS
                gpio_direction_output(GPIO_EN_BT_WIFI_VDD, 0);
            #endif
        }
#endif
    }
    mutex_unlock(&power_ctrl_mutex);
    return 0;
}
EXPORT_SYMBOL(rtk_wlan_bt_power_ctrl);
