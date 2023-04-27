/*
 * Copyright (c) 2015 Engenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * JZ_x1000 fpga board lcd setup routines.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/lcd.h>
#include <linux/regulator/consumer.h>
#include <mach/jzfb.h>
#include "../board_base.h"

/*ifdef is 18bit,6-6-6 ,ifndef default 5-6-6*/
//#define CONFIG_SLCD_TRULY_18BIT

#ifdef	CONFIG_SLCD_TRULY_18BIT
static int slcd_inited = 1;
#else
static int slcd_inited = 0;
#endif

struct qshy500q4011g_power{
	struct regulator *vlcdio;
	struct regulator *vlcdvcc;
	int inited;
};

static struct qshy500q4011g_power lcd_power = {
	NULL,
    NULL,
    0
};

int qshy500q4011g_power_init(struct lcd_device *ld)
{
    int ret ;
    if(GPIO_LCD_RST > 0){
        ret = gpio_request(GPIO_LCD_RST, "lcd rst");
        if (ret) {
            printk(KERN_ERR "can's request lcd rst\n");
            return ret;
        }
    }
    if(GPIO_LCD_CS > 0){
        ret = gpio_request(GPIO_LCD_CS, "lcd cs");
        if (ret) {
            printk(KERN_ERR "can's request lcd cs\n");
            return ret;
        }
    }
    if(GPIO_LCD_RD > 0){
        ret = gpio_request(GPIO_LCD_RD, "lcd rd");
        if (ret) {
            printk(KERN_ERR "can's request lcd rd\n");
            return ret;
        }
    }
    lcd_power.inited = 1;
    return 0;
}

int qshy500q4011g_power_reset(struct lcd_device *ld)
{
	if (!lcd_power.inited)
		return -EFAULT;
	gpio_direction_output(GPIO_LCD_RST, 0);
	mdelay(20);
	gpio_direction_output(GPIO_LCD_RST, 1);
	mdelay(10);

	return 0;
}

int qshy500q4011g_power_on(struct lcd_device *ld, int enable)
{
	if (!lcd_power.inited && qshy500q4011g_power_init(ld))
		return -EFAULT;

	if (enable) {
		gpio_direction_output(GPIO_LCD_CS, 1);
		gpio_direction_output(GPIO_LCD_RD, 1);

		qshy500q4011g_power_reset(ld);

		mdelay(5);
		gpio_direction_output(GPIO_LCD_CS, 0);

	} else {
		mdelay(5);
		gpio_direction_output(GPIO_LCD_CS, 0);
		gpio_direction_output(GPIO_LCD_RD, 0);
		gpio_direction_output(GPIO_LCD_RST, 0);
		slcd_inited = 0;
	}
	return 0;
}

static struct lcd_platform_data qshy500q4011g_pdata = {
	 .reset = qshy500q4011g_power_reset,
	 .power_on = qshy500q4011g_power_on,
};

/* LCD Panel Device */
struct platform_device qshy500q4011g_device = {
	.name = "qshy500q4011g_slcd",
	.dev = {
		.platform_data = &qshy500q4011g_pdata,
		},
};

static struct smart_lcd_data_table qshy500q4011g_data_table[] = {
	{SMART_CONFIG_CMD,	0x00E2},{SMART_CONFIG_DATA,	0x001D},{SMART_CONFIG_DATA,	0x0002},
									{SMART_CONFIG_DATA,     0x0054},
	{SMART_CONFIG_CMD,	0x00E0},{SMART_CONFIG_DATA,	0x0001},{SMART_CONFIG_UDELAY,	120000},
	{SMART_CONFIG_CMD,	0x00E0},{SMART_CONFIG_DATA,	0x0003},{SMART_CONFIG_UDELAY,	120000},
	{SMART_CONFIG_CMD,	0x0001},{SMART_CONFIG_UDELAY,   120000},
	{SMART_CONFIG_CMD,	0x00E6},{SMART_CONFIG_DATA,	0x0007},{SMART_CONFIG_DATA,	0x00A1},
									{SMART_CONFIG_DATA,     0x0020},
	{SMART_CONFIG_CMD,	0x00B0},{SMART_CONFIG_DATA,	0x0020},{SMART_CONFIG_DATA,	0x0000},//24bit
									{SMART_CONFIG_DATA,	(799 >> 8) & 0x00FF},
									{SMART_CONFIG_DATA,	799 & 0x00FF},
									{SMART_CONFIG_DATA,	(479 >> 8) & 0x00FF},
									{SMART_CONFIG_DATA,	479 & 0x00FF},
									{SMART_CONFIG_DATA,     0x0000},
	{SMART_CONFIG_CMD,	0x00B4},{SMART_CONFIG_DATA,	(1200 >> 8) & 0x00FF},
									{SMART_CONFIG_DATA,	1200 & 0x00FF},
									{SMART_CONFIG_DATA,	(93 >> 8) & 0x00FF},
									{SMART_CONFIG_DATA,	93 & 0x00FF},
									{SMART_CONFIG_DATA,	40},
									{SMART_CONFIG_DATA,	(46 >> 8) & 0x00FF},
									{SMART_CONFIG_DATA,	46 & 0x00FF},
									{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x00B6},{SMART_CONFIG_DATA,	(670 >> 8) & 0x00FF},
									{SMART_CONFIG_DATA,	670 & 0x00FF},
									{SMART_CONFIG_DATA,	(43 >> 8) & 0x00FF},
									{SMART_CONFIG_DATA,	43 & 0x00FF},
									{SMART_CONFIG_DATA,	20},
									{SMART_CONFIG_DATA,	(23 >> 8) & 0x00FF},
									{SMART_CONFIG_DATA,	23 & 0x00FF},
	{SMART_CONFIG_CMD,	0x00BA},{SMART_CONFIG_DATA,	0x000D},
	{SMART_CONFIG_CMD,	0x00B8},{SMART_CONFIG_DATA,	0x0007},{SMART_CONFIG_DATA,	0x0001},
	{SMART_CONFIG_CMD,	0x0036},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x00F0},{SMART_CONFIG_DATA,	0x0000},{SMART_CONFIG_UDELAY,   120000},//8bit
	{SMART_CONFIG_CMD,      0x002A},{SMART_CONFIG_DATA,     0x0000},{SMART_CONFIG_DATA,     0x0000},
									{SMART_CONFIG_DATA,     799 >> 8},
									{SMART_CONFIG_DATA,     799 & 0x00FF},
	{SMART_CONFIG_CMD,      0x002B},{SMART_CONFIG_DATA,     0x0000},{SMART_CONFIG_DATA,     0x0000},
									{SMART_CONFIG_DATA,     479 >> 8},
									{SMART_CONFIG_DATA,     479 & 0x00FF},
	{SMART_CONFIG_CMD,	0x00BE},{SMART_CONFIG_DATA,	0x0006},{SMART_CONFIG_DATA,	0x00FF},
									{SMART_CONFIG_DATA,     0x0001},
									{SMART_CONFIG_DATA,     0x00FF},
									{SMART_CONFIG_DATA,     0x0000},
									{SMART_CONFIG_DATA,     0x0000},
	{SMART_CONFIG_CMD,	0x00D0},{SMART_CONFIG_DATA,	0x000D},
	{SMART_CONFIG_CMD,      0x002C},
};

unsigned long qshy500q4011g_cmd_buf[] = {
	0x2c2c2c2c,
};

struct fb_videomode jzfb_videomode = {
	.name = "800x480",
	.refresh = 60,
	.xres = 800,
	.yres = 480,
	.pixclock = KHZ2PICOS(92000),
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

struct jzfb_platform_data jzfb_pdata = {

	.num_modes = 1,
	.modes = &jzfb_videomode,
	.lcd_type = LCD_TYPE_SLCD,
	.bpp = 24,
	.width = 108,
	.height = 64,
	.pinmd = 0,

	.smart_config.rsply_cmd_high = 0,
	.smart_config.csply_active_high = 0,
	.smart_config.newcfg_fmt_conv = 1,
	.smart_config.slcd_clear = 1,
	.smart_config.display_on = 0x29,

	.smart_config.write_gram_cmd = qshy500q4011g_cmd_buf,
	.smart_config.length_cmd = ARRAY_SIZE(qshy500q4011g_cmd_buf),
	.smart_config.bus_width = 8,
	.smart_config.data_table = qshy500q4011g_data_table,
	.smart_config.length_data_table = ARRAY_SIZE(qshy500q4011g_data_table),
	.dither_enable = 0,
};

#if defined(GPIO_BL_PWR_EN) && defined(GPIO_BL_PWR_EN_LEVEL)
struct bl_pwr_pin bl_pwr_en_pin = {
        .num = GPIO_BL_PWR_EN,
        .enable_level = GPIO_BL_PWR_EN_LEVEL,
};
#endif
