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

#include "../ingenic_drv.h"
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

	/**
	 * @prepare: the time (in milliseconds) that it takes for the panel to
	 *           become ready and start receiving video data
	 * @enable: the time (in milliseconds) that it takes for the panel to
	 *          display the first valid frame after starting to receive
	 *          video data
	 * @disable: the time (in milliseconds) that it takes for the panel to
	 *           turn the display off (no content is visible)
	 * @unprepare: the time (in milliseconds) that it takes for the panel
	 *             to power itself down completely
	 */
	struct {
		unsigned int prepare;
		unsigned int enable;
		unsigned int disable;
		unsigned int unprepare;
	} delay;

	u32 bus_format;
	u32 color_formats;
};

struct panel_dev {
	struct drm_panel base;
	/* ingenic frame buffer */
	struct device *dev;
	struct lcd_panel *panel;

	struct backlight_device *backlight;
	int power;
	const struct panel_desc *desc;

	struct regulator *vcc;
	struct board_gpio cs;
	struct board_gpio rst;
	struct board_gpio ssi_clk;
	struct board_gpio ssi_dt;
	struct board_gpio pwm;
};

#define SSI_CS(panel, n)	gpio_direction_output(panel->cs.gpio, n)
#define SSI_CLK(panel, n)	gpio_direction_output(panel->ssi_clk.gpio, n)
#define SSI_DT(panel, n)	gpio_direction_output(panel->ssi_dt.gpio, n)

static void lcd_send_cmd(struct panel_dev *panel, uint8_t cmd) {

	int i = 8;
	SSI_CS(panel, 0);
	SSI_CLK(panel, 0);
	SSI_DT(panel, 0);
	SSI_CLK(panel, 1);
	while(i--) {
		SSI_CLK(panel, 0);
		SSI_DT(panel, !!(cmd & 0x80));
		SSI_CLK(panel, 1);
		cmd = cmd << 1;
	}
	SSI_CLK(panel, 0);
	SSI_CS(panel, 1);
}

static void lcd_send_param(struct panel_dev *panel, uint8_t param) {

	int i = 8;
	SSI_CS(panel, 0);
	SSI_CLK(panel, 0);
	SSI_DT(panel, 1);
	SSI_CLK(panel,1);
	while(i--) {
		SSI_CLK(panel, 0);
		SSI_DT(panel, !!(param & 0x80));
		SSI_CLK(panel, 1);
		param = param << 1;
	}
	SSI_CLK(panel, 0);
	SSI_CS(panel, 1);

}

static void panel_init(struct panel_dev *panel) {
#define LCD_SEND_CMD(n) lcd_send_cmd(panel, n)
#define LCD_SEND_PARAM(n) lcd_send_param(panel, n)
	LCD_SEND_CMD(0xC0);
	LCD_SEND_PARAM(0x17);
	LCD_SEND_PARAM(0x17);

	LCD_SEND_CMD(0xC1);
	LCD_SEND_PARAM(0x44);

	LCD_SEND_CMD(0xC5);
	LCD_SEND_PARAM(0x00);
	LCD_SEND_PARAM(0x3A);
	LCD_SEND_PARAM(0x80);

	LCD_SEND_CMD(0x36);
	LCD_SEND_PARAM(0x48);

	LCD_SEND_CMD(0x3A); //Interface Mode Control
	LCD_SEND_PARAM(0x60);

	LCD_SEND_CMD(0xB1);   //Frame rate 70HZ
	LCD_SEND_PARAM(0xA0);

	LCD_SEND_CMD(0xB4);
	LCD_SEND_PARAM(0x02);

	LCD_SEND_CMD(0xB7);
	LCD_SEND_PARAM(0xC6);

	LCD_SEND_CMD(0xE9);
	LCD_SEND_PARAM(0x00);

	LCD_SEND_CMD(0XF7);
	LCD_SEND_PARAM(0xA9);
	LCD_SEND_PARAM(0x51);
	LCD_SEND_PARAM(0x2C);
	LCD_SEND_PARAM(0x82);

	LCD_SEND_CMD(0xE0);
	LCD_SEND_PARAM(0x01);
	LCD_SEND_PARAM(0x13);
	LCD_SEND_PARAM(0x1E);
	LCD_SEND_PARAM(0x00);
	LCD_SEND_PARAM(0x0D);
	LCD_SEND_PARAM(0x03);
	LCD_SEND_PARAM(0x3D);
	LCD_SEND_PARAM(0x55);
	LCD_SEND_PARAM(0x4F);
	LCD_SEND_PARAM(0x06);
	LCD_SEND_PARAM(0x10);
	LCD_SEND_PARAM(0x0B);
	LCD_SEND_PARAM(0x2C);
	LCD_SEND_PARAM(0x32);
	LCD_SEND_PARAM(0x0F);

	LCD_SEND_CMD(0xE1);
	LCD_SEND_PARAM(0x08);
	LCD_SEND_PARAM(0x10);
	LCD_SEND_PARAM(0x15);
	LCD_SEND_PARAM(0x03);
	LCD_SEND_PARAM(0x0E);
	LCD_SEND_PARAM(0x03);
	LCD_SEND_PARAM(0x32);
	LCD_SEND_PARAM(0x34);
	LCD_SEND_PARAM(0x44);
	LCD_SEND_PARAM(0x07);
	LCD_SEND_PARAM(0x10);
	LCD_SEND_PARAM(0x0E);
	LCD_SEND_PARAM(0x23);
	LCD_SEND_PARAM(0x2E);
	LCD_SEND_PARAM(0x0F);

	LCD_SEND_CMD(0xB6);
	LCD_SEND_PARAM(0xB0); //set rgb bypass mode
	LCD_SEND_PARAM(0x02); //GS,SS
	LCD_SEND_PARAM(0x3B);

	/****************************************/
	LCD_SEND_CMD(0XB0);  //Interface Mode Control
	LCD_SEND_PARAM(0x02);
	 /**************************************************/
	LCD_SEND_CMD(0x2A); //Frame rate control
	LCD_SEND_PARAM(0x00);
	LCD_SEND_PARAM(0x00);
	LCD_SEND_PARAM(0x01);
	LCD_SEND_PARAM(0x3F);

	LCD_SEND_CMD(0x2B); //Display function control
	LCD_SEND_PARAM(0x00);
	LCD_SEND_PARAM(0x00);
	LCD_SEND_PARAM(0x01);
	LCD_SEND_PARAM(0xDF);

	LCD_SEND_CMD(0x21);

	LCD_SEND_CMD(0x11);
	mdelay(20);
	LCD_SEND_CMD(0x29); //display on
	LCD_SEND_CMD(0x2c);
}

/* SGM3146 supports 16 brightness step */
#define MAX_BRIGHTNESS_STEP     16
/* System support 256 brightness step */
#define CONVERT_FACTOR          (256/MAX_BRIGHTNESS_STEP)

static int panel_update_status(struct backlight_device *bd)
{
	struct panel_dev *panel = dev_get_drvdata(&bd->dev);
	int brightness = bd->props.brightness;
	unsigned int i;
	int pulse_num = MAX_BRIGHTNESS_STEP - brightness / CONVERT_FACTOR - 1;

	if (bd->props.fb_blank == FB_BLANK_POWERDOWN) {
		return 0;
	}

	if (bd->props.state & BL_CORE_SUSPENDED)
		brightness = 0;

	if (brightness) {
		gpio_direction_output(panel->pwm.gpio,0);
		udelay(5000);
		gpio_direction_output(panel->pwm.gpio,1);
		udelay(100);

		for (i = pulse_num; i > 0; i--) {
			gpio_direction_output(panel->pwm.gpio,0);
			udelay(1);
			gpio_direction_output(panel->pwm.gpio,1);
			udelay(3);
		}
	} else
		gpio_direction_output(panel->pwm.gpio, 0);

	return 0;
}

static struct backlight_ops panel_backlight_ops = {
	.options = BL_CORE_SUSPENDRESUME,
	.update_status = panel_update_status,
};

static void panel_power_reset(struct board_gpio *rst)
{
	gpio_direction_output(rst->gpio, 0);
	mdelay(10);
	gpio_direction_output(rst->gpio, 1);
	mdelay(20);
}

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)
static int panel_set_power(struct panel_dev *panel, int power)
{
	struct board_gpio *cs = &panel->cs;
	struct board_gpio *rst = &panel->rst;
	if(POWER_IS_ON(power) && !POWER_IS_ON(panel->power)) {
		gpio_direction_output(rst->gpio, 1);
		gpio_direction_output(cs->gpio, 1);
		panel_power_reset(rst);
		gpio_direction_output(cs->gpio, 0);
		panel_init(panel);
	}
	if(!POWER_IS_ON(power) && POWER_IS_ON(panel->power)) {
		gpio_direction_output(cs->gpio, 1);
		gpio_direction_output(rst->gpio, 0);
	}

	panel->power = power;
        return 0;
}

static int of_panel_parse(struct device *dev)
{
	struct panel_dev *panel = dev_get_drvdata(dev);
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int ret = 0;

	panel->cs.gpio = of_get_named_gpio_flags(np, "ingenic,cs-gpio", 0, &flags);
	if(gpio_is_valid(panel->cs.gpio)) {
		panel->cs.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->cs.gpio, GPIOF_DIR_OUT, "cs");
		if(ret < 0) {
			dev_err(dev, "Failed to request cs pin!\n");
			return ret;
		}
	} else {
		dev_warn(dev, "invalid gpio cs.gpio: %d\n", panel->cs.gpio);
	}

	panel->rst.gpio = of_get_named_gpio_flags(np, "ingenic,rst-gpio", 0, &flags);
	if(gpio_is_valid(panel->rst.gpio)) {
		panel->rst.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->rst.gpio, GPIOF_DIR_OUT, "rst");
		if(ret < 0) {
			dev_err(dev, "Failed to request rst pin!\n");
			goto err_request_rst;
		}
	} else {
		dev_warn(dev, "invalid gpio rst.gpio: %d\n", panel->rst.gpio);
	}
	panel->ssi_clk.gpio = of_get_named_gpio_flags(np, "ingenic,ssi-clk-gpio", 0, &flags);
	if(gpio_is_valid(panel->ssi_clk.gpio)) {
		panel->ssi_clk.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->ssi_clk.gpio, GPIOF_DIR_OUT, "ssi_clk");
		if(ret < 0) {
			dev_err(dev, "Failed to request ssi_clk pin!\n");
			goto err_request_rst;
		}
	} else {
		dev_warn(dev, "invalid gpio ssi_clk.gpio: %d\n", panel->ssi_clk.gpio);
	}
	panel->ssi_dt.gpio = of_get_named_gpio_flags(np, "ingenic,ssi-dt-gpio", 0, &flags);
	if(gpio_is_valid(panel->ssi_dt.gpio)) {
		panel->ssi_dt.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->ssi_dt.gpio, GPIOF_DIR_OUT, "ssi_dt");
		if(ret < 0) {
			dev_err(dev, "Failed to request ssi_dt pin!\n");
			goto err_request_rst;
		}
	} else {
		dev_warn(dev, "invalid gpio ssi_dt.gpio: %d\n", panel->ssi_dt.gpio);
	}
	panel->pwm.gpio = of_get_named_gpio_flags(np, "ingenic,pwm-gpio", 0, &flags);
	if(gpio_is_valid(panel->pwm.gpio)) {
		panel->pwm.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->pwm.gpio, GPIOF_DIR_OUT, "pwm");
		if(ret < 0) {
			dev_err(dev, "Failed to request pwm pin!\n");
			goto err_request_rst;
		}
	} else {
		dev_warn(dev, "invalid gpio pwm.gpio: %d\n", panel->pwm.gpio);
	}

	return 0;
err_request_rst:
	gpio_free(panel->cs.gpio);
	return ret;
}
static inline struct panel_dev *to_panel_dev(struct drm_panel *panel)
{
	return container_of(panel, struct panel_dev, base);
}
static int panel_dev_unprepare(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);

	panel_set_power(p, 0);
	return 0;
}

static int panel_dev_prepare(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);
	panel_set_power(p, 1);
	return 0;
}

static int panel_dev_enable(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);

	if (p->backlight) {
		p->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(p->backlight);
	}

	return 0;
}

static int panel_dev_disable(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);

	if (p->backlight) {
		p->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(p->backlight);
	}
	return 0;
}

static const struct drm_display_mode kd035hvfmd057_mode = {
	.clock = 10800,
	.hdisplay = 320,
	.hsync_start = 320 + 60,
	.hsync_end = 320 + 60 + 20,
	.htotal = 320 + 60 + 20 + 3,
	.vdisplay = 480,
	.vsync_start = 480 + 2,
	.vsync_end = 480 + 2 + 2,
	.vtotal = 480 + 2 + 2 + 2,
	.vrefresh = 60,
	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
};

static const struct panel_desc kd035hvfmd057_desc = {
	.modes = &kd035hvfmd057_mode,
	.num_modes = 1,
	.bpc = 6,
	.size = {
		.width = 61,
		.height = 96,
	},
	.bus_format = MEDIA_BUS_FMT_RGB565_1X16,
};

static int panel_dev_get_fixed_modes(struct panel_dev *panel)
{
	struct drm_connector *connector = panel->base.connector;
	struct drm_device *drm = panel->base.drm;
	struct drm_display_mode *mode;
	unsigned int i, num = 0;

	if (!panel->desc)
		return 0;

	for (i = 0; i < panel->desc->num_timings; i++) {
		const struct display_timing *dt = &panel->desc->timings[i];
		struct videomode vm;

		videomode_from_timing(dt, &vm);
		mode = drm_mode_create(drm);
		if (!mode) {
			dev_err(drm->dev, "failed to add mode %ux%u\n",
				dt->hactive.typ, dt->vactive.typ);
			continue;
		}

		drm_display_mode_from_videomode(&vm, mode);
		drm_mode_set_name(mode);

		drm_mode_probed_add(connector, mode);
		num++;
	}

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

static int panel_dev_get_timings(struct drm_panel *panel,
				    unsigned int num_timings,
				    struct display_timing *timings)
{
	struct panel_dev *p = to_panel_dev(panel);
	unsigned int i;

	if (p->desc->num_timings < num_timings)
		num_timings = p->desc->num_timings;

	if (timings)
		for (i = 0; i < num_timings; i++)
			timings[i] = p->desc->timings[i];

	return p->desc->num_timings;
}

static const struct drm_panel_funcs drm_panel_funs = {
	.disable = panel_dev_disable,
	.unprepare = panel_dev_unprepare,
	.prepare = panel_dev_prepare,
	.enable = panel_dev_enable,
	.get_modes = panel_dev_get_modes,
	.get_timings = panel_dev_get_timings,
};

static int panel_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct panel_dev *panel;
	struct backlight_properties props;

	memset(&props, 0, sizeof(props));
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
	panel_set_power(panel, FB_BLANK_UNBLANK);

	props.type = BACKLIGHT_RAW;
	props.max_brightness = 255;
	panel->backlight = backlight_device_register("pwm-backlight.0",
						&pdev->dev, panel,
						&panel_backlight_ops,
						&props);
	if (IS_ERR_OR_NULL(panel->backlight)) {
		dev_err(panel->dev, "failed to register 'pwm-backlight.0'.\n");
		goto err_of_parse;
	}
	panel->backlight->props.brightness = props.max_brightness;
	backlight_update_status(panel->backlight);

	panel->desc = &kd035hvfmd057_desc;
	drm_panel_init(&panel->base);
	panel->base.dev = &pdev->dev;
	panel->base.funcs = &drm_panel_funs;
	drm_panel_add(&panel->base);

	return 0;

err_of_parse:
	kfree(panel);
	return ret;
}

static int panel_remove(struct platform_device *pdev)
{
	struct panel_dev *panel = dev_get_drvdata(&pdev->dev);

	panel_set_power(panel, FB_BLANK_POWERDOWN);
	return 0;
}

#ifdef CONFIG_PM
static int panel_suspend(struct device *dev)
{
	struct panel_dev *panel = dev_get_drvdata(dev);

	panel_set_power(panel, FB_BLANK_POWERDOWN);
	return 0;
}

static int panel_resume(struct device *dev)
{
	struct panel_dev *panel = dev_get_drvdata(dev);

	panel_set_power(panel, FB_BLANK_UNBLANK);
	return 0;
}

static const struct dev_pm_ops panel_pm_ops = {
	.suspend = panel_suspend,
	.resume = panel_resume,
};
#endif
static const struct of_device_id panel_of_match[] = {
	{ .compatible = "ingenic,kd035hvfmd057", },
	{},
};

static struct platform_driver panel_driver = {
	.probe		= panel_probe,
	.remove		= panel_remove,
	.driver		= {
		.name	= "kd035hvfmd057",
		.of_match_table = panel_of_match,
#ifdef CONFIG_PM
		.pm = &panel_pm_ops,
#endif
	},
};

module_platform_driver(panel_driver);
