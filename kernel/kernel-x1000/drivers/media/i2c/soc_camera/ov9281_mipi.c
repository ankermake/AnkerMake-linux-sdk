/*
 * ov9281 Camera Driver
 *
 * Copyright (C) 2014, Ingenic Semiconductor Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

//#define DEBUG	1
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-subdev.h>
#include <media/soc_camera.h>
#include <media/soc_mediabus.h>
#include <linux/sensor_board.h>

#define REG_CHIP_ID_HIGH        0x300A
#define REG_CHIP_ID_LOW         0x300B

#define CHIP_ID_HIGH            0x92
#define CHIP_ID_LOW				0x81

/* Private v4l2 controls */
#define V4L2_CID_PRIVATE_BALANCE  (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PRIVATE_EFFECT  (V4L2_CID_PRIVATE_BASE + 1)

#define REG14				0x14
#define REG14_HFLIP_IMG		0x01 /* Horizontal mirror image ON/OFF */
#define REG14_VFLIP_IMG     0x02 /* Vertical flip image ON/OFF */

/* whether sensor support high resolution (> vga) preview or not */
#define SUPPORT_HIGH_RESOLUTION_PRE		1

/*
 * Struct
 */
struct regval_list {
	u16 reg_num;
	u8 value;
};

struct mode_list {
	u8 index;
	const struct regval_list *mode_regs;
};

/* Supported resolutions */
enum ov9281_width {
	W_720P	= 1280,
	W_640 = 640,
};

enum ov9281_height {
	H_800P	= 800,
	H_720P	= 720,
	H_480	= 480,
};

struct ov9281_win_size {
	char *name;
	enum ov9281_width width;
	enum ov9281_height height;
	const struct regval_list *regs;
};


struct ov9281_priv {
	struct v4l2_subdev subdev;
	struct ov9281_camera_info *info;
	enum v4l2_mbus_pixelcode cfmt_code;
	const struct ov9281_win_size *win;
	int	model;
	u8 balance_value;
	u8 effect_value;
	u16	flag_vflip:1;
	u16	flag_hflip:1;
};
/*return: 0 is ok, negative errno*/
static inline int ov9281_write_reg(struct i2c_client * client, u16 addr, unsigned char value)
{
	u8 buf[3] = {addr >> 8,addr & 0xff, value};
	struct i2c_msg msg = {
		.addr   = client->addr,
		.flags  = 0,
		.len    = 3,
		.buf    = buf,
	};

	int ret;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

    return ret;
}

/*return: 0 is ok, negative errno*/
static inline char ov9281_read_reg(struct i2c_client *client, u16 addr)
{
	int ret;
	u8 val;

    u8 buf[2] = {addr >> 8, addr & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = client->addr,
            .flags  = 0,
            .len    = 2,
            .buf    = buf,
        },
        [1] = {
            .addr   = client->addr,
            .flags  = I2C_M_RD,
            .len    = 1,
            .buf    = &val,
        }
    };

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

	return val;
}

/*
 * Registers settings
 */

#define ENDMARKER { 0xff, 0xff }
static const struct regval_list ov9281_800p_regs[] = {
    {0x3808,0x05},
    {0x3809,0x00},
    {0x380a,0x03},
    {0x380b,0x20},
    {0x380c,0x02},    // HTS_H : 728 * 2
    {0x380d,0xd8},    // HTS_L
    {0x380e,0x03},    // VTS H  910
    {0x380f,0x8e},    // VTS l
	ENDMARKER,
};

static const struct regval_list ov9281_720p_regs[] = {
    {0x3808,0x05},
    {0x3809,0x00},      /* ISP Horizontal Output Width: 1280 */
    {0x380a,0x02},
    {0x380b,0xd0},      /* ISP Vertical Output Width : 720*/
    {0x380c,0x08},
    {0x380d,0xba},      /* HTS: Horizontal Timing Size: 2234 */
    {0x380e,0x03},
    {0x380f,0x8e},      /* VTS: Vertical Timing Size: 910 */
	ENDMARKER,
};

static const struct regval_list ov9281_vga_regs[] = {
    { 0x3808, 0x02 },
    { 0x3809, 0x80 },
    { 0x380a, 0x01 },
    { 0x380b, 0xe0 },
    { 0x380c, 0x02 },
    { 0x380d, 0xd8 },
    { 0x380e, 0x03 },
    { 0x380f, 0x54 },
	ENDMARKER,
};


/*
 * Interface    : MIPI - 2lane
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 608MHz(PLL1 multiplier)
 * resolution   : 1280*720
 * FrameRate    : 30fps
 * SYS_CLK      : 61MHz(PLL2_sys_clk),
 * HTS          : 2234
 * VTS          : 910
 */
static const struct regval_list ov9281_mipi_init_regs_1280_720_120fps_mipi[] = {
    {0x0103,0x01},      /* software reset */

    /* PLL Control */
    //{0x030a,0x00},    /* PLL1 pre divider0 */
    //{0x0300,0x01},    /* PLL1 pre divider */
    //{0x0301,0x00},    /* PLL1 loop 倍频 [9:8] */
    {0x0302,0x26},      /* PLL1 loop 倍频 [7:0] */
    //{0x030b,0x04},    /* PLL2 pre divider */
    //{0x030c,0x00},    /* PLL2 loop 倍频 [9:8] */
    {0x030d,0x3d},      /* PLL2 loop 倍频 [7:0] */
    //{0x0314,0x00},    /* PLL2 pre divider0 */
    {0x030e,0x02},      /* PLL2 system divider */

    /* system control */
    {0x3001,0x40},      /* 驱动能力控制 */
    {0x3004,0x00},      /* pin direction： GPIO2/D9 */
    {0x3005,0x00},      /* pin direction： D8 ~ D1 */
    {0x3006,0x04},      /* pin direction： D0/PCLK/HREF/Strobe/ILPWM/VSYNC */
    {0x3011,0x0a},
    {0x3013,0x18},      /* MIPI-PHY control */
    {0x301c,0xf0},      /* SCLK */
    {0x3022,0x01},      /* MIPI enable when rst sync */
    {0x3030,0x10},
    {0x3039,0x32},      /* Two-lane mode, MIPI enable */
    {0x303a,0x00},      /* MIPI lane disable */

    /* manual exposure */
    {0x3500,0x00},
    {0x3501,0x2a},
    {0x3502,0x90},      /* exposure[19:0] */
    {0x3503,0x08},      /* gain prec16 enable */
    {0x3505,0x8c},      /* dac finegain high bit */
    {0x3507,0x03},      /* gain shift option */
    {0x3508,0x00},
    {0x3509,0x10},      /* gain */

    /* analog control */
    {0x3610,0x80},      /* Reserved */
    {0x3611,0xa0},      /* Reserved */
    {0x3620,0x6e},      /* Reserved */
    {0x3632,0x56},      /* Reserved */
    {0x3633,0x78},      /* Reserved */
    {0x3662,0x07},      /* MIPI lane select: Bit[2]: 0-1lane 1-2lane    0x05:raw10,  0x07:raw8*/
                        /* Format select: Bit[1]: 0-RAW10  1-RAW8 */

    {0x3666,0x00},      /* VSYNC / FSIN */
    {0x366f,0x5a},      /* Reserved */
    {0x3680,0x84},      /* Reserved */

    /* sensor control */
    {0x3712,0x80},      /* Sensor Control Registers No Description */
    {0x372d,0x22},      /* Sensor Control Registers No Description */
    {0x3731,0x80},      /* Sensor Control Registers No Description */
    {0x3732,0x30},      /* Sensor Control Registers No Description */
    {0x3778,0x00},      /* Bit[4]: 2x vertical binning enable for mibochrome mode */
    {0x377d,0x22},      /* Sensor Control Registers No Description */
    {0x3788,0x02},      /* Sensor Control Registers No Description */
    {0x3789,0xa4},      /* Sensor Control Registers No Description */
    {0x378a,0x00},      /* Sensor Control Registers No Description */
    {0x378b,0x4a},      /* Sensor Control Registers No Description */
    {0x3799,0x20},      /* Sensor Control Registers No Description */

    /* timing control */
    {0x3800,0x00},
    {0x3801,0x00},      /* Horizontal Start Point : 0 */
    {0x3802,0x00},
    {0x3803,0x00},      /* Vertical Start Point   : 0 */
    {0x3804,0x05},
    {0x3805,0x0f},      /* Horizontal End Point   : 1295 */
    {0x3806,0x03},
    {0x3807,0x2f},      /* Vertical End Point     : 815 */
    {0x3808,0x05},
    {0x3809,0x00},      /* ISP Horizontal Output Width: 1280 */
    {0x380a,0x02},
    {0x380b,0xd0},      /* ISP Vertical Output Width : 720*/
    {0x380c,0x08},
    {0x380d,0xba},      /* HTS: Horizontal Timing Size: 2234 */
    {0x380e,0x03},
    {0x380f,0x8e},      /* VTS: Vertical Timing Size: 910 */
    {0x3810,0x00},
    {0x3811,0x08},      /* ISP Horizontal Windowing Offset: 8 */
    {0x3812,0x00},
    {0x3813,0x08},      /* ISP Vertical Windowing Offset: 8 */
    {0x3814,0x11},      /* X odd/even increase */
    {0x3815,0x11},      /* Y odd/even increase */
    {0x3820,0x40},      /* Timing Format1: VFlip_blc VFilp*/
    {0x3821,0x00},      /* Timing Format2: Mirr*/
    {0x382c,0x05},
    {0x382d,0xb0},      /* HTS global */

    /* Global shutter control */
    {0x3881,0x42},      /* Global Shutter Control Registers No Description */
    {0x3882,0x01},      /* Global Shutter Control Registers No Description */
    {0x3883,0x00},      /* Global Shutter Control Registers No Description */
    {0x3885,0x02},      /* Global Shutter Control Registers No Description */
    {0x389d,0x00},      /* Global Shutter Control Registers No Description */
    {0x38a8,0x02},      /* Global Shutter Control Registers No Description */
    {0x38a9,0x80},      /* Global Shutter Control Registers No Description */
    {0x38b1,0x00},      /* Global Shutter Control Registers No Description */
    {0x38b3,0x02},      /* Global Shutter Control Registers No Description */
    {0x38c4,0x00},      /* Global Shutter Control Registers No Description */
    {0x38c5,0xc0},      /* Global Shutter Control Registers No Description */
    {0x38c6,0x04},      /* Global Shutter Control Registers No Description */
    {0x38c7,0x80},      /* Global Shutter Control Registers No Description */

    /* PWM and strobe control */
    {0x3920,0xff},

    /* BLC control */
    {0x4003,0x40},
    {0x4008,0x04},
    {0x4009,0x0b},
    {0x400c,0x00},
    {0x400d,0x07},
    {0x4010,0x40},
    {0x4043,0x40},

    /* Format control */
    {0x4307,0x30},
    {0x4317,0x00},      /* DVP enable: 0-disable */

    /* Read out control */
    {0x4501,0x00},      /* Read Out Control Registers No Description */
    {0x4507,0x00},      /* Read Out Control Registers No Description */
    {0x4509,0x00},      /* Read Out Control Registers No Description */
    {0x450a,0x08},      /* Read Out Control Registers No Description */

    /* VFIFO control */
    {0x4600,0x00},
    {0x4601,0x04},      /* VFIFO Read Start Point */

    /* DVP control */
    {0x470f,0x00},

    /* MIPI Top control */
    /* mipi LPX timing:      0x03ff + Tui * 0xFF = 3378ns */
    {0x4800,0x01},      /* T-lpx select        bit[0]=0:auto calculate   =1:Use lpx_p_min */
    {0x4824,0x03},
    {0x4825,0xff},      /* lpx p min */
    {0x4830,0xff},      /* lpx_P min UI */


    {0x4802,0x84},      /* T-hs_prepare_sel     bit[7]=0:auto calculate  =1:Use hs_prepare_min */
                        /* T-hs_zero    sel     bit[2]=0:auto calculate  =1:Use hs_zero_min */

    /* mipi HS-prepare timing:  0x05 = 45ns */
    {0x4826,0x05},      /* hs_prepare min:ns */
    {0x4827,0xf5},      /* hs_prepare max:ns */
    {0x4831,0x00},      /* ui_hs_prepare_max[7:4] min[3:0]  权重低作用不大*/

    /* mipi HS-zero timing:  0x0018 + Tui * 0xFF  = 320ns */
    {0x4818,0x00},
    {0x4819,0x18},
    {0x482a,0xff},


    /* ISP Top control */
    {0x5000,0x9f},
    {0x5001,0x00},
    {0x5e00,0x00},//0x80 :color bar
    {0x5d00,0x07},
    {0x5d01,0x00},

    /* low power mode control */
    {0x4f00,0x04},
    {0x4f07,0x00},
    {0x4f10,0x00},
    {0x4f11,0x98},
    {0x4f12,0x0f},
    {0x4f13,0xc4},
    /* Stream ON */
/*    {0x0100,0x01},*/
	ENDMARKER,
};

static const struct regval_list ov9281_wb_auto_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov9281_wb_incandescence_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov9281_wb_daylight_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov9281_wb_fluorescent_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov9281_wb_cloud_regs[] = {
	ENDMARKER,
};

static const struct mode_list ov9281_balance[] = {
	{0, ov9281_wb_auto_regs}, {1, ov9281_wb_incandescence_regs},
	{2, ov9281_wb_daylight_regs}, {3, ov9281_wb_fluorescent_regs},
	{4, ov9281_wb_cloud_regs},
};


static const struct regval_list ov9281_effect_normal_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov9281_effect_grayscale_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov9281_effect_sepia_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov9281_effect_colorinv_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov9281_effect_sepiabluel_regs[] = {
	ENDMARKER,
};

static const struct mode_list ov9281_effect[] = {
	{0, ov9281_effect_normal_regs}, {1, ov9281_effect_grayscale_regs},
	{2, ov9281_effect_sepia_regs}, {3, ov9281_effect_colorinv_regs},
	{4, ov9281_effect_sepiabluel_regs},
};


#define OV9281_SIZE(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs = r }

static struct ov9281_win_size ov9281_supported_win_sizes[] = {
	OV9281_SIZE("800P", W_720P, H_800P, ov9281_800p_regs),
	OV9281_SIZE("720P", W_720P, H_720P, ov9281_720p_regs),
	OV9281_SIZE("vga", W_640,H_480,ov9281_vga_regs),
};

static enum v4l2_mbus_pixelcode ov9281_codes[] = {
	V4L2_MBUS_FMT_Y8_1X8,
};

/*
 * Supported balance menus
 */
static const struct v4l2_querymenu ov9281_balance_menus[] = {
	{
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 0,
		.name		= "auto",
	}, {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 1,
		.name		= "incandescent",
	}, {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 2,
		.name		= "fluorescent",
	},  {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 3,
		.name		= "daylight",
	},  {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 4,
		.name		= "cloudy-daylight",
	},

};

/*
 * Supported effect menus
 */
static const struct v4l2_querymenu ov9281_effect_menus[] = {
	{
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 0,
		.name		= "none",
	}, {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 1,
		.name		= "mono",
	}, {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 2,
		.name		= "sepia",
	},  {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 3,
		.name		= "negative",
	}, {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 4,
		.name		= "aqua",
	},
};


/*
 * General functions
 */
static struct ov9281_priv *to_ov9281(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ov9281_priv,
			    subdev);
}

static int ov9281_write_array(struct i2c_client *client,
			      const struct regval_list *vals)
{
	int ret = 0;

	while ((vals->reg_num != 0xff) || (vals->value != 0xff)) {
		ret = ov9281_write_reg(client, vals->reg_num, vals->value);
		if (ret < 0)
			return ret;
		vals++;
	}
	return ret;
}

static int ov9281_mask_set(struct i2c_client *client,
			   u16  reg, u8  mask, u8  set)
{
	s32 val = ov9281_read_reg(client, reg);
	if (val < 0)
		return val;

	val &= ~mask;
	val |= set & mask;

	dev_vdbg(&client->dev, "masks: 0x%02x, 0x%02x", reg, val);

	return ov9281_write_reg(client, reg, val);
}

/*
 * soc_camera_ops functions
 */
static int ov9281_s_stream(struct v4l2_subdev *sd, int enable)
{
    int ret;
    struct i2c_client  *client = v4l2_get_subdevdata(sd);

    if(enable)
        ret = ov9281_write_reg(client, 0x0100, 0x01);
    else
        ret = ov9281_write_reg(client, 0x0100, 0x00);
    return ret;
}

#if 0
static int ov9281_set_bus_param(struct soc_camera_device *icd,
				unsigned long flags)
{
	struct soc_camera_link *icl = to_soc_camera_link(icd);
	unsigned long width_flag = flags & SOCAM_DATAWIDTH_MASK;

	/* Only one width bit may be set */
	if (!is_power_of_2(width_flag))
		return -EINVAL;

	if (icl->set_bus_param)
		return icl->set_bus_param(icl, width_flag);

	/*
	 * Without board specific bus width settings we support only the
	 * sensors native bus width which are tested working
	 */
	if (width_flag & SOCAM_DATAWIDTH_8)
		return 0;

	return 0;
}

static unsigned long ov9281_query_bus_param(struct soc_camera_device *icd)
{
	struct soc_camera_link *icl = to_soc_camera_link(icd);
	unsigned long flags = SOCAM_PCLK_SAMPLE_FALLING | SOCAM_MASTER |
		SOCAM_VSYNC_ACTIVE_HIGH | SOCAM_HSYNC_ACTIVE_HIGH |
		SOCAM_DATA_ACTIVE_HIGH;

	if (icl->query_bus_param)
		flags |= icl->query_bus_param(icl) & SOCAM_DATAWIDTH_MASK;
	else
		flags |= SOCAM_DATAWIDTH_8;

	return soc_camera_apply_sensor_flags(icl, flags);
}
#endif
static int ov9281_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov9281_priv *priv = to_ov9281(client);

	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		ctrl->value = priv->flag_vflip;
		break;
	case V4L2_CID_HFLIP:
		ctrl->value = priv->flag_hflip;
		break;
	case V4L2_CID_PRIVATE_BALANCE:
		ctrl->value = priv->balance_value;
		break;
	case V4L2_CID_PRIVATE_EFFECT:
		ctrl->value = priv->effect_value;
		break;
	default:
		break;
	}
	return 0;
}

static int ov9281_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov9281_priv *priv = to_ov9281(client);
	int ret = 0;
	int i = 0;
	u8 value;

	int balance_count = ARRAY_SIZE(ov9281_balance);
	int effect_count = ARRAY_SIZE(ov9281_effect);

	switch (ctrl->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		if(ctrl->value > balance_count)
			return -EINVAL;

		for(i = 0; i < balance_count; i++) {
			if(ctrl->value == ov9281_balance[i].index) {
				ret = ov9281_write_array(client,
						ov9281_balance[ctrl->value].mode_regs);
				priv->balance_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		if(ctrl->value > effect_count)
			return -EINVAL;

		for(i = 0; i < effect_count; i++) {
			if(ctrl->value == ov9281_effect[i].index) {
				ret = ov9281_write_array(client,
						ov9281_effect[ctrl->value].mode_regs);
				priv->effect_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_VFLIP:
		value = ctrl->value ? REG14_VFLIP_IMG : 0x00;
		priv->flag_vflip = ctrl->value ? 1 : 0;
		ret = ov9281_mask_set(client, REG14, REG14_VFLIP_IMG, value);
		break;

	case V4L2_CID_HFLIP:
		value = ctrl->value ? REG14_HFLIP_IMG : 0x00;
		priv->flag_hflip = ctrl->value ? 1 : 0;
		ret = ov9281_mask_set(client, REG14, REG14_HFLIP_IMG, value);
		break;

	default:
		dev_err(&client->dev, "no V4L2 CID: 0x%x ", ctrl->id);
		return -EINVAL;
	}

	return ret;
}

static int ov9281_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *id)
{
	id->ident    = SUPPORT_HIGH_RESOLUTION_PRE;
	id->revision = 0;

	return 0;
}

static int ov9281_querymenu(struct v4l2_subdev *sd,
					struct v4l2_querymenu *qm)
{
	switch (qm->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		memcpy(qm->name, ov9281_balance_menus[qm->index].name,
				sizeof(qm->name));
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		memcpy(qm->name, ov9281_effect_menus[qm->index].name,
				sizeof(qm->name));
		break;
	}

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov9281_g_register(struct v4l2_subdev *sd,
			     struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	reg->size = 1;
	if (reg->reg > 0xff)
		return -EINVAL;

	ret = ov9281_read_reg(client, reg->reg);
	if (ret < 0)
		return ret;

	reg->val = ret;

	return 0;
}

static int ov9281_s_register(struct v4l2_subdev *sd,
			     struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (reg->reg > 0xff ||
	    reg->val > 0xff)
		return -EINVAL;

	return ov9281_write_reg(client, reg->reg, reg->val);
}
#endif

/* Select the nearest higher resolution for capture */
static struct ov9281_win_size *ov9281_select_win(u32 *width, u32 *height)
{
	int i, default_size = ARRAY_SIZE(ov9281_supported_win_sizes) - 1;
	for (i = 0; i < ARRAY_SIZE(ov9281_supported_win_sizes); i++) {
		if ((*width >= ov9281_supported_win_sizes[i].width) &&
		    (*height >= ov9281_supported_win_sizes[i].height)) {
			*width = ov9281_supported_win_sizes[i].width;
			*height = ov9281_supported_win_sizes[i].height;
			return &ov9281_supported_win_sizes[i];
		}
	}

	*width = ov9281_supported_win_sizes[default_size].width;
	*height = ov9281_supported_win_sizes[default_size].height;
	return &ov9281_supported_win_sizes[default_size];
}

static int ov9281_g_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov9281_priv *priv = to_ov9281(client);

	mf->width = priv->win->width;
	mf->height = priv->win->height;
	mf->code = priv->cfmt_code;

	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field	= V4L2_FIELD_NONE;

	return 0;
}

static int ov9281_s_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	/* current do not support set format, use unify format yuv422i */
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov9281_priv *priv = to_ov9281(client);
	int ret;

	priv->win = ov9281_select_win(&mf->width, &mf->height);
	/* set size win */
	ret = ov9281_write_array(client, priv->win->regs);
	if (ret < 0) {
		dev_err(&client->dev, "%s: Error\n", __func__);
		return ret;
	}

	return 0;
}

static int ov9281_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *mf)
{
	const struct ov9281_win_size *win;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	/*
	 * select suitable win
	 */
	win = ov9281_select_win(&mf->width, &mf->height);

	if(mf->field == V4L2_FIELD_ANY) {
		mf->field = V4L2_FIELD_NONE;
	} else if (mf->field != V4L2_FIELD_NONE) {
		dev_err(&client->dev, "Field type invalid.\n");
		return -ENODEV;
	}
	mf->code = V4L2_MBUS_FMT_Y8_1X8;
	mf->colorspace = V4L2_COLORSPACE_JPEG;
	return 0;
}

static int ov9281_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	if (index >= ARRAY_SIZE(ov9281_codes))
		return -EINVAL;

	*code = ov9281_codes[index];
	return 0;
}

static int ov9281_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	return 0;
}

static int ov9281_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	return 0;
}

static int ov9281_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9281_priv *priv = to_ov9281(client);
	int ret;
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);

	int bala_index = priv->balance_value;
	int effe_index = priv->effect_value;

	if(!on)
		return soc_camera_power_off(&client->dev,ssdd);
	ret = soc_camera_power_on(&client->dev,ssdd);
	if(ret < 0)
		return ret;

	///* initialize the sensor with default data */
	ret = ov9281_write_array(client, ov9281_mipi_init_regs_1280_720_120fps_mipi);
	if (ret < 0)
		goto err;

	dev_dbg(&client->dev, "%s: Init default", __func__);
	return 0;

err:
	dev_err(&client->dev, "%s: Error %d", __func__, ret);
	return ret;
}

static int ov9281_g_mbus_config(struct v4l2_subdev *sd,
		struct v4l2_mbus_config *cfg)
{

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);

	cfg->flags = V4L2_MBUS_PCLK_SAMPLE_FALLING | V4L2_MBUS_MASTER |
		    V4L2_MBUS_VSYNC_ACTIVE_HIGH | V4L2_MBUS_HSYNC_ACTIVE_HIGH |
			    V4L2_MBUS_DATA_ACTIVE_HIGH;
	cfg->type = V4L2_MBUS_PARALLEL;

	cfg->flags = soc_camera_apply_board_flags(ssdd, cfg);

	return 0;
}

static struct v4l2_subdev_core_ops ov9281_subdev_core_ops = {
//	.init		= ov9281_init,
	.s_power 	= ov9281_s_power,
	.g_ctrl		= ov9281_g_ctrl,
	.s_ctrl		= ov9281_s_ctrl,
	.g_chip_ident	= ov9281_g_chip_ident,
	.querymenu	= ov9281_querymenu,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register	= ov9281_g_register,
	.s_register	= ov9281_s_register,
#endif
};

static struct v4l2_subdev_video_ops ov9281_subdev_video_ops = {
	.s_stream	= ov9281_s_stream,
	.g_mbus_fmt	= ov9281_g_fmt,
	.s_mbus_fmt	= ov9281_s_fmt,
	.try_mbus_fmt	= ov9281_try_fmt,
	.cropcap	= ov9281_cropcap,
	.g_crop		= ov9281_g_crop,
	.enum_mbus_fmt	= ov9281_enum_fmt,
	.g_mbus_config  = ov9281_g_mbus_config,
};

static struct v4l2_subdev_ops ov9281_subdev_ops = {
	.core	= &ov9281_subdev_core_ops,
	.video	= &ov9281_subdev_video_ops,
};

/*
 * i2c_driver functions
 */

static int ov9281_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct ov9281_priv *priv;
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	int ret = 0,default_width = 1280,default_height=720;
	unsigned char retval_high = 0, retval_low = 0;

	if (!ssdd) {
		dev_err(&client->dev, "ov9281: missing platform data!\n");
		return -EINVAL;
	}

	if(!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE
			| I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "client not i2c capable\n");
		return -ENODEV;
	}

	priv = kzalloc(sizeof(struct ov9281_priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&adapter->dev,
			"Failed to allocate memory for private data!\n");
		return -ENOMEM;
	}

	v4l2_i2c_subdev_init(&priv->subdev, client, &ov9281_subdev_ops);

	priv->win = ov9281_select_win(&default_width, &default_height);

	priv->cfmt_code = V4L2_MBUS_FMT_Y8_1X8,

	ret = soc_camera_power_on(&client->dev,ssdd);

	retval_high = ov9281_read_reg(client, REG_CHIP_ID_HIGH);
	if (retval_high != CHIP_ID_HIGH) {
		dev_err(&client->dev, "read sensor %s chip_id high %x is error\n",
				client->name, retval_high);
		return -1;
	}
	retval_low = ov9281_read_reg(client, REG_CHIP_ID_LOW);
	if (retval_low != CHIP_ID_LOW) {
		dev_err(&client->dev, "read sensor %s chip_id low %x is error\n",
				client->name, retval_low);
		return -1;
	}
	printk("read sensor %s id high:0x%x,low:%x successed!\n",
			client->name, retval_high, retval_low);

	ret = soc_camera_power_off(&client->dev,ssdd);
    return 0;
}

static int ov9281_remove(struct i2c_client *client)
{
	struct ov9281_priv       *priv = to_ov9281(client);
//	struct soc_camera_device *icd = client->dev.platform_data;
//	icd->ops = NULL;
	kfree(priv);
	return 0;
}

static const struct i2c_device_id ov9281_id[] = {
	{ "ov9281",  0 },
	{ }
};


MODULE_DEVICE_TABLE(i2c, ov9281_id);

static struct i2c_driver ov9281_i2c_driver = {
	.driver = {
		.name = "ov9281",
	},
	.probe    = ov9281_probe,
	.remove   = ov9281_remove,
	.id_table = ov9281_id,
};

/*
 * Module functions
 */
static int __init ov9281_module_init(void)
{
	return i2c_add_driver(&ov9281_i2c_driver);
}

static void __exit ov9281_module_exit(void)
{
	i2c_del_driver(&ov9281_i2c_driver);
}

module_init(ov9281_module_init);
module_exit(ov9281_module_exit);

MODULE_DESCRIPTION("camera sensor ov9281 mipi driver");
MODULE_AUTHOR("zhang xiaoyan <xiaoyan.zhang@ingenic.com>");
MODULE_LICENSE("GPL");
