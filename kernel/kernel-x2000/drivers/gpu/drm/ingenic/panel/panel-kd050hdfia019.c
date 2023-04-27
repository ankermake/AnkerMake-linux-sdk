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

struct dsi_cmd_packet kd050hdfia019_cmd_list[] = {
	/* for KD050FWFIA019-C019A */
	{0x39, 0x06, 0x00, {0xFF, 0xFF, 0x98, 0x06, 0x04, 0x01}},
	{0x15, 0x08, 0x10},
	{0x15, 0x20, 0x00},
	{0x15, 0x21, 0x01},
	{0x15, 0x30, 0x01},
	{0x15, 0x31, 0x00},
	{0x15, 0x40, 0x16},
	{0x15, 0x41, 0x33},
	{0x15, 0x42, 0x03},
	{0x15, 0x43, 0x09},
	{0x15, 0x44, 0x06},
	{0x15, 0x50, 0x88},
	{0x15, 0x51, 0x88},
	{0x15, 0x52, 0x00},
	{0x15, 0x53, 0x49},
	{0x15, 0x55, 0x49},
	{0x15, 0x60, 0x07},
	{0x15, 0x61, 0x00},
	{0x15, 0x62, 0x07},
	{0x15, 0x63, 0x00},
	{0x15, 0xA0, 0x00},
	{0x15, 0xA1, 0x09},
	{0x15, 0xA2, 0x11},
	{0x15, 0xA3, 0x0B},
	{0x15, 0xA4, 0x05},
	{0x15, 0xA5, 0x08},
	{0x15, 0xA6, 0x06},
	{0x15, 0xA7, 0x04},
	{0x15, 0xA8, 0x09},
	{0x15, 0xA9, 0x0C},
	{0x15, 0xAA, 0x15},
	{0x15, 0xAB, 0x08},
	{0x15, 0xAC, 0x0F},
	{0x15, 0xAD, 0x12},
	{0x15, 0xAE, 0x09},
	{0x15, 0xAF, 0x00},
	{0x15, 0xC0, 0x00},
	{0x15, 0xC1, 0x09},
	{0x15, 0xC2, 0x10},
	{0x15, 0xC3, 0x0C},
	{0x15, 0xC4, 0x05},
	{0x15, 0xC5, 0x08},
	{0x15, 0xC6, 0x06},
	{0x15, 0xC7, 0x04},
	{0x15, 0xC8, 0x08},
	{0x15, 0xC9, 0x0C},
	{0x15, 0xCA, 0x14},
	{0x15, 0xCB, 0x08},
	{0x15, 0xCC, 0x0F},
	{0x15, 0xCD, 0x11},
	{0x15, 0xCE, 0x09},
	{0x15, 0xCF, 0x00},
	{0x39, 0x06, 0x00,{0xFF, 0xFF, 0x98, 0x06, 0x04, 0x06}},
	{0x15, 0x00, 0x20},
	{0x15, 0x01, 0x0A},
	{0x15, 0x02, 0x00},
	{0x15, 0x03, 0x00},
	{0x15, 0x04, 0x01},
	{0x15, 0x05, 0x01},
	{0x15, 0x06, 0x98},
	{0x15, 0x07, 0x06},
	{0x15, 0x08, 0x01},
	{0x15, 0x09, 0x80},
	{0x15, 0x0A, 0x00},
	{0x15, 0x0B, 0x00},
	{0x15, 0x0C, 0x01},
	{0x15, 0x0D, 0x01},
	{0x15, 0x0E, 0x05},
	{0x15, 0x0F, 0x00},
	{0x15, 0x10, 0xF0},
	{0x15, 0x11, 0xF4},
	{0x15, 0x12, 0x01},
	{0x15, 0x13, 0x00},
	{0x15, 0x14, 0x00},
	{0x15, 0x15, 0xC0},
	{0x15, 0x16, 0x08},
	{0x15, 0x17, 0x00},
	{0x15, 0x18, 0x00},
	{0x15, 0x19, 0x00},
	{0x15, 0x1A, 0x00},
	{0x15, 0x1B, 0x00},
	{0x15, 0x1C, 0x00},
	{0x15, 0x1D, 0x00},
	{0x15, 0x20, 0x01},
	{0x15, 0x21, 0x23},
	{0x15, 0x22, 0x45},
	{0x15, 0x23, 0x67},
	{0x15, 0x24, 0x01},
	{0x15, 0x25, 0x23},
	{0x15, 0x26, 0x45},
	{0x15, 0x27, 0x67},
	{0x15, 0x30, 0x11},
	{0x15, 0x31, 0x11},
	{0x15, 0x32, 0x00},
	{0x15, 0x33, 0xEE},
	{0x15, 0x34, 0xFF},
	{0x15, 0x35, 0xBB},
	{0x15, 0x36, 0xAA},
	{0x15, 0x37, 0xDD},
	{0x15, 0x38, 0xCC},
	{0x15, 0x39, 0x66},
	{0x15, 0x3A, 0x77},
	{0x15, 0x3B, 0x22},
	{0x15, 0x3C, 0x22},
	{0x15, 0x3D, 0x22},
	{0x15, 0x3E, 0x22},
	{0x15, 0x3F, 0x22},
	{0x15, 0x40, 0x22},
	{0x39, 0x06, 0x00,{0xFF, 0xFF, 0x98, 0x06, 0x04, 0x07}},
	{0x15, 0x17, 0x22},
	{0x15, 0x02, 0x77},
	{0x15, 0x26, 0xB2},
	{0x39, 0x06, 0x00,{0xFF, 0xFF, 0x98, 0x06, 0x04, 0x00}},
	{0x15, 0x3A, 0x70},
	{0x05, 0x11, 0x00},
//	{0x05, 0x29, 0x00}, display on
 };

static void panel_dev_sleep_in(struct panel_dev *panel)
{
	struct drm_connector *connector = panel->base.connector;
	struct dsi_cmd_packet data_to_send = {0x05, 0x10, 0x00};

	ingenic_dsi_write_comand(connector, &data_to_send);
}

static void panel_dev_sleep_out(struct panel_dev *panel)
{
	struct drm_connector *connector = panel->base.connector;
	struct dsi_cmd_packet data_to_send = {0x05, 0x11, 0x00};

	ingenic_dsi_write_comand(connector, &data_to_send);
}

static void panel_dev_display_on(struct panel_dev *panel)
{
	struct drm_connector *connector = panel->base.connector;
	struct dsi_cmd_packet data_to_send = {0x05, 0x29, 0x00};

	ingenic_dsi_write_comand(connector, &data_to_send);
}

static void panel_dev_display_off(struct panel_dev *panel)
{
	struct drm_connector *connector = panel->base.connector;
	struct dsi_cmd_packet data_to_send = {0x05, 0x28, 0x00};

	ingenic_dsi_write_comand(connector, &data_to_send);
}

static void panel_dev_panel_init(struct panel_dev *panel)
{
	struct drm_connector *connector = panel->base.connector;
	int  i;

	for(i = 0; i < ARRAY_SIZE(kd050hdfia019_cmd_list); i++) {
		ingenic_dsi_write_comand(connector,  &kd050hdfia019_cmd_list[i]);
	}
}

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)
static int panel_set_power(struct panel_dev *panel, int power)
{

	if(POWER_IS_ON(power) && !POWER_IS_ON(panel->power)) {
	}
	if(!POWER_IS_ON(power) && POWER_IS_ON(panel->power)) {
	}

	panel->power = power;
        return 0;
}

static int of_panel_parse(struct device *dev)
{
	return 0;
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
	struct panel_dev *p = to_panel_dev(panel);

	panel_dev_panel_init(p);
	msleep(120);
	panel_dev_sleep_out(p);
	msleep(120);
	panel_dev_display_on(p);
	msleep(80);

	return 0;
}

static int panel_dev_disable(struct drm_panel *panel)
{
	struct panel_dev *p = to_panel_dev(panel);

	panel_dev_display_off(p);
	panel_dev_sleep_in(p);

	return 0;
}

static struct jzdsi_data jzdsi_pdata = {
	.video_config.no_of_lanes = 2,
	.video_config.virtual_channel = 0,
	.video_config.color_coding = COLOR_CODE_24BIT,
	.video_config.video_mode = VIDEO_BURST_WITH_SYNC_PULSES,
	.video_config.receive_ack_packets = 0,	/* enable receiving of ack packets */
	.video_config.is_18_loosely = 0,
	.video_config.data_en_polarity = 1,
	.video_config.byte_clock = 0, // driver will auto calculate byte_clock.
	.video_config.byte_clock_coef = MIPI_PHY_BYTE_CLK_COEF_MUL3_DIV2, //for auto calculate byte clock

	.dsi_config.max_lanes = 2,
	.dsi_config.max_hs_to_lp_cycles = 100,
	.dsi_config.max_lp_to_hs_cycles = 40,
	.dsi_config.max_bta_cycles = 4095,
	.dsi_config.color_mode_polarity = 1,
	.dsi_config.shut_down_polarity = 1,
	.dsi_config.max_bps = 650,
	.bpp_info = 24,
};

static struct tft_config kd050hdfia019_cfg = {
	.de_dl = 0,
	.sync_dl = 0,
	.color_even = TFT_LCD_COLOR_EVEN_RGB,
	.color_odd = TFT_LCD_COLOR_ODD_RGB,
	.mode = TFT_LCD_MODE_PARALLEL_888,
};

static struct lcd_panel lcd_panel = {
	.dsi_pdata = &jzdsi_pdata,
	.tft_config = &kd050hdfia019_cfg,
};

static const struct drm_display_mode display_mode = {
	.clock = 24595,
	.hdisplay = 480,
	.hsync_start = 480 + 50, /*hdisplay + right_margin*/
	.hsync_end = 480 + 50 + 8, /*hdisplay + right_margin + hsync*/
	.htotal = 480 + 50 + 8 + 40, /*hdisplay + right_margin + hsync + left_margin*/
	.vdisplay = 854,
	.vsync_start = 854 + 4, /*vdisplay + lower_margin*/
	.vsync_end = 854 + 4 + 10, /*vdisplay + lower_margin + vsync*/
	.vtotal = 854 + 4 + 10 + 16, /*vdisplay + lower_margin + vsync + upper_margin*/
	.vrefresh = 60,
	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
	.private = (int *)&lcd_panel
};

static const struct panel_desc panel_desc = {
	.modes = &display_mode,
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

	panel->desc = &panel_desc;
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

	panel_dev_display_off(panel);
	panel_dev_sleep_in(panel);
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
	{ .compatible = "ingenic,kd050hdfia019", },
	{},
};

static struct platform_driver panel_driver = {
	.probe		= panel_probe,
	.remove		= panel_remove,
	.driver		= {
		.name	= "kd050hdfia019",
		.of_match_table = panel_of_match,
#ifdef CONFIG_PM
		.pm = &panel_pm_ops,
#endif
	},
};

module_platform_driver(panel_driver);
