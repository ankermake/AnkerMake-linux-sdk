/*
 * Copyright (c) 2012 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 *  dorado board lcd setup routines.
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

#include <mach/jzfb.h>
#include "../board_base.h"

int xinli_x078dtln06_init(struct lcd_device *lcd)
{
    int ret = 0;
    if(GPIO_MIPI_RST_N > 0){
        gpio_free(GPIO_MIPI_RST_N);
        ret = gpio_request(GPIO_MIPI_RST_N, "lcd mipi panel rst");
        if (ret) {
            printk(KERN_ERR "can's request lcd panel rst\n");
            return ret;
        }
    }

    if(GPIO_MIPI_PWR > 0){
        gpio_free(GPIO_MIPI_PWR);
        ret = gpio_request(GPIO_MIPI_PWR, "lcd mipi panel avcc");
        if (ret) {
            printk(KERN_ERR "can's request lcd panel avcc\n");
            return ret;
        }
    }
    return 0;
}

int xinli_x078dtln06_reset(struct lcd_device *lcd)
{
	gpio_direction_output(GPIO_MIPI_RST_N, 1);
	msleep(100);
	gpio_direction_output(GPIO_MIPI_RST_N, 0);
	msleep(150);
	gpio_direction_output(GPIO_MIPI_RST_N, 1);
	msleep(100);
	return 0;
}

int xinli_x078dtln06_power_on(struct lcd_device *lcd, int enable)
{
    if(xinli_x078dtln06_init(lcd))
        return -EFAULT;
    gpio_direction_output(GPIO_MIPI_PWR, enable); /* 2.8v en */
    msleep(2);
    return 0;
}
struct lcd_platform_data xinli_x078dtln06_data = {
	.reset = xinli_x078dtln06_reset,
	.power_on= xinli_x078dtln06_power_on,
};

struct mipi_dsim_lcd_device xinli_x078dtln06_device={
	.name		= "xinli_x078dtln06-lcd",
	.id = 0,
	.platform_data = &xinli_x078dtln06_data,
};

struct fb_videomode jzfb_videomode = {
	.name = "xinli_x078dtln06-lcd",
	.refresh = 30,
	.xres = 400,
	.yres = 1280,
	/* TODO : need update from manufacturer. */
	.left_margin = 100,
	.right_margin = 8,
	.upper_margin = 8,
	.lower_margin = 10,
	.hsync_len = 7,
	.vsync_len = 5,
	.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

struct jzdsi_data jzdsi_pdata = {
	.modes = &jzfb_videomode,
	.video_config.no_of_lanes = 2,
	.video_config.virtual_channel = 0,
	.video_config.color_coding = COLOR_CODE_24BIT,
	.video_config.video_mode = VIDEO_BURST_WITH_SYNC_PULSES,
	.video_config.receive_ack_packets = 0,	/* enable receiving of ack packets */
	/*loosely: R0R1R2R3R4R5__G0G1G2G3G4G5G6__B0B1B2B3B4B5B6,
	 * not loosely: R0R1R2R3R4R5G0G1G2G3G4G5B0B1B2B3B4B5*/
	.video_config.is_18_loosely = 0,
	.video_config.data_en_polarity = 1,

	.dsi_config.max_lanes = 4,
	.dsi_config.max_hs_to_lp_cycles = 100,
	.dsi_config.max_lp_to_hs_cycles = 40,
	.dsi_config.max_bta_cycles = 4095,
	.dsi_config.color_mode_polarity = 1,
	.dsi_config.shut_down_polarity = 1,
	.dsi_config.max_bps = 650,
//	.max_bps = 650,  // 650 Mbps
	.bpp_info = 24,
};

struct jzfb_platform_data jzfb_pdata = {
	.num_modes = 1,
	.modes = &jzfb_videomode,
	.dsi_pdata = &jzdsi_pdata,

	.lcd_type = LCD_TYPE_GENERIC_24_BIT,
	.bpp = 24,
	.width = 206,
	.height = 67,

	.pixclk_falling_edge = 0,
	.data_enable_active_low = 0,

};

/**************************************************************************************************/
#ifdef CONFIG_BACKLIGHT_PWM

static struct platform_pwm_backlight_data backlight_data = {
	.pwm_id		= 0,
	.max_brightness	= 255,
	.dft_brightness	= 100,
	.pwm_period_ns	= 30000,
};

struct platform_device backlight_device = {
	.name		= "pwm-backlight",
	.dev		= {
		.platform_data	= &backlight_data,
	},
};

#endif
