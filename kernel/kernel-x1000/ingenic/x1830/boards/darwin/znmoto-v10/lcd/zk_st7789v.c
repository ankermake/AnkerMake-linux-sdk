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

static struct smart_lcd_data_table st7789v_data_table[] = {
    {SMART_CONFIG_CMD,0x36},
    {SMART_CONFIG_PRM,0x00},

    {SMART_CONFIG_CMD,0x3a},
    {SMART_CONFIG_PRM,0x06},  //

    {SMART_CONFIG_CMD, 0x21},
    {SMART_CONFIG_CMD, 0xb2},
    {SMART_CONFIG_PRM,0x05},
    {SMART_CONFIG_PRM,0x05},
    {SMART_CONFIG_PRM,0x00},
    {SMART_CONFIG_PRM,0x33},
    {SMART_CONFIG_PRM,0x33},

    {SMART_CONFIG_CMD, 0xb7},
    {SMART_CONFIG_PRM,0x75},

    {SMART_CONFIG_CMD, 0xbb},
    {SMART_CONFIG_PRM,0x26},  //vcom

    {SMART_CONFIG_CMD, 0xc0},
    {SMART_CONFIG_PRM,0x2c},

    {SMART_CONFIG_CMD, 0xc2},
    {SMART_CONFIG_PRM,0x01},

    {SMART_CONFIG_CMD, 0xc3},
    {SMART_CONFIG_PRM,0x0e},

    {SMART_CONFIG_CMD, 0xc4},
    {SMART_CONFIG_PRM,0x20},

    {SMART_CONFIG_CMD, 0xc6},
    {SMART_CONFIG_PRM,0x0f},

    {SMART_CONFIG_CMD, 0xd0},
    {SMART_CONFIG_PRM,0xa4},
    {SMART_CONFIG_PRM,0xa1},

    {SMART_CONFIG_CMD, 0xb1}, // Memory Access Control
    {SMART_CONFIG_PRM,0x40},
    {SMART_CONFIG_PRM,0x10},
    {SMART_CONFIG_PRM,0x12},

    {SMART_CONFIG_CMD, 0xd6},
    {SMART_CONFIG_PRM,0xa1},

    {SMART_CONFIG_CMD, 0xe0},
    {SMART_CONFIG_PRM,0xd0},
    {SMART_CONFIG_PRM,0x05},
    {SMART_CONFIG_PRM,0x0a},
    {SMART_CONFIG_PRM,0x09},
    {SMART_CONFIG_PRM,0x08},
    {SMART_CONFIG_PRM,0x05},
    {SMART_CONFIG_PRM,0x2e},
    {SMART_CONFIG_PRM,0x44},
    {SMART_CONFIG_PRM,0x45},
    {SMART_CONFIG_PRM,0x0f},
    {SMART_CONFIG_PRM,0x17},
    {SMART_CONFIG_PRM,0x16},
    {SMART_CONFIG_PRM,0x2b},
    {SMART_CONFIG_PRM,0x33},

    {SMART_CONFIG_CMD, 0xe1},
    {SMART_CONFIG_PRM,0xd0},
    {SMART_CONFIG_PRM,0x05},
    {SMART_CONFIG_PRM,0x0a},
    {SMART_CONFIG_PRM,0x09},
    {SMART_CONFIG_PRM,0x08},
    {SMART_CONFIG_PRM,0x05},
    {SMART_CONFIG_PRM,0x2e},
    {SMART_CONFIG_PRM,0x43},
    {SMART_CONFIG_PRM,0x45},
    {SMART_CONFIG_PRM,0x0f},
    {SMART_CONFIG_PRM,0x16},
    {SMART_CONFIG_PRM,0x16},
    {SMART_CONFIG_PRM,0x2b},
    {SMART_CONFIG_PRM,0x33},

    {SMART_CONFIG_CMD, 0x2b},
    {SMART_CONFIG_PRM,0x00},
    {SMART_CONFIG_PRM,0x00},
    {SMART_CONFIG_PRM,0x01}, // 320
    {SMART_CONFIG_PRM,0x3f},

    {SMART_CONFIG_CMD, 0x2a},
    {SMART_CONFIG_PRM,0x00},
    {SMART_CONFIG_PRM,0x00},
    {SMART_CONFIG_PRM,0x00}, // 240
    {SMART_CONFIG_PRM,0xef},

    {SMART_CONFIG_CMD, 0x11}, //Exit Sleep
    {SMART_CONFIG_UDELAY, 120000},
    {SMART_CONFIG_CMD, 0x29},         //Display on
};


static struct fb_videomode jzfb_st7789v_videomode = {
    .name = "240x320",
    .refresh = 30,
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


static struct jzfb_smart_config st7789v_cfg = {
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
    .data_table = st7789v_data_table,
    .length_data_table = ARRAY_SIZE(st7789v_data_table),
    .fb_copy_type = FB_COPY_TYPE_ROTATE_0,
};

static unsigned int lcd_power_inited;

static int st7789v_gpio_init(void)
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

    // ret = gpio_request(GPIO_LCD_RD, "lcd rd");
    // if (ret) {
    //     printk(KERN_ERR "can's request lcd rd gpio\n");
    //     return ret;
    // }

    ret = gpio_request(GPIO_LCD_POWER_EN, "lcd power en");
    if (ret) {
        printk(KERN_ERR "can's request lcd power gpio\n");
        return ret;
    }

    lcd_power_inited = 1;

    return 0;
}

static int st7789v_power_on(void *data)
{
    if (!lcd_power_inited && st7789v_gpio_init())
        return -EFAULT;

    gpio_direction_output(GPIO_LCD_POWER_EN, 0);
    gpio_direction_output(GPIO_LCD_CS, 1);
    // gpio_direction_output(GPIO_LCD_RD, 0);
    gpio_direction_output(GPIO_LCD_RST, 0);
    mdelay(20);
    gpio_direction_output(GPIO_LCD_RST, 1);
    mdelay(20);
    gpio_direction_output(GPIO_LCD_CS, 0);

    return 0;
}

static int st7789v_power_off(void *data)
{

    if (!lcd_power_inited && st7789v_gpio_init())
        return -EFAULT;
    gpio_direction_output(GPIO_LCD_CS, 1);
    // gpio_direction_output(GPIO_LCD_RD, 1);
    gpio_direction_output(GPIO_LCD_RST, 0);
    gpio_direction_output(GPIO_LCD_POWER_EN, 1);

    return 0;
}

static struct lcd_callback_ops lcd_callback = {
    .lcd_power_on_begin = st7789v_power_on,
    .lcd_power_off_begin = st7789v_power_off,
};

struct jzfb_platform_data jzfb_pdata = {
    .num_modes = 1,
    .modes = &jzfb_st7789v_videomode,
    .lcd_type = LCD_TYPE_SLCD,
    .bpp = 16,
    .width = 240,
    .height = 320,

    .smart_config = &st7789v_cfg,
    .lcd_callback_ops = &lcd_callback,
};

/****************power and  backlight*********************/

static struct platform_pwm_backlight_data backlight_data = {
    .pwm_id = 6,
    .pwm_active_level = 1,
    .max_brightness = 255,
    .dft_brightness = 200,
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
