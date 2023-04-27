/*
 *  LCD control code for truly
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/fb.h>
#include <linux/regulator/consumer.h>

struct frd20024n_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct lcd_platform_data *ctrl;
};

static int frd20024n_set_power(struct lcd_device *lcd, int power)
{
	struct frd20024n_data *dev= lcd_get_data(lcd);

	if (!power && dev->lcd_power) {
		dev->ctrl->power_on(lcd, 1);
	} else if (power && !dev->lcd_power) {
		if (dev->ctrl->reset) {
			dev->ctrl->reset(lcd);
		}
		dev->ctrl->power_on(lcd, 0);
	}
	dev->lcd_power = power;
	return 0;
}

static int frd20024n_get_power(struct lcd_device *lcd)
{
	struct frd20024n_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int frd20024n_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops frd20024n_ops = {
	.set_power = frd20024n_set_power,
	.get_power = frd20024n_get_power,
	.set_mode = frd20024n_set_mode,
};

static int frd20024n_probe(struct platform_device *pdev)
{
	int ret;
	struct frd20024n_data *dev;

	dev = kzalloc(sizeof(struct frd20024n_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->ctrl = pdev->dev.platform_data;
	if (dev->ctrl == NULL) {
		dev_info(&pdev->dev, "no platform data!");
		return -EINVAL;
	}

	dev_set_drvdata(&pdev->dev, dev);

	dev->lcd = lcd_device_register("frd20024n_slcd", &pdev->dev,
				       dev, &frd20024n_ops);
	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device(FRD20024N) register success\n");
	}

	if (dev->ctrl->power_on) {
		dev->ctrl->power_on(dev->lcd, 1);
		dev->lcd_power = FB_BLANK_UNBLANK;
	}

	return 0;
}

static int frd20024n_remove(struct platform_device *pdev)
{
	struct frd20024n_data *dev = dev_get_drvdata(&pdev->dev);

	if (dev->lcd_power)
		dev->ctrl->power_on(dev->lcd, 0);

	lcd_device_unregister(dev->lcd);
	dev_set_drvdata(&pdev->dev, NULL);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM
static int frd20024n_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int frd20024n_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define frd20024n_suspend	NULL
#define frd20024n_resume	NULL
#endif

static struct platform_driver frd20024n_driver = {
	.probe		= frd20024n_probe,
	.remove		= frd20024n_remove,
	.suspend	= frd20024n_suspend,
	.resume		= frd20024n_resume,
	.driver	= {
		.name	= "frd20024n_slcd",
		.owner	= THIS_MODULE,
	},
};

static int __init frd20024n_init(void)
{
	return platform_driver_register(&frd20024n_driver);
}
#ifdef CONFIG_EARLY_INIT_RUN
rootfs_initcall(frd20024n_init);
#else
module_init(frd20024n_init);
#endif

static void __exit frd20024n_exit(void)
{
	platform_driver_unregister(&frd20024n_driver);
}
module_exit(frd20024n_exit);

MODULE_DESCRIPTION("XRM2002903 lcd panel driver");
MODULE_LICENSE("GPL");
