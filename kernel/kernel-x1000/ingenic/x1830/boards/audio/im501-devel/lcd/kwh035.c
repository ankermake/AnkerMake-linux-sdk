/*
 * ingenic_bsp/chip-x2000/fpga/dpu/truly_240_240.c
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
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/digital_pulse_backlight.h>
#include <linux/delay.h>
#include <linux/lcd.h>

#include <mach/jzfb.h>
#include <soc/gpio.h>
#include <board.h>

static struct smart_lcd_data_table kwh035_data_table[] = {
	{SMART_CONFIG_CMD, 0xE0},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x07},
	{SMART_CONFIG_PRM, 0x10},
	{SMART_CONFIG_PRM, 0x09},
	{SMART_CONFIG_PRM, 0x17},
	{SMART_CONFIG_PRM, 0x0B},
	{SMART_CONFIG_PRM, 0x40},
	{SMART_CONFIG_PRM, 0x8A},
	{SMART_CONFIG_PRM, 0x4B},
	{SMART_CONFIG_PRM, 0x0A},
	{SMART_CONFIG_PRM, 0x0D},
	{SMART_CONFIG_PRM, 0x0F},
	{SMART_CONFIG_PRM, 0x15},
	{SMART_CONFIG_PRM, 0x16},
	{SMART_CONFIG_PRM, 0x0F},

	{SMART_CONFIG_CMD, 0xE1},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x1A},
	{SMART_CONFIG_PRM, 0x1B},
	{SMART_CONFIG_PRM, 0x02},
	{SMART_CONFIG_PRM, 0x0D},
	{SMART_CONFIG_PRM, 0x05},
	{SMART_CONFIG_PRM, 0x30},
	{SMART_CONFIG_PRM, 0x35},
	{SMART_CONFIG_PRM, 0x43},
	{SMART_CONFIG_PRM, 0x02},
	{SMART_CONFIG_PRM, 0x0A},
	{SMART_CONFIG_PRM, 0x09},
	{SMART_CONFIG_PRM, 0x32},
	{SMART_CONFIG_PRM, 0x36},
	{SMART_CONFIG_PRM, 0x0F},

	{SMART_CONFIG_CMD, 0xB1},
	{SMART_CONFIG_PRM, 0xA0},
	{SMART_CONFIG_PRM, 0x11},

	{SMART_CONFIG_CMD, 0xB4},
	{SMART_CONFIG_PRM, 0x02},

	{SMART_CONFIG_CMD, 0xC0},
	{SMART_CONFIG_PRM, 0x17},
	{SMART_CONFIG_PRM, 0x15},

	{SMART_CONFIG_CMD, 0xC1},
	{SMART_CONFIG_PRM, 0x41},

	{SMART_CONFIG_CMD, 0xC5},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x0A},
	{SMART_CONFIG_PRM, 0x80},

	{SMART_CONFIG_CMD, 0xB6},
	{SMART_CONFIG_PRM, 0x02},


/**/
	{SMART_CONFIG_CMD, 0x2A},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x01},
	{SMART_CONFIG_PRM, 0xDF},

	{SMART_CONFIG_CMD, 0x2B},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x01},
	{SMART_CONFIG_PRM, 0x3F},
/**/


	{SMART_CONFIG_CMD, 0x36},
	{SMART_CONFIG_PRM, 0x28},

/*
	{SMART_CONFIG_CMD, 0x36},
	{SMART_CONFIG_PRM, 0x48},
*/

	{SMART_CONFIG_CMD, 0x3a},
	{SMART_CONFIG_PRM, 0x56},

	{SMART_CONFIG_CMD, 0xE9},
	{SMART_CONFIG_PRM, 0x00},

	{SMART_CONFIG_CMD, 0XF7},
	{SMART_CONFIG_PRM, 0xA9},
	{SMART_CONFIG_PRM, 0x51},
	{SMART_CONFIG_PRM, 0x2C},
	{SMART_CONFIG_PRM, 0x82},
#ifdef CONFIG_LCD_V14_SLCD_TE
	{SMART_CONFIG_CMD, 0x35},
	{SMART_CONFIG_PRM, 0x00},
#endif
	{SMART_CONFIG_CMD, 0x11},
	{SMART_CONFIG_UDELAY, 120000},
	{SMART_CONFIG_CMD, 0x29},
};

static struct fb_videomode jzfb_kwh035_videomode = {
	.name = "480x320",
	.xres = 480,
	.yres = 320,
	.pixclock = KHZ2PICOS(100000),
};

static struct jzfb_smart_config kwh035_cfg = {
	.frm_md = 0,
	.rdy_switch = 0,
	.rdy_dp = 0,
	.rdy_anti_jit = 0,
#ifndef CONFIG_LCD_V14_SLCD_TE
	.te_switch = 0,
	.te_dp = 0,
	.te_md = 0,
	.te_anti_jit = 0,
#else
	.te_switch = 1,
	.te_dp = 0,
	.te_md = 0,
	.te_anti_jit = 0,
#endif
	.cs_en = 0,
	.cs_dp = 0,
	.dc_md = 0,
	.wr_md = 1,
	.smart_type = SMART_LCD_TYPE_8080,
	.pix_fmt = SMART_LCD_FORMAT_888,
	.dwidth = SMART_LCD_DWIDTH_8_BIT,
	.cwidth = SMART_LCD_CWIDTH_8_BIT,

	.write_gram_cmd = 0x2c,
	.data_table = kwh035_data_table,
	.length_data_table = ARRAY_SIZE(kwh035_data_table),
};

static unsigned int lcd_power_inited;

static int kwh035_gpio_init(void)
{
	int ret;

	ret = gpio_request(GPIO_LCD_RST, "lcd rst");
	if (ret) {
		printk(KERN_ERR "can's request lcd rst gpio\n");
		return ret;
	}

	ret = gpio_request(GPIO_LCD_CS, "lcd cs");
	if (ret) {
		printk(KERN_ERR "can's request lcd cs gpio\n");
		return ret;
	}

	ret = gpio_request(GPIO_LCD_RD, "lcd rd");
	if (ret) {
		printk(KERN_ERR "can's request lcd rd gpio\n");
		return ret;
	}

	ret = gpio_request(GPIO_LCD_POWER_EN, "lcd power en");
	if (ret) {
		printk(KERN_ERR "can's request lcd power gpio\n");
		return ret;
	}

	lcd_power_inited = 1;

	return 0;
}

static int kwh035_power_on(void *data)
{
	if (!lcd_power_inited && kwh035_gpio_init())
		return -EFAULT;

	gpio_direction_output(GPIO_LCD_POWER_EN, 0);
	gpio_direction_output(GPIO_LCD_CS, 1);
	gpio_direction_output(GPIO_LCD_RD, 1);
	gpio_direction_output(GPIO_LCD_RST, 0);
	mdelay(20);
	gpio_direction_output(GPIO_LCD_RST, 1);
	mdelay(120);
	gpio_direction_output(GPIO_LCD_CS, 0);

	return 0;
}

static int kwh035_power_off(void *data)
{
	if (!lcd_power_inited && kwh035_gpio_init())
		return -EFAULT;

	gpio_direction_output(GPIO_LCD_CS, 1);
	gpio_direction_output(GPIO_LCD_RD, 1);
	gpio_direction_output(GPIO_LCD_RST, 0);
	gpio_direction_output(GPIO_LCD_POWER_EN, 1);

	return 0;
}

static struct lcd_callback_ops lcd_callback = {
	.lcd_power_on_begin = kwh035_power_on,
	.lcd_power_off_begin = kwh035_power_off,
};

struct jzfb_platform_data jzfb_pdata = {
	.num_modes = 1,
	.modes = &jzfb_kwh035_videomode,
	.lcd_type = LCD_TYPE_SLCD,
	.width = 480,
	.height = 320,

	.smart_config = &kwh035_cfg,
	.lcd_callback_ops = &lcd_callback,
};

/****************power and  backlight*********************/

static struct platform_pwm_backlight_data backlight_data = {
	.pwm_id = 3,
	.pwm_active_level = 1,
	.max_brightness = 255,
	.dft_brightness = 120,
	.pwm_period_ns = 30000,
};

struct platform_device backlight_device = {
	.name = "pwm-backlight",
	.dev = {
		.platform_data = &backlight_data,
	},
};

static int __init jzfb_backlight_init(void)
{
	platform_device_register(&backlight_device);
	return 0;
}

static void __exit jzfb_backlight_exit(void)
{
	platform_device_unregister(&backlight_device);
}

module_init(jzfb_backlight_init);
module_exit(jzfb_backlight_exit)
