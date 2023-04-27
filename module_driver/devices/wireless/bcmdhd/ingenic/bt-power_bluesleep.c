/*
 * Description:
 * Bluetooth power driver with rfkill interface ,work in with bluesleep.c , version of running consume.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/rfkill.h>
#include <linux/gpio.h>
// #include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include "bt-rfkill.h"
#include <soc/gpio.h>

#define DEV_NAME		"md_bcmdhd_bt_power"

static struct bt_rfkill_platform_data *pdata;
#if 1
#define	DBG_MSG(fmt, ...)	printk(fmt, ##__VA_ARGS__)
#else
#define DBG_MSG(fmt, ...)
#endif

int bt_power_state = 0;
static int bt_power_control(int enable);
static int bt_rst_n ;
static int bt_reg_on;
static int bt_wake;
static int bt_uart_rts;
// static struct gpio_reg_func rfunc;
static int bt_wake_irq = -1;

static int host_wake_bt;

static DEFINE_MUTEX(bt_power_lock);

extern void wlan_pw_en_enable(void);
extern void wlan_pw_en_disable(void);
extern void ingenic_rtc32k_enable(void);
extern void ingenic_rtc32k_disable(void);

extern int bluesleep_start(void);
extern void bluesleep_stop(void);

/* For compile only, remove later */
#define RFKILL_STATE_SOFT_BLOCKED	0
#define RFKILL_STATE_UNBLOCKED		1

static void bt_enable_power(void)
{
	wlan_pw_en_enable();
	if (bt_reg_on != -1)
		gpio_set_value(bt_reg_on, 1);
}

static void bt_disable_power(void)
{
	wlan_pw_en_disable();
	if (bt_reg_on != -1)
		gpio_set_value(bt_reg_on, 0);
}

static int bt_power_control(int enable)
{
	if (enable == bt_power_state)
		return 0;

	switch (enable)	{
	case RFKILL_STATE_SOFT_BLOCKED:
//		bluesleep_stop();
//		mdelay(15);
		ingenic_rtc32k_disable();
		bt_disable_power();
		mdelay(100);
		if (pdata->set_pin_status != NULL)
			(*pdata->set_pin_status)(enable);
		break;
	case RFKILL_STATE_UNBLOCKED:
		if (pdata->restore_pin_status != NULL)
			(*pdata->restore_pin_status)(enable);

		ingenic_rtc32k_enable();
		if (bt_rst_n > 0){
			gpio_direction_output(bt_rst_n,0);
		}
		bt_enable_power();
		mdelay(300);
		if(bt_rst_n > 0){
			gpio_set_value(bt_rst_n,1);
		}
//		mdelay(15);
//		bluesleep_start();
		break;
	default:
		break;
	}

	bt_power_state = enable;

	return 0;
}

static bool first_called = true;

static int bt_rfkill_set_block(void *data, bool blocked)
{
	int ret;

	if (!first_called) {
		mutex_lock(&bt_power_lock);
		ret = bt_power_control(blocked ? 0 : 1);
		mutex_unlock(&bt_power_lock);
	} else {
		first_called = false;
		return 0;
	}

	return ret;
}

static const struct rfkill_ops bt_rfkill_ops = {
	.set_block = bt_rfkill_set_block,
};

static int bt_power_rfkill_probe(struct platform_device *pdev)
{
	int ret = -ENOMEM;

	pdata = pdev->dev.platform_data;
	pdata->rfkill = rfkill_alloc("bluetooth", &pdev->dev, RFKILL_TYPE_BLUETOOTH,
                            &bt_rfkill_ops, NULL);

	if (!pdata->rfkill) {
		goto exit;
	}

	ret = rfkill_register(pdata->rfkill);
	if (ret) {
		rfkill_destroy(pdata->rfkill);
		return ret;
	} else {
		platform_set_drvdata(pdev, pdata->rfkill);
	}

exit:
	return ret;
}

static void bt_power_rfkill_remove(struct platform_device *pdev)
{
	pdata->rfkill = platform_get_drvdata(pdev);
	if (pdata->rfkill)
		rfkill_unregister(pdata->rfkill);

	platform_set_drvdata(pdev, NULL);
}

static irqreturn_t bt_wake_host_cb(int i, void *data)
{
	return IRQ_HANDLED;
}

static int bluesleep_suspend(struct platform_device *pdev, pm_message_t state)
{

	if(bt_wake >= 0)
		enable_irq_wake(bt_wake_irq);

	// if(bt_uart_rts >= 0) {
	// 	jz_gpio_save_reset_func(bt_uart_rts / 32, GPIO_OUTPUT1,
	// 							1 << (bt_uart_rts % 32), &rfunc);
	// }
	return 0;
}

static int bluesleep_resume(struct platform_device *pdev)
{
	if(bt_wake >= 0)
		disable_irq_wake(bt_wake_irq);

	// if(bt_uart_rts >= 0)
	// 	jz_gpio_restore_func(bt_uart_rts / 32,
	// 						 1 << (bt_uart_rts % 32), &rfunc);

	return 0;
}
static int __init_or_module bt_power_probe(struct platform_device *pdev)
{
	int ret = 0;
	if (!ret && !bt_power_state && bt_power_rfkill_probe(pdev)) {
		ret = -ENOMEM;
	}

	if(!pdata){
		printk("Can not find data about bt_rfkill_platform_data\n");
		return -ENODEV;
	}

	bt_rst_n = pdata->gpio.bt_rst_n;
	bt_reg_on = pdata->gpio.bt_reg_on;
	bt_wake = pdata->gpio.bt_int;
	bt_uart_rts = pdata->gpio.bt_uart_rts;
	host_wake_bt = pdata->gpio.bt_wake;

	if(bt_rst_n >= 0){
		ret = gpio_request(bt_rst_n,"bt_rst_n");
		if(unlikely(ret)){
			return ret;
		}
	}

	if (bt_reg_on >= 0){
		ret = gpio_request(bt_reg_on,"bt_reg_on");
		if(unlikely(ret)){
			// gpio_free(bt_rst_n);
			printk("bt_reg_on request failed\n");
			return ret;
		}
		gpio_direction_output(bt_reg_on, 0);
	}

	if(host_wake_bt >= 0){
		ret = gpio_request(host_wake_bt,"host_wake_bt");
		if(unlikely(ret)){
			// gpio_free(bt_rst_n);
			gpio_free(bt_reg_on);

			printk("host_wake_bt request failed\n");
			return ret;
		}
		gpio_direction_output(host_wake_bt, 1);
	}

	if(bt_wake < 0)
		return 0;

	ret = gpio_request(bt_wake,"bt_wake");
	if(unlikely(ret)){
		// gpio_free(bt_rst_n);
		gpio_free(bt_reg_on);
		gpio_free(host_wake_bt);
		printk("bt_wake request failed\n");
		return ret;
	}

	ret = gpio_direction_input(bt_wake);
	if (ret < 0) {
		pr_err("gpio-keys: failed to configure input"
				" direction for GPIO %d, error %d\n",
				bt_wake, ret);
		return ret;
	}

	bt_wake_irq = gpio_to_irq(bt_wake);
	if (bt_wake_irq < 0) {
		printk("couldn't find host_wake irq\n");
		return -1;
	}

	ret = request_irq(bt_wake_irq, bt_wake_host_cb,
			/*IRQF_DISABLED |*/ IRQF_TRIGGER_RISING,
			"bluetooth bthostwake", NULL);

	if (ret < 0) {
		printk("Couldn't acquire BT_HOST_WAKE IRQ err (%d)\n", ret);
		return -1;
	}

	return 0;
}

static int bt_power_remove(struct platform_device *pdev)
{
	int ret;

	bt_power_rfkill_remove(pdev);

	mutex_lock(&bt_power_lock);
	bt_power_state = 0;
	ret = bt_power_control(bt_power_state);
	mutex_unlock(&bt_power_lock);

	if (bt_rst_n >= 0)
		gpio_free(bt_rst_n);

	if (bt_reg_on >= 0)
		gpio_free(bt_reg_on);

	if (bt_wake >= 0)
		gpio_free(bt_wake);

	if (bt_uart_rts >= 0)
		gpio_free(bt_uart_rts);

	if (host_wake_bt >= 0)
		gpio_free(host_wake_bt);

	if (bt_wake_irq >= 0)
		free_irq(bt_wake_irq, NULL);

	return ret;
}

static struct platform_driver bt_power_driver = {
	.probe = bt_power_probe,
	.remove = bt_power_remove,
	.suspend = bluesleep_suspend,
	.resume = bluesleep_resume,
	.driver = {
		.name = DEV_NAME,
		.owner = THIS_MODULE,
	},
};

int bt_power_init(void)
{
	int ret;
	ret = platform_driver_register(&bt_power_driver);

	return ret;
}

int bt_power_exit(void)
{
	platform_driver_unregister(&bt_power_driver);
	return 0;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Bluetooth power control driver");
MODULE_VERSION("1.0");
