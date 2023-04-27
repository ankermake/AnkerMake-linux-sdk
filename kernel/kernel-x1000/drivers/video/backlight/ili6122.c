/*
 *  LCD control code for ili6122
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

struct ili6122_data {
    struct device *dev;
    struct lcd_device *lcd;
    struct lcd_platform_data *ctrl;
    struct regulator *lcd_vcc_reg;
    struct backlight_device *bd;
    int lcd_power;
};

extern int lcd_display_inited_by_uboot( void );

static int ili6122_set_power(struct lcd_device *lcd, int power)
{
    struct ili6122_data *dev = lcd_get_data(lcd);

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

static int ili6122_early_set_power(struct lcd_device *lcd, int power)
{
    return ili6122_set_power(lcd, power);
}

static int ili6122_get_power(struct lcd_device *lcd)
{
    struct ili6122_data *dev= lcd_get_data(lcd);

    return dev->lcd_power;
}

static int ili6122_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
    return 0;
}

static struct lcd_ops ili6122_ops = {
    .early_set_power        = ili6122_early_set_power,
    .set_power              = ili6122_set_power,
    .get_power              = ili6122_get_power,
    .set_mode               = ili6122_set_mode,
};

static int ili6122_probe(struct platform_device *pdev)
{
    int ret;
    struct ili6122_data *dev;

    dev = kzalloc(sizeof(struct ili6122_data), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->dev = &pdev->dev;
    dev->ctrl = pdev->dev.platform_data;
    if (dev->ctrl == NULL) {
        dev_info(&pdev->dev, "no platform data!\n");
        return -EINVAL;
    }

    dev_set_drvdata(&pdev->dev, dev);

    dev->lcd = lcd_device_register("ili6122_tft", &pdev->dev, dev, &ili6122_ops);
    if (IS_ERR(dev->lcd)) {
        ret = PTR_ERR(dev->lcd);
        dev->lcd = NULL;
        dev_info(&pdev->dev, "lcd device(ili6122) register error: %d\n", ret);
    } else {
        dev_info(&pdev->dev, "lcd device(ili6122) register success\n");
    }

    /* default the bachlight poweroff */
    dev->lcd_power = FB_BLANK_POWERDOWN;

    return 0;
}

static int ili6122_remove(struct platform_device *pdev)
{
    struct ili6122_data *dev = dev_get_drvdata(&pdev->dev);

    if (dev->lcd_power)
        dev->ctrl->power_on(dev->lcd, 0);

    lcd_device_unregister(dev->lcd);
    dev_set_drvdata(&pdev->dev, NULL);
    kfree(dev);

    return 0;
}

#ifdef CONFIG_PM

static int ili6122_suspend(struct platform_device *pdev, pm_message_t state)
{
    return 0;
}

static int ili6122_resume(struct platform_device *pdev)
{
    return 0;
}

#else

#define ili6122_suspend                 NULL
#define ili6122_resume                  NULL

#endif

static struct platform_driver ili6122_driver = {
    .driver     = {
        .name   = "ili6122_tft",
        .owner  = THIS_MODULE,
    },
    .probe              = ili6122_probe,
    .remove             = ili6122_remove,
    .suspend            = ili6122_suspend,
    .resume             = ili6122_resume,
};

static int __init ili6122_init(void)
{
    return platform_driver_register(&ili6122_driver);
}


static void __exit ili6122_exit(void)
{
    platform_driver_unregister(&ili6122_driver);
}

rootfs_initcall(ili6122_init);
module_exit(ili6122_exit);

MODULE_DESCRIPTION("ili6122 lcd panel driver");
MODULE_LICENSE("GPL");
