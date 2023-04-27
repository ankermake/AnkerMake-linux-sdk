/*
 * A V4L2 driver for OmniVision OV2735a cameras.
 *
 * Copyright 2006 One Laptop Per Child Association, Inc.  Written
 * by Jonathan Corbet with substantial inspiration from Mark
 * McClelland's ovcamchip code.
 *
 * Copyright 2006-7 Jonathan Corbet <corbet@lwn.net>
 *
 * This file may be distributed under the terms of the GNU General
 * Public License, version 2.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/notifier.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-clk.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-image-sizes.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <isp-sensor.h>
#include <riscv.h>
#include <isp-drv.h>

static bool debug;
module_param(debug, bool, 0644);
MODULE_PARM_DESC(debug, "Debug level (0-1)");

#define OV491_REG_END		0xff
#define OV491_REG_DELAY		0x00
#define OV491_PAGE_REG		0xfd

struct ov491_win_size {
	unsigned int width;
	unsigned int height;
	unsigned int mbus_code;
	struct sensor_info sensor_info;
	enum v4l2_colorspace colorspace;
	void *regs;

};

struct ov491_gpio {
	int pin;
	int active_level;
};

struct ov491_info {
	struct v4l2_subdev sd;
	struct v4l2_ctrl_handler hdl;
	struct v4l2_ctrl *gain;
	struct v4l2_ctrl *again;

	struct v4l2_clk *clk;
	struct clk *sclka;

	struct v4l2_ctrl *exposure;

	struct media_pad pad;

	struct v4l2_subdev_format *format;	/*current fmt.*/
	struct ov491_win_size *win;

	struct ov491_gpio fsync;
	struct ov491_gpio rest;

	struct notifier_block nb;
	int init_flag;
};


/*
 * the part of driver maybe modify about different sensor and different board.
 */

static inline struct ov491_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ov491_info, sd);
}

static inline struct v4l2_subdev *to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct ov491_info, hdl)->sd;
}
struct regval_list {
	uint16_t reg_num;
	unsigned char value;
};

int ov491_read(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char *value)
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

static int ov491_write(struct v4l2_subdev *sd, unsigned char reg,
			unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	uint8_t buf[3] = {(reg>>8)&0xff, reg&0xff, value};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 3,
		.buf	= buf,
	};
	int ret;
	unsigned int timeout  = 100;
	while(timeout--){
		ret = i2c_transfer(client->adapter, &msg, 1);
		if(ret == -EAGAIN){
			msleep(100);
			continue;
		}
		else
			break;
	}
	if (ret > 0)
		ret = 0;
	return ret;
}

static int ov491_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != OV491_REG_END) {
		if (vals->reg_num == OV491_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov491_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
			if (vals->reg_num == OV491_PAGE_REG){
				val &= 0xf8;
				val |= (vals->value & 0x07);
				ret = ov491_write(sd, vals->reg_num, val);
				ret = ov491_read(sd, vals->reg_num, &val);
			}
		}
		vals++;
	}
	return 0;
}
static int ov491_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != OV491_REG_END) {
		if (vals->reg_num == OV491_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov491_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}


/*
 * Stuff that knows about the sensor.
 */

static int ov491_xshutdn(struct v4l2_subdev *sd, u32 val)
{
	struct ov491_info *info = to_state(sd);

	return 0;
}

static int ov491_pwdn(struct v4l2_subdev *sd, u32 val)
{
	struct ov491_info *info = to_state(sd);

	return 0;
}

#if 0
static int ov491_hw_reset(struct v4l2_subdev *sd, u32 val)
{
	struct ov491_info *info = to_state(sd);

	return 0;
}
#endif
static int ov491_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;

	return 0;

}

static struct ov491_win_size ov491_win_sizes[] = {
	{
		.width				= 1920,
		.height				= 1080,
		.sensor_info.fps				= 30 << 16 | 1,
		.sensor_info.total_width			= 1928,
		.sensor_info.total_height			= 1152,
		.sensor_info.wdr_en				= 0,
		.sensor_info.bt_cfg.interlace_en		= 0,
		.sensor_info.bt_cfg.bt_sav_eav			= SAV_BF_EAV,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_2X8,
		.colorspace	= V4L2_COLORSPACE_SRGB,
	},
};

#if 0
static int ov491_enum_mbus_code(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad || code->index >= N_OV2735a_FMTS)
		return -EINVAL;

	code->code = ov491_formats[code->index].mbus_code;
	return 0;
}
#endif

/*
 * Set a format.
 */
static int ov491_set_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *format)
{
	if (format->pad)
		return -EINVAL;

	return 0;
}

static int ov491_get_fmt(struct v4l2_subdev *sd,
		       struct v4l2_subdev_pad_config *cfg,
		       struct v4l2_subdev_format *format)
{
	struct ov491_info *info = to_state(sd);
	struct ov491_win_size *wsize = info->win;
	struct v4l2_mbus_framefmt *fmt = &format->format;
	int ret = 0;

	if(!info->win) {
		dev_err(sd->dev, "sensor win_size not set!\n");
		return -EINVAL;
	}

	fmt->width = wsize->width;
	fmt->height = wsize->height;
	fmt->code = wsize->mbus_code;
	fmt->colorspace = wsize->colorspace;
	*(unsigned int *)fmt->reserved = (unsigned int)&wsize->sensor_info; /*reserved[0] reserved[1]*/

//	printk("----%s, %d, width: %d, height: %d, code: %x\n",
//			__func__, __LINE__, fmt->width, fmt->height, fmt->code);

	return ret;
}

static int ov491_s_wdr(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	struct ov491_info *info = to_state(sd);
	return ret;
}

static int ov491_s_brightness(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int ov491_s_contrast(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int ov491_s_hflip(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int ov491_s_vflip(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

/*
 * GAIN is split between REG_GAIN and REG_VREF[7:6].  If one believes
 * the data sheet, the VREF parts should be the most significant, but
 * experience shows otherwise.  There seems to be little value in
 * messing with the VREF bits, so we leave them alone.
 */
static int ov491_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	int ret = 0;
	unsigned char gain = 0;

	*value = gain;
	return ret;
}

static int ov491_s_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

//	printk("---%s, %d, s_gain: value: %d\n", __func__, __LINE__, value);

	return ret;
}


static int ov491_g_again(struct v4l2_subdev *sd, __s32 *value)
{
	char v = 0;
	unsigned int reg_val = 0;
	unsigned int fine_reg_val = 0;
	int ret = 0;

	return ret;

}

static int ov491_g_again_short(struct v4l2_subdev *sd, __s32 *value)
{
	char v = 0;
	unsigned int reg_val = 0;
	unsigned int fine_reg_val = 0;
	int ret = 0;

	return ret;

}

/*set analog gain db value, map value to sensor register.*/
static int ov491_s_again(struct v4l2_subdev *sd, int value)
{
	struct ov491_info *info = to_state(sd);
	unsigned int reg_value;
	unsigned int reg_fine_value;
	int ret = 0;

	return 0;
}

static int ov491_s_again_short(struct v4l2_subdev *sd, int value)
{
	struct ov491_info *info = to_state(sd);
	unsigned int reg_value;
	unsigned int reg_fine_value;
	int ret = 0;

	return 0;
}

/*
 * Tweak autogain.
 */
static int ov491_s_autogain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int ov491_s_exp(struct v4l2_subdev *sd, int value)
{
	//struct ov491_info *info = to_state(sd);

	return 0;
}

static int ov491_s_exp_short(struct v4l2_subdev *sd, int value)
{
	//struct ov491_info *info = to_state(sd);

	return 0;
}


static int ov491_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
	struct ov491_info *info = to_state(sd);

	switch (ctrl->id) {
	case V4L2_CID_AUTOGAIN:
		return ov491_g_gain(sd, &info->gain->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return ov491_g_again(sd, &info->again->val);
	}
	return -EINVAL;
}

static int ov491_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
	struct ov491_info *info = to_state(sd);

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return ov491_s_brightness(sd, ctrl->val);
	case V4L2_CID_CONTRAST:
		return ov491_s_contrast(sd, ctrl->val);
	case V4L2_CID_VFLIP:
		return ov491_s_vflip(sd, ctrl->val);
	case V4L2_CID_HFLIP:
		return ov491_s_hflip(sd, ctrl->val);
	case V4L2_CID_AUTOGAIN:
		/* Only set manual gain if auto gain is not explicitly
		   turned on. */
		if (!ctrl->val) {
			/* ov491_s_gain turns off auto gain */
			return ov491_s_gain(sd, info->gain->val);
		}
		return ov491_s_autogain(sd, ctrl->val);
	case V4L2_CID_GAIN:
		return ov491_s_gain(sd, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return ov491_s_again(sd, ctrl->val);
	case V4L2_CID_USER_ANALOG_GAIN_SHORT:
		return ov491_s_again_short(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return ov491_s_exp(sd, ctrl->val);
	}
	return -EINVAL;
}

static const struct v4l2_ctrl_ops ov491_ctrl_ops = {
	.s_ctrl = ov491_s_ctrl,
	.g_volatile_ctrl = ov491_g_volatile_ctrl,
};

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov491_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	unsigned char val = 0;
	int ret;

	ret = ov491_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 1;
	return ret;
}

static int ov491_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	ov491_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static int ov491_core_reset(struct v4l2_subdev *sd, u32 val)
{
	struct ov491_info *info = to_state(sd);
	unsigned char v;
	int ret;

	/*software reset*/
	if(info->init_flag)
		return 0;

	ret = ov491_read(sd, 0x0103, &v);

	if(val) {
		v |= 1;
		ret += ov491_write(sd, 0x0103, v);
	}
	return 0;
}

static int ov491_core_init(struct v4l2_subdev *sd, u32 val)
{
	struct ov491_info *info = to_state(sd);
	int ret = 0;


	return ret;
}




int ov491_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct ov491_info *info = to_state(sd);
	int ret = 0;

	if (enable) {
		printk("ov491 stream on\n");
	}
	else {
		printk("ov491 stream off\n");
	}
	return ret;
}


int ov491_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct ov491_info *info = to_state(sd);
	if(info->win->sensor_info.fps){
		param->parm.capture.timeperframe.numerator = info->win->sensor_info.fps & 0xffff;
		param->parm.capture.timeperframe.denominator = info->win->sensor_info.fps >> 16;
		return 0;
	}
	return -EINVAL;
}
/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops ov491_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov491_g_register,
	.s_register = ov491_s_register,
#endif
	.g_ext_ctrls = v4l2_subdev_g_ext_ctrls,
	.try_ext_ctrls = v4l2_subdev_try_ext_ctrls,
	.s_ext_ctrls = v4l2_subdev_s_ext_ctrls,
	.g_ctrl = v4l2_subdev_g_ctrl,
	.s_ctrl = v4l2_subdev_s_ctrl,
	.queryctrl = v4l2_subdev_queryctrl,
	.querymenu = v4l2_subdev_querymenu,
	.init = ov491_core_init,
	.reset = ov491_core_reset,

};

static const struct v4l2_subdev_video_ops ov491_video_ops = {
	.s_stream = ov491_s_stream,
	.g_parm	= ov491_g_parm,
};

static const struct v4l2_subdev_pad_ops ov491_pad_ops = {
	//.enum_frame_interval = ov491_enum_frame_interval,
	//.num_frame_size = ov491_enum_frame_size,
	//.enum_mbus_code = ov491_enum_mbus_code,
	.set_fmt = ov491_set_fmt,
	.get_fmt = ov491_get_fmt,
};

static const struct v4l2_subdev_ops ov491_ops = {
	.core = &ov491_core_ops,
	.video = &ov491_video_ops,
	.pad = &ov491_pad_ops,
};

/* ----------------------------------------------------------------------- */

#ifdef CONFIG_X2500_RISCV
extern int register_riscv_notifier(struct notifier_block *nb);
int ov491_riscv_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
	struct ov491_info *info = container_of(nb, struct sc230ai_info, nb);
	struct v4l2_subdev *sd = &info->sd;
	struct ispcam_device *ispcam = container_of(sd->v4l2_dev, struct ispcam_device, v4l2_dev);
	struct message *message = (unsigned int)action | 0x80000000;
	unsigned int *data_paddr;
	unsigned int *vaddr;

	while(message->type != MESSAGE_END){
		if((message->type == MESSAGE_INIT_SENSOR0 && ispcam->dev_nr == 0) ||
		   (message->type == MESSAGE_INIT_SENSOR1 && ispcam->dev_nr == 1) ||
		   (message->type == MESSAGE_INIT_SENSOR2 && ispcam->dev_nr == 2)){
			data_paddr = message->data;
			vaddr = (unsigned int)data_paddr | 0x80000000;
			info->init_flag = *vaddr;
			break;
		}
		message++;
	}
	return 0;
}
#endif


static int ov491_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct ov491_info *info;
	int ret;
	unsigned int ident = 0;
	int gpio = -EINVAL;
	unsigned int flags;
	unsigned long rate;
	int mclk_index = -1;

	info = devm_kzalloc(&client->dev, sizeof(*info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;

	gpio = of_get_named_gpio_flags(client->dev.of_node, "ingenic,fsync-gpio", 0, &flags);
	if(gpio_is_valid(gpio)) {
		info->fsync.pin = gpio;
		info->fsync.active_level = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	}
	gpio = of_get_named_gpio_flags(client->dev.of_node, "ingenic,rest-gpio", 0, &flags);
	if(gpio_is_valid(gpio)) {
		info->rest.pin = gpio;
		info->rest.active_level = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	}

	v4l2_i2c_subdev_init(sd, client, &ov491_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

#if 0
	/*clk*/
	char id_div[9];
	char id_mux[9];
	of_property_read_u32(client->dev.of_node, "ingenic,mclk", &mclk_index);
	if(mclk_index == 0) {
		memcpy(id_div, "div_cim0", sizeof(id_div));
		memcpy(id_mux, "mux_cim0", sizeof(id_mux));
	} else if(mclk_index == 1) {
		memcpy(id_div, "div_cim1", sizeof(id_div));
		memcpy(id_mux, "mux_cim1", sizeof(id_mux));
	} else if(mclk_index == 2) {
		memcpy(id_div, "div_cim2", sizeof(id_div));
		memcpy(id_mux, "mux_cim2", sizeof(id_mux));
	} else
		printk("Unkonwn mclk index\n");


	info->clk = v4l2_clk_get(&client->dev, id_div);
	if (IS_ERR(info->clk)) {
		ret = PTR_ERR(info->clk);
		goto err_clkget;
	}
	info->sclka = devm_clk_get(&client->dev, id_mux);

	rate = v4l2_clk_get_rate(info->clk);
	if (((rate / 1000) % 27000) != 0) {
		ret = clk_set_parent(info->sclka, clk_get(NULL, "epll"));
		info->sclka = devm_clk_get(&client->dev, "epll");
		if (IS_ERR(info->sclka)) {
			pr_err("get sclka failed\n");
		} else {
			rate = clk_get_rate(info->sclka);
			if (((rate / 1000) % 27000) != 0) {
				clk_set_rate(info->sclka, 891000000);
			}
		}
	}

	ret = v4l2_clk_set_rate(info->clk, 27000000);
	if(ret)
		dev_err(sd->dev, "clk_set_rate err!\n");

	ret = v4l2_clk_enable(info->clk);
	if(ret)
		dev_err(sd->dev, "clk_enable err!\n");
#endif

#if 0
	/* Make sure it's an ov491 */
	ret = ov491_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an ov491 chip.\n",
			client->addr << 1, client->adapter->name);
		return ret;
	}
#endif
	info->win = &ov491_win_sizes[0];

	v4l_info(client, "chip found @ 0x%02x (%s)\n",
			client->addr << 1, client->adapter->name);

	v4l2_ctrl_handler_init(&info->hdl, 8);
	v4l2_ctrl_new_std(&info->hdl, &ov491_ctrl_ops,
			V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&info->hdl, &ov491_ctrl_ops,
			V4L2_CID_CONTRAST, 0, 127, 1, 64);
	v4l2_ctrl_new_std(&info->hdl, &ov491_ctrl_ops,
			V4L2_CID_VFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&info->hdl, &ov491_ctrl_ops,
			V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&info->hdl, &ov491_ctrl_ops,
			V4L2_CID_WIDE_DYNAMIC_RANGE, 0, 1, 1, 0);
	info->gain = v4l2_ctrl_new_std(&info->hdl, &ov491_ctrl_ops,
			V4L2_CID_GAIN, 0, 255, 1, 128);
	info->again = v4l2_ctrl_new_std(&info->hdl, &ov491_ctrl_ops,
			V4L2_CID_ANALOGUE_GAIN, 0, 443135, 1, 10000);

	/*unit exposure lines: */
	info->exposure = v4l2_ctrl_new_std(&info->hdl, &ov491_ctrl_ops,
			V4L2_CID_EXPOSURE, 1,  1152 - 4 , 4, 600);

	sd->ctrl_handler = &info->hdl;
	if (info->hdl.error) {
		int err = info->hdl.error;

		v4l2_ctrl_handler_free(&info->hdl);
		return err;
	}
	v4l2_ctrl_handler_setup(&info->hdl);

	info->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_init(&info->sd.entity, 1, &info->pad, 0);
	if(ret < 0) {
		goto err_entity_init;
	}
	ret = v4l2_async_register_subdev(&info->sd);
	if (ret < 0)
		goto err_videoprobe;

#ifdef CONFIG_X2500_RISCV
	info->nb.notifier_call = ov491_riscv_notifier;
	register_riscv_notifier(&info->nb);
#endif

	dev_info(&client->dev, "ov491 Probed\n");
	return 0;
err_videoprobe:
err_entity_init:
	v4l2_clk_put(info->clk);
err_clkget:
	return ret;
}


static int ov491_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov491_info *info = to_state(sd);

	v4l2_device_unregister_subdev(sd);
	v4l2_ctrl_handler_free(&info->hdl);
	v4l2_clk_put(info->clk);
	return 0;
}

static const struct i2c_device_id ov491_id[] = {
	{ "ov491", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov491_id);

static const struct of_device_id ov491_of_match[] = {
	{.compatible = "ovit,ov491", },
	{},
};
MODULE_DEVICE_TABLE(of, ov491_of_match);


static struct i2c_driver ov491_driver = {
	.driver = {
		.name	= "ov491",
		.of_match_table = of_match_ptr(ov491_of_match),
	},
	.probe		= ov491_probe,
	.remove		= ov491_remove,
	.id_table	= ov491_id,
};

module_i2c_driver(ov491_driver);
MODULE_AUTHOR("qpz <aric.pzqi@ingenic.com>");
MODULE_DESCRIPTION("A low-level driver for SmartSens ov491 sensors");
MODULE_LICENSE("GPL");
