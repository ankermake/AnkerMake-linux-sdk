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
	u32 color_format;
	u32 lcd_type;
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
	struct board_gpio vdd_en;
	struct board_gpio cs;
	struct board_gpio rst;
	struct board_gpio pwm;
};

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)
static int panel_set_power(struct panel_dev *panel, int power)
{
	struct board_gpio *vdd_en = &panel->vdd_en;
	struct board_gpio *rst = &panel->rst;
	struct board_gpio *pwm = &panel->pwm;
	if(POWER_IS_ON(power) && !POWER_IS_ON(panel->power)) {
		gpio_direction_output(vdd_en->gpio, 1);
		gpio_direction_output(rst->gpio, 1);
		gpio_direction_output(pwm->gpio, 1);
	}
	if(!POWER_IS_ON(power) && POWER_IS_ON(panel->power)) {
		gpio_direction_output(vdd_en->gpio, 0);
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

	panel->vdd_en.gpio = of_get_named_gpio_flags(np, "ingenic,vdd-en-gpio", 0, &flags);
	if(gpio_is_valid(panel->vdd_en.gpio)) {
		panel->vdd_en.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->vdd_en.gpio, GPIOF_DIR_OUT, "vdd_en");
		if(ret < 0) {
			dev_err(dev, "Failed to request vdd_en pin!\n");
			return ret;
		}
	} else {
		dev_warn(dev, "invalid gpio vdd_en.gpio: %d\n", panel->vdd_en.gpio);
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
	panel->pwm.gpio = of_get_named_gpio_flags(np, "ingenic,pwm-gpio", 0, &flags);
	if(gpio_is_valid(panel->pwm.gpio)) {
		panel->pwm.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->pwm.gpio, GPIOF_DIR_OUT, "pwm");
		if(ret < 0) {
			dev_err(dev, "Failed to request pwm pin!\n");
			goto err_request_pwm;
		}
	} else {
		dev_warn(dev, "invalid gpio pwm.gpio: %d\n", panel->pwm.gpio);
	}

	return 0;
err_request_pwm:
	if(gpio_is_valid(panel->rst.gpio))
		gpio_free(panel->rst.gpio);
err_request_rst:
	if(gpio_is_valid(panel->vdd_en.gpio))
		gpio_free(panel->vdd_en.gpio);
	return ret;
}
static inline struct panel_dev *to_panel_dev(struct drm_panel *panel)
{
	return container_of(panel, struct panel_dev, base);
}
static int panel_dev_unprepare(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);

	panel_set_power(p, FB_BLANK_POWERDOWN);
	return 0;
}

static int panel_dev_prepare(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);

	panel_set_power(p, FB_BLANK_UNBLANK);
	return 0;
}

static int panel_dev_enable(struct drm_panel *panel)
{
//	struct panel_dev *p = to_panel_dev(panel);

	return 0;
}

static int panel_dev_disable(struct drm_panel *panel)
{
//	struct panel_dev *p = to_panel_dev(panel);

	return 0;
}

static struct tft_config y88249_cfg = {
	.pix_clk_inv = 0,
	.de_dl = 0,
	.sync_dl = 0,
	.color_even = TFT_LCD_COLOR_EVEN_RGB,
	.color_odd = TFT_LCD_COLOR_ODD_RGB,
	.mode = TFT_LCD_MODE_PARALLEL_888,
};

static struct lcd_panel lcd_panel = {
	.tft_config = &y88249_cfg,
};

static const struct drm_display_mode y88249_mode = {
	.clock = 20460,
	.hdisplay = 640,
	.hsync_start = 640 + 20,
	.hsync_end = 640 + 20 + 2,
	.htotal = 640 + 20 + 2 + 20,
	.vdisplay = 480,
	.vsync_start = 480 + 12,
	.vsync_end = 480 + 12 + 2,
	.vtotal = 480 + 12 + 2 + 6,
	.vrefresh = 60,
	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
	.private = (int *)&lcd_panel,
};

static const struct panel_desc y88249_desc = {
	.modes = &y88249_mode,
	.num_modes = 1,
	.bpc = 6,
	.size = {
		.width = 69,
		.height = 52,
	},
	.bus_format = MEDIA_BUS_FMT_RGB888_1X24,
};

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

	panel->desc = &y88249_desc;
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
	{ .compatible = "ingenic,y88249", },
	{},
};

static struct platform_driver panel_driver = {
	.probe		= panel_probe,
	.remove		= panel_remove,
	.driver		= {
		.name	= "y88249",
		.of_match_table = panel_of_match,
#ifdef CONFIG_PM
		.pm = &panel_pm_ops,
#endif
	},
};

module_platform_driver(panel_driver);
