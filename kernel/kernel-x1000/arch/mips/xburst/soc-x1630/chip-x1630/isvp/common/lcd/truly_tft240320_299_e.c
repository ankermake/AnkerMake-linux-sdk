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

static int truly_slcd240320_on_begin(struct lcdc_gpio_struct *gpio)
{
	return 0;
}

static int truly_slcd240320_off_begin(struct lcdc_gpio_struct *gpio)
{

	return 0;
}

struct lcd_callback_ops truly_slcd240320_ops = {
	.lcd_power_on_begin  = (void*)truly_slcd240320_on_begin,
	.lcd_power_off_begin = (void*)truly_slcd240320_off_begin,
};

static struct smart_lcd_data_table truly_slcd240320_data_table[] = {
	/* LCD init code */
	{SMART_CONFIG_CMD, 0x11},
	{SMART_CONFIG_UDELAY, 500000},	/* sleep out 500 ms  */

	{SMART_CONFIG_CMD, 0x36},
	{SMART_CONFIG_PRM, 0x00},

	{SMART_CONFIG_CMD, 0x3A},
	{SMART_CONFIG_PRM, 0x05},
	//{SMART_CONFIG_PRM, 0x06}, //RGB666

	{SMART_CONFIG_CMD, 0xB2},
	{SMART_CONFIG_PRM, 0x0C},
	{SMART_CONFIG_PRM, 0x0C},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x33},
	{SMART_CONFIG_PRM, 0x33},

	{SMART_CONFIG_CMD, 0xB7},
	{SMART_CONFIG_PRM, 0x35},

	{SMART_CONFIG_CMD, 0xBB},
	{SMART_CONFIG_PRM, 0x14},

	{SMART_CONFIG_CMD, 0xC0},
	{SMART_CONFIG_PRM, 0x2C},

	{SMART_CONFIG_CMD, 0xC2},
	{SMART_CONFIG_PRM, 0x01},

	{SMART_CONFIG_CMD, 0xC3},
	{SMART_CONFIG_PRM, 0x17},

	{SMART_CONFIG_CMD, 0xC4},
	{SMART_CONFIG_PRM, 0x20},

	{SMART_CONFIG_CMD, 0xE4},
	{SMART_CONFIG_PRM, 0x27},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x00},

	{SMART_CONFIG_CMD, 0xC6},
	{SMART_CONFIG_PRM, 0x0F},

	{SMART_CONFIG_CMD, 0xD0},
	{SMART_CONFIG_PRM, 0xA4},
	{SMART_CONFIG_PRM, 0xA1},

	{SMART_CONFIG_CMD, 0xE0},
	{SMART_CONFIG_PRM, 0xD0},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x14},
	{SMART_CONFIG_PRM, 0x15},
	{SMART_CONFIG_PRM, 0x13},
	{SMART_CONFIG_PRM, 0x2C},
	{SMART_CONFIG_PRM, 0x42},
	{SMART_CONFIG_PRM, 0x43},
	{SMART_CONFIG_PRM, 0x4E},
	{SMART_CONFIG_PRM, 0x09},
	{SMART_CONFIG_PRM, 0x16},
	{SMART_CONFIG_PRM, 0x14},
	{SMART_CONFIG_PRM, 0x18},
	{SMART_CONFIG_PRM, 0x21},

	{SMART_CONFIG_CMD, 0xE1},
	{SMART_CONFIG_PRM, 0xD0},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x14},
	{SMART_CONFIG_PRM, 0x15},
	{SMART_CONFIG_PRM, 0x13},
	{SMART_CONFIG_PRM, 0x0B},
	{SMART_CONFIG_PRM, 0x43},
	{SMART_CONFIG_PRM, 0x55},
	{SMART_CONFIG_PRM, 0x53},
	{SMART_CONFIG_PRM, 0x0C},
	{SMART_CONFIG_PRM, 0x17},
	{SMART_CONFIG_PRM, 0x14},
	{SMART_CONFIG_PRM, 0x23},
	{SMART_CONFIG_PRM, 0x20},

	{SMART_CONFIG_CMD, 0xE9},
	{SMART_CONFIG_PRM, 0x11},
	{SMART_CONFIG_PRM, 0x11},
	{SMART_CONFIG_PRM, 0x03},

	{SMART_CONFIG_CMD, 0x20},
	{SMART_CONFIG_CMD, 0x29},
	{SMART_CONFIG_UDELAY, 50000},
};

struct fb_videomode jzfb_truly_240_320_videomode = {
	.name = "240x320",
	.refresh = 60,
	.xres = 240,
	.yres = 320,
	.pixclock = KHZ2PICOS(30000),
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.hsync_len = 0,
	.vsync_len = 0,
	.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

struct jzfb_smart_config truly_240_320_cfg = {
	.frm_md = 0,
	.rdy_switch = 0,
	.rdy_dp = 1,
	.rdy_anti_jit = 0,
	.te_switch = 0,
	.te_dp = 1,
	.te_md = 0,
	.te_anti_jit = 1,
	.cs_en = 0,
	.cs_dp = 0,
	.dc_md = 0,
	.wr_md = 1,
	.smart_type = SMART_LCD_TYPE_8080,
	.pix_fmt = SMART_LCD_FORMAT_565,
	.dwidth = SMART_LCD_DWIDTH_16_BIT,
	.cwidth = SMART_LCD_CWIDTH_8_BIT,

	.write_gram_cmd = 0x2c,
	.data_table = truly_slcd240320_data_table,
	.length_data_table = ARRAY_SIZE(truly_slcd240320_data_table),
};

struct jzfb_platform_data jzfb_pdata = {
	.num_modes = 1,
	.modes = &jzfb_truly_240_320_videomode,
	.lcd_type = LCD_TYPE_SLCD,
	.width = 240,
	.height = 320,

	.smart_config = &truly_240_320_cfg,

	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,
	.lcd_callback_ops = &truly_slcd240320_ops,
};

/****************power and  backlight*********************/

#define GPIO_LCD_BL_EN	GPIO_PB(17)
#define GPIO_LCD_CS	GPIO_PD(18)
#define GPIO_LCD_RD	GPIO_PD(26)
#define GPIO_LCD_RST	GPIO_PB(19)

struct truly_slcd240320_power {
	struct regulator *vlcdio;
	struct regulator *vlcdvcc;
	int inited;
};

static struct truly_slcd240320_power lcd_power = {
	NULL,
	NULL,
	0
};

static int truly_slcd240320_power_init(void)
{
	int ret;

	ret = gpio_request(GPIO_LCD_RST, "lcd rst");
	if (ret) {
		printk(KERN_ERR "can's request lcd rst\n");
		return ret;
	}

	ret = gpio_request(GPIO_LCD_CS, "lcd cs");
	if (ret) {
		printk(KERN_ERR "can's request lcd cs\n");
		return ret;
	}

	ret = gpio_request(GPIO_LCD_RD, "lcd rd");
	if (ret) {
		printk(KERN_ERR "can's request lcd rd\n");
		return ret;
	}

	ret = gpio_request(GPIO_LCD_BL_EN, "lcd backlight en");
	if (ret) {
		printk(KERN_ERR "can's request lcd rd\n");
		return ret;
	}

	lcd_power.inited = 1;

	return 0;
}

static int truly_slcd240320_power_reset(struct lcd_device *ld)
{
	if (!lcd_power.inited)
		return -EFAULT;
	gpio_direction_output(GPIO_LCD_RST, 0);
	mdelay(20);
	gpio_direction_output(GPIO_LCD_RST, 1);
	mdelay(10);

	return 0;
}

static int truly_slcd240320_power_on(struct lcd_device *ld, int enable)
{
	if (!lcd_power.inited && truly_slcd240320_power_init())
		return -EFAULT;

	if (enable) {
		gpio_direction_output(GPIO_LCD_CS, 1);
		gpio_direction_output(GPIO_LCD_RD, 1);
		mdelay(5);
		gpio_direction_output(GPIO_LCD_CS, 0);
		gpio_direction_output(GPIO_LCD_BL_EN, 1);
	} else {
		gpio_direction_output(GPIO_LCD_BL_EN, 0);
		gpio_direction_output(GPIO_LCD_CS, 0);
		gpio_direction_output(GPIO_LCD_RD, 0);
		gpio_direction_output(GPIO_LCD_RST, 0);
	}

	return 0;
}

static struct lcd_platform_data truly_slcd240320_pdata = {
	.reset    = truly_slcd240320_power_reset,
	.power_on = truly_slcd240320_power_on,
};

/* LCD Panel Device */
static struct platform_device truly_slcd240320_device = {
	.name           = "truly_tft240320_slcd",
	.dev            = {
		.platform_data  = &truly_slcd240320_pdata,
	},
};

int __init jzfb_backlight_init(void)
{
	platform_device_register(&truly_slcd240320_device);
	return 0;
}

static void __exit jzfb_backlight_exit(void)
{
	platform_device_unregister(&truly_slcd240320_device);
}
rootfs_initcall(jzfb_backlight_init);
module_exit(jzfb_backlight_exit)
