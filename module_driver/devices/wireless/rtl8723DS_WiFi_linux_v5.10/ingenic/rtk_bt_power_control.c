#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#define DEVNAME "rtl_bt_en"

int rtk_wlan_bt_power_ctrl(int is_on);
void rtk_bt_power_ctrl(int is_on);

static int is_enable = 0;

static int rtk_bt_enable(void)
{
	int ret = 0;
	pr_info("INFO: %s fun running\n", __func__);

	if (is_enable)
		return 0;
	is_enable = 1;

	rtk_wlan_bt_power_ctrl(1);
	rtk_bt_power_ctrl(1);
	return ret;
}

struct rtk_bt_data {
	int (*rtk_bt_en)(void);
};

struct rtk_bt_data rtk_bt_data = {
	.rtk_bt_en = rtk_bt_enable,
};

static void m_release(struct device *dev)
{

}

struct platform_device rtk_bt_dev = {
	.name = DEVNAME,
	.id	= -1,
	.dev = {
	    .platform_data = &rtk_bt_data,
		.release = m_release,
	},
};

int rtk_bt_init(void)
{
	return platform_device_register(&rtk_bt_dev);
}

void rtk_bt_deinit(void)
{
	platform_device_unregister(&rtk_bt_dev);
}
