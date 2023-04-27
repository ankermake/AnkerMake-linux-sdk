/*
 * driver/video/fbdev/ingenic/x2000_v12/displays/panel-ili9488.c
 * Copyright (C) 2016 Ingenic Semiconductor Inc.
 * This program is free software, you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#if 0
//device tree config:

 &pwm {
 	pinctrl-names = "default";
 	pinctrl-0 = <&pwm1_pc>;
 	status = "okay";
 };
 &dpu {
 	status = "okay";
 	ingenic,disable-rdma-fb = <1>;
 	/*Defines the init state of composer fb export infomations.*/
 	ingenic,layer-exported = <1 1 0 0>;
 	ingenic,layer-frames   = <2 2 2 2>;
 	ingenic,layer-framesize = <320 480>, <320 480>, <320 240>, <320 240>;   /*Max framesize for each layer.*/
 	layer,color_mode	= <0 0 0 0>;					/*src fmt,*/
 	layer,src-size       	= <320 480>, <320 480>, <320 240>, <240 200>;	/*Layer src size should smaller than framesize*/
 	layer,target-size	= <320 480>, <320 480>, <160 240>, <240 200>;	/*Target Size should smaller than src_size.*/
 	layer,target-pos	= <0 0>, <0 0>, <320 480>, <100 480>;	/*target pos , the start point of the target panel.*/
 	layer,enable		= <1 0 0 0>;					/*layer enabled or disabled.*/
 	ingenic,logo-pan-layer	= <0>;						/*on which layer should init logo pan on.*/
 	port {
 		dpu_out_ep: endpoint {
 			remote-endpoint = <&panel_ili9488_ep>;
 	    };
 	};
 };
 	display-dbi {
 		compatible = "simple-bus";
 		#interrupt-cells = <1>;
 		#address-cells = <1>;
 		#size-cells = <1>;
 		ranges = <>;
 		panel_ili9488 {
 			compatible = "ingenic,ili9488";
 			status = "okay";
 			pinctrl-names = "default";
 			ingenic,rst-gpio = <&gpa 5 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
 			port {
 				panel_ili9488_ep: endpoint {
 					remote-endpoint = <&dpu_out_ep>;
 				};
 			};
 		};
 	};
 backlight {
 		compatible = "pwm-backlight";
 		pwms = <&pwm 1 1000000>; /* arg1: pwm channel id [0~15]. arg2: period in ns. */
 		brightness-levels = <0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15>;
 		default-brightness-level = <15>;
 	};
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/of_gpio.h>
#include <linux/fb.h>
#include <linux/backlight.h>

#include "../ingenicfb.h"
#include "../jz_dsim.h"


static char panel_ili9488_debug = 0;
#define PANEL_DEBUG_MSG(msg...)			\
	do {					\
		if (panel_ili9488_debug)		\
			printk(">>>>>>>>>>>>>>>>PANEL ili9488: " msg);		\
	} while(0)

struct board_gpio {
	short gpio;
	short active_level;
};

struct panel_dev {
	/* ingenic frame buffer */
	struct device *dev;
	struct lcd_panel *panel;

	/* common lcd framework */
	struct lcd_device *lcd;
	struct backlight_device *backlight;
	int power;

	struct regulator *vcc;
	struct board_gpio vdd_en;
	struct board_gpio rst;
	struct board_gpio lcd_te;
	struct board_gpio lcd_pwm;

	struct mipi_dsim_lcd_device *dsim_dev;
};

struct panel_dev *panel;

#define lcd_to_master(a)	(a->dsim_dev->master)
#define lcd_to_master_ops(a)	((lcd_to_master(a))->master_ops)

#define PANEL_ili9488_ID	(0x98815c)

struct ili9488 {
	struct device *dev;
	unsigned int power;
	unsigned int id;

	struct lcd_device *ld;
	struct backlight_device *bd;

	struct mipi_dsim_lcd_device *dsim_dev;

};

static struct dsi_cmd_packet fitipower_ili9488_320_480_test_cmd_list[] =
{
	{0x39, 0x03, 0x00, {0xC0, 0x0F, 0x0F}},
	{0x39, 0x02, 0x00, {0xC1, 0x47}}, //VGH = 5*VCI   VGL = -3*VCI
//	{0x39, 0x04, 0x00, {0xC5, 0x00, 0x49, 0x80}},//4E 51  CD   52
	{0x39, 0x04, 0x00, {0xC5, 0x00, 0x4D, 0x80}},//4E 51  CD   52
	{0x39, 0x02, 0x00, {0xB1, 0xA0}},
	{0x39, 0x02, 0x00, {0xB4, 0x02}},
	{0x39, 0x02, 0x00, {0x36, 0x48}},
	{0x39, 0x02, 0x00, {0x3A, 0x66}},//55 56
	{0x39, 0x02, 0x00, {0x21, 0x00}},//IPS
//	{0x39, 0x03, 0x00, {0xC0, 0xE9, 0x01, 0x00}},
	{0x39, 0x02, 0x00, {0xE9, 0x00}},
	{0x39, 0x05, 0x00, {0xF7, 0xA9, 0x51, 0x2C, 0x82}},
	{0x39, 0x10, 0x00, {0xE0, 0x00, 0x07, 0x0B, 0x03, 0x0F, 0x05, 0x30, 0x56, 0x47, 0x04, 0x0B, 0x0A, 0x2D, 0x37, 0x0F}},
	{0x39, 0x10, 0x00, {0xE1, 0x00, 0x0E, 0x13, 0x04, 0x11, 0x07, 0x39, 0x45, 0x50, 0x07, 0x10, 0x0D, 0x32, 0x36, 0x0F}},
};

static void panel_dev_sleep_in(struct panel_dev *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x10, 0x00};
	PANEL_DEBUG_MSG("enter %s %d \n",__func__, __LINE__);

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void panel_dev_sleep_out(struct panel_dev *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x11, 0x00};
	PANEL_DEBUG_MSG("enter %s %d \n",__func__, __LINE__);

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void panel_dev_display_on(struct panel_dev *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x29, 0x00};
	PANEL_DEBUG_MSG("enter %s %d \n",__func__, __LINE__);

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void panel_dev_display_off(struct panel_dev *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x28, 0x00};
	PANEL_DEBUG_MSG("enter %s %d \n",__func__, __LINE__);

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void panel_dev_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power)
{
	struct board_gpio *vdd_en = &panel->vdd_en;
	struct board_gpio *rst = &panel->rst;
	struct board_gpio *lcd_te = &panel->lcd_te;
	struct board_gpio *pwm = &panel->lcd_pwm;
	PANEL_DEBUG_MSG("enter %s %d \n",__func__, __LINE__);

	msleep(200);
	gpio_direction_output(rst->gpio, 0);
	msleep(200);
	gpio_direction_output(rst->gpio, 1);
	msleep(200);

	panel->power = power;
}

static int panel_dev_read_id(struct mipi_dsim_lcd_device *dsim_dev, unsigned char *buf, int count)
{
	struct dsi_master_ops *ops = dsim_dev->master->master_ops;

	struct dsi_cmd_packet data_to_send0 = {0x14, 0xDA, 0x0};
	struct dsi_cmd_packet data_to_send1 = {0x14, 0xDB, 0x0};
	struct dsi_cmd_packet data_to_send2 = {0x14, 0xDC, 0x0};
	int ret = 0;
	int panel_id = 0;

//	panel_dev_power_on(dsim_dev, 1);
	ret = ops->cmd_read(dsim_dev->master, data_to_send0, &buf[0], 1);
	ret = ops->cmd_read(dsim_dev->master, data_to_send1, &buf[1], 1);
	ret = ops->cmd_read(dsim_dev->master, data_to_send2, &buf[2], 1);
//	panel_dev_power_on(dsim_dev, 0);

	panel_id = (buf[0] << 16) + (buf[1] << 8) + buf[2];
	printk("----------%s, (%d) id[0]: %x id[1]: %x, id[2]: %x, id:%08x \n", __func__, __LINE__, buf[0], buf[1], buf[2], panel_id);

//	if(panel_id == PANEL_ili9488_ID)
//		return 0;
//	else
//		return -1;
	return 0;
}

static void panel_dev_panel_init(struct panel_dev *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	int  i;
	int ret;
	PANEL_DEBUG_MSG("enter %s %d \n",__func__, __LINE__);
	for (i = 0; i < ARRAY_SIZE(fitipower_ili9488_320_480_test_cmd_list); i++)
	{
		ret = ops->cmd_write(dsi,  fitipower_ili9488_320_480_test_cmd_list[i]);
	}
}

static void panel_dev_set_sequence(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct ili9488 *lcd = dev_get_drvdata(&dsim_dev->dev);
	PANEL_DEBUG_MSG("enter %s %d \n",__func__, __LINE__);

	panel_dev_panel_init(panel);
	panel_dev_sleep_out(panel);
	msleep(120);
	panel_dev_display_on(panel);
//	msleep(5);
	msleep(20);
	dump_dsi_reg(dsim_dev->master);

	lcd->power = FB_BLANK_UNBLANK;
}


static struct fb_videomode panel_modes = {
	.name = "fitipower_ili9488-lcd",
	.xres = 320,
	.yres = 480,

	.refresh = 30,
//	.pixclock               = KHZ2PICOS(33264),

	.left_margin = 150,	//hbp
	.right_margin = 150,	//hfp
	.hsync_len = 80,	//hsync

	.upper_margin = 6,	//vbp
	.lower_margin = 8,	//vfp
	.vsync_len = 2,	//vsync

	.sync                   = FB_SYNC_HOR_HIGH_ACT & FB_SYNC_VERT_HIGH_ACT,
	.vmode                  = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

struct jzdsi_data jzdsi_pdata = {
	.modes = &panel_modes,
	.video_config.no_of_lanes = 1,
	.video_config.virtual_channel = 0,
	.video_config.color_coding = COLOR_CODE_24BIT,
	.video_config.video_mode = VIDEO_BURST_WITH_SYNC_PULSES,
	.video_config.receive_ack_packets = 0,	/* enable receiving of ack packets */
	.video_config.is_18_loosely = 0,
	.video_config.data_en_polarity = 1,
	.video_config.byte_clock = 0, // driver will auto calculate byte_clock.
	.video_config.byte_clock_coef = MIPI_PHY_BYTE_CLK_COEF_MUL6_DIV5, // byte_clock *6/5.

	.dsi_config.max_lanes = 2,
	.dsi_config.max_hs_to_lp_cycles = 100,
	.dsi_config.max_lp_to_hs_cycles = 40,
	.dsi_config.max_bta_cycles = 4095,
	.dsi_config.color_mode_polarity = 1,
	.dsi_config.shut_down_polarity = 1,
	.dsi_config.max_bps = 2750,
	.bpp_info = 24,
};

static struct tft_config ili9488_cfg = {
	.pix_clk_inv = 0,
	.de_dl = 0,
	.sync_dl = 0,
	.color_even = TFT_LCD_COLOR_EVEN_RGB,
	.color_odd = TFT_LCD_COLOR_ODD_RGB,
	.mode = TFT_LCD_MODE_PARALLEL_888,
};

struct lcd_panel lcd_panel = {
	.num_modes = 1,
	.modes = &panel_modes,
	.dsi_pdata = &jzdsi_pdata,

	.lcd_type = LCD_TYPE_MIPI_TFT,
	.tft_config = &ili9488_cfg,
	.bpp = 24,
	.width = 49,
	.height = 73,

	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,
};

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)
static int panel_set_power(struct lcd_device *lcd, int power)
{
        return 0;
}

static int panel_get_power(struct lcd_device *lcd)
{
	struct panel_dev *panel = lcd_get_data(lcd);

	return panel->power;
}

/*
* @ pannel_ili9488_lcd_ops, register to kernel common backlight/lcd.c framworks.
*/
static struct lcd_ops panel_lcd_ops = {
	.early_set_power = panel_set_power,
	.set_power = panel_set_power,
	.get_power = panel_get_power,
};

static int of_panel_parse(struct device *dev)
{
	struct panel_dev *panel = dev_get_drvdata(dev);
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int ret = 0;
	PANEL_DEBUG_MSG("enter %s %d \n",__func__, __LINE__);

	panel->rst.gpio = of_get_named_gpio_flags(np, "ingenic,rst-gpio", 0, &flags);
	if(gpio_is_valid(panel->rst.gpio)) {
		panel->rst.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		gpio_direction_output(panel->rst.gpio, !panel->rst.active_level);
		ret = gpio_request_one(panel->rst.gpio, GPIOF_DIR_OUT, "rst");
		if(ret < 0) {
			dev_err(dev, "Failed to request rst pin!\n");
			return ret;
		}
	} else {
		dev_warn(dev, "invalid gpio rst.gpio: %d\n", panel->rst.gpio);
	}
	return 0;

err_request_lcd_pwm:
	if(gpio_is_valid(panel->vdd_en.gpio))
		gpio_free(panel->vdd_en.gpio);
err_request_lcd_vdd:
	if(gpio_is_valid(panel->rst.gpio))
		gpio_free(panel->rst.gpio);
	return ret;
}

static int panel_dev_probe(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct ili9488 *lcd;
	char buffer[4];
	int ret;

//	ret = panel_dev_read_id(dsim_dev, buffer, 4);
//	if(ret < 0) {
//		return -EINVAL;
//	}

	lcd = devm_kzalloc(&dsim_dev->dev, sizeof(struct ili9488), GFP_KERNEL);
	if (!lcd)
	{
		dev_err(&dsim_dev->dev, "failed to allocate fitipower_ili9488 structure.\n");
		return -ENOMEM;
	}

	lcd->dsim_dev = dsim_dev;
	lcd->dev = &dsim_dev->dev;

	lcd->ld = lcd_device_register("fitipower_ili9488", lcd->dev, lcd,
	                              &panel_lcd_ops);
	if (IS_ERR(lcd->ld))
	{
		dev_err(lcd->dev, "failed to register lcd ops.\n");
		return PTR_ERR(lcd->ld);
	}

	dev_set_drvdata(&dsim_dev->dev, lcd);


	dev_dbg(lcd->dev, "probed fitipower_ili9488 panel driver.\n");


	panel->dsim_dev = dsim_dev;


	return 0;

}

static int panel_suspend(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct board_gpio *vdd_en = &panel->vdd_en;

	panel_dev_display_off(panel);
	panel_dev_sleep_in(panel);
	gpio_direction_output(vdd_en->gpio, !vdd_en->active_level);

	return 0;
}

static int panel_resume(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct board_gpio *vdd_en = &panel->vdd_en;

	gpio_direction_output(vdd_en->gpio, vdd_en->active_level);

	return 0;
}

static struct mipi_dsim_lcd_driver panel_dev_dsim_ddi_driver = {
	.name = "fitipower_ili9488-lcd",
	.id = -1,

	.power_on = panel_dev_power_on,
	.set_sequence = panel_dev_set_sequence,
	.probe = panel_dev_probe,
	.suspend = panel_suspend,
	.resume = panel_resume,
};


struct mipi_dsim_lcd_device panel_dev_device={
	.name		= "fitipower_ili9488-lcd",
	.id = 0,
};

/*
* @panel_probe

* 	1. Register to ingenicfb.
* 	2. Register to lcd.
* 	3. Register to backlight if possible.

* @pdev

* @Return -
*/
static int panel_probe(struct platform_device *pdev)
{
	int ret = 0;
	char buffer[4];
	PANEL_DEBUG_MSG("enter %s %d \n",__func__, __LINE__);

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

	mipi_dsi_register_lcd_device(&panel_dev_device);
	mipi_dsi_register_lcd_driver(&panel_dev_dsim_ddi_driver);

	ret = ingenicfb_register_panel(&lcd_panel);
	if(ret < 0) {
		dev_err(&pdev->dev, "Failed to register lcd panel!\n");
		goto err_of_parse;
	}
	return 0;

err_of_parse:
	if(gpio_is_valid(panel->lcd_pwm.gpio))
		gpio_free(panel->lcd_pwm.gpio);
	if(gpio_is_valid(panel->rst.gpio))
		gpio_free(panel->rst.gpio);
	if(gpio_is_valid(panel->vdd_en.gpio))
		gpio_free(panel->vdd_en.gpio);

	kfree(panel);
	return ret;
}

static int panel_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id panel_of_match[] = {
	{ .compatible = "ingenic,ili9488", },
	{},
};

static struct platform_driver panel_driver = {
	.probe		= panel_probe,
	.remove		= panel_remove,
	.driver		= {
		.name	= "ili9488",
		.of_match_table = panel_of_match,
	},
};

module_platform_driver(panel_driver);

MODULE_LICENSE("GPL");
