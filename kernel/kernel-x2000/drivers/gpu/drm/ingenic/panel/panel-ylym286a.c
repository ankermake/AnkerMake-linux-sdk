/*
 * drivers/gpu/drm/ingenic/panel/panel-ylym286a.c
 *
 * Copyright (C) 2020 Ingenic Semiconductor Inc.
 *
 * This program is free software, you can redistribute it and/or modify it
 *
 * under the terms of the GNU General Public License version 2 as published by
 *
 * the Free Software Foundation.
 */

#include <linux/backlight.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <video/display_timing.h>
#include <video/videomode.h>
#include "../jz_dsim.h"
#include "../ingenic_drv.h"

static int lt9211_state = 0;
static int panel_probe(struct platform_device *pdev);

struct board_gpio {
	short gpio;
	short active_level;
};

struct panel_desc {
	const struct drm_display_mode *modes;
	unsigned int num_modes;
	const struct display_timing *timings;
	unsigned int num_timings;

	unsigned int bpc;

	struct {
		unsigned int width;
		unsigned int height;
	} size;
	struct {
		unsigned int prepare;
		unsigned int enable;
		unsigned int disable;
		unsigned int unprepare;
	} delay;

	u32 bus_format;
	u32 color_format;
	u32 lcd_type;
};

struct panel_dev {
	struct drm_panel base;
	/* ingenic frame buffer */
	struct i2c_client *client;
	struct device *dev;
	struct lcd_panel *panel;

	/* common lcd framework */
	struct lcd_device *lcd;
	int power;
	const struct panel_desc *desc;

	int i2c_id;
	struct board_gpio reset;
	struct board_gpio lcd_en;
	struct board_gpio bl_en;
	char *panel_name;
};

#define FPS    60
#define HACT   1920
#if 1
#define VACT   540

#define HFP    200
#define HBP    200
#else
#define VACT   1080

#define HFP    88
#define HBP    148
#endif
#define HS     44

#define VFP    4
#define VBP    36
#define VS     5

#include "lt9211.c"

static struct panel_dev *lt9211_info;

#if 0
static struct fb_videomode panel_modes = {
	.name                   = "lt9211-ylym286a",
	.refresh                = FPS,
	.xres                   = HACT,
	.yres                   = VACT,
	.left_margin            = HFP,
	.right_margin           = HBP,
	.upper_margin           = VFP,
	.lower_margin           = VBP,

	.hsync_len              = HS,
	.vsync_len              = VS,
	.sync                   = FB_SYNC_HOR_HIGH_ACT & FB_SYNC_VERT_HIGH_ACT,
	.vmode                  = FB_VMODE_NONINTERLACED,
	.flag                   = 0,
};
#endif

struct jzdsi_data jzdsi_pdata = {
	/* .modes = &panel_modes, */
	.video_config.no_of_lanes = 2,
	.video_config.virtual_channel = 0,
	.video_config.color_coding = COLOR_CODE_24BIT,
	.video_config.video_mode = VIDEO_BURST_WITH_SYNC_PULSES,
	.video_config.receive_ack_packets = 0,	/* enable receiving of ack packets */
	.video_config.is_18_loosely = 0,
	.video_config.data_en_polarity = 1,
	.video_config.byte_clock = 0, // driver will auto calculate byte_clock.
	.video_config.byte_clock_coef = MIPI_PHY_BYTE_CLK_COEF_MUL1,

	.dsi_config.max_lanes = 2,
	.dsi_config.max_hs_to_lp_cycles = 100,
	.dsi_config.max_lp_to_hs_cycles = 40,
	.dsi_config.max_bta_cycles = 4095,
	.dsi_config.color_mode_polarity = 1,
	.dsi_config.shut_down_polarity = 1,
	.dsi_config.max_bps = 2750,
	.bpp_info = 24,
};

static struct tft_config kd050hdfia019_cfg = {
	.pix_clk_inv = 0,
	.de_dl = 0,
	.sync_dl = 0,
	.color_even = TFT_LCD_COLOR_EVEN_RGB,
	.color_odd = TFT_LCD_COLOR_ODD_RGB,
	.mode = TFT_LCD_MODE_PARALLEL_888,
};

#if 0
struct lcd_panel lcd_panel[] = {
	[0] = {
		.lcd_type = LCD_TYPE_TFT,

		.tft_config = &kd050hdfia019_cfg,
		.dsi_pdata = &jzdsi_pdata,
	},
};

#else
static struct lcd_panel lcd_panel = {
	.dsi_pdata = &jzdsi_pdata,
	.tft_config = &kd050hdfia019_cfg,
};
#endif

static const struct drm_display_mode display_mode = {
	.clock = PIXCLK,
	.hdisplay = HACT,
	.hsync_start = HFP + HACT,
	.hsync_end = HFP + HACT + HBP,
	.htotal = HFP + HACT + HBP + HS,
	.vdisplay = VACT,
	.vsync_start = VFP + VACT,
	.vsync_end = VFP + VACT + VBP,
	.vtotal = VFP + VACT + VBP + VS,
	.vrefresh = 60,
	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
	.private = (int *)&lcd_panel
};

static const struct panel_desc panel_desc = {
	.modes = &display_mode,
	.num_modes = 1,
	.bpc = 6,
	.size = {
		.width = 699,
		.height = 197,
	},
	.bus_format = MEDIA_BUS_FMT_RGB888_1X24,
};

static unsigned char i2c_read(struct i2c_client *client, unsigned char addr)
{
	return i2c_smbus_read_byte_data(client, addr);
}

static unsigned int i2c_write(struct i2c_client *client,unsigned char addr,unsigned char value)
{
	return i2c_smbus_write_byte_data(client, addr, value);
}

void lt9211_reset(struct panel_dev *lt9211_info)
{
	struct board_gpio *rst = &lt9211_info->reset;

	gpio_direction_output(rst->gpio, 1);
	mdelay(1);
	gpio_direction_output(rst->gpio, 0);
	mdelay(100);
	gpio_direction_output(rst->gpio, 1);
	mdelay(100);
}

int panel_bridge_init(void)
{
	int ret = 0;
	struct i2c_client *client = lt9211_info->client;

	ret = LT9211_MIPI2LVDS_Config(client);

	return ret;
}

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)
static int panel_set_power(struct panel_dev *panel, int power)
{
	struct i2c_client *client = panel->client;

	struct board_gpio *lcd_en = &panel->lcd_en;
	struct board_gpio *bl_en = &panel->bl_en;
	struct board_gpio *rst = &panel->reset;

	if(POWER_IS_ON(power) && !POWER_IS_ON(panel->power)) {
		gpio_direction_output(rst->gpio, 0);
		gpio_direction_output(lcd_en->gpio, 1);
		mdelay(15);

		lt9211_reset(panel);
		lt9211_state = 0;
		/* if (LT9211_MIPI2LVDS_Config(client)) */
		/* 	return -1; */
		gpio_direction_output(bl_en->gpio, 1);
	}
	if(!POWER_IS_ON(power) && POWER_IS_ON(panel->power)) {
		gpio_direction_output(bl_en->gpio, 0);
		gpio_direction_output(lcd_en->gpio, 0);
	}

	panel->power = power;
	return 0;
}

static inline struct panel_dev *to_panel_dev(struct drm_panel *panel)
{
	return container_of(panel, struct panel_dev, base);
}

static int panel_dev_unprepare(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);
	printk("\033[33m(l:%d, f:%s, F: %s) %d %s\033[0m\n", __LINE__, __func__, __FILE__, 0, "");

	/* panel_set_power(p, FB_BLANK_POWERDOWN); */
	return 0;
}

static int panel_dev_prepare(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);
	printk("\033[33m(l:%d, f:%s, F: %s) %d %s\033[0m\n", __LINE__, __func__, __FILE__, 0, "");

	/* panel_set_power(p, FB_BLANK_UNBLANK); */
	return 0;
}

static int panel_dev_enable(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);
	printk("\033[33m(l:%d, f:%s, F: %s) %d %s\033[0m\n", __LINE__, __func__, __FILE__, 0, "");

	panel_set_power(p, FB_BLANK_UNBLANK);
	return 0;
}

static int panel_dev_disable(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);
	printk("\033[33m(l:%d, f:%s, F: %s) %d %s\033[0m\n", __LINE__, __func__, __FILE__, 0, "");

	panel_set_power(p, FB_BLANK_POWERDOWN);

	return 0;
}

static int panel_dev_get_fixed_modes(struct panel_dev *panel)
{
	struct drm_connector *connector = panel->base.connector;
	struct drm_device *drm = panel->base.drm;
	struct drm_display_mode *mode;
	unsigned int i, num = 0;

	if (!panel->desc)
		return 0;

	for (i = 0; i < panel->desc->num_modes; i++) {
		const struct drm_display_mode *m = &panel->desc->modes[i];

		mode = drm_mode_duplicate(drm, m);
		if (!mode) {
			dev_err(drm->dev, "failed to add mode %ux%u@%u\n",
				m->hdisplay, m->vdisplay, m->vrefresh);
			continue;
		}

		drm_mode_set_name(mode);

		drm_mode_probed_add(connector, mode);
		num++;
	}

	connector->display_info.bpc = panel->desc->bpc;
	connector->display_info.width_mm = panel->desc->size.width;
	connector->display_info.height_mm = panel->desc->size.height;
/* printk("\033[33m panel->desc->size.width = %d |\033[0m\n", panel->desc->size.width); */
/* printk("\033[33m panel->desc->size.height = %d |\033[0m\n", panel->desc->size.height); */
/* 	printk("\033[33m panel->desc->bus_format = %d |\033[0m\n", panel->desc->bus_format); */
	if (panel->desc->bus_format)
		drm_display_info_set_bus_formats(&connector->display_info,
						 &panel->desc->bus_format, 1);

	return num;
}

static int panel_dev_get_modes(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);
	int num = 0;

	num += panel_dev_get_fixed_modes(p);

	return num;
}

static const struct drm_panel_funcs drm_panel_funs = {
	.disable = panel_dev_disable,
	.unprepare = panel_dev_unprepare,
	.prepare = panel_dev_prepare,
	.enable = panel_dev_enable,
	.get_modes = panel_dev_get_modes,
};

#if 0
static struct lcd_ops panel_lcd_ops = {
	.early_set_power = panel_set_power,
	.set_power = panel_set_power,
	.get_power = panel_get_power,
};
#endif


int _of_get_named_gpio_lvl(struct device *dev, short *gpio, short *lvl, char *of_name)
{
	int ret = 0;
	enum of_gpio_flags flags;

	/* devm_gpio_request_one */
	*gpio = of_get_named_gpio_flags(dev->of_node, of_name, 0, &flags);
	if(gpio_is_valid(*gpio)) {
		*lvl = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(*gpio, GPIOF_DIR_OUT, of_name);
		if(ret < 0) {
			dev_err(dev, "Failed to request reset pin!\n");
			return ret;
		}
	} else {
		dev_warn(dev, "invalid gpio: %s\n", of_name);
		return -1;
	}
	return 0;
}

static int of_panel_parse(struct device *dev)
{
	struct panel_dev *lt9211_info = dev_get_drvdata(dev);
	int ret = 0;

	if ((ret = _of_get_named_gpio_lvl(dev,
				&lt9211_info->reset.gpio,
				&lt9211_info->reset.active_level,
				"ingenic,reset-gpio")))
		return ret;

	if ((ret = _of_get_named_gpio_lvl(dev,
				&lt9211_info->lcd_en.gpio,
				&lt9211_info->lcd_en.active_level,
				"ingenic,lcd_en-gpio")))
		goto err_request_reset;

	if ((ret = _of_get_named_gpio_lvl(dev,
				&lt9211_info->bl_en.gpio,
				&lt9211_info->bl_en.active_level,
				"ingenic,bl_en-gpio")))
		goto err_request_reset;

	if (ret = of_property_read_u32(dev->of_node,
					"ingenic,lt9211-i2c-id",
					(const u32 *)&lt9211_info->i2c_id))
		goto err_request_reset;
	/* devm_ */

	return 0;
err_request_reset:
	if(gpio_is_valid(lt9211_info->reset.gpio))
		gpio_free(lt9211_info->reset.gpio);
	return ret;
}
#if 1
static int panel_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int panel_suspend(struct device *dev)
{
	struct panel_dev *lt9211_info = dev_get_drvdata(dev);

	panel_set_power(lt9211_info, FB_BLANK_POWERDOWN);
	return 0;
}

static int panel_resume(struct device *dev)
{
	struct panel_dev *lt9211_info = dev_get_drvdata(dev);

	panel_set_power(lt9211_info, FB_BLANK_UNBLANK);
	return 0;
}

static const struct dev_pm_ops panel_pm_ops = {
	.suspend = panel_suspend,
	.resume = panel_resume,
};
#endif

static const struct of_device_id panel_of_match[] = {
	{ .compatible = "ingenic,ylym286a", },
	{},
};

static struct platform_driver panel_driver = {
	.probe		= panel_probe,
	.remove		= panel_remove,
	.driver		= {
		.name	= "ylym286a",
		.of_match_table = panel_of_match,
#ifdef CONFIG_PM
		.pm = &panel_pm_ops,
#endif
	},
};
#endif

struct mipi_dsim_lcd_device panel_dev_device={
	.name		= "lt9211-ylym286a",
	.id = 0,
};

static struct i2c_board_info lt9211_i2c_device = {
    I2C_BOARD_INFO("lt9211", 0x2d),
};

static int panel_probe(struct platform_device *pdev)
/* static int lt9211_probe(struct i2c_client *client, const struct i2c_device_id *id) */
{
	int ret = 0, i;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
    struct i2c_adapter *lt9211_i2c_adapter = NULL;

	lt9211_info = kzalloc(sizeof(struct panel_dev), GFP_KERNEL);
	if(lt9211_info == NULL) {
		dev_err(dev, "Failed to alloc memory!");
		return -ENOMEM;
	}
	lt9211_info->dev = dev;
	dev_set_drvdata(dev, lt9211_info);

	// set dev_info.
	ret = of_panel_parse(dev);
	if(ret < 0) {
		goto err_of_parse;
	}

    lt9211_i2c_adapter = i2c_get_adapter(lt9211_info->i2c_id);
    if(!lt9211_i2c_adapter){
        printk("%s: Can not found i2c id %d\n", __func__, lt9211_info->i2c_id);
    }else{
        lt9211_info->client = i2c_new_device(lt9211_i2c_adapter, &lt9211_i2c_device);
    }

	of_property_read_string(np, "lt9211,panel_name", (const char**)&lt9211_info->panel_name);

#if 0
	ret = ingenicfb_register_panel(&lcd_panel[i]);
	if(ret < 0) {
		dev_err(dev, "Failed to register lcd panel!\n");
		goto err_lcd_register;
	}
#endif
	/* TODO: should this power status sync from uboot */
	lt9211_info->power = FB_BLANK_POWERDOWN;

	lt9211_info->desc = &panel_desc;
	drm_panel_init(&lt9211_info->base);
	lt9211_info->base.dev = dev;
	lt9211_info->base.funcs = &drm_panel_funs;
	drm_panel_add(&lt9211_info->base);

	if (panel_set_power(lt9211_info, FB_BLANK_UNBLANK))
		goto err_lcd_register;

	printk("\033[32mregister %s sucess.\033[0m\n", lt9211_info->panel_name);

	return 0;

err_lcd_register:
	/* lcd_device_unregister(lt9211_info->lcd); */
err_of_parse:
	kfree(lt9211_info);
	printk("\033[31mregister %s dailed.\033[0m\n", lt9211_info->panel_name);
	return ret;
}

static int lt9211_remove(struct i2c_client *client)
{
	struct panel_dev *lt9211_info = dev_get_drvdata(&client->dev);

	panel_set_power(lt9211_info, FB_BLANK_POWERDOWN);

	return 0;
}

#if 0
#ifdef CONFIG_OF
static const struct of_device_id lt9211_match_table[] = {
		{.compatible = "mipi2lvds,lt9211",},
		{ },
};
MODULE_DEVICE_TABLE(of, lt9211_match_table);
#endif

static const struct i2c_device_id lt9211_id[] = {
	{ "lt9211", },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lt9211_id);

static struct platform_driver panel_driver = {
/* static struct i2c_driver lt9211_driver = { */
    .probe      = lt9211_probe,
    .remove     = lt9211_remove,
    .driver = {
        .name     = "lt9211",
        .owner    = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(lt9211_match_table),
#endif
#ifdef CONFIG_PM
		.pm = &panel_pm_ops,
#endif
    },
	.id_table = lt9211_id,
};
#endif

module_platform_driver(panel_driver);
/* module_i2c_driver(lt9211_driver); */

MODULE_DESCRIPTION("LT9211 Driver");
MODULE_LICENSE("GPL");

#if 0
struct panel_dev {
	struct drm_panel base;
	/* ingenic frame buffer */
	struct device *dev;
	struct lcd_panel *panel;

	struct backlight_device *backlight;
	int power;
	const struct panel_desc *desc;

	struct regulator *vcc;
	struct board_gpio vdd_en;
	struct board_gpio cs;
	struct board_gpio rst;
	struct board_gpio pwm;
	struct board_gpio oled;
	struct board_gpio swire;
};

static int panel_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct panel_dev *panel;

	panel = kzalloc(sizeof(struct panel_dev), GFP_KERNEL);
	if(panel == NULL) {
		dev_err(&pdev->dev, "Faile to alloc memory!");
		return -ENOMEM;
	}
	panel->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, panel);

	ret = of_panel_parse(&pdev->dev);
	if(ret < 0) {
		goto err_of_parse;
	}

	panel->power = FB_BLANK_POWERDOWN;

	panel->desc = &panel_desc;
	drm_panel_init(&panel->base);
	panel->base.dev = &pdev->dev;
	panel->base.funcs = &drm_panel_funs;
	drm_panel_add(&panel->base);
}
#endif
