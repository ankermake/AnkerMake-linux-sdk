#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/syscore_ops.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <utils/gpio.h>

static DEFINE_MUTEX(power_ctrl_mutex);

static int bt_wifi_power_en = -1;
static int bt_dis_n = -1;
static int wl_dis_n = -1;

module_param_gpio_named(gpio_bt_dis_n, bt_dis_n, 0644);
module_param_gpio_named(gpio_wl_dis_n, wl_dis_n, 0644);
module_param_gpio_named(gpio_bt_wifi_power_en, bt_wifi_power_en, 0644);

static int m_gpio_request(int gpio, unsigned long flags, const char *name)
{
	if (gpio < 0)
		return 0;

	int ret = gpio_request_one(gpio, flags,  name);
	if (ret < 0)
		pr_err("ERROR: request %s pin failed\n", name);

	return ret;
}

static void m_gpio_free(int gpio)
{
	if (gpio >= 0)
		gpio_free(gpio);
}

static void m_gpio_direction_ouput(int gpio, int value)
{
	if (gpio >= 0)
		gpio_direction_output(gpio, value);
}

static void rtk_wlan_bt_power_shutdown(void)
{
	m_gpio_direction_ouput(bt_dis_n, 0);

	m_gpio_direction_ouput(wl_dis_n, 0);

	m_gpio_direction_ouput(bt_wifi_power_en, 0);

	msleep(2000);
}

static struct syscore_ops *syscore_ops;

void rtk_wlan_power_ctrl(int is_on)
{
	m_gpio_direction_ouput(wl_dis_n, !!is_on);
}

void rtk_bt_power_ctrl(int is_on)
{
	m_gpio_direction_ouput(bt_dis_n, !!is_on);
}

int rtk_wlan_bt_power_ctrl(int is_on)
{
	static int ref_count = 0;
	int  ret;

	mutex_lock(&power_ctrl_mutex);
	if (is_on) {
		if (ref_count++ == 0) {
			m_gpio_direction_ouput(bt_wifi_power_en, 0);
			msleep(200);
			m_gpio_direction_ouput(bt_dis_n, 1);
		}
	} else {
		if (--ref_count == 0) {
			m_gpio_direction_ouput(bt_dis_n, 0);
			m_gpio_direction_ouput(bt_wifi_power_en, 1);
		}
	}
	mutex_unlock(&power_ctrl_mutex);

	return 0;
}

int rtk_platform_device_init(void);
void rtk_platform_device_deinit(void);
int rtk_bt_init(void);
void rtk_bt_deinit(void);

int rtk_power_init(void)
{
	int ret;

	ret = m_gpio_request(bt_dis_n, GPIOF_OUT_INIT_LOW, "rtl_bt_en");
	if (ret)
		return ret;

	ret = m_gpio_request(wl_dis_n, GPIOF_OUT_INIT_LOW, "rtl_wifi_en");
	if (ret)
		goto err_wl_dis_n;

	ret = m_gpio_request(bt_wifi_power_en, GPIOF_OUT_INIT_HIGH, "bt_wifi_power_en");
	if (ret)
		goto err_bt_wifi_en;

	ret = rtk_platform_device_init();
	if (ret)
		goto err_platform_init;

	ret = rtk_bt_init();
	if (ret)
		goto err_bt_init;

	syscore_ops = kzalloc(sizeof(*syscore_ops), GFP_KERNEL);
	syscore_ops->shutdown = rtk_wlan_bt_power_shutdown;
	register_syscore_ops(syscore_ops);
	return 0;
err_bt_init:
	rtk_platform_device_deinit();
err_platform_init:
	m_gpio_free(bt_wifi_power_en);
err_bt_wifi_en:
	m_gpio_free(wl_dis_n);
err_wl_dis_n:
	m_gpio_free(bt_dis_n);
	return ret;
}

int rtk_power_deinit(void)
{
	unregister_syscore_ops(syscore_ops);
	kfree(syscore_ops);
	rtk_bt_deinit();
	rtk_platform_device_deinit();
	m_gpio_free(bt_dis_n);
	m_gpio_free(wl_dis_n);
	m_gpio_free(bt_wifi_power_en);
	return 0;
}
