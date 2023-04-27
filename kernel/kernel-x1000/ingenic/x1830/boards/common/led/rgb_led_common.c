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
#include <common.h>
#include <linux/slab.h>
#include <board.h>
#include <linux/platform_device.h>

#define DEVNAME "rgb_leds"

struct rgb_led_gpio {
	int r_led_gpio;
	int g_led_gpio;
	int b_led_gpio;
};

static struct rgb_led_gpio led_gpio_data = {
		.r_led_gpio = GPIO_LED_R,
		.g_led_gpio = GPIO_LED_G,
		.b_led_gpio = GPIO_LED_B,
};

static struct platform_device led_dev = {
    .name = DEVNAME,
    .id = -1,
    .dev = {
        .platform_data = &led_gpio_data,
    },
};

static int __init rgb_led_init(void)
{
	int ret;

	ret = platform_device_register(&led_dev);
	if (ret < 0)
		pr_err("failed to register led_dev\n");

	return ret;
}

static void __exit rgb_led_exit(void)
{
	platform_device_unregister(&led_dev);
}

module_init(rgb_led_init);
module_exit(rgb_led_exit);
MODULE_LICENSE("GPL");
