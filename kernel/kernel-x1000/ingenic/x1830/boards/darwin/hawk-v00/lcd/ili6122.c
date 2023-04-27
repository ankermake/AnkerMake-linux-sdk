/*
 *
 * Copyright (C) 2019 Ingenic Semiconductor Inc.
 *
 * Author:YangHuanHuan<huanhuan.yang@ingenic.com>
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
#include <board.h>

static int ili6122_power_on_begin(struct lcdc_gpio_struct *gpio)
{
    pr_debug("%s:%s =====> %d\n",__FILE__,__func__,__LINE__);

    return 0;
}

static int ili6122_power_off_begin(struct lcdc_gpio_struct *gpio)
{
    pr_debug("%s:%s =====> %d\n",__FILE__,__func__,__LINE__);

    return 0;
}

static struct lcd_callback_ops ili6122_ops = {
    .lcd_power_on_begin     = (void*)ili6122_power_on_begin,
    .lcd_power_off_begin    = (void*)ili6122_power_off_begin,
};

#define ILI6122_LCD_X_RES   (800)
#define ILI6122_LCD_Y_RES   (480)

static struct fb_videomode jzfb_ili6122_videomode = {
    .name                   = "800x480",
    .refresh                = 60,
    .xres                   = ILI6122_LCD_X_RES,
    .yres                   = ILI6122_LCD_Y_RES,
    .pixclock               = KHZ2PICOS(33264),
    .left_margin            = 88,
    .right_margin           = 40,
    .upper_margin           = 8,
    .lower_margin           = 35,
    .hsync_len              = 128,
    .vsync_len              = 2,
    .sync                   = ~FB_SYNC_HOR_HIGH_ACT & ~FB_SYNC_VERT_HIGH_ACT,
    .vmode                  = FB_VMODE_NONINTERLACED,
    .flag                   = 0,
};

static struct jzfb_tft_config ili6122_cfg = {
    .pix_clk_inv            = 0,
    .de_dl                  = 0,
    .sync_dl                = 0,
    .color_even             = TFT_LCD_COLOR_EVEN_RGB,
    .color_odd              = TFT_LCD_COLOR_ODD_RGB,
    .mode                   = TFT_LCD_MODE_PARALLEL_24B,
    .fb_copy_type           = FB_COPY_TYPE_NONE,
};

struct jzfb_platform_data jzfb_pdata = {
    .num_modes              = 1,
    .modes                  = &jzfb_ili6122_videomode,
    .lcd_type               = LCD_TYPE_TFT,
    .bpp                    = 24,
    .width                  = ILI6122_LCD_X_RES,
    .height                 = ILI6122_LCD_Y_RES,

    .tft_config             = &ili6122_cfg,

    .dither_enable          = 0,
    .dither.dither_red      = 0,
    .dither.dither_green    = 0,
    .dither.dither_blue     = 0,
    .lcd_callback_ops       = &ili6122_ops,
};


/*
 * Power and Backlight
 */
struct ili6122_power {
    struct regulator *vlcdio;
    struct regulator *vlcdvcc;
    int inited;
};

static struct ili6122_power lcd_power = {
    NULL,
    NULL,
    0
};

static int ili6122_power_init(void)
{
    int ret = 0;

    pr_debug("%s:%s =====> %d\n",__FILE__,__func__,__LINE__);

    if (GPIO_LCD_POWER_EN != -1) {
        ret = gpio_request(GPIO_LCD_POWER_EN, "lcd power");
        if (ret) {
            printk(KERN_ERR "can't request lcd power enable pin\n");
            goto power_failed;
        }
        gpio_direction_output(GPIO_LCD_POWER_EN, !GPIO_LCD_POWER_ACTIVE_LEVEL);
    }

    if (GPIO_LCD_BL_EN != -1) {
        ret = gpio_request(GPIO_LCD_BL_EN, "lcd bl");
        if (ret) {
            printk(KERN_ERR "can't request lcd bl\n");
            goto backlight_failed;
        }
        gpio_direction_output(GPIO_LCD_BL_EN,  !GPIO_LCD_BL_ACTIVE_LEVEL);
    }

    if (GPIO_LCD_RST != -1) {
        ret = gpio_request(GPIO_LCD_RST, "lcd rst");
        if (ret) {
            printk(KERN_ERR "can't request lcd reset\n");
            goto reset_failed;
        }
    }

    lcd_power.inited = 1;

    return 0;

reset_failed:
    if (GPIO_LCD_BL_EN != -1) {
        gpio_free(GPIO_LCD_BL_EN);
    }
backlight_failed:
    if (GPIO_LCD_POWER_EN != -1) {
        gpio_free(GPIO_LCD_POWER_EN);
    }
power_failed:
    return ret;
}

static int ili6122_power_reset(struct lcd_device *ld)
{
    pr_debug("%s:%s =====> %d\n",__FILE__,__func__,__LINE__);

    if (!lcd_power.inited)
        return -EFAULT;

    if (GPIO_LCD_RST != -1) {
        gpio_direction_output(GPIO_LCD_RST, !GPIO_LCD_RST_ACTIVE_LEVEL);
        mdelay(20);
        gpio_direction_output(GPIO_LCD_RST, GPIO_LCD_RST_ACTIVE_LEVEL);
        mdelay(20);
    }

    return 0;
}

static int ili6122_power_on(struct lcd_device *ld, int enable)
{
    if (!lcd_power.inited && ili6122_power_init())
        return -EFAULT;

    if (enable) {
        /* PowerON */
        if (GPIO_LCD_POWER_EN != -1) {
            gpio_direction_output(GPIO_LCD_POWER_EN, GPIO_LCD_POWER_ACTIVE_LEVEL);
            mdelay(180);
        }

        if (GPIO_LCD_BL_EN != -1) {
            gpio_direction_output(GPIO_LCD_BL_EN, GPIO_LCD_BL_ACTIVE_LEVEL);
        }
    } else {
        /* PowerOFF */
        if (GPIO_LCD_BL_EN != -1) {
            gpio_direction_output(GPIO_LCD_BL_EN,  !GPIO_LCD_BL_ACTIVE_LEVEL);
        }

        if (GPIO_LCD_POWER_EN != -1) {
            gpio_direction_output(GPIO_LCD_POWER_EN, !GPIO_LCD_POWER_ACTIVE_LEVEL);
            mdelay(50);
        }
    }

    return 0;
}

static struct lcd_platform_data ili6122_pdata = {
    .reset                  = ili6122_power_reset,
    .power_on               = ili6122_power_on,
};

/* LCD Panel Device */
static struct platform_device ili6122_device = {
    .name   = "ili6122_tft",
    .dev    = {
        .platform_data      = &ili6122_pdata,
    },
};

static struct platform_pwm_backlight_data backlight_data = {
    .pwm_id                 = 3,
    .pwm_active_level       = 1,
    .max_brightness         = 255,
    .dft_brightness         = 254,
    .pwm_period_ns          = 30000,
};


struct platform_device backlight_device = {
    .name = "pwm-backlight",
    .dev = {
        .platform_data = &backlight_data,
    },
};


int __init jzfb_backlight_init(void)
{
    int ret = 0;

    pr_debug("======lcd ili6122=====backlight==init=================\n");
    ret = platform_device_register(&ili6122_device);
    if (ret < 0) {
        printk("register platform device ili6122 failed.\n");
        return ret;
    }

    ret = platform_device_register(&backlight_device);
    if (ret < 0) {
        printk("register platform device pwm backlight failed.\n");
        return ret;
    }

    return 0;
}

static void __exit jzfb_backlight_exit(void)
{
    platform_device_unregister(&backlight_device);
    platform_device_unregister(&ili6122_device);
}

rootfs_initcall(jzfb_backlight_init);
module_exit(jzfb_backlight_exit);
