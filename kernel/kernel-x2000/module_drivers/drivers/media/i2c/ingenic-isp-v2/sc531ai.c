/*
 * A V4L2 driver for OmniVision SC531AI cameras.
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


#define SC531AI_CHIP_ID_H	(0x9e)
#define SC531AI_CHIP_ID_L	(0x39)
#define SC531AI_REG_END		0xff
#define SC531AI_REG_DELAY	0x00
#define SC531AI_PAGE_REG	0xfd


static bool debug;
module_param(debug, bool, 0644);
MODULE_PARM_DESC(debug, "Debug level (0-1)");

struct sc531ai_win_size {
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

struct sc531ai_gpio {
	int pin;
	int active_level;
};

struct sc531ai_info {
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
	struct sc531ai_win_size *win;

	struct sc531ai_gpio xshutdn;
	struct sc531ai_gpio pwdn;
	struct sc531ai_gpio efsync;
	struct sc531ai_gpio led;

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

struct again_lut sc531ai_again_lut[] = {
	{0x00,0x80,0},
	{0x00,0x82,1501},
	{0x00,0x84,2886},
	{0x00,0x86,4343},
	{0x00,0x88,5776},
	{0x00,0x8a,7101},
	{0x00,0x8c,8494},
	{0x00,0x8e,9782},
	{0x00,0x90,11136},
	{0x00,0x92,12471},
	{0x00,0x94,13706},
	{0x00,0x96,15006},
	{0x00,0x98,16288},
	{0x00,0x9a,17474},
	{0x00,0x9c,18724},
	{0x00,0x9e,19880},
	{0x00,0xa0,21098},
	{0x00,0xa2,22300},
	{0x00,0xa4,23414},
	{0x00,0xa6,24588},
	{0x00,0xa8,25747},
	{0x00,0xaa,26821},
	{0x00,0xac,27953},
	{0x00,0xae,29003},
	{0x00,0xb0,30109},
	{0x00,0xb2,31203},
	{0x00,0xb4,32217},
	{0x00,0xb6,33287},
	{0x00,0xb8,34345},
	{0x00,0xba,35326},
	{0x00,0xbc,36362},
	{0x00,0xbe,37322},
	{0x00,0xc0,38336},
	{0x00,0xc2,39339},
	{0x00,0xc4,40270},
	{0x00,0xc6,41253},
	{0x00,0xc8,42226},
	{0x00,0xca,43129},
	{0x00,0xcc,44083},
	{0x00,0xce,44968},
	{0x00,0xd0,45904},
	{0x00,0xd2,46830},
	{0x00,0xd4,47691},
	{0x00,0xd6,48600},
	{0x00,0xd8,49500},
	{0x00,0xda,50337},
	{0x00,0xdc,51221},
	{0x00,0xde,52042},
	{0x00,0xe0,52911},
	{0x00,0xe2,53771},
	{0x00,0xe4,54571},
	{0x00,0xe6,55417},
	{0x00,0xe8,56255},
	{0x00,0xea,57034},
	{0x00,0xec,57858},
	{0x00,0xee,58624},
	{0x00,0xf0,59434},
	{0x00,0xf2,60237},
	{0x00,0xf4,60984},
	{0x00,0xf6,61775},
	{0x00,0xf8,62559},
	{0x00,0xfa,63288},
	{0x00,0xfc,64059},
	{0x00,0xfe,64777},
	{0x01,0x80,65536},
	{0x01,0x82,66990},
	{0x01,0x84,68468},
	{0x01,0x86,69879},
	{0x01,0x88,71268},
	{0x01,0x8a,72637},
	{0x01,0x8c,74030},
	{0x01,0x8e,75360},
	{0x01,0x90,76672},
	{0x01,0x92,77966},
	{0x01,0x94,79283},
	{0x01,0x96,80542},
	{0x01,0x98,81784},
	{0x01,0x9a,83010},
	{0x01,0x9c,84260},
	{0x01,0x9e,85454},
	{0x01,0xa0,86634},
	{0x01,0xa2,87799},
	{0x40,0x80,88506},
	{0x40,0x82,90850},
	{0x40,0x84,93137},
	{0x40,0x86,95370},
	{0x40,0x88,97551},
	{0x40,0x8a,99684},
	{0x40,0x8c,101769},
	{0x40,0x8e,103809},
	{0x40,0x90,105806},
	{0x40,0x92,107762},
	{0x40,0x94,109678},
	{0x40,0x96,111556},
	{0x40,0x98,113398},
	{0x40,0x9a,115204},
	{0x40,0x9c,116977},
	{0x40,0x9e,118717},
	{0x40,0xa0,120425},
	{0x40,0xa2,122103},
	{0x40,0xa4,123752},
	{0x40,0xa6,125373},
	{0x40,0xa8,126966},
	{0x40,0xaa,128533},
	{0x40,0xac,130074},
	{0x40,0xae,131591},
	{0x40,0xb0,133083},
	{0x40,0xb2,134553},
	{0x40,0xb4,136000},
	{0x40,0xb6,137425},
	{0x40,0xb8,138829},
	{0x40,0xba,140212},
	{0x40,0xbc,141576},
	{0x40,0xbe,142920},
	{0x40,0xc0,144245},
	{0x40,0xc2,145552},
	{0x40,0xc4,146841},
	{0x40,0xc6,148113},
	{0x40,0xc8,149368},
	{0x40,0xca,150606},
	{0x40,0xcc,151829},
	{0x40,0xce,153036},
	{0x48,0x80,154042},
	{0x48,0x82,156386},
	{0x48,0x84,158673},
	{0x48,0x86,160906},
	{0x48,0x88,163087},
	{0x48,0x8a,165220},
	{0x48,0x8c,167305},
	{0x48,0x8e,169345},
	{0x48,0x90,171342},
	{0x48,0x92,173298},
	{0x48,0x94,175214},
	{0x48,0x96,177092},
	{0x48,0x98,178934},
	{0x48,0x9a,180740},
	{0x48,0x9c,182513},
	{0x48,0x9e,184253},
	{0x48,0xa0,185961},
	{0x48,0xa2,187639},
	{0x48,0xa4,189288},
	{0x48,0xa6,190909},
	{0x48,0xa8,192502},
	{0x48,0xaa,194069},
	{0x48,0xac,195610},
	{0x48,0xae,197127},
	{0x48,0xb0,198619},
	{0x48,0xb2,200089},
	{0x48,0xb4,201536},
	{0x48,0xb6,202961},
	{0x48,0xb8,204365},
	{0x48,0xba,205748},
	{0x48,0xbc,207112},
	{0x48,0xbe,208456},
	{0x48,0xc0,209781},
	{0x48,0xc2,211088},
	{0x48,0xc4,212377},
	{0x48,0xc6,213649},
	{0x48,0xc8,214904},
	{0x48,0xca,216142},
	{0x48,0xcc,217365},
	{0x48,0xce,218572},
	{0x49,0x80,219578},
	{0x49,0x82,221922},
	{0x49,0x84,224209},
	{0x49,0x86,226442},
	{0x49,0x88,228623},
	{0x49,0x8a,230756},
	{0x49,0x8c,232841},
	{0x49,0x8e,234881},
	{0x49,0x90,236878},
	{0x49,0x92,238834},
	{0x49,0x94,240750},
	{0x49,0x96,242628},
	{0x49,0x98,244470},
	{0x49,0x9a,246276},
	{0x49,0x9c,248049},
	{0x49,0x9e,249789},
	{0x49,0xa0,251497},
	{0x49,0xa2,253175},
	{0x49,0xa4,254824},
	{0x49,0xa6,256445},
	{0x49,0xa8,258038},
	{0x49,0xaa,259605},
	{0x49,0xac,261146},
	{0x49,0xae,262663},
	{0x49,0xb0,264155},
	{0x49,0xb2,265625},
	{0x49,0xb4,267072},
	{0x49,0xb6,268497},
	{0x49,0xb8,269901},
	{0x49,0xba,271284},
	{0x49,0xbc,272648},
	{0x49,0xbe,273992},
	{0x49,0xc0,275317},
	{0x49,0xc2,276624},
	{0x49,0xc4,277913},
	{0x49,0xc6,279185},
	{0x49,0xc8,280440},
	{0x49,0xca,281678},
	{0x49,0xcc,282901},
	{0x49,0xce,284108},
	{0x4b,0x80,285114},
	{0x4b,0x82,287458},
	{0x4b,0x84,289745},
	{0x4b,0x86,291978},
	{0x4b,0x88,294159},
	{0x4b,0x8a,296292},
	{0x4b,0x8c,298377},
	{0x4b,0x8e,300417},
	{0x4b,0x90,302414},
	{0x4b,0x92,304370},
	{0x4b,0x94,306286},
	{0x4b,0x96,308164},
	{0x4b,0x98,310006},
	{0x4b,0x9a,311812},
	{0x4b,0x9c,313585},
	{0x4b,0x9e,315325},
	{0x4b,0xa0,317033},
	{0x4b,0xa2,318711},
	{0x4b,0xa4,320360},
	{0x4b,0xa6,321981},
	{0x4b,0xa8,323574},
	{0x4b,0xaa,325141},
	{0x4b,0xac,326682},
	{0x4b,0xae,328199},
	{0x4b,0xb0,329691},
	{0x4b,0xb2,331161},
	{0x4b,0xb4,332608},
	{0x4b,0xb6,334033},
	{0x4b,0xb8,335437},
	{0x4b,0xba,336820},
	{0x4b,0xbc,338184},
	{0x4b,0xbe,339528},
	{0x4b,0xc0,340853},
	{0x4b,0xc2,342160},
	{0x4b,0xc4,343449},
	{0x4b,0xc6,344721},
	{0x4b,0xc8,345976},
	{0x4b,0xca,347214},
	{0x4b,0xcc,348437},
	{0x4b,0xce,349644},
	{0x4f,0x80,350650},
	{0x4f,0x82,352994},
	{0x4f,0x84,355281},
	{0x4f,0x86,357514},
	{0x4f,0x88,359695},
	{0x4f,0x8a,361828},
	{0x4f,0x8c,363913},
	{0x4f,0x8e,365953},
	{0x4f,0x90,367950},
	{0x4f,0x92,369906},
	{0x4f,0x94,371822},
	{0x4f,0x96,373700},
	{0x4f,0x98,375542},
	{0x4f,0x9a,377348},
	{0x4f,0x9c,379121},
	{0x4f,0x9e,380861},
	{0x4f,0xa0,382569},
	{0x4f,0xa2,384247},
	{0x4f,0xa4,385896},
	{0x4f,0xa6,387517},
	{0x4f,0xa8,389110},
	{0x4f,0xaa,390677},
	{0x4f,0xac,392218},
	{0x4f,0xae,393735},
	{0x4f,0xb0,395227},
	{0x4f,0xb2,396697},
	{0x4f,0xb4,398144},
	{0x4f,0xb6,399569},
	{0x4f,0xb8,400973},
	{0x4f,0xba,402356},
	{0x4f,0xbc,403720},
	{0x4f,0xbe,405064},
	{0x4f,0xc0,406389},
	{0x4f,0xc2,407696},
	{0x4f,0xc4,408985},
	{0x4f,0xc6,410257},
	{0x4f,0xc8,411512},
	{0x4f,0xca,412750},
	{0x4f,0xcc,413973},
	{0x4f,0xce,415180},
	{0x5f,0x80,416186},
};

static inline struct sc531ai_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sc531ai_info, sd);
}

static inline struct v4l2_subdev *to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct sc531ai_info, hdl)->sd;
}

struct regval_list {
	uint16_t reg_num;
	unsigned char value;
};

static struct regval_list sc531ai_init_regs_2880_1620_30fps_MIPI[] = {
	{0x0103,0x01},
	{0x0100,0x00},

	{0x36e9,0x80},
	{0x37f9,0x80},
	{0x301f,0x68},
	{0x3200,0x00},
	{0x3201,0x00},
	{0x3202,0x00},
	{0x3203,0x00},
	{0x3204,0x0b},
	{0x3205,0x4b},
	{0x3206,0x06},
	{0x3207,0x5b},
	{0x3208,0x0b},
	{0x3209,0x48},
	{0x320a,0x06},
	{0x320b,0x58},
	{0x3210,0x00},
	{0x3211,0x02},
	{0x3212,0x00},
	{0x3213,0x02},
	{0x3250,0x40},
	{0x3251,0x98},
	{0x3253,0x0c},
	{0x325f,0x20},
	{0x3301,0x08},
	{0x3304,0x50},
	{0x3306,0x88},
	{0x3308,0x14},
	{0x3309,0x70},
	{0x330a,0x00},
	{0x330b,0xf8},
	{0x330d,0x10},
	{0x330e,0x42},
	{0x331e,0x41},
	{0x331f,0x61},
	{0x3333,0x10},
	{0x335d,0x60},
	{0x335e,0x06},
	{0x335f,0x08},
	{0x3364,0x56},
	{0x3366,0x01},
	{0x337c,0x02},
	{0x337d,0x0a},
	{0x3390,0x01},
	{0x3391,0x03},
	{0x3392,0x07},
	{0x3393,0x08},
	{0x3394,0x08},
	{0x3395,0x08},
	{0x3396,0x40},
	{0x3397,0x48},
	{0x3398,0x4b},
	{0x3399,0x08},
	{0x339a,0x08},
	{0x339b,0x08},
	{0x339c,0x1d},
	{0x33a2,0x04},
	{0x33ae,0x30},
	{0x33af,0x50},
	{0x33b1,0x80},
	{0x33b2,0x48},
	{0x33b3,0x30},
	{0x349f,0x02},
	{0x34a6,0x48},
	{0x34a7,0x4b},
	{0x34a8,0x30},
	{0x34a9,0x18},
	{0x34f8,0x5f},
	{0x34f9,0x08},
	{0x3632,0x48},
	{0x3633,0x32},
	{0x3637,0x27},
	{0x3638,0xc1},
	{0x363b,0x20},
	{0x363d,0x02},
	{0x3670,0x09},
	{0x3674,0x8b},
	{0x3675,0xc6},
	{0x3676,0x8b},
	{0x367c,0x40},
	{0x367d,0x48},
	{0x3690,0x32},
	{0x3691,0x43},
	{0x3692,0x33},
	{0x3693,0x40},
	{0x3694,0x4b},
	{0x3698,0x85},
	{0x3699,0x8f},
	{0x369a,0xa0},
	{0x369b,0xc3},
	{0x36a2,0x49},
	{0x36a3,0x4b},
	{0x36a4,0x4f},
	{0x36d0,0x01},
	{0x36ea,0x0b},
	{0x36eb,0x04},
	{0x36ec,0x13},
	{0x36ed,0x34},
	{0x370f,0x01},
	{0x3722,0x00},
	{0x3728,0x10},
	{0x37b0,0x03},
	{0x37b1,0x03},
	{0x37b2,0x83},
	{0x37b3,0x48},
	{0x37b4,0x49},
	{0x37fa,0x0b},
	{0x37fb,0x24},
	{0x37fc,0x01},
	{0x37fd,0x34},
	{0x3901,0x00},
	{0x3902,0xc5},
	{0x3904,0x08},
	{0x3905,0x8c},
	{0x3909,0x00},
	{0x391d,0x04},
	{0x391f,0x44},
	{0x3926,0x21},
	{0x3929,0x18},
	{0x3933,0x82},
	{0x3934,0x0a},
	{0x3937,0x5f},
	{0x3939,0x00},
	{0x393a,0x00},
	{0x39dc,0x02},
	{0x3e01,0xcd},
	{0x3e02,0xa0},
	{0x440e,0x02},
	{0x4509,0x20},
	{0x4837,0x28},
	{0x5010,0x10},
	{0x5799,0x06},
	{0x57ad,0x00},
	{0x5ae0,0xfe},
	{0x5ae1,0x40},
	{0x5ae2,0x30},
	{0x5ae3,0x2a},
	{0x5ae4,0x24},
	{0x5ae5,0x30},
	{0x5ae6,0x2a},
	{0x5ae7,0x24},
	{0x5ae8,0x3c},
	{0x5ae9,0x30},
	{0x5aea,0x28},
	{0x5aeb,0x3c},
	{0x5aec,0x30},
	{0x5aed,0x28},
	{0x5aee,0xfe},
	{0x5aef,0x40},
	{0x5af4,0x30},
	{0x5af5,0x2a},
	{0x5af6,0x24},
	{0x5af7,0x30},
	{0x5af8,0x2a},
	{0x5af9,0x24},
	{0x5afa,0x3c},
	{0x5afb,0x30},
	{0x5afc,0x28},
	{0x5afd,0x3c},
	{0x5afe,0x30},
	{0x5aff,0x28},
	{0x36e9,0x20},
	{0x37f9,0x20},
//	{0x4501,0xbc},//test
//	{0x0100,0x01},

	{SC531AI_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sc531ai_stream_on[] = {
	{0x0100,0x01},
	{SC531AI_REG_END, 0x00},
};

static struct regval_list sc531ai_stream_off[] = {
	{0x0100,0x00},
	{SC531AI_REG_END, 0x00},
};


int sc531ai_read(struct v4l2_subdev *sd, unsigned short reg,
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

static int sc531ai_write(struct v4l2_subdev *sd, unsigned short reg,
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

static int sc531ai_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != SC531AI_REG_END) {
		if (vals->reg_num == SC531AI_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = sc531ai_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
			if (vals->reg_num == SC531AI_PAGE_REG){
				val &= 0xf8;
				val |= (vals->value & 0x07);
				ret = sc531ai_write(sd, vals->reg_num, val);
				ret = sc531ai_read(sd, vals->reg_num, &val);
			}
		}
		vals++;
	}
	return 0;
}

static int sc531ai_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SC531AI_REG_END) {
		if (vals->reg_num == SC531AI_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = sc531ai_write(sd, vals->reg_num, vals->value);
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
static int sc531ai_xshutdn(struct v4l2_subdev *sd, u32 val)
{
	struct sc531ai_info *info = to_state(sd);

	if(val) {
		gpio_direction_output(info->xshutdn.pin, info->xshutdn.active_level);
	} else {
		gpio_direction_output(info->xshutdn.pin, !info->xshutdn.active_level);
	}
	return 0;
}

static int sc531ai_pwdn(struct v4l2_subdev *sd, u32 val)
{
	struct sc531ai_info *info = to_state(sd);

	if(val) {
		gpio_direction_output(info->pwdn.pin, info->pwdn.active_level);
		msleep(10);
	} else {
		gpio_direction_output(info->pwdn.pin, !info->pwdn.active_level);
	}
	return 0;
}

static int sc531ai_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;

	ret = sc531ai_read(sd, 0x3107, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SC531AI_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = sc531ai_read(sd, 0x3108, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SC531AI_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static struct sc531ai_win_size sc531ai_win_sizes[] = {
	{
		.width				= 2888,
		.height				= 1624,
		.sensor_info.fps				= 30 << 16 | 1,
		.sensor_info.total_width			= 0xb48,
		.sensor_info.total_height			= 0x658,
		.sensor_info.wdr_en				= 0,
		.sensor_info.mipi_cfg.clk			= 396,
		.sensor_info.mipi_cfg.twidth		= 2888,
		.sensor_info.mipi_cfg.theight		= 1624,
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
		.regs 		= sc531ai_init_regs_2880_1620_30fps_MIPI,
	},
};

#if 0
static int sc531ai_enum_mbus_code(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad || code->index >= N_OV2735a_FMTS)
		return -EINVAL;

	code->code = sc531ai_formats[code->index].mbus_code;
	return 0;
}
#endif

/*
 * Set a format.
 */
static int sc531ai_set_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *format)
{
	if (format->pad)
		return -EINVAL;

	return 0;
}

static int sc531ai_get_fmt(struct v4l2_subdev *sd,
		       struct v4l2_subdev_pad_config *cfg,
		       struct v4l2_subdev_format *format)
{
	struct sc531ai_info *info = to_state(sd);
	struct sc531ai_win_size *wsize = info->win;
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

static int sc531ai_s_wdr(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	struct sc531ai_info *info = to_state(sd);
	if(value)
		info->win = &sc531ai_win_sizes[0];
	else
		info->win = &sc531ai_win_sizes[0];
	return ret;
}

static int sc531ai_s_brightness(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int sc531ai_s_contrast(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int sc531ai_s_hflip(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int sc531ai_s_vflip(struct v4l2_subdev *sd, int value)
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
static int sc531ai_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	int ret = 0;
	unsigned char gain = 0;

	*value = gain;
	return ret;
}

static int sc531ai_s_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

//	printk("---%s, %d, s_gain: value: %d\n", __func__, __LINE__, value);

	return ret;
}

static unsigned int again_to_regval(int gain, unsigned int *value, unsigned int *fine_value)
{
	struct again_lut *lut = NULL;
	int i = 0;
	for(i = 0; i < ARRAY_SIZE(sc531ai_again_lut); i++) {
		lut = &sc531ai_again_lut[i];

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
	for(i = 0; i < ARRAY_SIZE(sc531ai_again_lut); i++) {
		lut = &sc531ai_again_lut[i];

		if(regval == lut->value && fine_reg_val == lut->fine_value) {
			return lut->gain;
		}
	}
	printk("%s, %d regval not mapped to isp gain\n", __func__, __LINE__);
	return -EINVAL;
}

static int sc531ai_g_again(struct v4l2_subdev *sd, __s32 *value)
{
	char v = 0;
	unsigned int reg_val = 0;
	unsigned int fine_reg_val = 0;
	int ret = 0;

	ret = sc531ai_read(sd, 0x3e09, &v);
	reg_val = v ;
	ret = sc531ai_read(sd, 0x3e07, &v);
	fine_reg_val = v ;

	*value = regval_to_again(reg_val, fine_reg_val);

	return ret;

}
#if 0
static int sc531ai_g_again_short(struct v4l2_subdev *sd, __s32 *value)
{
	char v = 0;
	unsigned int reg_val = 0;
	unsigned int fine_reg_val = 0;
	int ret = 0;


	ret = sc531ai_read(sd, 0x3e13, &v);
	reg_val = v ;
	ret = sc531ai_read(sd, 0x3e11, &v);
	fine_reg_val = v ;

	*value = regval_to_again(reg_val, fine_reg_val);

	return ret;

}
#endif
/*set analog gain db value, map value to sensor register.*/
static int sc531ai_s_again(struct v4l2_subdev *sd, int value)
{
	struct sc531ai_info *info = to_state(sd);
	unsigned int reg_value;
	unsigned int reg_fine_value;
	int ret = 0;

	if(value < info->again->minimum || value > info->again->maximum) {
		/* use default value. */
		again_to_regval(info->again->default_value, &reg_value, &reg_fine_value);
	} else {
		again_to_regval(value, &reg_value, &reg_fine_value);
	}

	ret += sc531ai_write(sd, 0x3e09, (unsigned char)(reg_value));
	ret += sc531ai_write(sd, 0x3e07, (unsigned char)(reg_fine_value));
	if (ret < 0){
		printk("sc531ai_write error  %d\n" ,__LINE__ );
		return ret;
	}

	return 0;
}
#if 0
static int sc531ai_s_again_short(struct v4l2_subdev *sd, int value)
{
	struct sc531ai_info *info = to_state(sd);
	unsigned int reg_value;
	unsigned int reg_fine_value;
	int ret = 0;

	if(value < info->again->minimum || value > info->again->maximum) {
		/* use default value. */
		again_to_regval(info->again->default_value, &reg_value, &reg_fine_value);
	} else {
		again_to_regval(value, &reg_value, &reg_fine_value);
	}

	ret += sc531ai_write(sd, 0x3e13, (unsigned char)(reg_value));
	ret += sc531ai_write(sd, 0x3e11, (unsigned char)(reg_fine_value));
	if (ret < 0){
		printk("sc531ai_write error  %d\n" ,__LINE__ );
		return ret;
	}

	return 0;
}
#endif
/*
 * Tweak autogain.
 */
static int sc531ai_s_autogain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	return ret;
}

static int sc531ai_s_exp(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	value *= 2; /*unit in half line*/
	ret += sc531ai_write(sd, 0x3e00, (unsigned char)((value >> 12) & 0xf));
	ret += sc531ai_write(sd, 0x3e01, (unsigned char)((value >> 4) & 0xff));
	ret += sc531ai_write(sd, 0x3e02, (unsigned char)(value & 0xf) << 4);

	if (ret < 0) {
		printk("sc531ai_write error  %d\n" ,__LINE__);
		return ret;
	}

	return 0;
}
#if 0
static int sc531ai_s_exp_short(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	value *= 2; /*unit in half line*/
	ret += sc531ai_write(sd, 0x3e22, (unsigned char)((value >> 12) & 0xf));
	ret += sc531ai_write(sd, 0x3e04, (unsigned char)((value >> 4) & 0xff));
	ret += sc531ai_write(sd, 0x3e05, (unsigned char)(value & 0xf) << 4);

	if (ret < 0) {
		printk("sc531ai_write error  %d\n" ,__LINE__);
		return ret;
	}

	return 0;
}
#endif

static int sc531ai_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
	struct sc531ai_info *info = to_state(sd);

	switch (ctrl->id) {
	case V4L2_CID_AUTOGAIN:
		return sc531ai_g_gain(sd, &info->gain->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return sc531ai_g_again(sd, &info->again->val);
	//case V4L2_CID_USER_ANALOG_GAIN_SHORT:
	//	return sc531ai_g_again_short(sd, &info->again_short->val);
	}
	return -EINVAL;
}

static int sc531ai_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
	struct sc531ai_info *info = to_state(sd);

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sc531ai_s_brightness(sd, ctrl->val);
	case V4L2_CID_CONTRAST:
		return sc531ai_s_contrast(sd, ctrl->val);
	case V4L2_CID_VFLIP:
		return sc531ai_s_vflip(sd, ctrl->val);
	case V4L2_CID_HFLIP:
		return sc531ai_s_hflip(sd, ctrl->val);
	case V4L2_CID_AUTOGAIN:
	      /* Only set manual gain if auto gain is not explicitly
	         turned on. */
		if (!ctrl->val) {
	      	/* sc531ai_s_gain turns off auto gain */
			return sc531ai_s_gain(sd, info->gain->val);
		}
			return sc531ai_s_autogain(sd, ctrl->val);
	case V4L2_CID_GAIN:
		return sc531ai_s_gain(sd, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return sc531ai_s_again(sd, ctrl->val);
	//case V4L2_CID_USER_ANALOG_GAIN_SHORT:
	//	return sc531ai_s_again_short(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sc531ai_s_exp(sd, ctrl->val);
	//case V4L2_CID_USER_EXPOSURE_SHORT:
	//	return sc531ai_s_exp_short(sd, ctrl->val);
	case V4L2_CID_WIDE_DYNAMIC_RANGE:
		return sc531ai_s_wdr(sd, ctrl->val);
	}
	return -EINVAL;
}

static const struct v4l2_ctrl_ops sc531ai_ctrl_ops = {
	.s_ctrl = sc531ai_s_ctrl,
	.g_volatile_ctrl = sc531ai_g_volatile_ctrl,
};


#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sc531ai_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	unsigned char val = 0;
	int ret;

	ret = sc531ai_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 1;
	return ret;
}

static int sc531ai_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	sc531ai_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static int sc531ai_core_reset(struct v4l2_subdev *sd, u32 val)
{
	struct sc531ai_info *info = to_state(sd);
	unsigned char v;
	int ret;

	/*software reset*/
	if(info->init_flag)
		return 0;

	ret = sc531ai_read(sd, 0x0103, &v);

	if(val) {
		v |= 1;
		ret += sc531ai_write(sd, 0x0103, v);
	}
	return 0;
}

static int sc531ai_core_init(struct v4l2_subdev *sd, u32 val)
{
	struct sc531ai_info *info = to_state(sd);
	int ret = 0;

	if(!info->init_flag) {
		ret = sc531ai_write_array(sd, info->win->regs);
	}

	return ret;
}

int sc531ai_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = sc531ai_write_array(sd, sc531ai_stream_on);
		printk("sc531ai stream on\n");

	}
	else {
		ret = sc531ai_write_array(sd, sc531ai_stream_off);
		printk("sc531ai stream off\n");
	}

	return ret;
}

int sc531ai_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct sc531ai_info *info = to_state(sd);
	if(info->win->sensor_info.fps){
		param->parm.capture.timeperframe.numerator = info->win->sensor_info.fps & 0xffff;
		param->parm.capture.timeperframe.denominator = info->win->sensor_info.fps >> 16;
		return 0;
	}
	return -EINVAL;
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sc531ai_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = sc531ai_g_register,
	.s_register = sc531ai_s_register,
#endif
	.g_ext_ctrls = v4l2_subdev_g_ext_ctrls,
	.try_ext_ctrls = v4l2_subdev_try_ext_ctrls,
	.s_ext_ctrls = v4l2_subdev_s_ext_ctrls,
	.g_ctrl = v4l2_subdev_g_ctrl,
	.s_ctrl = v4l2_subdev_s_ctrl,
	.queryctrl = v4l2_subdev_queryctrl,
	.querymenu = v4l2_subdev_querymenu,
	.init = sc531ai_core_init,
	.reset = sc531ai_core_reset,

};

static const struct v4l2_subdev_video_ops sc531ai_video_ops = {
	.s_stream = sc531ai_s_stream,
	.g_parm	= sc531ai_g_parm,
};

static const struct v4l2_subdev_pad_ops sc531ai_pad_ops = {
	//.enum_mbus_code = sc531ai_enum_mbus_code,
	.set_fmt = sc531ai_set_fmt,
	.get_fmt = sc531ai_get_fmt,
};

static const struct v4l2_subdev_ops sc531ai_ops = {
	.core = &sc531ai_core_ops,
	.video = &sc531ai_video_ops,
	.pad = &sc531ai_pad_ops,
};

/* ----------------------------------------------------------------------- */

#ifdef CONFIG_X2500_RISCV
extern int register_riscv_notifier(struct notifier_block *nb);
int sc531ai_riscv_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
	struct sc531ai_info *info = container_of(nb, struct sc531ai_info, nb);
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


static int sc531ai_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sc531ai_info *info;
	int ret;
	unsigned int ident = 0;
	int gpio = -EINVAL;
	unsigned int flags;
	int mclk_index = -1;
	char id_div[9];
	//struct v4l2_ctrl_config cfg = {0};

	dev_info(&client->dev, "sc531ai Probe !!!\n");

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


	v4l2_i2c_subdev_init(sd, client, &sc531ai_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	sc531ai_xshutdn(sd, 1);
	sc531ai_pwdn(sd, 1);
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

	/* Make sure it's an sc531ai */
	ret = sc531ai_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an sc531ai chip.\n",
			client->addr << 1, client->adapter->name);
		return ret;
	}

	v4l_info(client, "chip found @ 0x%02x (%s)\n",
			client->addr << 1, client->adapter->name);

	v4l2_ctrl_handler_init(&info->hdl, 8);
	v4l2_ctrl_new_std(&info->hdl, &sc531ai_ctrl_ops,
			V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&info->hdl, &sc531ai_ctrl_ops,
			V4L2_CID_CONTRAST, 0, 127, 1, 64);
	v4l2_ctrl_new_std(&info->hdl, &sc531ai_ctrl_ops,
			V4L2_CID_VFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&info->hdl, &sc531ai_ctrl_ops,
			V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&info->hdl, &sc531ai_ctrl_ops,
			V4L2_CID_WIDE_DYNAMIC_RANGE, 0, 1, 1, 0);
	info->gain = v4l2_ctrl_new_std(&info->hdl, &sc531ai_ctrl_ops,
			V4L2_CID_GAIN, 0, 255, 1, 128);
	info->again = v4l2_ctrl_new_std(&info->hdl, &sc531ai_ctrl_ops,
			V4L2_CID_ANALOGUE_GAIN, 0, 416186, 1, 10000);

	/*unit exposure lines: */
	info->exposure = v4l2_ctrl_new_std(&info->hdl, &sc531ai_ctrl_ops,
			V4L2_CID_EXPOSURE, 3, 3290 , 1, 600);

	/*
	cfg.ops = &sc531ai_ctrl_ops;
	cfg.id = V4L2_CID_USER_EXPOSURE_SHORT;
	cfg.name = "expo short";
	cfg.type = V4L2_CTRL_TYPE_INTEGER;
	cfg.min = 1;
	cfg.max = 131;
	cfg.step = 4;
	cfg.def = 50;
	info->exposure_short = v4l2_ctrl_new_custom(&info->hdl, &cfg, NULL);

	memset(&cfg, 0, sizeof(cfg));
	cfg.ops = &sc531ai_ctrl_ops;
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

	info->win = &sc531ai_win_sizes[0];
	sc531ai_write_array(sd, info->win->regs);

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
	info->nb.notifier_call = sc531ai_riscv_notifier;
	register_riscv_notifier(&info->nb);
#endif

	dev_info(&client->dev, "sc531ai Probed\n");
	return 0;
err_videoprobe:
err_entity_init:
	v4l2_clk_put(info->clk);
err_clkget:
	return ret;
}


static int sc531ai_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sc531ai_info *info = to_state(sd);

	v4l2_device_unregister_subdev(sd);
	v4l2_ctrl_handler_free(&info->hdl);
	v4l2_clk_put(info->clk);
	return 0;
}

static const struct i2c_device_id sc531ai_id[] = {
	{ "sc531ai", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, sc531ai_id);

static const struct of_device_id sc531ai_of_match[] = {
	{.compatible = "smartsens,sc531ai", },
	{},
};
MODULE_DEVICE_TABLE(of, sc531ai_of_match);


static struct i2c_driver sc531ai_driver = {
	.driver = {
		.name	= "sc531ai",
		.of_match_table = of_match_ptr(sc531ai_of_match),
	},
	.probe		= sc531ai_probe,
	.remove		= sc531ai_remove,
	.id_table	= sc531ai_id,
};

module_i2c_driver(sc531ai_driver);
MODULE_AUTHOR("qpz <aric.pzqi@ingenic.com>");
MODULE_DESCRIPTION("A low-level driver for SmartSens sc531ai sensors");
MODULE_LICENSE("GPL");
