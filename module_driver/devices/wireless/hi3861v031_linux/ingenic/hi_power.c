#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/syscore_ops.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <soc/gpio.h>

#include <utils/gpio.h>
#include "hi_power.h"

static DEFINE_MUTEX(power_ctrl_mutex);

int wifi_power_en = -1;
int wifi_power_on = -1;

module_param_gpio_named(gpio_wifi_power_on, wifi_power_on, 0644);   //PB25
module_param_gpio_named(gpio_wifi_power_en, wifi_power_en, 0644);   //PD11

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    return gpio_request(gpio, name);
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

static void hi_wlan_power_shutdown(void)
{
	m_gpio_direction_ouput(wifi_power_on, 0);

	m_gpio_direction_ouput(wifi_power_en, 0);

	msleep(2000);
}

static struct syscore_ops *syscore_ops;

void hi_wlan_power_ctrl(int is_on)
{
	m_gpio_direction_ouput(wifi_power_on, !!is_on);
}

int hi_power_init(void)
{
	int ret;

	ret = m_gpio_request(wifi_power_on, "wifi_power_on");
	if (ret)
		goto err_wifi_power_on;
	m_gpio_direction_ouput(wifi_power_on, 0);

	ret = m_gpio_request(wifi_power_en, "wifi_power_en");
	if (ret)
		goto err_wifi_power_en;
	m_gpio_direction_ouput(wifi_power_en, 1);

	syscore_ops = kzalloc(sizeof(*syscore_ops), GFP_KERNEL);
	syscore_ops->shutdown = hi_wlan_power_shutdown;
	register_syscore_ops(syscore_ops);
	return 0;

err_wifi_power_en:
	m_gpio_free(wifi_power_on);
err_wifi_power_on:

	return ret;
}

int hi_power_deinit(void)
{
	unregister_syscore_ops(syscore_ops);
	kfree(syscore_ops);
	m_gpio_free(wifi_power_on);
	m_gpio_free(wifi_power_en);
	return 0;
}
