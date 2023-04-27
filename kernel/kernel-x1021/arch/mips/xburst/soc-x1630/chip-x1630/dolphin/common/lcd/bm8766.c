/*
 * bm8766.c
 *
 * Copyright (C) 2016 Ingenic Semiconductor Inc.
 *
 * Author:clwang<chunlei.wang@ingenic.com>
 *
 * This program is free software, you can redistribute it and/or modify it
 *
 * under the terms of the GNU General Public License version 2 as published by
 *
 * the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/digital_pulse_backlight.h>
#include <linux/lcd.h>

#include <mach/jzfb.h>
#include <soc/gpio.h>

static int bm8766_power_on_begin(struct lcdc_gpio_struct *gpio)
{
	return 0;
}

static int bm8766_power_off_begin(struct lcdc_gpio_struct *gpio)
{
	return 0;
}

static struct lcd_callback_ops bm8766_ops = {
	.lcd_power_on_begin  = (void*)bm8766_power_on_begin,
	.lcd_power_off_begin = (void*)bm8766_power_off_begin,
};

static struct fb_videomode jzfb_bm8766_videomode = {
	.name = "800x480",
	.refresh = 60,
	.xres = 800,
	.yres = 480,
	.pixclock = KHZ2PICOS(33264),
	.left_margin = 88,
	.right_margin = 40,
	.upper_margin = 8,
	.lower_margin = 35,
	.hsync_len = 128,
	.vsync_len = 2,
	.sync = ~FB_SYNC_HOR_HIGH_ACT & ~FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

static struct jzfb_tft_config bm8766_cfg = {
	.pix_clk_inv = 0,
	.de_dl = 0,
	.sync_dl = 0,
	.color_even = TFT_LCD_COLOR_EVEN_RGB,
	.color_odd = TFT_LCD_COLOR_ODD_RGB,
	.mode = TFT_LCD_MODE_PARALLEL_24B,
};

struct jzfb_platform_data jzfb_pdata = {
	.num_modes = 1,
	.modes = &jzfb_bm8766_videomode,
	.lcd_type = LCD_TYPE_TFT,
	.bpp = 24,
	.width = 800,
	.height = 480,

	.tft_config = &bm8766_cfg,

	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,
	.lcd_callback_ops = &bm8766_ops,
};


/****************power and  backlight*********************/

#define GPIO_LCD_DISP_EN	GPIO_PB(19)
#define GPIO_LCD_BL_EN		GPIO_PB(22)

struct bm8766_power {
	struct regulator *vlcdio;
	struct regulator *vlcdvcc;
	int inited;
};

static struct bm8766_power lcd_power = {
	NULL,
	NULL,
	0
};

static int bm8766_power_init(void)
{
	int ret = 0;

	ret = gpio_request(GPIO_LCD_DISP_EN, "lcd disp");
	if (ret) {
		printk(KERN_ERR "can't request lcd disp\n");
		goto failed;
	}

	ret = gpio_request(GPIO_LCD_BL_EN, "lcd bl");
	if (ret) {
		printk(KERN_ERR "can't request lcd bl\n");
		goto failed;
	}

	lcd_power.inited = 1;

failed:
	return ret;
}

static int bm8766_power_reset(struct lcd_device *ld)
{
	if (!lcd_power.inited)
		return -EFAULT;

	return 0;
}

static int bm8766_power_on(struct lcd_device *ld, int enable)
{
	if (!lcd_power.inited && bm8766_power_init())
		return -EFAULT;

	if (enable) {
		gpio_direction_output(GPIO_LCD_BL_EN, 1);
		mdelay(1);
		gpio_direction_output(GPIO_LCD_DISP_EN, 1);
	} else {
		gpio_direction_output(GPIO_LCD_DISP_EN, 0);
		mdelay(1);
		gpio_direction_output(GPIO_LCD_BL_EN, 0);
	}

	return 0;
}

static struct lcd_platform_data bm8766_pdata = {
	.reset    = bm8766_power_reset,
	.power_on = bm8766_power_on,
};

/* LCD Panel Device */
static struct platform_device bm8766_device = {
	.name           = "bm8766_tft",
	.dev            = {
		.platform_data  = &bm8766_pdata,
	},
};

int __init jzfb_backlight_init(void)
{
	platform_device_register(&bm8766_device);
	return 0;
}

static void __exit jzfb_backlight_exit(void)
{
	platform_device_unregister(&bm8766_device);
}
rootfs_initcall(jzfb_backlight_init);
module_exit(jzfb_backlight_exit);
