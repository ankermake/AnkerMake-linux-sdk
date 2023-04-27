#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
//#include <media/ov2710.h>
#include <mach/ovisp-v4l2.h>


#define OV2710_CHIP_ID_H	(0x27)
#define OV2710_CHIP_ID_L	(0x10)

#define SXGA_WIDTH		1280
#define SXGA_HEIGHT		960
#define VGA_WIDTH		640
#define VGA_HEIGHT		480
#define _720P_WIDTH		1280
#define _720P_HEIGHT		720
#define _1080P_WIDTH		1920
#define _1080P_HEIGHT	1080

#define MAX_WIDTH		2592
#define MAX_HEIGHT		1944

#define OV2710_REG_END		0xffff
#define OV2710_REG_DELAY	0xfffe

//#define OV2710_YUV

struct ov2710_format_struct;
struct ov2710_info {
	struct v4l2_subdev sd;
	struct ov2710_format_struct *fmt;
	struct ov2710_win_setting *win;
};

struct regval_list {
	unsigned short reg_num;
	unsigned char value;
};


#ifdef CONFIG_MIPI_CAMERA_BYPASS_MODE

/**
* @brief common init.
*/
static const struct regval_list ov2710_init_regs[] = {

	{0x3103, 0x93},
	{0x3008, 0x82},
	{0x3017, 0x7f},
	{0x3018, 0xfc},
	{0x3706, 0x61},
	{0x3712, 0x0c},
	{0x3630, 0x6d},
	{0x3801, 0xb4},
	{0x3621, 0x04},
	{0x3604, 0x60},
	{0x3603, 0xa7},
	{0x3631, 0x26},
	{0x3600, 0x04},
	{0x3620, 0x37},
	{0x3623, 0x00},
	{0x3702, 0x9e},
	{0x3703, 0x5c},
	{0x3704, 0x40},
	{0x370d, 0x0f},
	{0x3713, 0x9f},
	{0x3714, 0x4c},
	{0x3710, 0x9e},
	{0x3801, 0xc4},
	{0x3605, 0x05},
	{0x3606, 0x3f},
	{0x302d, 0x90},
	{0x370b, 0x40},
	{0x3716, 0x31},
	{0x380d, 0x74},
	{0x5181, 0x20},
	{0x518f, 0x00},
	{0x4301, 0xff},
	{0x4303, 0x00},
	{0x3a00, 0x78},
	{0x300f, 0x88},
	{0x3011, 0x28},
	{0x3a1a, 0x06},
	{0x3a18, 0x00},
	{0x3a19, 0x7a},
	{0x3a13, 0x54},
	{0x382e, 0x0f},
	{0x381a, 0x1a},
	{0x401d, 0x02},
	{0x5688, 0x03},
	{0x5684, 0x07},
	{0x5685, 0xa0},
	{0x5686, 0x04},
	{0x5687, 0x43},
	{0x3011, 0x0a},
	{0x300f, 0x8a},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x300e, 0x04},
	{0x4801, 0x0f},
	{0x300f, 0xc3},
	{0x3a0f, 0x40},
	{0x3a10, 0x38},
	{0x3a1b, 0x48},
	{0x3a1e, 0x30},
	{0x3a11, 0x90},
	{0x3a1f, 0x10},
	{0x4800, 0x24},
	{0x4805, 0x90},
	{OV2710_REG_END, 0x00},	/* END MARKER */
};

static const struct regval_list ov2710_vga_regs[] = {
	//Sysclk = 42Mhz, MIPI 2 lane 168MBps

	{OV2710_REG_END, 0x00},	/* END MARKER */
};
static const struct regval_list ov2710_720p_regs[] = {

	//Sysclk = 42Mhz, MIPI 2 lane 168MBps
	//0x3612, 0xa9,

	{OV2710_REG_END, 0x00},	/* END MARKER */
};

static const struct regval_list ov2710_960p_regs[] = {
	// Sysclk = 56Mhz, MIPI 2 lane 224MBps
	//0x3612, 0xa9,

	{OV2710_REG_END, 0x00},	/* END MARKER */
};
static const struct regval_list ov2710_5M_regs[] = {
	// Sysclk = 84Mhz, 2 lane mipi, 336MBps
	// //0x3612, 0xab,
	{OV2710_REG_END, 0x00},	/* END MARKER */
};
#else

static const struct regval_list ov2710_init_regs[] = {
	{0x3103, 0x93},
	{0x3008, 0x82},
	{0x3017, 0x7f},
	{0x3018, 0xfc},
	{0x3706, 0x61},
	{0x3712, 0x0c},
	{0x3630, 0x6d},
	{0x3801, 0xb4},
	{0x3621, 0x04},
	{0x3604, 0x60},
	{0x3603, 0xa7},
	{0x3631, 0x26},
	{0x3600, 0x04},
	{0x3620, 0x37},
	{0x3623, 0x00},
	{0x3702, 0x9e},
	{0x3703, 0x5c},
	{0x3704, 0x40},
	{0x370d, 0x0f},
	{0x3713, 0x9f},
	{0x3714, 0x4c},
	{0x3710, 0x9e},
	{0x3801, 0xc4},
	{0x3605, 0x05},
	{0x3606, 0x3f},
	{0x302d, 0x90},
	{0x370b, 0x40},
	{0x3716, 0x31},
	{0x380d, 0x74},
	{0x5181, 0x20},
	{0x518f, 0x00},
	{0x4301, 0xff},
	{0x4303, 0x00},
	{0x3a00, 0x78},
	{0x300f, 0x88},
	{0x3011, 0x28},
	{0x3a1a, 0x06},
	{0x3a18, 0x00},
	{0x3a19, 0x7a},
	{0x3a13, 0x54},
	{0x382e, 0x0f},
	{0x381a, 0x1a},
	{0x401d, 0x02},
	{0x5688, 0x03},
	{0x5684, 0x07},
	{0x5685, 0xa0},
	{0x5686, 0x04},
	{0x5687, 0x43},
	{0x3011, 0x0a},
	{0x300f, 0x8a},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x300e, 0x04},
	{0x4801, 0x0f},
	{0x300f, 0xc3},
	{0x3a0f, 0x40},
	{0x3a10, 0x38},
	{0x3a1b, 0x48},
	{0x3a1e, 0x30},
	{0x3a11, 0x90},
	{0x3a1f, 0x10},
	{0x4800, 0x24},
	{0x4805, 0x90},
	{OV2710_REG_END, 0x00},	/* END MARKER */
};
static struct regval_list ov2710_init_regs_672Mbps_5M[] = {
/*
	@@ MIPI_2lane_5M(RAW8) 15fps MIPI CLK = 336MHz
99 2592 1944
98 1 0
64 1000 7
102 3601 5dc
;
;OV2710 setting Version History
;dated 08/28/2013 A07
;--updated VGA/SXGA binning setting
;--updated CIP auto gain setting.
;
;dated 08/07/2013 A06b
;--updated DPC settings
;
;dated 08/28/2012 A05
;--Based on v05 release
;
;dated 06/13/2012 A03
;--Based on v03 release
;
;dated 08/28/2012 A05
;--Based on V05 release
;
;dated 04/09/2012
;--Add 4050/4051 BLC level trigger
*/

	{OV2710_REG_END, 0x00},	/* END MARKER */

};

static struct regval_list ov2710_init_regs_672Mbps_1080p[] = {
	/*
	   @@ MIPI_2lane_1080P(RAW8) 30fps MIPI CLK = 336MHz
	   99 1920 1080
	   98 1 0
	   64 1000 7
	   102 3601 bb8
	   */

	{OV2710_REG_END, 0x00},	/* END MARKER */

};

static struct regval_list ov2710_init_regs_raw10[] = {

	/* @@ MIPI_2lane_720P_RAW10_30fps */
	/* 99 1280 720 */
	/* 98 1 0 */
	/* 64 1000 7 */

       {OV2710_REG_END, 0x00},	/* END MARKER */
};

#endif

static struct regval_list ov2710_stream_on[] = {
	{0x4202, 0x00},
	{0x4800, 0x04},

	{OV2710_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov2710_stream_off[] = {
	/* Sensor enter LP11*/
	{0x4202, 0x0f},
	{0x4800, 0x24},

	{OV2710_REG_END, 0x00},	/* END MARKER */
};

int ov2710_read(struct v4l2_subdev *sd, unsigned short reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[2] = {reg >> 8, reg & 0xff};
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


static int ov2710_write(struct v4l2_subdev *sd, unsigned short reg,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 3,
		.buf	= buf,
	};
	int ret;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}


static int ov2710_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != OV2710_REG_END) {
		if (vals->reg_num == OV2710_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = ov2710_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		printk("vals->reg_num:%x, vals->value:%x\n",vals->reg_num, val);
		//mdelay(200);
		vals++;
	}
	//printk("vals->reg_num:%x, vals->value:%x\n", vals->reg_num, vals->value);
	ov2710_write(sd, vals->reg_num, vals->value);
	return 0;
}
static int ov2710_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != OV2710_REG_END) {
		if (vals->reg_num == OV2710_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = ov2710_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		//printk("vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
		//mdelay(200);
		vals++;
	}
	//printk("vals->reg_num:%x, vals->value:%x\n", vals->reg_num, vals->value);
	//ov2710_write(sd, vals->reg_num, vals->value);
	return 0;
}

/* R/G and B/G of typical camera module is defined here. */
static unsigned int rg_ratio_typical = 0x58;
static unsigned int bg_ratio_typical = 0x5a;

/*
	R_gain, sensor red gain of AWB, 0x400 =1
	G_gain, sensor green gain of AWB, 0x400 =1
	B_gain, sensor blue gain of AWB, 0x400 =1
	return 0;
*/
static int ov2710_update_awb_gain(struct v4l2_subdev *sd,
				unsigned int R_gain, unsigned int G_gain, unsigned int B_gain)
{
#if 0
	printk("[ov2710] problem function:%s, line:%d\n", __func__, __LINE__);
	printk("R_gain:%04x, G_gain:%04x, B_gain:%04x \n ", R_gain, G_gain, B_gain);
	if (R_gain > 0x400) {
		ov2710_write(sd, 0x5186, (unsigned char)(R_gain >> 8));
		ov2710_write(sd, 0x5187, (unsigned char)(R_gain & 0x00ff));
	}

	if (G_gain > 0x400) {
		ov2710_write(sd, 0x5188, (unsigned char)(G_gain >> 8));
		ov2710_write(sd, 0x5189, (unsigned char)(G_gain & 0x00ff));
	}

	if (B_gain > 0x400) {
		ov2710_write(sd, 0x518a, (unsigned char)(B_gain >> 8));
		ov2710_write(sd, 0x518b, (unsigned char)(B_gain & 0x00ff));
	}
#endif

	return 0;
}

static int ov2710_reset(struct v4l2_subdev *sd, u32 val)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov2710_detect(struct v4l2_subdev *sd);
static int ov2710_init(struct v4l2_subdev *sd, u32 val)
{
	struct ov2710_info *info = container_of(sd, struct ov2710_info, sd);
	int ret = 0;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);

	info->win = NULL;

	//ret = ov2710_write_array(sd, ov2710_init_regs_raw10);
	//ret = ov2710_write_array(sd, ov2710_init_regs_672Mbps_1080p);
	//ret = ov2710_write_array(sd, ov2710_init_regs_672Mbps_5M);
	ret = ov2710_write_array(sd, ov2710_init_regs);
	if (ret < 0)
		return ret;

	printk("--%s:%d\n", __func__, __LINE__);
	if (ret < 0)
		return ret;

	return 0;
}
static int ov2710_get_sensor_vts(struct v4l2_subdev *sd, unsigned short *value)
{
	unsigned char h,l;
	int ret = 0;
	ret = ov2710_read(sd, 0x380e, &h);
	if (ret < 0)
		return ret;
	ret = ov2710_read(sd, 0x380f, &l);
	if (ret < 0)
		return ret;
	*value = h;
	*value = (*value << 8) | l;
	return ret;
}

static int ov2710_get_sensor_lans(struct v4l2_subdev *sd, unsigned char *value)
{
	int ret = 0;
	unsigned char v = 0;
	ret = ov2710_read(sd, 0x4800, &v);
	printk("ov2710 reg 4800 is : 0x%x\n", v);
	if (ret < 0)
		return ret;
	*value = ((v >> 3) & 0x1) + 1;
	if(*value > 2 || *value < 1)
		ret = -EINVAL;
	return ret;
}
static int ov2710_detect(struct v4l2_subdev *sd)
{
	unsigned char v;
	int ret;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
#if 0
	/*test gpio e*/
	{
		printk("0x10010410:%x\n", *((volatile unsigned int *)0xb0010410));
		printk("0x10010420:%x\n", *((volatile unsigned int *)0xb0010420));
		printk("0x10010430:%x\n", *((volatile unsigned int *)0xb0010430));
		printk("0x10010440:%x\n", *((volatile unsigned int *)0xb0010440));
		printk("0x10010010:%x\n", *((volatile unsigned int *)0xb0010010));
		printk("0x10010020:%x\n", *((volatile unsigned int *)0xb0010020));
		printk("0x10010030:%x\n", *((volatile unsigned int *)0xb0010030));
		printk("0x10010040:%x\n", *((volatile unsigned int *)0xb0010040));
	}
#endif

	ret = ov2710_read(sd, 0x300a, &v);
	printk("-----%s: %d ret = %d\n", __func__, __LINE__, ret);
	if (ret < 0)
		return ret;
	printk("-----%s: %d v = %08X\n", __func__, __LINE__, v);
	if (v != OV2710_CHIP_ID_H)
		return -ENODEV;
	ret = ov2710_read(sd, 0x300b, &v);
	printk("-----%s: %d ret = %d\n", __func__, __LINE__, ret);
	if (ret < 0)
		return ret;
	printk("-----%s: %d v = %08X\n", __func__, __LINE__, v);
	if (v != OV2710_CHIP_ID_L)
		return -ENODEV;
	return 0;
}

static struct ov2710_format_struct {
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
} ov2710_formats[] = {
#ifdef CONFIG_MIPI_CAMERA_BYPASS_MODE
	{
		.mbus_code = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace	 = V4L2_COLORSPACE_JPEG,
	}

#else
	{
		/*RAW8 FORMAT, 8 bit per pixel*/
		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,
		.colorspace	= V4L2_COLORSPACE_SRGB,
	},
	{
		/*RAW10 FORMAT, 10 bit per pixel*/
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
	},
#endif
	/*add other format supported*/
};
#define N_OV2710_FMTS ARRAY_SIZE(ov2710_formats)

static struct ov2710_win_setting {
	int	width;
	int	height;
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
	struct regval_list *regs; /* Regs to tweak */
} ov2710_win_sizes[] = {
#ifdef CONFIG_MIPI_CAMERA_BYPASS_MODE

	/* 2592*1944 */
	{
		.width		= MAX_WIDTH,
		.height		= MAX_HEIGHT,
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace	= V4L2_COLORSPACE_JPEG,
		.regs 		= ov2710_5M_regs,
	},

	/* 1280 * 960 */
	{
		.width		= 1280,
		.height		= 960,
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace	= V4L2_COLORSPACE_JPEG,
		.regs 		= ov2710_960p_regs,
	},

	/* 1280 * 720 */
	{
		.width		= 1280,
		.height		= 720,
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace	= V4L2_COLORSPACE_JPEG,
		.regs 		= ov2710_720p_regs,
	},

	/* 640 * 480 */
	{
		.width		= 640,
		.height		= 480,
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace	= V4L2_COLORSPACE_JPEG,
		.regs 		= ov2710_vga_regs,
	},

#else

	/* 1280*720 */
	{
		.width		= 1280,
		.height		= 720,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ov2710_init_regs_raw10,
	},

#endif
};
#define N_WIN_SIZES (ARRAY_SIZE(ov2710_win_sizes))

static int ov2710_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned index,
					enum v4l2_mbus_pixelcode *code)
{
	if (index >= N_OV2710_FMTS)
		return -EINVAL;

	*code = ov2710_formats[index].mbus_code;
	return 0;
}

static int ov2710_try_fmt_internal(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt,
		struct ov2710_win_setting **ret_wsize)
{
	struct ov2710_win_setting *wsize;

	if(fmt->width > MAX_WIDTH || fmt->height > MAX_HEIGHT)
		return -EINVAL;

	for (wsize = ov2710_win_sizes; wsize < ov2710_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;
	if (wsize >= ov2710_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	fmt->width = wsize->width;
	fmt->height = wsize->height;
	fmt->code = wsize->mbus_code;
	fmt->field = V4L2_FIELD_NONE;
	fmt->colorspace = wsize->colorspace;
	return 0;
}

static int ov2710_try_mbus_fmt(struct v4l2_subdev *sd,
			    struct v4l2_mbus_framefmt *fmt)
{
	return ov2710_try_fmt_internal(sd, fmt, NULL);
}


static int ov2710_s_mbus_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *fmt)
{

	struct ov2710_info *info = container_of(sd, struct ov2710_info, sd);
	struct v4l2_fmt_data *data = v4l2_get_fmt_data(fmt);
	struct ov2710_win_setting *wsize;
	int ret;


	ret = ov2710_try_fmt_internal(sd, fmt, &wsize);
	if (ret)
		return ret;

	if ((info->win != wsize) && wsize->regs) {
		ret = ov2710_write_array(sd, wsize->regs);
		if (ret)
			return ret;
	}

	data->i2cflags = V4L2_I2C_ADDR_16BIT;
	data->mipi_clk = 282;
	ret = ov2710_get_sensor_vts(sd, &(data->vts));
	if(ret < 0){
		return ret;
	}
	ret = ov2710_get_sensor_lans(sd, &(data->lans));
	if(ret < 0){
		return ret;
	}

	info->win = wsize;

	return 0;
}

static int ov2710_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	if (enable) {
		ret = ov2710_write_array(sd, ov2710_stream_on);
	}
	else {
		ret = ov2710_write_array(sd, ov2710_stream_off);
	}
	return ret;
}

static int ov2710_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	a->c.left	= 0;
	a->c.top	= 0;
	a->c.width	= MAX_WIDTH;
	a->c.height	= MAX_HEIGHT;
	a->type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;

	return 0;
}

static int ov2710_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	a->bounds.left			= 0;
	a->bounds.top			= 0;
	a->bounds.width			= MAX_WIDTH;
	a->bounds.height		= MAX_HEIGHT;
	a->defrect			= a->bounds;
	a->type				= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	a->pixelaspect.numerator	= 1;
	a->pixelaspect.denominator	= 1;

	return 0;
}

static int ov2710_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov2710_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov2710_frame_rates[] = { 30, 15, 10, 5, 1 };

static int ov2710_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	if (interval->index >= ARRAY_SIZE(ov2710_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = ov2710_frame_rates[interval->index];
	return 0;
}

static int ov2710_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;

	/*
	 * If a minimum width/height was requested, filter out the capture
	 * windows that fall outside that.
	 */
	for (i = 0; i < N_WIN_SIZES; i++) {
		struct ov2710_win_setting *win = &ov2710_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int ov2710_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	return 0;
}

static int ov2710_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	return 0;
}

static int ov2710_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	return 0;
}

static int ov2710_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

//	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_OV2710, 0);
	return v4l2_chip_ident_i2c_client(client, chip, 123, 0);
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov2710_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ov2710_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int ov2710_s_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ov2710_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static int ov2710_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static const struct v4l2_subdev_core_ops ov2710_core_ops = {
	.g_chip_ident = ov2710_g_chip_ident,
	.g_ctrl = ov2710_g_ctrl,
	.s_ctrl = ov2710_s_ctrl,
	.queryctrl = ov2710_queryctrl,
	.reset = ov2710_reset,
	.init = ov2710_init,
	.s_power = ov2710_s_power,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov2710_g_register,
	.s_register = ov2710_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ov2710_video_ops = {
	.enum_mbus_fmt = ov2710_enum_mbus_fmt,
	.try_mbus_fmt = ov2710_try_mbus_fmt,
	.s_mbus_fmt = ov2710_s_mbus_fmt,
	.s_stream = ov2710_s_stream,
	.cropcap = ov2710_cropcap,
	.g_crop	= ov2710_g_crop,
	.s_parm = ov2710_s_parm,
	.g_parm = ov2710_g_parm,
	.enum_frameintervals = ov2710_enum_frameintervals,
	.enum_framesizes = ov2710_enum_framesizes,
};

static const struct v4l2_subdev_ops ov2710_ops = {
	.core = &ov2710_core_ops,
	.video = &ov2710_video_ops,
};

static ssize_t ov2710_rg_ratio_typical_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", rg_ratio_typical);
}

static ssize_t ov2710_rg_ratio_typical_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	char *endp;
	int value;

	value = simple_strtoul(buf, &endp, 0);
	if (buf == endp)
		return -EINVAL;

	rg_ratio_typical = (unsigned int)value;

	return size;
}

static ssize_t ov2710_bg_ratio_typical_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", bg_ratio_typical);
}

static ssize_t ov2710_bg_ratio_typical_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	char *endp;
	int value;

	value = simple_strtoul(buf, &endp, 0);
	if (buf == endp)
		return -EINVAL;

	bg_ratio_typical = (unsigned int)value;

	return size;
}

static DEVICE_ATTR(ov2710_rg_ratio_typical, 0664, ov2710_rg_ratio_typical_show, ov2710_rg_ratio_typical_store);
static DEVICE_ATTR(ov2710_bg_ratio_typical, 0664, ov2710_bg_ratio_typical_show, ov2710_bg_ratio_typical_store);

static int ov2710_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct ov2710_info *info;
	int ret;

	info = kzalloc(sizeof(struct ov2710_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &ov2710_ops);

	/* Make sure it's an ov2710 */
//aaa:
	ret = ov2710_detect(sd);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an ov2710 chip.\n",
			client->addr, client->adapter->name);
//		goto aaa;
		kfree(info);
		return ret;
	}
	v4l_info(client, "ov2710 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);

	ret = device_create_file(&client->dev, &dev_attr_ov2710_rg_ratio_typical);
	if(ret){
		v4l_err(client, "create dev_attr_ov2710_rg_ratio_typical failed!\n");
		goto err_create_dev_attr_ov2710_rg_ratio_typical;
	}

	ret = device_create_file(&client->dev, &dev_attr_ov2710_bg_ratio_typical);
	if(ret){
		v4l_err(client, "create dev_attr_ov2710_bg_ratio_typical failed!\n");
		goto err_create_dev_attr_ov2710_bg_ratio_typical;
	}

	printk("probe ok ------->ov2710\n");
	return 0;

err_create_dev_attr_ov2710_bg_ratio_typical:
	device_remove_file(&client->dev, &dev_attr_ov2710_rg_ratio_typical);
err_create_dev_attr_ov2710_rg_ratio_typical:
	kfree(info);
	return ret;
}

static int ov2710_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov2710_info *info = container_of(sd, struct ov2710_info, sd);

	device_remove_file(&client->dev, &dev_attr_ov2710_rg_ratio_typical);
	device_remove_file(&client->dev, &dev_attr_ov2710_bg_ratio_typical);

	v4l2_device_unregister_subdev(sd);
	kfree(info);
	return 0;
}

static const struct i2c_device_id ov2710_id[] = {
	{ "ov2710", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov2710_id);

static struct i2c_driver ov2710_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ov2710",
	},
	.probe		= ov2710_probe,
	.remove		= ov2710_remove,
	.id_table	= ov2710_id,
};

static __init int init_ov2710(void)
{
	printk("init_ov2710 #########\n");
	return i2c_add_driver(&ov2710_driver);
}

static __exit void exit_ov2710(void)
{
	i2c_del_driver(&ov2710_driver);
}

module_init(init_ov2710);
module_exit(exit_ov2710);

MODULE_DESCRIPTION("A low-level driver for OmniVision ov2710 sensors");
MODULE_LICENSE("GPL");
