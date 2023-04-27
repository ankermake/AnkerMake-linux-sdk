#include <linux/version.h>
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

extern int rtk_wlan_bt_power_ctrl(int is_on);
extern int jzmmc_manual_detect(int index, int on);
extern int jzmmc_clk_ctrl(int index, int on);

#define WLAN_SDIO_INDEX	1
#define RESET 0
#define NORMAL 1
#define DEVNAME "rtl8723ds_wifi"

static int wlan_mmc_num = -1; // 1

module_param(wlan_mmc_num, int, 0644);

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


void rtk_wlan_power_ctrl(int is_on);

static int is_init = 0;

static int rtk_wlan_init(void)
{
	struct wake_lock	*wifi_wake_lock = &rtl_8723_data.wifi_wake_lock;
	pr_info("INFO: %s fun runing\n", __func__);
	rtk_wlan_bt_power_ctrl(1);
	wake_lock_init(wifi_wake_lock, WAKE_LOCK_SUSPEND, "wifi_wake_lock");
	return 0;
}

static void rtk_wlan_deinit(void)
{
	struct wake_lock	*wifi_wake_lock = &rtl_8723_data.wifi_wake_lock;

	wake_lock_destroy(wifi_wake_lock);
}

static int rtk_wlan_power_on(int flag)
{
	static struct wake_lock	*wifi_wake_lock = &rtl_8723_data.wifi_wake_lock;

	pr_info("INFO: %s fun runing\n", __func__);
	if (!is_init) {
		is_init = 1;
		rtk_wlan_init();
	}

	msleep(10);
	switch(flag) {
	case RESET:
		jzmmc_clk_ctrl(wlan_mmc_num, 1);
		rtk_wlan_power_ctrl(0);
		msleep(10);
		rtk_wlan_power_ctrl(1);
		break;
	case NORMAL:
		rtk_wlan_power_ctrl(1);
		jzmmc_manual_detect(wlan_mmc_num, 1);
		break;
	}
	wake_lock(wifi_wake_lock);
	return 0;
}

static int rtk_wlan_power_off(int flag)
{
	static struct wake_lock	*wifi_wake_lock = &rtl_8723_data.wifi_wake_lock;
	pr_info("INFO: %s fun runing\n", __func__);

	switch(flag) {
	case RESET:
		rtk_wlan_power_ctrl(0);
		break;

	case NORMAL:
		rtk_wlan_power_ctrl(0);
		break;
	}
	msleep(100);
	wake_unlock(wifi_wake_lock);
	return 0;
}

static struct rtl_power_data fun_data = {
	.rtk_power_init = NULL,
	.rtk_power_on = rtk_wlan_power_on,
	.rtk_power_off = rtk_wlan_power_off,
};

static void m_release(struct device *dev)
{

}

static struct platform_device rtk_dev = {
	.name = DEVNAME,
	.id = -1,
	.dev = {
		.platform_data = &fun_data,
		.release = m_release,
	},
};

int rtk_platform_device_init(void)
{
	return platform_device_register(&rtk_dev);
}

void rtk_platform_device_deinit(void)
{
	platform_device_unregister(&rtk_dev);
	if (is_init) {
		rtk_wlan_deinit();
		is_init = 0;
	}
}
