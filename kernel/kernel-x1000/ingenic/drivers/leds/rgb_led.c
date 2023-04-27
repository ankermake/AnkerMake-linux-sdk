/*
 *  Copyright (C) 2018 Hu lianqin <lianqin.hu@ingenic.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <common.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>

#define DEVNAME "rgb_leds"

#define LEDTYPE 'D'
#define LED_GET_BRIGHT _IOR(LEDTYPE, 1, int)
#define LED_SET_BRIGHT _IOW(LEDTYPE, 2, int)

struct rgb_led_gpio {
	int r_led_gpio;
	int g_led_gpio;
	int b_led_gpio;
};

struct led_bright {
	unsigned char r_val;
	unsigned char g_val;
	unsigned char b_val;
};

struct led_drv {
	struct rgb_led_gpio *led_gpio;
	struct miscdevice misc;
	struct file_operations fops;
	struct mutex mutex;
};

static struct led_drv *drv_data;

static int get_led_bri_value(struct led_bright *bright_value)
{
	if (!bright_value)
		return -1;

	mutex_lock(&drv_data->mutex);
	bright_value->r_val = !gpio_get_value(drv_data->led_gpio->r_led_gpio);
	bright_value->g_val = !gpio_get_value(drv_data->led_gpio->g_led_gpio);
	bright_value->b_val = !gpio_get_value(drv_data->led_gpio->b_led_gpio);
	mutex_unlock(&drv_data->mutex);

	return 0;
}

static int set_led_bri_value(struct led_bright *bright_value)
{
	int ret = 0;
	struct led_bright bright_data = {0};
	if (!bright_value)
		return -1;

	mutex_lock(&drv_data->mutex);
	ret = copy_from_user(&bright_data, bright_value, sizeof(*bright_value));
	assert(!ret);

	gpio_set_value(drv_data->led_gpio->r_led_gpio, !bright_data.r_val);
	gpio_set_value(drv_data->led_gpio->g_led_gpio, !bright_data.g_val);
	gpio_set_value(drv_data->led_gpio->b_led_gpio, !bright_data.b_val);
	mutex_unlock(&drv_data->mutex);

	return 0;
}

static long led_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	int ret = 0;

	if (_IOC_TYPE(cmd) == LEDTYPE) {
		switch (cmd) {
		case LED_GET_BRIGHT:
			ret = get_led_bri_value((struct led_bright *)args);
			break;

		case LED_SET_BRIGHT:
			ret = set_led_bri_value((struct led_bright *)args);
			break;

		default:
			return -EINVAL;
		}
	}

	return ret;
}

static int rgb_led_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct led_drv *pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -1;

	drv_data = pdata;
	pdata->led_gpio = (struct rgb_led_gpio *)pdev->dev.platform_data;

	if (pdata->led_gpio->r_led_gpio < 0 || pdata->led_gpio->g_led_gpio < 0
			|| pdata->led_gpio->b_led_gpio < 0) {
		kfree(pdata);
		return -1;
	}

	ret = gpio_request(pdata->led_gpio->r_led_gpio, "r_led_gpio");
	if (ret < 0)
		dev_err(&pdev->dev, "ERROR: failed to request r_led_gpio %d\n", pdata->led_gpio->r_led_gpio);
	gpio_direction_output(pdata->led_gpio->r_led_gpio, 1);

	ret = gpio_request(pdata->led_gpio->g_led_gpio, "g_led_gpio");
	if (ret < 0)
		dev_err(&pdev->dev, "ERROR: failed to request g_led_gpio %d\n", pdata->led_gpio->g_led_gpio);
	gpio_direction_output(pdata->led_gpio->g_led_gpio, 1);

	ret = gpio_request(pdata->led_gpio->b_led_gpio, "b_led_gpio");
	if (ret < 0)
		dev_err(&pdev->dev, "ERROR: failed to request b_led_gpio %d\n", pdata->led_gpio->b_led_gpio);
	gpio_direction_output(pdata->led_gpio->b_led_gpio, 1);

	pdata->fops.unlocked_ioctl = led_ioctl;

	pdata->misc.minor = MISC_DYNAMIC_MINOR;
	pdata->misc.name = "rgb_led";
	pdata->misc.fops = &pdata->fops;

	ret = misc_register(&pdata->misc);
	if (ret < 0)
		dev_err(&pdev->dev, "ERROR: failed to register misc led_drvier\n");

	mutex_init(&pdata->mutex);
	platform_set_drvdata(pdev, pdata);

	return 0;
}

static int rgb_led_remove(struct platform_device *pdev)
{
	struct led_drv *pdata = platform_get_drvdata(pdev);

	gpio_free(pdata->led_gpio->r_led_gpio);
	gpio_free(pdata->led_gpio->g_led_gpio);
	gpio_free(pdata->led_gpio->b_led_gpio);
	kfree(pdata);

	return 0;
}

static struct platform_device_id id_tables[] = {
    {DEVNAME, 0},
    {"rgb_leds_in", 1}
};

static struct platform_driver led_drv_data = {
		.probe = rgb_led_probe,
		.remove = rgb_led_remove,
		.driver = {
				.name = DEVNAME,
		},
		.id_table = id_tables,
};

static int __init rgb_led_init(void)
{
	int ret;

	ret = platform_driver_register(&led_drv_data);
	if (ret < 0)
		pr_err("ERROR: failed to register led_drv\n");

	return ret;
}

static void __exit rgb_led_exit(void)
{
	platform_driver_unregister(&led_drv_data);
}

module_init(rgb_led_init);
module_exit(rgb_led_exit);

MODULE_LICENSE("GPL");
