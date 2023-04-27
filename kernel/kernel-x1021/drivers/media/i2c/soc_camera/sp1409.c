/*
 * sp1409.c
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <linux/videodev2.h>
#include <media/soc_camera.h>
#include <media/soc_mediabus.h>

#define SP1409_CHIP_ID_H		(0x14)
#define SP1409_CHIP_ID_L		(0x09)

#define SP1409_REG_END			0xff
#define SP1409_REG_DELAY		0xfefe

#define SP1409_SUPPORT_PCLK (24000*1000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5
#define DRIVE_CAPABILITY_1

struct sensor_info {
	struct v4l2_subdev sd;
	struct sensor_format_struct *fmt;  /* Current format */
	int	width;
	int	height;
	int hflip;
	int vflip;
	unsigned int max_again;
	unsigned int max_dgain;
	unsigned int min_integration_time;
	unsigned int min_integration_time_native;
	unsigned int max_integration_time_native;
	unsigned int integration_time_limit;
	unsigned int total_width;
	unsigned int total_height;
	unsigned int max_integration_time;
	unsigned int integration_time_apply_delay;
	unsigned int again_apply_delay;
	unsigned int dgain_apply_delay;
};

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

static struct regval_list sp1409_init_regs_1280_720_25fps[] = {
	{0xfd, 0x00},
	{0x24, 0x01},
//	{0x27, 0x31},
	{0x27, 0x31},
//	{0x26, 0x03},
	{0x26, 0x03},
	{0x28, 0x00},
	{0x40, 0x00},
	{0x25, 0x00},
	{0x41, 0x01},
	{0x2e, 0x01}, //

	{0x89, 0x01},
	{0x90, 0x02},
	{0x69, 0x05},
	{0x6a, 0x00},
	{0x6b, 0x02},
	{0x6c, 0xD0},
	{0xfd, 0x01},
	{0x1f, 0x00},
//	{0x1e, 0x00},

	{0xfd, 0x00},
	{0x3d, 0x00},
	{0x43, 0x03},
	{0x45, 0x05},
	{0x46, 0x03},
	{0x47, 0x07},
	{0x49, 0x00},
	{0x3f, 0x09},
	{0x1d, 0x65},
	{0x1e, 0x55},
	{0x58, 0x08},
	//{0x38, 0x00},
	//{0x55, 0x00},


	{0xfd, 0x01},
	{0x15, 0x02},
	{0x0a, 0x01},
	{0x2e, 0x0a},
	{0x30, 0x20},
	{0x31, 0x02},
	{0x33, 0x02},
	{0x34, 0x40},
	{0x36, 0x10},
	{0x39, 0x01},
	{0x3a, 0x67},
	{0x3c, 0x07},
	{0x3d, 0x03},
	{0x3e, 0x00},
	{0x3f, 0x0B},
	{0x40, 0x00},
	{0x41, 0x3B},
	{0x4a, 0x49},
	{0x4b, 0x03},
	{0x4c, 0x90},
	{0x4f, 0x00},
	{0x50, 0x10},
	{0x4d, 0x03},
	{0x4e, 0x00},
	{0x5c, 0x03},
	{0x64, 0x01},
	{0x65, 0xD0},
	{0x75, 0x06},
	{0x7b, 0x01},
	{0x7c, 0x55},
	{0x20, 0x30},
	{0x7e, 0x01},
	{0x1a, 0x01},
	{0x1b, 0xbe},
	{0xfe, 0x01},

	{0xfd, 0x01},
	{0x77, 0x00},
	{0x78, 0x07},
	{0xfd, 0x02},
	{0x20, 0x00},
	{0x50, 0x03},
	{0x53, 0x03},
	{0x51, 0x03},
	{0x52, 0x04},
	{0x31, 0x01},
	{0x32, 0x81},
	{0x33, 0x03},
	{0x34, 0x80},
	{0x35, 0x00},
	{0x30, 0x09},//auto blc 0x09
	{0x80, 0x0f},//bad pixel
	{0x81, 0x1b},
	{0x92, 0x20},

	{0xfd, 0x01},
	{0x16, 0x00},
	{0x17, 0x0e},
	{0xfd, 0x01},

	{0x06, 0x05},
	{0x07, 0x00},
	{0x08, 0x02},
	{0x09, 0xD0},

	{0xfd, 0x01},
	{0x02, 0x00},
	{0x03, 0x18},
	{0x04, 0x00},
	{0x05, 0x10},

	{0xfd, 0x01},
	{0x0c, 0x02},
	{0x0d, 0xeb},
	{0xfd, 0x01},
	{0x24, 0x20},
	{0x22, 0x00},
	{0x23, 0x10},
	{0xfd, 0x02},
	{0x70, 0x10},
	{0x60, 0x00},
	{0x61, 0x80},
	{0x62, 0x00},
	{0x63, 0x80},
	{0x64, 0x00},
	{0x65, 0x80},
	{0x66, 0x00},
	{0x67, 0x80},
	{0x68, 0x00},
	{0x69, 0x40},
	{0x6a, 0x00},
	{0x6b, 0x40},
	{0x6c, 0x00},
	{0x6d, 0x40},
	{0x6e, 0x00},
	{0x6f, 0x40},
	/* {0xfd, 0x02}, */
	/* {0x20, 0x01},//color */
	{0xfe, 0x01},
	{0xfe, 0x01},
	{0xfd, 0x00},
	{0x1e, 0x00},
	{SP1409_REG_END, 0x00},	/* END MARKER */
};

/*
 * the part of driver was fixed.
 */

struct sensor_format_struct {
	__u8 *desc;
	//__u32 pixelformat;
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
	struct regval_list *regs;
	int	regs_size;
	int bpp;   /* Bytes per pixel */
};

struct sensor_format_struct sensor_formats[] = {
	{
		.desc		= "grey",
		.mbus_code	= V4L2_MBUS_FMT_Y8_1X8,
		.colorspace	= V4L2_COLORSPACE_JPEG,
		.regs 		= NULL,
		.regs_size	= 0,
		.bpp		= 1,
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

static struct sensor_win_size {
	int	width;
	int	height;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	struct regval_list *regs; /* Regs to tweak */
	int regs_size;
	int (*set_size) (struct v4l2_subdev *sd);
} sensor_win_sizes[] = {
	{
		.width		= 1280,
		.height		= 720,
		.regs 		= NULL,
		.regs_size	= 0,
		.set_size	= NULL,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static struct regval_list sp1409_stream_on[] = {
	{0xfd, 0x01},
	{0x1e, 0x00},
	{0xfe, 0x02},
	{SP1409_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sp1409_stream_off[] = {
	{0xfd, 0x01},
	{0x1e, 0xff},
	{0xfd, 0x00},
	{0xfe, 0x02},
	{SP1409_REG_END, 0x00},	/* END MARKER */
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}

int sp1409_read(struct v4l2_subdev *sd, unsigned short reg, unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[2] = {
		[0] = {
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= &reg,
		},
		[1] = {
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= value,
		}
	};
	int ret;
	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;

	return ret;

}

int sp1409_write(struct v4l2_subdev *sd, unsigned char reg, unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[2] = {reg, value};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 2,
		.buf	= buf,
	};
	int ret;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}

static int sp1409_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != SP1409_REG_END) {
		if (vals->reg_num == SP1409_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = sp1409_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}

		vals++;
	}

	return 0;
}

static int sp1409_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SP1409_REG_END) {
		if (vals->reg_num == SP1409_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = sp1409_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}

		vals++;
	}

	return 0;
}

static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned index,
                 enum v4l2_mbus_pixelcode *code)
{
	if (index >= N_FMTS)
		return -EINVAL;

	*code = sensor_formats[index].mbus_code;

	return 0;
}


static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt,
		struct sensor_format_struct **ret_fmt,
		struct sensor_win_size **ret_wsize)
{
	int index;
	struct sensor_win_size *wsize;
	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].mbus_code == fmt->code)
			break;

	if (index >= N_FMTS) {
		/* default to first format */
		index = 0;
		fmt->code = sensor_formats[0].mbus_code;
	}

	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;

	/*
	 * Fields: the sensor devices claim to be progressive.
	 */
	fmt->field = V4L2_FIELD_NONE;


	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;

	if (wsize >= sensor_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;
	fmt->height = wsize->height;

	return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd,
             struct v4l2_mbus_framefmt *fmt)
{
	return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

/*
 * Set a format.
 */
static int sensor_s_fmt(struct v4l2_subdev *sd,
             struct v4l2_mbus_framefmt *fmt)
{
	int ret;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);

	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;


	if (wsize->regs)
	{
		ret = sp1409_write_array(sd, sensor_fmt->regs);
		if (ret < 0)
			return ret;
	}

	if (wsize->regs)
	{
		ret = sp1409_write_array(sd, wsize->regs);
		if (ret < 0)
			return ret;
	}

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;

	return 0;
}

static int sp1409_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	int ret;
	unsigned char v;
	unsigned char page=0;
	sp1409_read(sd, 0xfd, &page);
	if(page!=0){
		sp1409_write(sd, 0xfd, 0x0);
		sp1409_write(sd, 0xfe, 0x2);
	}

	ret = sp1409_read(sd, 0x04, &v);
	if (ret < 0)
		return ret;

	if (v != SP1409_CHIP_ID_H)
		return -ENODEV;
	*ident = v;
	ret = sp1409_read(sd, 0x05, &v);
	if (ret < 0)
		return ret;

	if (v != SP1409_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;

	return 0;
}

static int sp1409_init(struct v4l2_subdev *sd, u32 enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = 0;

	ret = sp1409_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an sp1409 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}

	ret = sp1409_write_array(sd, sp1409_init_regs_1280_720_25fps);
	if (ret)
		return ret;
	return 0;
}

static int sp1409_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = sp1409_write_array(sd, sp1409_stream_on);
		printk("sp1409 stream on\n");
	}
	else {
		ret = sp1409_write_array(sd, sp1409_stream_off);
		printk("sp1409 stream off\n");
	}
	return ret;
}

static int sp1409_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = 0;

	ret = sp1409_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an sp1409 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "sp1409 chip found @ 0x%02x (%s)\n",
		 client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int sp1409_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);
	int ret;

	if (!on)
		return soc_camera_power_off(&client->dev, ssdd);

	ret = soc_camera_power_on(&client->dev, ssdd);
	if (ret < 0)
		return ret;

	return sp1409_init(sd, on);
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sp1409_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = sp1409_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int sp1409_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	sp1409_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
		struct v4l2_mbus_config *cfg)
{

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);

	cfg->flags = V4L2_MBUS_PCLK_SAMPLE_RISING | V4L2_MBUS_MASTER |
		    V4L2_MBUS_VSYNC_ACTIVE_HIGH | V4L2_MBUS_HSYNC_ACTIVE_HIGH |
			    V4L2_MBUS_DATA_ACTIVE_HIGH;
	cfg->type = V4L2_MBUS_PARALLEL;

	cfg->flags = soc_camera_apply_board_flags(ssdd, cfg);

	return 0;
}

static const struct v4l2_subdev_core_ops sp1409_core_ops = {
	.g_chip_ident = sp1409_g_chip_ident,
	.init = sp1409_init,
	.s_power = sp1409_s_power,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = sp1409_g_register,
	.s_register = sp1409_s_register,
#endif
};

static const struct v4l2_subdev_video_ops sp1409_video_ops = {
	.enum_mbus_fmt = sensor_enum_fmt,
	.try_mbus_fmt = sensor_try_fmt,
	.s_mbus_fmt = sensor_s_fmt,
	.s_stream = sp1409_s_stream,
	.g_mbus_config  = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sp1409_ops = {
	.core = &sp1409_core_ops,
	.video = &sp1409_video_ops,
};

static int sp1409_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *sensor;

	sensor = (struct sensor_info *)kzalloc(sizeof(*sensor), GFP_KERNEL);
	if(!sensor){
		printk("Failed to allocate sensor subdev.\n");
		return -ENOMEM;
	}
	memset(sensor, 0 ,sizeof(*sensor));
	sensor->fmt = &sensor_formats[0];
	sd = &sensor->sd;

	v4l2_i2c_subdev_init(sd, client, &sp1409_ops);

	return 0;
}

static int sp1409_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sp1409_id[] = {
	{ "sp1409_front", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, sp1409_id);

static struct i2c_driver sp1409_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "sp1409_front",
	},
	.probe		= sp1409_probe,
	.remove		= sp1409_remove,
	.id_table	= sp1409_id,
};

static __init int init_sp1409(void)
{
	return i2c_add_driver(&sp1409_driver);
}

static __exit void exit_sp1409(void)
{
	i2c_del_driver(&sp1409_driver);
}

module_init(init_sp1409);
module_exit(exit_sp1409);

MODULE_DESCRIPTION("A low-level driver for sp1409 sensors");
MODULE_LICENSE("GPL");
