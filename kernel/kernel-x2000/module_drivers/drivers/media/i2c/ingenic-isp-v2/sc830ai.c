/*
 * A V4L2 driver for OmniVision SC830AI cameras.
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


#define SC830AI_CHIP_ID_H	(0xc1)
#define SC830AI_CHIP_ID_L	(0x43)
#define SC830AI_REG_END		0xff
#define SC830AI_REG_DELAY	0x00
#define SC830AI_PAGE_REG	0xfd


static bool debug;
module_param(debug, bool, 0644);
MODULE_PARM_DESC(debug, "Debug level (0-1)");

struct sc830ai_win_size {
	unsigned int width;
	unsigned int height;
	unsigned int mbus_code;
	struct sensor_info sensor_info;
	enum v4l2_colorspace colorspace;
	enum v4l2_xfer_func xfer_func;
	enum v4l2_field field;
	enum v4l2_ycbcr_encoding ycbcr_enc;
	enum v4l2_quantization quantization;
	void *regs;

};

struct sc830ai_gpio {
	int pin;
	int active_level;
};

struct sc830ai_info {
	struct v4l2_subdev sd;
	struct v4l2_ctrl_handler hdl;
	struct v4l2_ctrl *gain;
	struct v4l2_ctrl *again;
	struct v4l2_ctrl *again_short;

	struct v4l2_clk *clk;
	struct clk *sclka;

	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *exposure_short;

	struct media_pad pad;

	struct v4l2_subdev_format *format;	/*current fmt.*/
	struct sc830ai_win_size *win;

	struct sc830ai_gpio xshutdn;
	struct sc830ai_gpio pwdn;
	struct sc830ai_gpio efsync;
	struct sc830ai_gpio led;

	struct notifier_block nb;
	int init_flag;
};


/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
	unsigned int value;	/*sensor regs value*/
	unsigned int fine_value;	/*sensor regs value*/
	unsigned int gain;	/*isp gain*/
};

struct again_lut sc830ai_again_lut[] = {
	{0x40,0x80,0},
	{0x40,0x84,2886},
	{0x40,0x88,5776},
	{0x40,0x8c,8494},
	{0x40,0x90,11136},
	{0x40,0x94,13706},
	{0x40,0x98,16288},
	{0x40,0x9c,18724},
	{0x40,0xa0,21098},
	{0x40,0xa4,23414},
	{0x40,0xa8,25747},
	{0x40,0xac,27953},
	{0x40,0xb0,30109},
	{0x40,0xb4,32217},
	{0x40,0xb8,34345},
	{0x40,0xbc,36362},
	{0x40,0xc0,38336},
	{0x40,0xc4,40270},
	{0x40,0xc8,42226},
	{0x40,0xcc,44083},
	{0x40,0xd0,45904},
	{0x40,0xd4,47691},
	{0x40,0xd8,49500},
	{0x40,0xdc,51221},
	{0x40,0xe0,52911},
	{0x40,0xe4,54571},
	{0x40,0xe8,56255},
	{0x40,0xec,57858},
	{0x40,0xf0,59434},
	{0x40,0xf4,60984},
	{0x40,0xf8,62559},
	{0x40,0xfc,64059},
	{0x48,0x80,65536},
	{0x48,0x84,68468},
	{0x48,0x88,71268},
	{0x48,0x8c,74030},
	{0x48,0x90,76672},
	{0x48,0x94,79283},
	{0x48,0x98,81784},
	{0x48,0x9c,84260},
	{0x48,0xa0,86634},
	{0x48,0xa4,88987},
	{0x48,0xa8,91247},
	{0x48,0xac,93489},
	{0x48,0xb0,95645},
	{0x48,0xb4,97787},
	{0x48,0xb8,99848},
	{0x48,0xbc,101898},
	{0x48,0xc0,103872},
	{0x48,0xc4,105837},
	{0x48,0xc8,107732},
	{0x48,0xcc,109619},
	{0x48,0xd0,111440},
	{0x48,0xd4,113255},
	{0x48,0xd8,115008},
	{0x48,0xdc,116757},
	{0x48,0xe0,118447},
	{0x48,0xe4,120134},
	{0x48,0xe8,121765},
	{0x48,0xec,123394},
	{0x48,0xf0,124970},
	{0x48,0xf4,126545},
	{0x48,0xf8,128070},
	{0x48,0xfc,129595},
	{0x49,0x80,131072},
	{0x49,0x84,133981},
	{0x49,0x88,136803},
	{0x49,0x8c,139544},
	{0x49,0x90,142208},
	{0x49,0x94,144798},
	{0x49,0x98,147320},
	{0x49,0x9c,149776},
	{0x49,0xa0,152169},
	{0x49,0xa4,154504},
	{0x49,0xa8,156782},
	{0x49,0xac,159007},
	{0x49,0xb0,161181},
	{0x49,0xb4,163306},
	{0x49,0xb8,165384},
	{0x49,0xbc,167417},
	{0x49,0xc0,169408},
	{0x49,0xc4,171357},
	{0x49,0xc8,173267},
	{0x49,0xcc,175140},
	{0x49,0xd0,176976},
	{0x49,0xd4,178776},
	{0x49,0xd8,180544},
	{0x49,0xdc,182279},
	{0x49,0xe0,183982},
	{0x49,0xe4,185656},
	{0x49,0xe8,187300},
	{0x49,0xec,188916},
	{0x49,0xf0,190505},
	{0x49,0xf4,192068},
	{0x49,0xf8,193606},
	{0x49,0xfc,195119},
	{0x4b,0x80,196608},
	{0x4b,0x84,199517},
	{0x4b,0x88,202339},
	{0x4b,0x8c,205080},
	{0x4b,0x90,207744},
	{0x4b,0x94,210334},
	{0x4b,0x98,212856},
	{0x4b,0x9c,215312},
	{0x4b,0xa0,217705},
	{0x4b,0xa4,220040},
	{0x4b,0xa8,222318},
	{0x4b,0xac,224543},
	{0x4b,0xb0,226717},
	{0x4b,0xb4,228842},
	{0x4b,0xb8,230920},
	{0x4b,0xbc,232953},
	{0x4b,0xc0,234944},
	{0x4b,0xc4,236893},
	{0x4b,0xc8,238803},
	{0x4b,0xcc,240676},
	{0x4b,0xd0,242512},
	{0x4b,0xd4,244312},
	{0x4b,0xd8,246080},
	{0x4b,0xdc,247815},
	{0x4b,0xe0,249518},
	{0x4b,0xe4,251192},
	{0x4b,0xe8,252836},
	{0x4b,0xec,254452},
	{0x4b,0xf0,256041},
	{0x4b,0xf4,257604},
	{0x4b,0xf8,259142},
	{0x4b,0xfc,260655},
	{0x4f,0x80,262144},
	{0x4f,0x84,265053},
	{0x4f,0x88,267875},
	{0x4f,0x8c,270616},
	{0x4f,0x90,273280},
	{0x4f,0x94,275870},
	{0x4f,0x98,278392},
	{0x4f,0x9c,280848},
	{0x4f,0xa0,283241},
	{0x4f,0xa4,285576},
	{0x4f,0xa8,287854},
	{0x4f,0xac,290079},
	{0x4f,0xb0,292253},
	{0x4f,0xb4,294378},
	{0x4f,0xb8,296456},
	{0x4f,0xbc,298489},
	{0x4f,0xc0,300480},
	{0x4f,0xc4,302429},
	{0x4f,0xc8,304339},
	{0x4f,0xcc,306212},
	{0x4f,0xd0,308048},
	{0x4f,0xd4,309848},
	{0x4f,0xd8,311616},
	{0x4f,0xdc,313351},
	{0x4f,0xe0,315054},
	{0x4f,0xe4,316728},
	{0x4f,0xe8,318372},
	{0x4f,0xec,319988},
	{0x4f,0xf0,321577},
	{0x4f,0xf4,323140},
	{0x4f,0xf8,324678},
	{0x4f,0xfc,326191},
	{0x5f,0x80,327680},
	{0x5f,0x84,330589},
	{0x5f,0x88,333411},
	{0x5f,0x8c,336152},
	{0x5f,0x90,338816},
	{0x5f,0x94,341406},
	{0x5f,0x98,343928},
	{0x5f,0x9c,346384},
	{0x5f,0xa0,348777},
	{0x5f,0xa4,351112},
	{0x5f,0xa8,353390},
	{0x5f,0xac,355615},
	{0x5f,0xb0,357789},
	{0x5f,0xb4,359914},
	{0x5f,0xb8,361992},
	{0x5f,0xbc,364025},
	{0x5f,0xc0,366016},
	{0x5f,0xc4,367965},
	{0x5f,0xc8,369875},
	{0x5f,0xcc,371748},
	{0x5f,0xd0,373584},
	{0x5f,0xd4,375384},
	{0x5f,0xd8,377152},
	{0x5f,0xdc,378887},
	{0x5f,0xe0,380590},
	{0x5f,0xe4,382264},
	{0x5f,0xe8,383908},
	{0x5f,0xec,385524},
	{0x5f,0xf0,387113},
	{0x5f,0xf4,388676},
	{0x5f,0xf8,390214},
	{0x5f,0xfc,391727},
};

static inline struct sc830ai_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sc830ai_info, sd);
}

static inline struct v4l2_subdev *to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct sc830ai_info, hdl)->sd;
}

struct regval_list {
	uint16_t reg_num;
	unsigned char value;
};

static struct regval_list sc830ai_init_regs_3840_2160_40fps_MIPI[] = {
	{0x0103,0x01},
	{0x0100,0x00},

	{0x36e9,0x80},
	{0x37f9,0x80},
	{0x301f,0x1e},
	{0x320c,0x08},
	{0x320d,0x34},
	{0x320e,0x08},
	{0x320f,0xec},
	{0x3281,0x80},
	{0x3301,0x0e},
	{0x3303,0x18},
	{0x3306,0x50},
	{0x3308,0x20},
	{0x330a,0x00},
	{0x330b,0xd8},
	{0x330c,0x20},
	{0x330e,0x40},
	{0x330f,0x08},
	{0x3314,0x16},
	{0x3317,0x07},
	{0x3319,0x0c},
	{0x3321,0x0c},
	{0x3324,0x09},
	{0x3325,0x09},
	{0x3327,0x16},
	{0x3328,0x10},
	{0x3329,0x1c},
	{0x332b,0x0d},
	{0x3333,0x10},
	{0x333e,0x0e},
	{0x3352,0x0c},
	{0x3353,0x0c},
	{0x335e,0x06},
	{0x335f,0x08},
	{0x3364,0x5e},
	{0x3366,0x01},
	{0x337c,0x02},
	{0x337d,0x0a},
	{0x3390,0x01},
	{0x3391,0x0b},
	{0x3392,0x1f},
	{0x3393,0x0e},
	{0x3394,0x30},
	{0x3395,0x30},
	{0x3396,0x01},
	{0x3397,0x0b},
	{0x3398,0x1f},
	{0x3399,0x09},
	{0x339a,0x0e},
	{0x339b,0x30},
	{0x339c,0x30},
	{0x339f,0x0e},
	{0x33a2,0x04},
	{0x33ad,0x3c},
	{0x33af,0x68},
	{0x33b1,0x80},
	{0x33b2,0x58},
	{0x33b3,0x40},
	{0x33ba,0x0c},
	{0x33f9,0x80},
	{0x33fb,0xa0},
	{0x33fc,0x4b},
	{0x33fd,0x5f},
	{0x349f,0x03},
	{0x34a0,0x0e},
	{0x34a6,0x4b},
	{0x34a7,0x5f},
	{0x34a8,0x20},
	{0x34a9,0x10},
	{0x34aa,0x01},
	{0x34ab,0x10},
	{0x34ac,0x01},
	{0x34ad,0x28},
	{0x34f8,0x5f},
	{0x34f9,0x10},
	{0x3630,0xc8},
	{0x3632,0x46},
	{0x3633,0x33},
	{0x3637,0x2a},
	{0x3638,0xc3},
	{0x363c,0x40},
	{0x363d,0x40},
	{0x363e,0x70},
	{0x3670,0x01},
	{0x3674,0xc6},
	{0x3675,0x8c},
	{0x3676,0x8c},
	{0x367c,0x4b},
	{0x367d,0x5f},
	{0x3698,0x82},
	{0x3699,0x8d},
	{0x369a,0x9c},
	{0x369b,0xba},
	{0x369e,0xba},
	{0x369f,0xba},
	{0x36a2,0x49},
	{0x36a3,0x4b},
	{0x36a4,0x4f},
	{0x36a5,0x5f},
	{0x36a6,0x5f},
	{0x36d0,0x01},
	{0x36ea,0x08},
	{0x36eb,0x04},
	{0x36ec,0x03},
	{0x36ed,0x22},
	{0x370f,0x01},
	{0x3721,0x9c},
	{0x3722,0x03},
	{0x3724,0x31},
	{0x37b0,0x03},
	{0x37b1,0x03},
	{0x37b2,0x03},
	{0x37b3,0x4b},
	{0x37b4,0x4f},
	{0x37fa,0x08},
	{0x37fb,0x30},
	{0x37fc,0x00},
	{0x37fd,0x04},
	{0x3903,0x40},
	{0x3905,0x4c},
	{0x391e,0x09},
	{0x3929,0x18},
	{0x3933,0x80},
	{0x3934,0x03},
	{0x3935,0x00},
	{0x3936,0x34},
	{0x3937,0x6a},
	{0x3938,0x69},
	{0x3e00,0x01},
	{0x3e01,0x1c},
	{0x3e02,0x60},
	{0x3e09,0x40},
	{0x3e10,0x00},
	{0x3e11,0x80},
	{0x3e12,0x03},
	{0x3e13,0x40},
	{0x3e23,0x00},
	{0x3e24,0x88},
	{0x440e,0x02},
	{0x4837,0x16},
	{0x5010,0x01},
	{0x5799,0x77},
	{0x57aa,0xeb},
	{0x57d9,0x00},
	{0x5ae0,0xfe},
	{0x5ae1,0x40},
	{0x5ae2,0x38},
	{0x5ae3,0x30},
	{0x5ae4,0x28},
	{0x5ae5,0x38},
	{0x5ae6,0x30},
	{0x5ae7,0x28},
	{0x5ae8,0x3f},
	{0x5ae9,0x34},
	{0x5aea,0x2c},
	{0x5aeb,0x3f},
	{0x5aec,0x34},
	{0x5aed,0x2c},
	{0x5aee,0xfe},
	{0x5aef,0x40},
	{0x5af4,0x38},
	{0x5af5,0x30},
	{0x5af6,0x28},
	{0x5af7,0x38},
	{0x5af8,0x30},
	{0x5af9,0x28},
	{0x5afa,0x3f},
	{0x5afb,0x34},
	{0x5afc,0x2c},
	{0x5afd,0x3f},
	{0x5afe,0x34},
	{0x5aff,0x2c},
	{0x5f00,0x05},
	{0x36e9,0x53},
	{0x37f9,0x27},
//	{0x4501,0xbc},//test

	{SC830AI_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sc830ai_stream_on[] = {
	{0x0100,0x01},
	{SC830AI_REG_END, 0x00},
};

static struct regval_list sc830ai_stream_off[] = {
	{0x0100,0x00},
	{SC830AI_REG_END, 0x00},
};


int sc830ai_read(struct v4l2_subdev *sd, unsigned short reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	uint8_t buf[2] = {(reg>>8)&0xff, reg&0xff};
	struct i2c_msg msg[2] = {
		[0] = {
			.addr	= client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= buf,
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

static int sc830ai_write(struct v4l2_subdev *sd, unsigned short reg,
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

static int sc830ai_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != SC830AI_REG_END) {
		if (vals->reg_num == SC830AI_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = sc830ai_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
			if (vals->reg_num == SC830AI_PAGE_REG){
				val &= 0xf8;
				val |= (vals->value & 0x07);
				ret = sc830ai_write(sd, vals->reg_num, val);
				ret = sc830ai_read(sd, vals->reg_num, &val);
			}
		}
		vals++;
	}
	return 0;
}

static int sc830ai_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SC830AI_REG_END) {
		if (vals->reg_num == SC830AI_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = sc830ai_write(sd, vals->reg_num, vals->value);
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
static int sc830ai_xshutdn(struct v4l2_subdev *sd, u32 val)
{
	struct sc830ai_info *info = to_state(sd);

	if(val) {
		gpio_direction_output(info->xshutdn.pin, info->xshutdn.active_level);
	} else {
		gpio_direction_output(info->xshutdn.pin, !info->xshutdn.active_level);
	}
	return 0;
}

static int sc830ai_pwdn(struct v4l2_subdev *sd, u32 val)
{
	struct sc830ai_info *info = to_state(sd);

	if(val) {
		gpio_direction_output(info->pwdn.pin, info->pwdn.active_level);
		msleep(10);
	} else {
		gpio_direction_output(info->pwdn.pin, !info->pwdn.active_level);
	}
	return 0;
}

static int sc830ai_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;

	ret = sc830ai_read(sd, 0x3107, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SC830AI_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = sc830ai_read(sd, 0x3108, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SC830AI_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static struct sc830ai_win_size sc830ai_win_sizes[] = {
	{
		.width				= 3840,
		.height				= 2160,
		.sensor_info.fps				= 30 << 16 | 1,
		.sensor_info.total_width			= 0x834*2,
		.sensor_info.total_height			= 0x8ec,
		.sensor_info.wdr_en				= 0,
		.sensor_info.mipi_cfg.clk			= 720,
		.sensor_info.mipi_cfg.twidth		= 3840,
		.sensor_info.mipi_cfg.theight		= 2160,
		.sensor_info.mipi_cfg.mipi_mode		= SENSOR_MIPI_OTHER_MODE,
		.sensor_info.mipi_cfg.mipi_vcomp_en		= 0,
		.sensor_info.mipi_cfg.mipi_hcomp_en		= 0,
		.sensor_info.mipi_cfg.mipi_crop_start0x	= 0,
		.sensor_info.mipi_cfg.mipi_crop_start0y	= 0,
		.sensor_info.mipi_cfg.mipi_crop_start1x	= 0,
		.sensor_info.mipi_cfg.mipi_crop_start1y	= 0,
		.sensor_info.mipi_cfg.mipi_crop_start2x	= 0,
		.sensor_info.mipi_cfg.mipi_crop_start2y	= 0,
		.sensor_info.mipi_cfg.mipi_crop_start3x	= 0,
		.sensor_info.mipi_cfg.mipi_crop_start3y	= 0,
		.sensor_info.mipi_cfg.hcrop_diff_en		= 0,
		.sensor_info.mipi_cfg.line_sync_mode	= 0,
		.sensor_info.mipi_cfg.work_start_flag	= 0,
		.sensor_info.mipi_cfg.data_type_en		= 0,
		.sensor_info.mipi_cfg.data_type_value	= RAW10,
		.sensor_info.mipi_cfg.del_start		= 0,
		.sensor_info.mipi_cfg.sensor_frame_mode	= TX_SENSOR_DEFAULT_FRAME_MODE,
		.sensor_info.mipi_cfg.sensor_fid_mode	= 0,
		.sensor_info.mipi_cfg.sensor_mode		= TX_SENSOR_DEFAULT_MODE,
		.sensor_info.mipi_cfg.sensor_csi_fmt		= TX_SENSOR_RAW10,
		.mbus_code	= MEDIA_BUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.xfer_func   	= V4L2_XFER_FUNC_SRGB,
		.field          = V4L2_FIELD_NONE,
		.ycbcr_enc      = V4L2_YCBCR_ENC_DEFAULT,
		.quantization   = V4L2_QUANTIZATION_FULL_RANGE,
		.regs 		= sc830ai_init_regs_3840_2160_40fps_MIPI,
	},
};

#if 0
static int sc830ai_enum_mbus_code(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad || code->index >= N_OV2735a_FMTS)
		return -EINVAL;

	code->code = sc830ai_formats[code->index].mbus_code;
	return 0;
}
#endif

/*
 * Set a format.
 */
static int sc830ai_set_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *format)
{
	if (format->pad)
		return -EINVAL;

	return 0;
}

static int sc830ai_get_fmt(struct v4l2_subdev *sd,
		       struct v4l2_subdev_pad_config *cfg,
		       struct v4l2_subdev_format *format)
{
	struct sc830ai_info *info = to_state(sd);
	struct sc830ai_win_size *wsize = info->win;
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
	fmt->xfer_func = wsize->xfer_func;
	fmt->field = wsize->field;
	fmt->ycbcr_enc = wsize->ycbcr_enc;
	fmt->quantization = wsize->quantization;

	*(unsigned int *)fmt->reserved = (unsigned int)&wsize->sensor_info; /*reserved[0] reserved[1]*/

//	printk("----%s, %d, width: %d, height: %d, code: %x\n",
//			__func__, __LINE__, fmt->width, fmt->height, fmt->code);

	return ret;
}

static int sc830ai_s_wdr(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	struct sc830ai_info *info = to_state(sd);
	if(value)
		info->win = &sc830ai_win_sizes[0];
	else
		info->win = &sc830ai_win_sizes[0];
	return ret;
}

static int sc830ai_s_brightness(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int sc830ai_s_contrast(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int sc830ai_s_hflip(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int sc830ai_s_vflip(struct v4l2_subdev *sd, int value)
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
static int sc830ai_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	int ret = 0;
	unsigned char gain = 0;

	*value = gain;
	return ret;
}

static int sc830ai_s_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

//	printk("---%s, %d, s_gain: value: %d\n", __func__, __LINE__, value);

	return ret;
}

static unsigned int again_to_regval(int gain, unsigned int *value, unsigned int *fine_value)
{
	struct again_lut *lut = NULL;
	int i = 0;
	for(i = 0; i < ARRAY_SIZE(sc830ai_again_lut); i++) {
		lut = &sc830ai_again_lut[i];

		if(gain <= lut->gain) {
			*value = lut->value;
			*fine_value = lut->fine_value;
			return lut->value;
		}
	}
	/*last value.*/
	*value = lut->value;
	*fine_value = lut->fine_value;
	return lut->value;
}

static int regval_to_again(unsigned int regval, unsigned int fine_reg_val)
{
	struct again_lut *lut = NULL;
	int i = 0;
	for(i = 0; i < ARRAY_SIZE(sc830ai_again_lut); i++) {
		lut = &sc830ai_again_lut[i];

		if(regval == lut->value && fine_reg_val == lut->fine_value) {
			return lut->gain;
		}
	}
	printk("%s, %d regval not mapped to isp gain\n", __func__, __LINE__);
	return -EINVAL;
}

static int sc830ai_g_again(struct v4l2_subdev *sd, __s32 *value)
{
	char v = 0;
	unsigned int reg_val = 0;
	unsigned int fine_reg_val = 0;
	int ret = 0;

	ret = sc830ai_read(sd, 0x3e09, &v);
	reg_val = v ;
	ret = sc830ai_read(sd, 0x3e07, &v);
	fine_reg_val = v ;

	*value = regval_to_again(reg_val, fine_reg_val);

	return ret;

}
#if 0
static int sc830ai_g_again_short(struct v4l2_subdev *sd, __s32 *value)
{
	char v = 0;
	unsigned int reg_val = 0;
	unsigned int fine_reg_val = 0;
	int ret = 0;


	ret = sc830ai_read(sd, 0x3e13, &v);
	reg_val = v ;
	ret = sc830ai_read(sd, 0x3e11, &v);
	fine_reg_val = v ;

	*value = regval_to_again(reg_val, fine_reg_val);

	return ret;

}
#endif
/*set analog gain db value, map value to sensor register.*/
static int sc830ai_s_again(struct v4l2_subdev *sd, int value)
{
	struct sc830ai_info *info = to_state(sd);
	unsigned int reg_value;
	unsigned int reg_fine_value;
	int ret = 0;

	if(value < info->again->minimum || value > info->again->maximum) {
		/* use default value. */
		again_to_regval(info->again->default_value, &reg_value, &reg_fine_value);
	} else {
		again_to_regval(value, &reg_value, &reg_fine_value);
	}

	ret += sc830ai_write(sd, 0x3e09, (unsigned char)(reg_value));
	ret += sc830ai_write(sd, 0x3e07, (unsigned char)(reg_fine_value));
	if (ret < 0){
		printk("sc830ai_write error  %d\n" ,__LINE__ );
		return ret;
	}

	return 0;
}
#if 0
static int sc830ai_s_again_short(struct v4l2_subdev *sd, int value)
{
	struct sc830ai_info *info = to_state(sd);
	unsigned int reg_value;
	unsigned int reg_fine_value;
	int ret = 0;

	if(value < info->again->minimum || value > info->again->maximum) {
		/* use default value. */
		again_to_regval(info->again->default_value, &reg_value, &reg_fine_value);
	} else {
		again_to_regval(value, &reg_value, &reg_fine_value);
	}

	ret += sc830ai_write(sd, 0x3e13, (unsigned char)(reg_value));
	ret += sc830ai_write(sd, 0x3e11, (unsigned char)(reg_fine_value));
	if (ret < 0){
		printk("sc830ai_write error  %d\n" ,__LINE__ );
		return ret;
	}

	return 0;
}
#endif
/*
 * Tweak autogain.
 */
static int sc830ai_s_autogain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int sc830ai_s_exp(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	value *= 2; /*unit in half line*/
	ret += sc830ai_write(sd, 0x3e00, (unsigned char)((value >> 12) & 0xf));
	ret += sc830ai_write(sd, 0x3e01, (unsigned char)((value >> 4) & 0xff));
	ret += sc830ai_write(sd, 0x3e02, (unsigned char)(value & 0xf) << 4);

	if (ret < 0) {
		printk("sc830ai_write error  %d\n" ,__LINE__);
		return ret;
	}

	return 0;
}
#if 0
static int sc830ai_s_exp_short(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	value *= 2; /*unit in half line*/
	ret += sc830ai_write(sd, 0x3e22, (unsigned char)((value >> 12) & 0xf));
	ret += sc830ai_write(sd, 0x3e04, (unsigned char)((value >> 4) & 0xff));
	ret += sc830ai_write(sd, 0x3e05, (unsigned char)(value & 0xf) << 4);

	if (ret < 0) {
		printk("sc830ai_write error  %d\n" ,__LINE__);
		return ret;
	}

	return 0;
}
#endif

static int sc830ai_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
	struct sc830ai_info *info = to_state(sd);

	switch (ctrl->id) {
	case V4L2_CID_AUTOGAIN:
		return sc830ai_g_gain(sd, &info->gain->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return sc830ai_g_again(sd, &info->again->val);
	//case V4L2_CID_USER_ANALOG_GAIN_SHORT:
	//	return sc830ai_g_again_short(sd, &info->again_short->val);
	}
	return -EINVAL;
}

static int sc830ai_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
	struct sc830ai_info *info = to_state(sd);

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sc830ai_s_brightness(sd, ctrl->val);
	case V4L2_CID_CONTRAST:
		return sc830ai_s_contrast(sd, ctrl->val);
	case V4L2_CID_VFLIP:
		return sc830ai_s_vflip(sd, ctrl->val);
	case V4L2_CID_HFLIP:
		return sc830ai_s_hflip(sd, ctrl->val);
	case V4L2_CID_AUTOGAIN:
	      /* Only set manual gain if auto gain is not explicitly
	         turned on. */
		if (!ctrl->val) {
	      	/* sc830ai_s_gain turns off auto gain */
			return sc830ai_s_gain(sd, info->gain->val);
		}
			return sc830ai_s_autogain(sd, ctrl->val);
	case V4L2_CID_GAIN:
		return sc830ai_s_gain(sd, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return sc830ai_s_again(sd, ctrl->val);
	//case V4L2_CID_USER_ANALOG_GAIN_SHORT:
	//	return sc830ai_s_again_short(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sc830ai_s_exp(sd, ctrl->val);
	//case V4L2_CID_USER_EXPOSURE_SHORT:
	//	return sc830ai_s_exp_short(sd, ctrl->val);
	case V4L2_CID_WIDE_DYNAMIC_RANGE:
		return sc830ai_s_wdr(sd, ctrl->val);
	}
	return -EINVAL;
}

static const struct v4l2_ctrl_ops sc830ai_ctrl_ops = {
	.s_ctrl = sc830ai_s_ctrl,
	.g_volatile_ctrl = sc830ai_g_volatile_ctrl,
};


#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sc830ai_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	unsigned char val = 0;
	int ret;

	ret = sc830ai_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 1;
	return ret;
}

static int sc830ai_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	sc830ai_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static int sc830ai_core_reset(struct v4l2_subdev *sd, u32 val)
{
	struct sc830ai_info *info = to_state(sd);
	unsigned char v;
	int ret;

	/*software reset*/
	if(info->init_flag)
		return 0;

	ret = sc830ai_read(sd, 0x0103, &v);

	if(val) {
		v |= 1;
		ret += sc830ai_write(sd, 0x0103, v);
	}
	return 0;
}

static int sc830ai_core_init(struct v4l2_subdev *sd, u32 val)
{
	struct sc830ai_info *info = to_state(sd);
	int ret = 0;

	if(!info->init_flag) {
		ret = sc830ai_write_array(sd, info->win->regs);
	}

	return ret;
}

int sc830ai_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = sc830ai_write_array(sd, sc830ai_stream_on);
		printk("sc830ai stream on\n");

	}
	else {
		ret = sc830ai_write_array(sd, sc830ai_stream_off);
		printk("sc830ai stream off\n");
	}

	return ret;
}

int sc830ai_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct sc830ai_info *info = to_state(sd);
	if(info->win->sensor_info.fps){
		param->parm.capture.timeperframe.numerator = info->win->sensor_info.fps & 0xffff;
		param->parm.capture.timeperframe.denominator = info->win->sensor_info.fps >> 16;
		return 0;
	}
	return -EINVAL;
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sc830ai_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = sc830ai_g_register,
	.s_register = sc830ai_s_register,
#endif
	.g_ext_ctrls = v4l2_subdev_g_ext_ctrls,
	.try_ext_ctrls = v4l2_subdev_try_ext_ctrls,
	.s_ext_ctrls = v4l2_subdev_s_ext_ctrls,
	.g_ctrl = v4l2_subdev_g_ctrl,
	.s_ctrl = v4l2_subdev_s_ctrl,
	.queryctrl = v4l2_subdev_queryctrl,
	.querymenu = v4l2_subdev_querymenu,
	.init = sc830ai_core_init,
	.reset = sc830ai_core_reset,

};

static const struct v4l2_subdev_video_ops sc830ai_video_ops = {
	.s_stream = sc830ai_s_stream,
	.g_parm	= sc830ai_g_parm,
};

static const struct v4l2_subdev_pad_ops sc830ai_pad_ops = {
	//.enum_mbus_code = sc830ai_enum_mbus_code,
	.set_fmt = sc830ai_set_fmt,
	.get_fmt = sc830ai_get_fmt,
};

static const struct v4l2_subdev_ops sc830ai_ops = {
	.core = &sc830ai_core_ops,
	.video = &sc830ai_video_ops,
	.pad = &sc830ai_pad_ops,
};

/* ----------------------------------------------------------------------- */

#ifdef CONFIG_X2500_RISCV
extern int register_riscv_notifier(struct notifier_block *nb);
int sc830ai_riscv_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
	struct sc830ai_info *info = container_of(nb, struct sc830ai_info, nb);
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


static int sc830ai_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sc830ai_info *info;
	int ret;
	unsigned int ident = 0;
	int gpio = -EINVAL;
	unsigned int flags;
	int mclk_index = -1;
	char id_div[9];
	//struct v4l2_ctrl_config cfg = {0};

	dev_info(&client->dev, "sc830ai Probe !!!\n");

	info = devm_kzalloc(&client->dev, sizeof(*info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;

	gpio = of_get_named_gpio_flags(client->dev.of_node, "ingenic,xshutdn-gpio", 0, &flags);
	if(gpio_is_valid(gpio)) {
		info->xshutdn.pin = gpio;
		info->xshutdn.active_level = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	}
	gpio = of_get_named_gpio_flags(client->dev.of_node, "ingenic,pwdn-gpio", 0, &flags);
	if(gpio_is_valid(gpio)) {
		info->pwdn.pin = gpio;
		info->pwdn.active_level = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	}
	gpio = of_get_named_gpio_flags(client->dev.of_node, "ingenic,efsync-gpio", 0, &flags);
	if(gpio_is_valid(gpio)) {
		info->efsync.pin = gpio;
		info->efsync.active_level = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	}


	v4l2_i2c_subdev_init(sd, client, &sc830ai_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	sc830ai_xshutdn(sd, 1);
	sc830ai_pwdn(sd, 1);
	msleep(5);

	/*clk*/
	of_property_read_u32(client->dev.of_node, "ingenic,mclk", &mclk_index);
	if(mclk_index == 0) {
		memcpy(id_div, "div_cim0", sizeof(id_div));
	} else if(mclk_index == 1) {
		memcpy(id_div, "div_cim1", sizeof(id_div));
	} else if(mclk_index == 2) {
		memcpy(id_div, "div_cim2", sizeof(id_div));
	} else
		printk("Unkonwn mclk index\n");

	info->clk = v4l2_clk_get(&client->dev, id_div);
	if (IS_ERR(info->clk)) {
		ret = PTR_ERR(info->clk);
		goto err_clkget;
	}

	ret = v4l2_clk_set_rate(info->clk, 24000000);
	if(ret)
		dev_err(sd->dev, "clk_set_rate err!\n");

	ret = v4l2_clk_enable(info->clk);
	if(ret)
		dev_err(sd->dev, "clk_enable err!\n");

	/* Make sure it's an sc830ai */
	ret = sc830ai_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an sc830ai chip.\n",
			client->addr << 1, client->adapter->name);
		return ret;
	}

	v4l_info(client, "chip found @ 0x%02x (%s)\n",
			client->addr << 1, client->adapter->name);

	v4l2_ctrl_handler_init(&info->hdl, 8);
	v4l2_ctrl_new_std(&info->hdl, &sc830ai_ctrl_ops,
			V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&info->hdl, &sc830ai_ctrl_ops,
			V4L2_CID_CONTRAST, 0, 127, 1, 64);
	v4l2_ctrl_new_std(&info->hdl, &sc830ai_ctrl_ops,
			V4L2_CID_VFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&info->hdl, &sc830ai_ctrl_ops,
			V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&info->hdl, &sc830ai_ctrl_ops,
			V4L2_CID_WIDE_DYNAMIC_RANGE, 0, 1, 1, 0);
	info->gain = v4l2_ctrl_new_std(&info->hdl, &sc830ai_ctrl_ops,
			V4L2_CID_GAIN, 0, 255, 1, 128);
	info->again = v4l2_ctrl_new_std(&info->hdl, &sc830ai_ctrl_ops,
			V4L2_CID_ANALOGUE_GAIN, 0, 391727, 1, 10000);

	/*unit exposure lines: */
	info->exposure = v4l2_ctrl_new_std(&info->hdl, &sc830ai_ctrl_ops,
			V4L2_CID_EXPOSURE, 3, 4551 , 1, 600);

	/*
	cfg.ops = &sc830ai_ctrl_ops;
	cfg.id = V4L2_CID_USER_EXPOSURE_SHORT;
	cfg.name = "expo short";
	cfg.type = V4L2_CTRL_TYPE_INTEGER;
	cfg.min = 1;
	cfg.max = 131;
	cfg.step = 4;
	cfg.def = 50;
	info->exposure_short = v4l2_ctrl_new_custom(&info->hdl, &cfg, NULL);

	memset(&cfg, 0, sizeof(cfg));
	cfg.ops = &sc830ai_ctrl_ops;
	cfg.id = V4L2_CID_USER_ANALOG_GAIN_SHORT;
	cfg.name = "analog gain short";
	cfg.type = V4L2_CTRL_TYPE_INTEGER;
	cfg.min = 0;
	cfg.max = 589824;
	cfg.step = 1;
	cfg.def = 10000;
	info->again_short = v4l2_ctrl_new_custom(&info->hdl, &cfg, NULL);
	*/

	sd->ctrl_handler = &info->hdl;
	if (info->hdl.error) {
		int err = info->hdl.error;

		v4l2_ctrl_handler_free(&info->hdl);
		return err;
	}
	v4l2_ctrl_handler_setup(&info->hdl);

	info->win = &sc830ai_win_sizes[0];
	sc830ai_write_array(sd, info->win->regs);


	info->pad.flags = MEDIA_PAD_FL_SOURCE;
	info->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	ret = media_entity_init(&info->sd.entity, 1, &info->pad, 0);
	if(ret < 0) {
		goto err_entity_init;
	}
	ret = v4l2_async_register_subdev(&info->sd);
	if (ret < 0)
		goto err_videoprobe;

#ifdef CONFIG_X2500_RISCV
	info->nb.notifier_call = sc830ai_riscv_notifier;
	register_riscv_notifier(&info->nb);
#endif

	dev_info(&client->dev, "sc830ai Probed\n");
	return 0;
err_videoprobe:
err_entity_init:
	v4l2_clk_put(info->clk);
err_clkget:
	return ret;
}


static int sc830ai_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sc830ai_info *info = to_state(sd);

	v4l2_device_unregister_subdev(sd);
	v4l2_ctrl_handler_free(&info->hdl);
	v4l2_clk_put(info->clk);
	return 0;
}

static const struct i2c_device_id sc830ai_id[] = {
	{ "sc830ai", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, sc830ai_id);

static const struct of_device_id sc830ai_of_match[] = {
	{.compatible = "smartsens,sc830ai", },
	{},
};
MODULE_DEVICE_TABLE(of, sc830ai_of_match);


static struct i2c_driver sc830ai_driver = {
	.driver = {
		.name	= "sc830ai",
		.of_match_table = of_match_ptr(sc830ai_of_match),
	},
	.probe		= sc830ai_probe,
	.remove		= sc830ai_remove,
	.id_table	= sc830ai_id,
};

module_i2c_driver(sc830ai_driver);
MODULE_AUTHOR("qpz <aric.pzqi@ingenic.com>");
MODULE_DESCRIPTION("A low-level driver for SmartSens sc830ai sensors");
MODULE_LICENSE("GPL");
