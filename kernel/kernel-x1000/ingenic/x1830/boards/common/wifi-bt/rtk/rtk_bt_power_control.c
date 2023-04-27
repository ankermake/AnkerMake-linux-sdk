#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <board.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <board.h>

#define DEVNAME "rtl_bt_en"

extern int rtk_wlan_bt_power_ctrl(int is_on);

static int rtk_bt_enable(void)
{
	int ret = 0;
	pr_info("INFO: %s fun running\n", __func__);
	rtk_wlan_bt_power_ctrl(1);
	return ret;
}

struct rtk_bt_data {
	int (*rtk_bt_en)(void);
};

struct rtk_bt_data rtk_bt_data = {
	.rtk_bt_en = rtk_bt_enable,
};

struct platform_device rtk_bt_dev = {
	.name = DEVNAME,
	.id	= -1,
	.dev = {
	    .platform_data = &rtk_bt_data,
	},
};

static int __init rtk_bt_init(void)
{
	return platform_device_register(&rtk_bt_dev);
}
arch_initcall(rtk_bt_init);
