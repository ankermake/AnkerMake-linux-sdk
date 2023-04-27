/* linux/drivers/video/exynos/visionox_t078zc04h01.c
 *
 * MIPI-DSI based visionox_t078zc04h01 AMOLED lcd 4.65 inch panel driver.
 *
 * Inki Dae, <inki.dae@samsung.com>
 * Donghwa Lee, <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/lcd.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <mach/jz_dsim.h>

#define POWER_IS_ON(pwr)    ((pwr) == FB_BLANK_UNBLANK)
#define POWER_IS_OFF(pwr)   ((pwr) == FB_BLANK_POWERDOWN)
#define POWER_IS_NRM(pwr)   ((pwr) == FB_BLANK_NORMAL)

#define lcd_to_master(a)    (a->dsim_dev->master)
#define lcd_to_master_ops(a)    ((lcd_to_master(a))->master_ops)

struct visionox_t078zc04h01
{
	struct device   *dev;
	unsigned int            power;
	unsigned int            id;

	struct lcd_device   *ld;
	struct backlight_device *bd;

	struct mipi_dsim_lcd_device *dsim_dev;
	struct lcd_platform_data    *ddi_pd;
	struct mutex            lock;
	struct regulator *lcd_vcc_reg;
	bool  enabled;
};

static struct dsi_cmd_packet visionox_t078zc04h01_cmd_list1[] =
{
	{0x15, 0xCD, 0x99},

	{0x39, 23, 0x00,  {0x24, 0x0A, 0x06, 0x09, 0x05, 0x07, 0x0B, 0x08, 0x0C, 0x01, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x11, 0x12, 0x14, 0x14, 0x13, 0x10, 0x14}},

	{0x15, 0x2B, 0x08},

	{0x39, 23, 0x00,  {0x25, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14}},

	{0x39, 3, 0x00, {0x72, 0x00, 0x80}},

	{0x15, 0x3A, 0x10},
	{0x15, 0x29, 0x91},
	{0x15, 0x36, 0x40},
	{0x15, 0x67, 0x82},
	{0x15, 0x69, 0x47},
	{0x15, 0x6C, 0x08},

	{0x39, 20, 0x00, {0x53, 0x1F, 0x1C, 0x1B, 0x18, 0x19, 0x1A, 0x1C, 0x1E, 0x1E, 0x19, 0x14, 0x12, 0x10, 0x12, 0x10, 0x11, 0x0E, 0x0D, 0x0C}},

	{0x39, 20, 0x00, {0x54, 0x1F, 0x1C, 0x1B, 0x18, 0x19, 0x1A, 0x1C, 0x1E, 0x1E, 0x19, 0x14, 0x12, 0x10, 0x12, 0x10, 0x11, 0x0E, 0x0D, 0x0C}},

	{0x15, 0x6D, 0x04},
	{0x15, 0x58, 0x01},

	{0x39, 9, 0x00, {0x55, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}},

	{0x15, 0x33, 0x38},
	{0x15, 0x32, 0x00},
	{0x15, 0x35, 0x27},
	{0x15, 0x63, 0x45}, // 2 lane
	{0x15, 0x4F, 0x3A},

	{0x39, 17, 0x00, {0x56, 0x31, 0x10, 0x31, 0x10, 0x31, 0x10, 0x31, 0x10, 0x31, 0x10, 0x31, 0x10, 0x31, 0x10, 0x31, 0x10}},

	{0x15, 0x4E, 0x3A},

	{0x39, 9, 0x00, {0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x02}},

	{0x15, 0x41, 0x4B},
	{0x15, 0x73, 0x30},
	{0x15, 0x74, 0x10},
	{0x15, 0x76, 0x36},
	{0x15, 0x77, 0x00},
	{0x15, 0x28, 0x31},
	{0x15, 0x7C, 0x80},
	{0x15, 0x2E, 0x04},
	{0x15, 0x4C, 0x80},
	{0x15, 0x47, 0x1F},
	{0x15, 0x48, 0x09},
	{0x15, 0x50, 0xC0},
	{0x15, 0x78, 0x6E},
	{0x15, 0x2D, 0x31},
	{0x15, 0x4D, 0x00},
	{0x15, 0xCD, 0xAA},
	{0x15, 0xCD, 0x99},
	{0x15, 0xCD, 0xAA},
	{0x15, 0x1F, 0x13},
	{0x15, 0x41, 0x68},
};
static struct dsi_cmd_packet visionox_t078zc04h01_cmd_list2[] =
{

	{0x15, 0xCD, 0x99},
	{0x15, 0xCD, 0xAA},
	{0x15, 0x2B, 0x00},
	{0x39, 23, 0x00, {0x25, 0x0A, 0x06, 0x09, 0x05, 0x07, 0x0B, 0x08, 0x0C, 0x01, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x11, 0x12, 0x14, 0x14, 0x14, 0x13, 0x10}},
	{0x15, 0x4D, 0x00},

	{0x15, 0xCD, 0xAA},
	{0x15, 0xCD, 0x99},
	{0x15, 0xCD, 0xAA},
	{0x15, 0x1F, 0x13},
	{0x15, 0x41, 0x68},
};

static void visionox_t078zc04h01_regulator_enable(struct visionox_t078zc04h01 *lcd)
{
	struct lcd_platform_data *pd = NULL;

	pd = lcd->ddi_pd;
	mutex_lock(&lcd->lock);
	if (!regulator_is_enabled(lcd->lcd_vcc_reg))
	{
		regulator_enable(lcd->lcd_vcc_reg);
	}

	msleep(pd->power_on_delay);
	mutex_unlock(&lcd->lock);
}

static void visionox_t078zc04h01_regulator_disable(struct visionox_t078zc04h01 *lcd)
{
	mutex_lock(&lcd->lock);
	if (regulator_is_enabled(lcd->lcd_vcc_reg))
	{
		regulator_disable(lcd->lcd_vcc_reg);
	}
	mutex_unlock(&lcd->lock);
}

static void visionox_t078zc04h01_sleep_in(struct visionox_t078zc04h01 *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x10, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void visionox_t078zc04h01_sleep_out(struct visionox_t078zc04h01 *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x11, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void visionox_t078zc04h01_display_on(struct visionox_t078zc04h01 *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x29, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);

	/* ops->cmd_write(lcd_to_master(lcd), data_to_send); */
	/* msleep(100); */
	/* { */
	/*  struct dsi_cmd_packet data = {0x3e, 0x14, 0x00, {0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,}}; */
	/*  int i; */
	/*  for (i=0;i< 1000;i++) */
	/*      ops->cmd_write(lcd_to_master(lcd), data); */
	/* } */
}

static void visionox_t078zc04h01_display_off(struct visionox_t078zc04h01 *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x28, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void visionox_t078zc04h01_panel_init(struct visionox_t078zc04h01 *lcd)
{
	int  i;
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	for (i = 0; i < ARRAY_SIZE(visionox_t078zc04h01_cmd_list1); i++)
	{
		ops->cmd_write(dsi,  visionox_t078zc04h01_cmd_list1[i]);
	}
	msleep(200);
	for (i = 0; i < ARRAY_SIZE(visionox_t078zc04h01_cmd_list2); i++)
	{
		ops->cmd_write(dsi,  visionox_t078zc04h01_cmd_list2[i]);
	}
}

static int visionox_t078zc04h01_set_power(struct lcd_device *ld, int power)
{
	int ret = 0;
#if 0
	struct visionox_t078zc04h01 *lcd = lcd_get_data(ld);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
	        power != FB_BLANK_NORMAL)
	{
		dev_err(lcd->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	if ((power == FB_BLANK_UNBLANK) && ops->set_blank_mode)
	{
		/* LCD power on */
		if ((POWER_IS_ON(power) && POWER_IS_OFF(lcd->power))
		        || (POWER_IS_ON(power) && POWER_IS_NRM(lcd->power)))
		{
			ret = ops->set_blank_mode(lcd_to_master(lcd), power);
			if (!ret && lcd->power != power)
				lcd->power = power;
		}
	}
	else if ((power == FB_BLANK_POWERDOWN) && ops->set_early_blank_mode)
	{
		/* LCD power off */
		if ((POWER_IS_OFF(power) && POWER_IS_ON(lcd->power)) ||
		        (POWER_IS_ON(lcd->power) && POWER_IS_NRM(power)))
		{
			ret = ops->set_early_blank_mode(lcd_to_master(lcd),
			                                power);
			if (!ret && lcd->power != power)
				lcd->power = power;
		}
	}

#endif
	return ret;
}

static int visionox_t078zc04h01_get_power(struct lcd_device *ld)
{
	struct visionox_t078zc04h01 *lcd = lcd_get_data(ld);

	return lcd->power;
}


static struct lcd_ops visionox_t078zc04h01_lcd_ops =
{
	.set_power = visionox_t078zc04h01_set_power,
	.get_power = visionox_t078zc04h01_get_power,
};


static void visionox_t078zc04h01_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power)
{
	struct visionox_t078zc04h01 *lcd = dev_get_drvdata(&dsim_dev->dev);

	/* lcd power on */
	/* if (power) */
	/*  visionox_t078zc04h01_regulator_enable(lcd); */
	/* else */
	/*  visionox_t078zc04h01_regulator_disable(lcd); */
	if (lcd->ddi_pd->power_on)
		lcd->ddi_pd->power_on(lcd->ld, power);
	/* return ; */
	/* lcd reset */
	if (lcd->ddi_pd->reset)
		lcd->ddi_pd->reset(lcd->ld);
}

extern void dump_dsi_reg(struct dsi_device *dsi);
static void visionox_t078zc04h01_set_sequence(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct visionox_t078zc04h01 *lcd = dev_get_drvdata(&dsim_dev->dev);
	/* struct dsi_device *dsi = lcd_to_master(lcd); */
	/* return ; */

	visionox_t078zc04h01_panel_init(lcd);
	visionox_t078zc04h01_sleep_out(lcd);
	msleep(120);
	visionox_t078zc04h01_display_on(lcd);
	msleep(10);
	/* dump_dsi_reg(dsi); */
	lcd->power = FB_BLANK_UNBLANK;
}

static int visionox_t078zc04h01_probe(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct visionox_t078zc04h01 *lcd;
	lcd = devm_kzalloc(&dsim_dev->dev, sizeof(struct visionox_t078zc04h01), GFP_KERNEL);
	if (!lcd)
	{
		dev_err(&dsim_dev->dev, "failed to allocate visionox_t078zc04h01 structure.\n");
		return -ENOMEM;
	}

	lcd->dsim_dev = dsim_dev;
	lcd->ddi_pd = (struct lcd_platform_data *)dsim_dev->platform_data;
	lcd->dev = &dsim_dev->dev;

	mutex_init(&lcd->lock);

	/* lcd->lcd_vcc_reg = regulator_get(NULL, "lcd_1v8"); */
	/* if (IS_ERR(lcd->lcd_vcc_reg)) { */
	/*  dev_err(lcd->dev, "failed to get regulator vlcd\n"); */
	/*  return PTR_ERR(lcd->lcd_vcc_reg); */
	/* } */

	lcd->ld = lcd_device_register("visionox_t078zc04h01", lcd->dev, lcd,
	                              &visionox_t078zc04h01_lcd_ops);
	if (IS_ERR(lcd->ld))
	{
		dev_err(lcd->dev, "failed to register lcd ops.\n");
		return PTR_ERR(lcd->ld);
	}

	dev_set_drvdata(&dsim_dev->dev, lcd);

	dev_dbg(lcd->dev, "probed visionox_t078zc04h01 panel driver.\n");

	return 0;
}

#ifdef CONFIG_PM
static int visionox_t078zc04h01_suspend(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct visionox_t078zc04h01 *lcd = dev_get_drvdata(&dsim_dev->dev);

	visionox_t078zc04h01_sleep_in(lcd);
	msleep(lcd->ddi_pd->power_off_delay);
	visionox_t078zc04h01_display_off(lcd);

	/* power off */
	if (lcd->ddi_pd->power_on)
		lcd->ddi_pd->power_on(lcd->ld, 0);

	visionox_t078zc04h01_regulator_disable(lcd);

	return 0;
}

static int visionox_t078zc04h01_resume(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct visionox_t078zc04h01 *lcd = dev_get_drvdata(&dsim_dev->dev);

#if 0
	visionox_t078zc04h01_regulator_enable(lcd);

	visionox_t078zc04h01_sleep_out(lcd);
	msleep(lcd->ddi_pd->power_on_delay);

	visionox_t078zc04h01_set_sequence(dsim_dev);
#endif
	return 0;
}
#else
#define visionox_t078zc04h01_suspend       NULL
#define visionox_t078zc04h01_resume        NULL
#endif

static struct mipi_dsim_lcd_driver visionox_t078zc04h01_dsim_ddi_driver =
{
	.name = "visionox_t078zc04h01-lcd",
	.id = -1,

	.power_on = visionox_t078zc04h01_power_on,
	.set_sequence = visionox_t078zc04h01_set_sequence,
	.probe = visionox_t078zc04h01_probe,
	.suspend = visionox_t078zc04h01_suspend,
	.resume = visionox_t078zc04h01_resume,
};

static int visionox_t078zc04h01_init(void)
{
	mipi_dsi_register_lcd_driver(&visionox_t078zc04h01_dsim_ddi_driver);
	return 0;
}

static void visionox_t078zc04h01_exit(void)
{
	return;
}

module_init(visionox_t078zc04h01_init);
module_exit(visionox_t078zc04h01_exit);

MODULE_DESCRIPTION("visionox_t078zc04h01 lcd driver");
MODULE_LICENSE("GPL");
