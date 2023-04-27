/*
 *  LCD control code for bm8766
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/lcd.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>

struct bm8766_data {
	struct device *dev;
	int lcd_power;
	struct lcd_device *lcd;
	struct lcd_platform_data *ctrl;
	struct regulator *lcd_vcc_reg;
	struct backlight_device *bd;
};

static int bm8766_set_power(struct lcd_device *lcd, int power)
{
	struct bm8766_data *dev= lcd_get_data(lcd);
	if (!power && dev->lcd_power) {
		if(!regulator_is_enabled(dev->lcd_vcc_reg))
			regulator_enable(dev->lcd_vcc_reg);
		dev->ctrl->power_on(lcd, 1);
	} else if (power && !dev->lcd_power) {
		if (dev->ctrl->reset) {
			dev->ctrl->reset(lcd);
		}
		dev->ctrl->power_on(lcd, 0);
		if(regulator_is_enabled(dev->lcd_vcc_reg))
			regulator_disable(dev->lcd_vcc_reg);
	}
	dev->lcd_power = power;
	return 0;
}

static int bm8766_get_power(struct lcd_device *lcd)
{
	struct bm8766_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int bm8766_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops bm8766_ops = {
	.early_set_power = bm8766_set_power,
	.set_power = bm8766_set_power,
	.get_power = bm8766_get_power,
	.set_mode = bm8766_set_mode,
};

static int bm8766_probe(struct platform_device *pdev)
{
	int ret;
	struct bm8766_data *dev;

	dev = kzalloc(sizeof(struct bm8766_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->dev = &pdev->dev;
	dev->ctrl = pdev->dev.platform_data;
	if (dev->ctrl == NULL) {
		dev_info(&pdev->dev, "no platform data!");
		return -EINVAL;
	}

	dev_set_drvdata(&pdev->dev, dev);

	dev->lcd = lcd_device_register("bm8766_tft", &pdev->dev,
				       dev, &bm8766_ops);
	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device(bm8766) register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device(bm8766) register success\n");
	}

        if (dev->ctrl->power_on)
                dev->ctrl->power_on(dev->lcd, 1);
        dev->lcd_power = FB_BLANK_UNBLANK;

	dev->lcd_vcc_reg = regulator_get(NULL,"lcd_3v3");
	regulator_enable(dev->lcd_vcc_reg);

	return 0;
}

static int bm8766_remove(struct platform_device *pdev)
{
	struct bm8766_data *dev = dev_get_drvdata(&pdev->dev);

	if (dev->lcd_power)
		dev->ctrl->power_on(dev->lcd, 0);
	regulator_put(dev->lcd_vcc_reg);
	lcd_device_unregister(dev->lcd);
	dev_set_drvdata(&pdev->dev, NULL);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM
static int bm8766_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int bm8766_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define bm8766_suspend	NULL
#define bm8766_resume	NULL
#endif

static struct platform_driver bm8766_driver = {
	.driver		= {
		.name	= "bm8766_tft",
		.owner	= THIS_MODULE,
	},
	.probe		= bm8766_probe,
	.remove		= bm8766_remove,
	.suspend	= bm8766_suspend,
	.resume		= bm8766_resume,
};

static int __init bm8766_init(void)
{
	return platform_driver_register(&bm8766_driver);
}
rootfs_initcall(bm8766_init);

static void __exit bm8766_exit(void)
{
	platform_driver_unregister(&bm8766_driver);
}
module_exit(bm8766_exit);

MODULE_DESCRIPTION("bm8766 lcd panel driver");
MODULE_LICENSE("GPL");
