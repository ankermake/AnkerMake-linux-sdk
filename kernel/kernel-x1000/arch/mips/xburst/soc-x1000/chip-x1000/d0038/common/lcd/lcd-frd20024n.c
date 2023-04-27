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
#include <linux/types.h>
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
#include "board.h"

/*ifdef is 18bit,6-6-6 ,ifndef default 5-6-6*/
//#define CONFIG_SLCD_TRULY_18BIT

#ifdef	CONFIG_SLCD_TRULY_18BIT
static int slcd_inited = 1;
#else
static int slcd_inited = 0;
#endif

struct frd20024n_power{
	struct regulator *vlcdio;
	struct regulator *vlcdvcc;
	int inited;
};

static struct frd20024n_power lcd_power = {
	NULL,
	NULL,
	0
};

static int frd20024n_power_init(struct lcd_device *ld)
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

static int frd20024n_power_reset(struct lcd_device *ld)
{
	if (!lcd_power.inited)
		return -EFAULT;
	if (GPIO_LCD_RST > 0)
		gpio_direction_output(GPIO_LCD_RST, 1);
	mdelay(10);
	if (GPIO_LCD_RST > 0)
		gpio_direction_output(GPIO_LCD_RST, 0);
	mdelay(500);
	if (GPIO_LCD_RST > 0)
		gpio_direction_output(GPIO_LCD_RST, 1);
	mdelay(520);

	return 0;
}

static int frd20024n_power_on(struct lcd_device *ld, int enable)
{
	if (!lcd_power.inited && frd20024n_power_init(ld))
		return -EFAULT;

	if (enable) {
		if (GPIO_LCD_CS > 0)
			gpio_direction_output(GPIO_LCD_CS, 1);
		if (GPIO_LCD_RD > 0)
			gpio_direction_output(GPIO_LCD_RD, 1);

		frd20024n_power_reset(ld);

		mdelay(5);
		if (GPIO_LCD_CS > 0)
			gpio_direction_output(GPIO_LCD_CS, 0);
		mdelay(5);

	} else {
		mdelay(5);
		if (GPIO_LCD_CS > 0)
			gpio_direction_output(GPIO_LCD_CS, 0);
		if (GPIO_LCD_RD > 0)
			gpio_direction_output(GPIO_LCD_RD, 0);
		if (GPIO_LCD_RST > 0)
			gpio_direction_output(GPIO_LCD_RST, 0);
		slcd_inited = 0;
	}

	return 0;
}

static struct lcd_platform_data frd20024n_pdata = {
	 .reset = frd20024n_power_reset,
	 .power_on = frd20024n_power_on,
};

/* LCD Panel Device */
struct platform_device frd20024n_device = {
	.name = "frd20024n_slcd",
	.dev = {
		.platform_data = &frd20024n_pdata,
	},
};

static struct smart_lcd_data_table frd20024n_data_table[] = {
	{SMART_CONFIG_CMD,	0x0001},{SMART_CONFIG_DATA,	0x01},{SMART_CONFIG_DATA,	0x1c},
	{SMART_CONFIG_CMD,	0x0002},{SMART_CONFIG_DATA,	0x01},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x0003},{SMART_CONFIG_DATA,	0x12},{SMART_CONFIG_DATA,	0x30},
	{SMART_CONFIG_CMD,	0x0008},{SMART_CONFIG_DATA,	0x08},{SMART_CONFIG_DATA,	0x08},
	{SMART_CONFIG_CMD,	0x000c},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x000f},{SMART_CONFIG_DATA,	0x0e},{SMART_CONFIG_DATA,	0x01},
	{SMART_CONFIG_CMD,	0x0010},{SMART_CONFIG_DATA,	0x0a},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x0011},{SMART_CONFIG_DATA,	0x10},{SMART_CONFIG_DATA,	0x38},{SMART_CONFIG_UDELAY,10},
	{SMART_CONFIG_CMD,	0x0030},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x0031},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0xdb},
	{SMART_CONFIG_CMD,	0x0032},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x0033},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x0034},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0xdb},
	{SMART_CONFIG_CMD,	0x0035},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x0036},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0xaf},
	{SMART_CONFIG_CMD,	0x0037},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x0038},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0xdb},
	{SMART_CONFIG_CMD,	0x0039},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_UDELAY,10},
	{SMART_CONFIG_CMD,	0x00ff},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x03},
	{SMART_CONFIG_CMD,	0x00b0},{SMART_CONFIG_DATA,	0x16},{SMART_CONFIG_DATA,	0x01},
	{SMART_CONFIG_CMD,	0x00b1},{SMART_CONFIG_DATA,	0x06},{SMART_CONFIG_DATA,	0x06},
	{SMART_CONFIG_CMD,	0x0050},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x02},
	{SMART_CONFIG_CMD,	0x0051},{SMART_CONFIG_DATA,	0x08},{SMART_CONFIG_DATA,	0x07},
	{SMART_CONFIG_CMD,	0x0052},{SMART_CONFIG_DATA,	0x01},{SMART_CONFIG_DATA,	0x07},
	{SMART_CONFIG_CMD,	0x0053},{SMART_CONFIG_DATA,	0x18},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x0054},{SMART_CONFIG_DATA,	0x06},{SMART_CONFIG_DATA,	0x01},
	{SMART_CONFIG_CMD,	0x0055},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x02},
	{SMART_CONFIG_CMD,	0x0056},{SMART_CONFIG_DATA,	0x07},{SMART_CONFIG_DATA,	0x07},
	{SMART_CONFIG_CMD,	0x0057},{SMART_CONFIG_DATA,	0x01},{SMART_CONFIG_DATA,	0x06},
	{SMART_CONFIG_CMD,	0x0058},{SMART_CONFIG_DATA,	0x18},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x0059},{SMART_CONFIG_DATA,	0x05},{SMART_CONFIG_DATA,	0x01},
	{SMART_CONFIG_CMD,	0x00ff},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x0007},{SMART_CONFIG_DATA,	0x10},{SMART_CONFIG_DATA,	0x17},
	{SMART_CONFIG_CMD,	0x0020},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x00},
	{SMART_CONFIG_CMD,	0x0021},{SMART_CONFIG_DATA,	0x00},{SMART_CONFIG_DATA,	0x00},
};

static unsigned long frd20024n_cmd_buf[] = {
	0x22222222,
};

static struct fb_videomode jzfb_videomode = {
	.name = "176x220",
	.refresh = 60,
	.xres = 176,
	.yres = 220,
	.pixclock = KHZ2PICOS(5760),
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
	.bpp = 18,
	.width = 32,
	.height = 40,
	.pinmd = 0,

	.smart_config.rsply_cmd_high = 0,
	.smart_config.csply_active_high = 0,
	.smart_config.newcfg_fmt_conv = 1,

	.smart_config.write_gram_cmd = frd20024n_cmd_buf,
	.smart_config.length_cmd = ARRAY_SIZE(frd20024n_cmd_buf),
	.smart_config.bus_width = 8,
	.smart_config.data_table = frd20024n_data_table,
	.smart_config.length_data_table = ARRAY_SIZE(frd20024n_data_table),
	.dither_enable = 0,
};

#ifdef CONFIG_BACKLIGHT_PWM
static bool bl_first_notify;
static int backlight_init(struct device *dev)
{
	bl_first_notify = true;
	return 0;
}

static int backlight_notify(struct device *dev, int brightness)
{
	int brightness_ret = bl_first_notify ? 0 : brightness;
	bl_first_notify = false;
	return brightness_ret;
}

static void backlight_exit(struct device *dev)
{
	gpio_free(GPIO_LCD_PWM);
}

static struct platform_pwm_backlight_data backlight_data = {
	.pwm_id		= 4,
	.max_brightness	= 255,
	.dft_brightness	= 255,
	.pwm_period_ns	= 30000,
	.init		= backlight_init,
	.exit		= backlight_exit,
	.notify		= backlight_notify,
};

struct platform_device backlight_device = {
	.name	= "pwm-backlight",
	.id	= 0,
	.dev = {
		.platform_data	= &backlight_data,
	},
};

#include <linux/pwm.h>

struct pwm_lookup backlight_pwm_lookup[] = {
	PWM_LOOKUP("jz-pwm", 4, "pwm-backlight.0", NULL),
};

void backlight_pwm_add_table(void)
{
	pwm_add_table(backlight_pwm_lookup, ARRAY_SIZE(backlight_pwm_lookup));
}
#endif
