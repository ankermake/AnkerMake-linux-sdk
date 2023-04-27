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
#include "../jz_dsim.h"

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
	struct board_gpio oled;
	struct board_gpio swire;
};

static struct dsi_cmd_packet visionox_ma0060_720p_cmd_list2[] =
{
	{0x15, 0xFE, 0x00},
	{0x15, 0xC2, 0x08},
	{0x15, 0x35, 0x00},
};

static struct dsi_cmd_packet visionox_ma0060_720p_cmd_list11[] =
{
	{0X39, 0X02, 0X00, {0XFE, 0XD0}},
	{0X39, 0X02, 0X00, {0X07, 0X84}},
	{0X39, 0X02, 0X00, {0X40, 0X02}},
	{0X39, 0X02, 0X00, {0X4B, 0X4C}},
	{0X39, 0X02, 0X00, {0X49, 0X01}},
	{0X39, 0X02, 0X00, {0XFE, 0X40}},
	{0X39, 0X02, 0X00, {0XC7, 0X85}},
	{0X39, 0X02, 0X00, {0XC8, 0X32}},
	{0X39, 0X02, 0X00, {0XC9, 0X18}},
	{0X39, 0X02, 0X00, {0XCA, 0X09}},
	{0X39, 0X02, 0X00, {0XCB, 0X22}},
	{0X39, 0X02, 0X00, {0XCC, 0X44}},
	{0X39, 0X02, 0X00, {0XCD, 0X11}},
	{0X39, 0X02, 0X00, {0X05, 0X0F}},
	{0X39, 0X02, 0X00, {0X06, 0X09}},
	{0X39, 0X02, 0X00, {0X08, 0X0F}},
	{0X39, 0X02, 0X00, {0X09, 0X09}},
	{0X39, 0X02, 0X00, {0X0A, 0XE6}},
	{0X39, 0X02, 0X00, {0X0B, 0X88}},
	{0X39, 0X02, 0X00, {0X0D, 0X90}},
	{0X39, 0X02, 0X00, {0X0E, 0X10}},
	{0X39, 0X02, 0X00, {0X20, 0X93}},
	{0X39, 0X02, 0X00, {0X21, 0X93}},
	{0X39, 0X02, 0X00, {0X24, 0X02}},
	{0X39, 0X02, 0X00, {0X26, 0X02}},
	{0X39, 0X02, 0X00, {0X28, 0X05}},
	{0X39, 0X02, 0X00, {0X2A, 0X05}},
	{0X39, 0X02, 0X00, {0X2D, 0X23}},
	{0X39, 0X02, 0X00, {0X2F, 0X23}},
	{0X39, 0X02, 0X00, {0X30, 0X23}},
	{0X39, 0X02, 0X00, {0X31, 0X23}},
	{0X39, 0X02, 0X00, {0X36, 0X55}},
	{0X39, 0X02, 0X00, {0X37, 0X80}},
	{0X39, 0X02, 0X00, {0X38, 0X50}},
	{0X39, 0X02, 0X00, {0X39, 0X00}},
	{0X39, 0X02, 0X00, {0X46, 0X27}},
	{0X39, 0X02, 0X00, {0X6F, 0X00}},
	{0X39, 0X02, 0X00, {0X74, 0X2F}},
	{0X39, 0X02, 0X00, {0X75, 0X19}},
	{0X39, 0X02, 0X00, {0X79, 0X00}},
	{0X39, 0X02, 0X00, {0XAD, 0X00}},
	{0X39, 0X02, 0X00, {0XFE, 0X60}},
	{0X39, 0X02, 0X00, {0X00, 0XC4}},
	{0X39, 0X02, 0X00, {0X01, 0X00}},
	{0X39, 0X02, 0X00, {0X02, 0X02}},
	{0X39, 0X02, 0X00, {0X03, 0X00}},
	{0X39, 0X02, 0X00, {0X04, 0X00}},
	{0X39, 0X02, 0X00, {0X05, 0X07}},
	{0X39, 0X02, 0X00, {0X06, 0X00}},
	{0X39, 0X02, 0X00, {0X07, 0X83}},
	{0X39, 0X02, 0X00, {0X09, 0XC4}},
	{0X39, 0X02, 0X00, {0X0A, 0X00}},
	{0X39, 0X02, 0X00, {0X0B, 0X02}},
	{0X39, 0X02, 0X00, {0X0C, 0X00}},
	{0X39, 0X02, 0X00, {0X0D, 0X00}},
	{0X39, 0X02, 0X00, {0X0E, 0X08}},
	{0X39, 0X02, 0X00, {0X0F, 0X00}},
	{0X39, 0X02, 0X00, {0X10, 0X83}},
	{0X39, 0X02, 0X00, {0X12, 0XCC}},
	{0X39, 0X02, 0X00, {0X13, 0X0F}},
	{0X39, 0X02, 0X00, {0X14, 0XFF}},
	{0X39, 0X02, 0X00, {0X15, 0X01}},
	{0X39, 0X02, 0X00, {0X16, 0X00}},
	{0X39, 0X02, 0X00, {0X17, 0X02}},
	{0X39, 0X02, 0X00, {0X18, 0X00}},
	{0X39, 0X02, 0X00, {0X19, 0X00}},
	{0X39, 0X02, 0X00, {0X1B, 0XC4}},
	{0X39, 0X02, 0X00, {0X1C, 0X00}},
	{0X39, 0X02, 0X00, {0X1D, 0X02}},
	{0X39, 0X02, 0X00, {0X1E, 0X00}},
	{0X39, 0X02, 0X00, {0X1F, 0X00}},
	{0X39, 0X02, 0X00, {0X20, 0X08}},
	{0X39, 0X02, 0X00, {0X21, 0X00}},
	{0X39, 0X02, 0X00, {0X22, 0X89}},
	{0X39, 0X02, 0X00, {0X24, 0XC4}},
	{0X39, 0X02, 0X00, {0X25, 0X00}},
	{0X39, 0X02, 0X00, {0X26, 0X02}},
	{0X39, 0X02, 0X00, {0X27, 0X00}},
	{0X39, 0X02, 0X00, {0X28, 0X00}},
	{0X39, 0X02, 0X00, {0X29, 0X07}},
	{0X39, 0X02, 0X00, {0X2A, 0X00}},
	{0X39, 0X02, 0X00, {0X2B, 0X89}},
	{0X39, 0X02, 0X00, {0X2F, 0XC4}},
	{0X39, 0X02, 0X00, {0X30, 0X00}},
	{0X39, 0X02, 0X00, {0X31, 0X02}},
	{0X39, 0X02, 0X00, {0X32, 0X00}},
	{0X39, 0X02, 0X00, {0X33, 0X00}},
	{0X39, 0X02, 0X00, {0X34, 0X06}},
	{0X39, 0X02, 0X00, {0X35, 0X00}},
	{0X39, 0X02, 0X00, {0X36, 0X89}},
	{0X39, 0X02, 0X00, {0X38, 0XC4}},
	{0X39, 0X02, 0X00, {0X39, 0X00}},
	{0X39, 0X02, 0X00, {0X3A, 0X02}},
	{0X39, 0X02, 0X00, {0X3B, 0X00}},
	{0X39, 0X02, 0X00, {0X3D, 0X00}},
	{0X39, 0X02, 0X00, {0X3F, 0X07}},
	{0X39, 0X02, 0X00, {0X40, 0X00}},
	{0X39, 0X02, 0X00, {0X41, 0X89}},
	{0X39, 0X02, 0X00, {0X4C, 0XC4}},
	{0X39, 0X02, 0X00, {0X4D, 0X00}},
	{0X39, 0X02, 0X00, {0X4E, 0X04}},
	{0X39, 0X02, 0X00, {0X4F, 0X01}},
	{0X39, 0X02, 0X00, {0X50, 0X00}},
	{0X39, 0X02, 0X00, {0X51, 0X08}},
	{0X39, 0X02, 0X00, {0X52, 0X00}},
	{0X39, 0X02, 0X00, {0X53, 0X61}},
	{0X39, 0X02, 0X00, {0X55, 0XC4}},
	{0X39, 0X02, 0X00, {0X56, 0X00}},
	{0X39, 0X02, 0X00, {0X58, 0X04}},
	{0X39, 0X02, 0X00, {0X59, 0X01}},
	{0X39, 0X02, 0X00, {0X5A, 0X00}},
	{0X39, 0X02, 0X00, {0X5B, 0X06}},
	{0X39, 0X02, 0X00, {0X5C, 0X00}},
	{0X39, 0X02, 0X00, {0X5D, 0X61}},
	{0X39, 0X02, 0X00, {0X5F, 0XCE}},
	{0X39, 0X02, 0X00, {0X60, 0X00}},
	{0X39, 0X02, 0X00, {0X61, 0X07}},
	{0X39, 0X02, 0X00, {0X62, 0X05}},
	{0X39, 0X02, 0X00, {0X63, 0X00}},
	{0X39, 0X02, 0X00, {0X64, 0X04}},
	{0X39, 0X02, 0X00, {0X65, 0X00}},
	{0X39, 0X02, 0X00, {0X66, 0X60}},
	{0X39, 0X02, 0X00, {0X67, 0X80}},
	{0X39, 0X02, 0X00, {0X9B, 0X03}},
	{0X39, 0X02, 0X00, {0XA9, 0X10}},
	{0X39, 0X02, 0X00, {0XAA, 0X10}},
	{0X39, 0X02, 0X00, {0XAB, 0X02}},
	{0X39, 0X02, 0X00, {0XAC, 0X04}},
	{0X39, 0X02, 0X00, {0XAD, 0X03}},
	{0X39, 0X02, 0X00, {0XAE, 0X10}},
	{0X39, 0X02, 0X00, {0XAF, 0X10}},
	{0X39, 0X02, 0X00, {0XB0, 0X10}},
	{0X39, 0X02, 0X00, {0XB1, 0X10}},
	{0X39, 0X02, 0X00, {0XB2, 0X10}},
	{0X39, 0X02, 0X00, {0XB3, 0X10}},
	{0X39, 0X02, 0X00, {0XB4, 0X10}},
	{0X39, 0X02, 0X00, {0XB5, 0X10}},
	{0X39, 0X02, 0X00, {0XB6, 0X10}},
	{0X39, 0X02, 0X00, {0XB7, 0X10}},
	{0X39, 0X02, 0X00, {0XB8, 0X10}},
	{0X39, 0X02, 0X00, {0XB9, 0X08}},
	{0X39, 0X02, 0X00, {0XBA, 0X09}},
	{0X39, 0X02, 0X00, {0XBB, 0X0A}},
	{0X39, 0X02, 0X00, {0XBC, 0X05}},
	{0X39, 0X02, 0X00, {0XBD, 0X06}},
	{0X39, 0X02, 0X00, {0XBE, 0X02}},
	{0X39, 0X02, 0X00, {0XBF, 0X10}},
	{0X39, 0X02, 0X00, {0XC0, 0X10}},
	{0X39, 0X02, 0X00, {0XC1, 0X03}},
	{0X39, 0X02, 0X00, {0XC4, 0X80}},
	{0X39, 0X02, 0X00, {0XFE, 0X70}},
	{0X39, 0X02, 0X00, {0X48, 0X05}},
	{0X39, 0X02, 0X00, {0X52, 0X00}},
	{0X39, 0X02, 0X00, {0X5A, 0XFF}},
	{0X39, 0X02, 0X00, {0X5C, 0XF6}},
	{0X39, 0X02, 0X00, {0X5D, 0X07}},
	{0X39, 0X02, 0X00, {0X7D, 0X75}},
	{0X39, 0X02, 0X00, {0X86, 0X07}},
	{0X39, 0X02, 0X00, {0XA7, 0X02}},
	{0X39, 0X02, 0X00, {0XA9, 0X2C}},
	{0X39, 0X02, 0X00, {0XFE, 0XA0}},
	{0X39, 0X02, 0X00, {0X2B, 0X18}},
	{0x39, 0x02, 0x00, {0xFE, 0xD0}}, /* 【2 lane 设置】 */
	{0x39, 0x02, 0x00, {0x1E, 0x05}},

	{0x39, 0x02, 0x00, {0xFE, 0x00}},
	{0x39, 0x05, 0x00, {0x2A, 0x00, 0x00, 0x02, 0xCF}},
	{0x39, 0x05, 0x00, {0x2B, 0x00, 0x00, 0x04, 0xFF}},
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

	for (i = 0; i < ARRAY_SIZE(visionox_ma0060_720p_cmd_list11); i++)
	{
		ingenic_dsi_write_comand(connector,
			&visionox_ma0060_720p_cmd_list11[i]);
	}
	msleep(20);
	for (i = 0; i < ARRAY_SIZE(visionox_ma0060_720p_cmd_list2); i++)
	{
		ingenic_dsi_write_comand(connector,
			&visionox_ma0060_720p_cmd_list2[i]);
	}
}

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)
static int panel_set_power(struct panel_dev *panel, int power)
{
	struct board_gpio *vdd_en = &panel->vdd_en;
	struct board_gpio *rst = &panel->rst;
	struct board_gpio *oled = &panel->oled;
	struct board_gpio *swire = &panel->swire;

	if(POWER_IS_ON(power) && !POWER_IS_ON(panel->power)) {
		gpio_direction_output(vdd_en->gpio, vdd_en->active_level);
		if (gpio_is_valid(swire->gpio)) {
			gpio_direction_input(swire->gpio);
		}

		if (gpio_is_valid(oled->gpio)) {
			gpio_direction_input(oled->gpio);
		}
		msleep(100);

		gpio_direction_output(rst->gpio, 1);
		msleep(100);
		gpio_direction_output(rst->gpio, 0);
		msleep(150);
		gpio_direction_output(rst->gpio, 1);
		msleep(100);
	}
	if(!POWER_IS_ON(power) && POWER_IS_ON(panel->power)) {
		gpio_direction_output(vdd_en->gpio, !vdd_en->active_level);
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
		panel->vdd_en.active_level = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
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
		panel->rst.active_level = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->rst.gpio, GPIOF_DIR_OUT, "rst");
		if(ret < 0) {
			dev_err(dev, "Failed to request rst pin!\n");
			goto err_request_rst;
		}
	} else {
		dev_warn(dev, "invalid gpio rst.gpio: %d\n", panel->rst.gpio);
	}
	panel->pwm.gpio = of_get_named_gpio_flags(np, "ingenic,lcd-pwm-gpio", 0, &flags);
	if(gpio_is_valid(panel->pwm.gpio)) {
		panel->pwm.active_level = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->pwm.gpio, GPIOF_DIR_OUT, "pwm");
		if(ret < 0) {
			dev_err(dev, "Failed to request pwm pin!\n");
			goto err_request_pwm;
		}
	} else {
		dev_warn(dev, "invalid gpio pwm.gpio: %d\n", panel->pwm.gpio);
	}

	panel->oled.gpio = of_get_named_gpio_flags(np, "ingenic,oled-gpio", 0, &flags);
	if(gpio_is_valid(panel->oled.gpio)) {
		ret = gpio_request_one(panel->oled.gpio, GPIOF_DIR_OUT, "oled");
		if(ret < 0) {
			dev_err(dev, "Failed to request oled pin!\n");
			goto err_request_oled;
		}
	} else {
		dev_warn(dev, "invalid gpio oled.gpio: %d\n", panel->oled.gpio);
	}

	panel->swire.gpio = of_get_named_gpio_flags(np, "ingenic,swire-gpio", 0, &flags);
	if(gpio_is_valid(panel->swire.gpio)) {
		ret = gpio_request_one(panel->swire.gpio, GPIOF_DIR_OUT, "swire");
		if(ret < 0) {
			dev_err(dev, "Failed to request swire pin!\n");
			goto err_request_swire;
		}
	} else {
		dev_warn(dev, "invalid gpio swire.gpio: %d\n", panel->swire.gpio);
	}

	return 0;
err_request_swire:
	if(gpio_is_valid(panel->oled.gpio))
		gpio_free(panel->oled.gpio);
err_request_oled:
	if(gpio_is_valid(panel->pwm.gpio))
		gpio_free(panel->pwm.gpio);
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
	.video_config.byte_clock_coef = MIPI_PHY_BYTE_CLK_COEF_MUL6_DIV5, //for auto calculate byte clock

	.dsi_config.max_lanes = 2,
	.dsi_config.max_hs_to_lp_cycles = 100,
	.dsi_config.max_lp_to_hs_cycles = 40,
	.dsi_config.max_bta_cycles = 4095,
	.dsi_config.color_mode_polarity = 1,
	.dsi_config.shut_down_polarity = 1,
	.dsi_config.max_bps = 2750,
	.bpp_info = 24,
};

struct smart_config smart_cfg = {
	.dc_md = 0,
	.wr_md = 1,
	.smart_type = SMART_LCD_TYPE_8080,
	.pix_fmt = SMART_LCD_FORMAT_888,
	.dwidth = SMART_LCD_DWIDTH_24_BIT,
	.te_mipi_switch = 1,
	.te_switch = 1,
};

static struct lcd_panel lcd_panel = {
	.dsi_pdata = &jzdsi_pdata,
	.smart_config = &smart_cfg,
};

static const struct drm_display_mode display_mode = {
	.clock = 57296,
	.hdisplay = 720,
	.hsync_start = 720,
	.hsync_end = 720,
	.htotal = 720,
	.vdisplay = 1280,
	.vsync_start = 1280,
	.vsync_end = 1280,
	.vtotal = 1280,
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
	{ .compatible = "ingenic,ma0060", },
	{},
};

static struct platform_driver panel_driver = {
	.probe		= panel_probe,
	.remove		= panel_remove,
	.driver		= {
		.name	= "ma0060",
		.of_match_table = panel_of_match,
#ifdef CONFIG_PM
		.pm = &panel_pm_ops,
#endif
	},
};

module_platform_driver(panel_driver);
