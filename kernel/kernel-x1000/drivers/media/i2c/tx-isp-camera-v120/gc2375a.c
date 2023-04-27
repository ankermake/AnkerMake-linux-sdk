/*
 * gc2375a Camera Driver
 *
 * Copyright (C) 2018, Ingenic Semiconductor Inc.
 *
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
#include <linux/proc_fs.h>
#include <soc/gpio.h>

#include <linux/sensor_board.h>
#include <tx-isp/sensor-common.h>
#include <tx-isp/apical-isp/apical_math.h>
#include <linux/delay.h>


#define GC2375A_CHIP_ID_H               (0x23)
#define GC2375A_CHIP_ID_L               (0xa5)
#define GC2375A_FLAG_END                0xff
#define GC2375A_FLAG_DELAY              0x00
#define GC2375A_PAGE_REG                0xfe

#define GC2375A_SUPPORT_30FPS_SCLK      (78000000)
#define GC2375A_SUPPORT_15FPS_SCLK      (38000000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5
#define SENSOR_VERSION                  "H20190225"
#define GC2375A_WIDTH                   1600
#define GC2375A_HEIGHT                  1200


static int power_gpio = -1;
module_param(power_gpio, int, S_IRUGO);
MODULE_PARM_DESC(power_gpio, "Power on GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int reset_gpio = -1;
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int cam_en = -1;
module_param(cam_en, int, S_IRUGO);
MODULE_PARM_DESC(cam_en, "En Cam1");

struct regval_list {
    unsigned char reg_num;
    unsigned char value;
};


struct tx_isp_sensor_attribute gc2375a_attr;
static struct sensor_board_info *gc2375a_board_info;


/*
#define ANALOG_GAIN_1 64   // 1.00x
#define ANALOG_GAIN_2 92   // 1.43x
#define ANALOG_GAIN_3 128  // 2.00x
#define ANALOG_GAIN_4 182  // 2.84x
#define ANALOG_GAIN_5 254  // 3.97x
#define ANALOG_GAIN_6 363  // 5.68x
#define ANALOG_GAIN_7 521  // 8.14x
#define ANALOG_GAIN_8 725  // 11.34x
#define ANALOG_GAIN_9 1038 // 16.23x
*/
const unsigned int  ANALOG_GAIN_1 = (1<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.0*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_2 = (1<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.43*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_3 = (2<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.0*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_4 = (2<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.84*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_5 = (3<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.97*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_6 = (5<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.68*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_7 = (8<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.14*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_8 =(11<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.34*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_9 =(16<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.23*(1<<TX_ISP_GAIN_FIXED_POINT)));

unsigned int fix_point_mult2(unsigned int a, unsigned int b)
{
    unsigned int x1,x2,x;
    unsigned int a1,a2,b1,b2;
    unsigned int mask = (((unsigned int)0xffffffff)>>(32-TX_ISP_GAIN_FIXED_POINT));

    a1 = a>>TX_ISP_GAIN_FIXED_POINT;
    a2 = a&mask;
    b1 = b>>TX_ISP_GAIN_FIXED_POINT;
    b2 = b&mask;
    x1 = a1*b1;
    x1 += (a1*b2)>>TX_ISP_GAIN_FIXED_POINT;
    x1 += (a2*b1)>>TX_ISP_GAIN_FIXED_POINT;
    x2 = (a1*b2)&mask;
    x2 += (a2*b1)&mask;
    x2 += (a2*b2)>>TX_ISP_GAIN_FIXED_POINT;
    x = (x1<<TX_ISP_GAIN_FIXED_POINT)+x2;

    return x;
}

unsigned int fix_point_mult3(unsigned int a, unsigned int b, unsigned int c)
{
    unsigned int x = 0;
    x = fix_point_mult2(a,b);
    x = fix_point_mult2(x,c);
    return x;
}

#define ANALOG_GAIN_MAX     (fix_point_mult2(ANALOG_GAIN_9, (0xf<<TX_ISP_GAIN_FIXED_POINT) + (0x3f<<(TX_ISP_GAIN_FIXED_POINT-6))))
unsigned int gc2375a_gainone_to_reg(unsigned int gain_one, unsigned int *regs)
{
    unsigned int gain_one1 = 0;
    unsigned int gain_tmp = 0;
    unsigned char regb6 = 0;
    unsigned char regb1 =0x1;
    unsigned char regb2 = 0;
    int i,j;
    unsigned int gain_one_max = fix_point_mult2(ANALOG_GAIN_9, (0xf<<TX_ISP_GAIN_FIXED_POINT) + (0x3f<<(TX_ISP_GAIN_FIXED_POINT-6)));

    if (gain_one < ANALOG_GAIN_1) {
        gain_one1 = ANALOG_GAIN_1;
        regb6 = 0x00;
        regb1 = 0x01;
        regb2 = 0x00;
        goto done;
    } else if (gain_one < (ANALOG_GAIN_2)) {
        gain_one1 = gain_tmp = ANALOG_GAIN_1;
        regb1 = 0;
        regb2 = 0;
        for (i = 1; i <= 0xf; i++ )
            for (j = 0; j <= 0x3f; j++) {
                gain_tmp = fix_point_mult2(ANALOG_GAIN_1, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
                if (gain_one < gain_tmp) {
                    goto done;
                }
                gain_one1 = gain_tmp;
                regb6 = 0x00;
                regb1 = i;
                regb2 = j;
            }
    } else if (gain_one < ANALOG_GAIN_3) {
        gain_one1 = gain_tmp = ANALOG_GAIN_2;
        regb1 = 0;
        regb2 = 0;
        for (i = 1; i <= 0xf; i++ )
            for (j = 0; j <= 0x3f; j++) {
                gain_tmp = fix_point_mult2(ANALOG_GAIN_2, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
                if (gain_one < gain_tmp) {
                    goto done;
                }
                gain_one1 = gain_tmp;
                regb6 = 0x01;
                regb1 = i;
                regb2 = j;
            }
    } else if (gain_one < ANALOG_GAIN_4) {
        gain_one1 = gain_tmp = ANALOG_GAIN_3;
        regb1 = 0;
        regb2 = 0;
        for (i = 1; i <= 0xf; i++ )
            for (j = 0; j <= 0x3f; j++) {
                gain_tmp = fix_point_mult2(ANALOG_GAIN_3, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
                if (gain_one < gain_tmp) {
                    goto done;
                }
                gain_one1 = gain_tmp;
                regb6 = 0x02;
                regb1 = i;
                regb2 = j;
            }
    } else if (gain_one < ANALOG_GAIN_5) {
        gain_one1 = gain_tmp = ANALOG_GAIN_4;
        regb1 = 0;
        regb2 = 0;
        for (i = 1; i <= 0xf; i++ )
            for (j = 0; j <= 0x3f; j++) {
                gain_tmp = fix_point_mult2(ANALOG_GAIN_4, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
                if (gain_one < gain_tmp) {
                    goto done;
                }
                gain_one1 = gain_tmp;
                regb6 = 0x03;
                regb1 = i;
                regb2 = j;
            }
    } else if (gain_one < ANALOG_GAIN_6) {
        gain_one1 = gain_tmp = ANALOG_GAIN_5;
        regb1 = 0;
        regb2 = 0;
        for (i = 1; i <= 0xf; i++ )
            for (j = 0; j <= 0x3f; j++) {
                gain_tmp = fix_point_mult2(ANALOG_GAIN_5, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
                if (gain_one < gain_tmp) {
                    goto done;
                }
                gain_one1 = gain_tmp;
                regb6 = 0x04;
                regb1 = i;
                regb2 = j;
            }
    } else if (gain_one < ANALOG_GAIN_7) {
        gain_one1 = gain_tmp = ANALOG_GAIN_6;
        regb1 = 0;
        regb2 = 0;
        for (i = 1; i <= 0xf; i++ )
            for (j = 0; j <= 0x3f; j++) {
                gain_tmp = fix_point_mult2(ANALOG_GAIN_6, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
                if (gain_one < gain_tmp) {
                    goto done;
                }
                gain_one1 = gain_tmp;
                regb6 = 0x05;
                regb1 = i;
                regb2 = j;
            }
    } else if (gain_one < ANALOG_GAIN_8) {
        gain_one1 = gain_tmp = ANALOG_GAIN_7;
        regb1 = 0;
        regb2 = 0;
        for (i = 1; i <= 0xf; i++ )
            for (j = 0; j <= 0x3f; j++) {
                gain_tmp = fix_point_mult2(ANALOG_GAIN_7, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
                if (gain_one < gain_tmp) {
                    goto done;
                }
                gain_one1 = gain_tmp;
                regb6 = 0x06;
                regb1 = i;
                regb2 = j;
            }
    } else if (gain_one < ANALOG_GAIN_9) {
        gain_one1 = gain_tmp = ANALOG_GAIN_8;
        regb1 = 0;
        regb2 = 0;
        for (i = 1; i <= 0xf; i++ )
            for (j = 0; j <= 0x3f; j++) {
                gain_tmp = fix_point_mult2(ANALOG_GAIN_8, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
                if (gain_one < gain_tmp) {
                    goto done;
                }
                gain_one1 = gain_tmp;
                regb6 = 0x07;
                regb1 = i;
                regb2 = j;
            }
    } else if (gain_one < gain_one_max) {
        gain_one1 = gain_tmp = ANALOG_GAIN_9;
        regb1 = 0;
        regb2 = 0;
        for (i = 1; i <= 0xf; i++ )
            for (j = 0; j <= 0x3f; j++) {
                gain_tmp = fix_point_mult2(ANALOG_GAIN_9, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
                if (gain_one < gain_tmp) {
                    goto done;
                }
                gain_one1 = gain_tmp;
                regb6 = 0x08;
                regb1 = i;
                regb2 = j;
            }
    } else {
        gain_one1 = gain_one_max;
        regb6 = 0x08;
        regb1 = 0xf;
        regb2 = 0x3f;
        goto done;
    }
    gain_one1 = ANALOG_GAIN_1;

done:
    *regs = (regb6<<12)|(regb1<<8)|(regb2);
    return gain_one1;
}

unsigned int gc2375a_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    unsigned int gain_one = 0;
    unsigned int gain_one1 = 0;
    unsigned int regs = 0;
    unsigned int isp_gain1 = 0;

    gain_one = math_exp2(isp_gain, shift, TX_ISP_GAIN_FIXED_POINT);
    gain_one1 = gc2375a_gainone_to_reg(gain_one, &regs);
    isp_gain1 = log2_fixed_to_fixed(gain_one1, TX_ISP_GAIN_FIXED_POINT, shift);
    *sensor_again = regs;

    return isp_gain1;
}

unsigned int gc2375a_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}


struct tx_isp_sensor_attribute gc2375a_attr={
    .name                               = "gc2375a",
    .chip_id                            = 0x23a5,
    .cbus_type                          = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask                          = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
    .cbus_device                        = 0x37,
    .dbus_type                          = TX_SENSOR_DATA_INTERFACE_MIPI,
    .mipi = {
        .clk    = 624,
        .lans   = 1,
    },
    .max_again                          = 263489,
    .max_dgain                          = 0,
    .min_integration_time               = 2,
    .min_integration_time_native        = 2,
    .max_integration_time_native        = 1240 - 4,
    .integration_time_limit             = 1240 - 4,
    .total_width                        = 2080,
    .total_height                       = 1240,
    .max_integration_time               = 1240 - 4,
    .integration_time_apply_delay       = 2,
    .again_apply_delay                  = 2,
    .dgain_apply_delay                  = 0,
    .sensor_ctrl.alloc_again            = gc2375a_alloc_again,
    .sensor_ctrl.alloc_dgain            = gc2375a_alloc_dgain,
    //    void priv; /* point to struct tx_isp_sensor_board_info */
};

/*
 * Actual_window_size=1600*1200,MIPI 1lane
 * MCLK=24Mhz,MIPI_clcok=624MHz,row_time=26.66us
 * Pixel_line=2080 line_frame=1240,frame_rate=30fps
 */
static struct regval_list gc2375a_init_regs_1600_1200_30fps_mipi[] = {
    { 0xfe,0xF0},
    { 0xfe,0xF0},
    { 0xfe,0x00},
    { 0xfe,0x00},
    { 0xfe,0x00},
    { 0xf7,0x01},
    { 0xf8,0x0c},
    { 0xf9,0x42},
    { 0xfa,0x88},
    { 0xfc,0x8e},
    { 0xfe,0x00},
    { 0x88,0x03},

    /*Crop*/
    { 0x90,0x01}, //crop mode en
    { 0x91,0x00}, //win_y_h :8
    { 0x92,0x08}, //win_y_l
    { 0x93,((1616-GC2375A_WIDTH)/2)/256}, //win_x_h :8
    { 0x94,((1616-GC2375A_WIDTH)/2)%256}, //win_x_l
    { 0x95,GC2375A_HEIGHT/256}, //win_height_h :1200
    { 0x96,GC2375A_HEIGHT%256}, //win_height_l
    { 0x97,GC2375A_WIDTH/256}, //win_width_h  :1600
    { 0x98,GC2375A_WIDTH%256}, //win_width_l

    /*Analog*/
    { 0x03,0x04}, //exp_in_h :1125
    { 0x04,0x65}, //exp_in_l
    { 0x05,0x02}, //HB_h :613
    { 0x06,0x65}, //HB_l
    { 0x07,0x00}, //VB_h :16
    { 0x08,0x10}, //VB_l
    { 0x09,0x00}, //row_start_h :0
    { 0x0a,0x00}, //row_start_l
    { 0x0b,0x00}, //col_start_h :20
    { 0x0c,0x14}, //col_start_l
    { 0x0d,0x04}, //win_height_h :1216
    { 0x0e,0xc0}, //win_height_l
    { 0x0f,0x06}, //win_width_h :1616
    { 0x10,0x50}, //win_width_l
    { 0x17,0xc0}, //mirror & flip
    { 0x19,0x17},
    { 0x1c,0x10},
    { 0x1d,0x13},

    { 0x20,0x0d},
    { 0x21,0x6d},
    { 0x22,0x0d},
    { 0x25,0xc1},
    { 0x26,0x0d},
    { 0x27,0x22},
    { 0x29,0x5f},
    { 0x2b,0x88},
    { 0x2f,0x12},

    { 0x38,0x86},
    { 0x3d,0x00},
    { 0xcd,0xa3},
    { 0xce,0x57},
    { 0xd0,0x09},
    { 0xd1,0xca},
    { 0xd2,0x74},
    { 0xd3,0xbb},
    { 0xd8,0x60},
    { 0xe0,0x08},
    { 0xe1,0x1f},
    { 0xe4,0xf8},
    { 0xe5,0x0c},
    { 0xe6,0x10},
    { 0xe7,0xcc},
    { 0xe8,0x02},
    { 0xe9,0x01},
    { 0xea,0x02},
    { 0xeb,0x01},

    /*BLK*/
    { 0x18,0x02},
    { 0x1a,0x18},
    { 0x28,0x00},
    { 0x3f,0x40},
    { 0x40,0x26},
    { 0x41,0x00},
    { 0x43,0x03},
    { 0x4a,0x00},
    { 0x4e,0x3c},
    { 0x4f,0x00},
    { 0x60,0x00},
    { 0x61,0x80},
    { 0x66,0xc0},
    { 0x67,0x00},
    { 0xfe,0x01},
    { 0x41,0x00},
    { 0x42,0x00},
    { 0x43,0x00},
    { 0x44,0x00},

    /*Dark sun*/
    { 0xfe,0x00},
    { 0x68,0x00},

    /*Gain*/
    { 0xb0,0x58}, //global gain :88
    { 0xb1,0x01}, //auto_pregain_h[3:0] :64
    { 0xb2,0x00}, //auto_pregain_l[7:2]
    { 0xb6,0x00}, //gain code[3:0]

    /*MIPI*/
    { 0xfe,0x03},
    { 0x01,0x03}, //phy lane0 en, phy clk en
    { 0x02,0x33},
    { 0x03,0x90},
    { 0x04,0x04},
    { 0x05,0x00},
    { 0x06,0x80},
    { 0x11,0x2b}, //RAW10 ouput
    { 0x12,(GC2375A_WIDTH*5/4)%256}, //LWC_set_l :RAW10:win_width*5/4:2000
    { 0x13,(GC2375A_WIDTH*5/4)/256}, //LWC_set_h
    { 0x15,0x00},
    { 0x42,GC2375A_WIDTH%256}, //buf_win_width_l
    { 0x43,GC2375A_WIDTH/256}, //buf_win_width_h

    { 0x21,0x08},
    { 0x22,0x05},
    { 0x23,0x13},
    { 0x24,0x02},
    { 0x25,0x13},
    { 0x26,0x08},
    { 0x29,0x06},
    { 0x2a,0x08},
    { 0x2b,0x08},

    { 0xfe,0x00},
    //{ 0xef,0x90}, //stream on
    {GC2375A_FLAG_END,0x0},    /* END MARKER */
};


/*
 * the order of the gc2375a_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting gc2375a_win_sizes[] = {
    {
        .width          = GC2375A_WIDTH,
        .height         = GC2375A_HEIGHT,
        .fps            = 30 << 16 | 1,
        .mbus_code      = V4L2_MBUS_FMT_SRGGB10_1X10,
        .colorspace     = V4L2_COLORSPACE_SRGB,
        .regs           = gc2375a_init_regs_1600_1200_30fps_mipi,
    }
};

/*
 * the part of driver was fixed.
 */

static struct regval_list gc2375a_stream_on_mipi[] = {
    { 0xfe,0x00},
    { 0xef,0x90},
    {GC2375A_FLAG_END,0x0},    /* END MARKER */
};

static struct regval_list gc2375a_stream_off_mipi[] = {
    { 0xfe,0x00},
    { 0xef,0x00},
    {GC2375A_FLAG_END,0x0},    /* END MARKER */
};

int gc2375a_read(struct v4l2_subdev *sd, unsigned char reg, unsigned char *value)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct i2c_msg msg[2] = {
        [0] = {
            .addr       = client->addr,
            .flags      = 0,
            .len        = 1,
            .buf        = &reg,
        },
        [1] = {
            .addr       = client->addr,
            .flags      = I2C_M_RD,
            .len        = 1,
            .buf        = value,
        }
    };

    int ret;
    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

    return ret;
}

int gc2375a_write(struct v4l2_subdev *sd, unsigned char reg, unsigned char value)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr       = client->addr,
        .flags      = 0,
        .len        = 2,
        .buf        = buf,
    };

    int ret;
    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;
}

__attribute__ ((unused)) static int gc2375a_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != GC2375A_FLAG_END) {

        if (vals->reg_num == GC2375A_FLAG_DELAY) {
                msleep(vals->value);
        } else {
            ret = gc2375a_read(sd, vals->reg_num, &val);
            if (ret < 0)
                return ret;
            if (vals->reg_num == GC2375A_PAGE_REG){
                val &= 0xf8;
                val |= (vals->value & 0x07);
                ret = gc2375a_write(sd, vals->reg_num, val);
                ret = gc2375a_read(sd, vals->reg_num, &val);
            }
        }

        vals++;
    }

    return 0;
}

static int gc2375a_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != GC2375A_FLAG_END) {
        if (vals->reg_num == GC2375A_FLAG_DELAY) {
                msleep(vals->value);
        } else {
            ret = gc2375a_write(sd, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int gc2375a_reset(struct v4l2_subdev *sd, u32 val)
{
    return 0;
}

static int gc2375a_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
    unsigned char v;
    int ret;

    ret = gc2375a_read(sd, 0xf0, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != GC2375A_CHIP_ID_H)
        return -ENODEV;

    *ident = v;

    ret = gc2375a_read(sd, 0xf1, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != GC2375A_CHIP_ID_L)
        return -ENODEV;

    *ident = (*ident << 8) | v;

    return 0;
}

static int gc2375a_set_integration_time(struct v4l2_subdev *sd, int value)
{
    int ret = 0;
    unsigned int expo = value;

    ret = gc2375a_write(sd, 0xfe, 0x00);
    ret = gc2375a_write(sd,  0x04, (unsigned char)(expo & 0xff));
    ret += gc2375a_write(sd, 0x03, (unsigned char)((expo >> 8) & 0x3f));
    if (ret < 0) {
        printk("gc2375a_write error  %d\n" ,__LINE__ );
        return ret;
    }

    return 0;
}
/**************************************
  analog gain P0:0xb6
  global gain P0:0xb0, 0x40 --> x1 (not use)
  pre gain: P0:0xb1,  [3:0] Auto_pregain[9:6]
            P0:0xb2,  [7:2] Auto_pregain[5:0]
**************************************/
static int gc2375a_set_analog_gain(struct v4l2_subdev *sd, u16 gain)
{
    int ret = 0;
    //unsigned char tmp = 0;

    ret = gc2375a_write(sd, 0xfe, 0x00);
    ret += gc2375a_write(sd, 0xb6, (gain >> 12) & 0xf);
    ret += gc2375a_write(sd, 0xb1, (gain >> 8) & 0xf);
    ret += gc2375a_write(sd, 0xb2, (gain << 2) & 0xff);
    if (ret < 0) {
        printk("gc2375a_write error  %d" ,__LINE__ );
        return ret;
    }

    /*** dark sun ***/
/*
    ret = gc2375a_read(sd, 0xb6, &tmp);
    if (ret < 0) {
        return ret;
    }
    if(tmp >= 0x08){
        ret = gc2375a_write(sd, 0x21, 0x2c);
        if (ret < 0)
            return ret;
    }
    else{
        ret = gc2375a_write(sd, 0x21, 0x28);
        if (ret < 0)
            return ret;
    }
*/
    return 0;
}

static int gc2375a_set_digital_gain(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int gc2375a_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int gc2375a_init(struct v4l2_subdev *sd, u32 enable)
{
    struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
    struct tx_isp_notify_argument arg;
    struct tx_isp_sensor_win_setting *wsize = &gc2375a_win_sizes[0];
    int ret = 0;

    if(!enable)
        return ISP_SUCCESS;

    sensor->video.mbus.width        = wsize->width;
    sensor->video.mbus.height       = wsize->height;
    sensor->video.mbus.code         = wsize->mbus_code;
    sensor->video.mbus.field        = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace   = wsize->colorspace;
    sensor->video.fps = wsize->fps;
    ret = gc2375a_write_array(sd, wsize->regs);
    if (ret)
        return ret;

    arg.value = (int)&sensor->video;
    sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
    sensor->priv = wsize;

    return 0;
}

static int gc2375a_s_stream(struct v4l2_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = gc2375a_write_array(sd, gc2375a_stream_on_mipi);
        pr_debug("gc2375a stream on\n");
    } else {

        ret = gc2375a_write_array(sd, gc2375a_stream_off_mipi);
        pr_debug("gc2375a stream off\n");
    }

    return ret;
}
static int gc2375a_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    return 0;
}

static int gc2375a_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    return 0;
}

static int gc2375a_set_fps(struct v4l2_subdev *sd, int fps)
{
    return 0;
#if 0
    struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
    struct tx_isp_notify_argument arg;
    unsigned int wpclk      = 0;
    unsigned short win_high =0;
    unsigned short vts      = 0;
    unsigned short hb       =0;
    unsigned short vb       = 0;
    unsigned short hts      =0;
    unsigned int max_fps    = 0;
    unsigned int newformat = 0; //the format is 24.8
    int ret = 0;
    unsigned char tmp;

    wpclk = GC2375A_SUPPORT_30FPS_SCLK;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk("warn: fps(%x) no in range\n", fps);
        return -1;
    }

    //H Blanking
    ret = gc2375a_read(sd, 0x05, &tmp);
    hb = tmp;
    ret += gc2375a_read(sd, 0x06, &tmp);
    if(ret < 0)
        return -1;

    hb = (hb << 8) + tmp;
    hts = hb << 2;

    //P0:0x0d win_height[10:8], P0:0x0e win_height[7:0]
    ret = gc2375a_read(sd, 0x0d, &tmp);
    win_high = tmp;
    ret += gc2375a_read(sd, 0x0e, &tmp);
    if(ret < 0)
        return -1;

    win_high = (win_high << 8) + tmp;

    vts = wpclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    vb = vts - win_high - 16;

    //V Blanking
    ret = gc2375a_write(sd, 0x08, (unsigned char)(vb & 0xff));
    ret += gc2375a_write(sd, 0x07, (unsigned char)(vb >> 8) & 0x1f);
    if(ret < 0)
        return -1;

    sensor->video.fps = fps;
    sensor->video.attr->max_integration_time_native = vts - 4;
    sensor->video.attr->integration_time_limit      = vts - 4;
    sensor->video.attr->total_height                = vts;
    sensor->video.attr->max_integration_time        = vts - 4;
    arg.value = (int)&sensor->video;
    sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);

    return ret;
#endif
}

static int gc2375a_set_mode(struct v4l2_subdev *sd, int value)
{
    struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
    struct tx_isp_notify_argument arg;
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    if (value == TX_ISP_SENSOR_FULL_RES_MAX_FPS) {
        wsize = &gc2375a_win_sizes[0];
    } else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS) {
        wsize = &gc2375a_win_sizes[0];
    }

    if (wsize) {
        sensor->video.mbus.width        = wsize->width;
        sensor->video.mbus.height       = wsize->height;
        sensor->video.mbus.code         = wsize->mbus_code;
        sensor->video.mbus.field        = V4L2_FIELD_NONE;
        sensor->video.mbus.colorspace   = wsize->colorspace;
        sensor->video.fps               = wsize->fps;

        arg.value = (int)&sensor->video;
        sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
    }

    return ret;
}

__attribute__ ((unused)) static int gc2375a_set_vflip(struct tx_isp_sensor *sensor, int enable)
{
    struct tx_isp_notify_argument arg;
    struct v4l2_subdev *sd = &sensor->sd;
    int ret = 0;
    unsigned char val = 0xc0;

    ret = gc2375a_write(sd, 0xfe, 0x00);
    if (enable){
        val &= ~0x3;
        val |= 0x2;
        sensor->video.mbus.code = V4L2_MBUS_FMT_SGBRG10_1X10;
    } else {
        val &= 0xfd;
        sensor->video.mbus.code = V4L2_MBUS_FMT_SRGGB10_1X10;
    }
    ret += gc2375a_write(sd, 0x17, val);

    if(!ret) {
        arg.value = (int)&sensor->video;
    }
    sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);

    return ret;
}

static int gc2375a_g_chip_ident(struct v4l2_subdev *sd,
        struct v4l2_dbg_chip_ident *chip)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gc2375a_power");
        if (!ret) {
            gpio_direction_output(power_gpio, 1);
            msleep(50);
        } else {
            printk("gpio requrest fail %d\n", power_gpio);
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio,"gc2375a_pwdn");
        if (!ret) {
            gpio_direction_output(pwdn_gpio, 1);
            msleep(50);
            gpio_direction_output(pwdn_gpio, 0);
            msleep(10);
        } else {
            printk("gpio requrest fail %d\n",pwdn_gpio);
        }
    }

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio,"gc2375a_reset");
        if (!ret) {
            gpio_direction_output(reset_gpio, 1);
            msleep(5);
            gpio_direction_output(reset_gpio, 0);
            msleep(30);
            gpio_direction_output(reset_gpio, 1);
            msleep(35);
        } else {
            printk("gpio requrest fail %d\n",reset_gpio);
        }
    }

    ret = gc2375a_detect(sd, &ident);
    if (ret) {
        printk("chip found @ 0x%x (%s) is not an gc2375a chip.\n",
                client->addr, client->adapter->name);
        return ret;
    }
    v4l_info(client, "gc2375a chip found @ 0x%02x (%s)\n",
        client->addr, client->adapter->name);

    return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int gc2375a_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
    struct v4l2_subdev *sd = &sensor->sd;
    long ret = 0;
    switch(ctrl->cmd){
        case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
            ret = gc2375a_set_integration_time(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
            ret = gc2375a_set_analog_gain(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
            ret = gc2375a_set_digital_gain(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
            ret = gc2375a_get_black_pedestal(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
            ret = gc2375a_set_mode(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
            ret = gc2375a_write_array(sd, gc2375a_stream_off_mipi);
            break;
        case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
            ret = gc2375a_write_array(sd, gc2375a_stream_on_mipi);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
            ret = gc2375a_set_fps(sd, ctrl->value);
            break;
        default:
            break;;
    }

    return 0;
}
static long gc2375a_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
    struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
    int ret;

    switch (cmd) {
    case VIDIOC_ISP_PRIVATE_IOCTL:
        ret = gc2375a_ops_private_ioctl(sensor, arg);
        break;
    default:
        return -1;
        break;
    }

    return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int gc2375a_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned char val = 0;
    int ret = 0;

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;

    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    ret = gc2375a_read(sd, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;

    return ret;
}

static int gc2375a_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;

    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    gc2375a_write(sd, reg->reg & 0xff, reg->val & 0xff);

    return 0;
}
#endif

static int gc2375a_s_power(struct v4l2_subdev *sd, int on)
{
    return 0;
}

static struct v4l2_subdev_core_ops gc2375a_core_ops = {
    .g_chip_ident       = gc2375a_g_chip_ident,
    .reset              = gc2375a_reset,
    .init               = gc2375a_init,
    .s_power            = gc2375a_s_power,
    .ioctl              = gc2375a_ops_ioctl,
    /*.ioctl = gc2375a_ops_ioctl,*/
#ifdef CONFIG_VIDEO_ADV_DEBUG
    .g_register         = gc2375a_g_register,
    .s_register         = gc2375a_s_register,
#endif
};

static struct v4l2_subdev_video_ops gc2375a_video_ops = {
    .s_stream           = gc2375a_s_stream,
    .s_parm             = gc2375a_s_parm,
    .g_parm             = gc2375a_g_parm,
};



static struct v4l2_subdev_ops gc2375a_ops = {
    .core               = &gc2375a_core_ops,
    .video              = &gc2375a_video_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device gc2375a_platform_device = {
    .name           = "gc2375a",
    .id             = -1,
    .num_resources  = 0,
    .dev = {
        .dma_mask           = &tx_isp_module_dma_mask,
        .coherent_dma_mask  = 0xffffffff,
        .platform_data      = NULL,
    },

};

static int gc2375a_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct v4l2_subdev *sd;
    struct tx_isp_video_in *video;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &gc2375a_win_sizes[0];

    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if (!sensor) {
        printk("Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }

    memset(sensor, 0 ,sizeof(*sensor));
#ifndef MODULE
    gc2375a_board_info = get_sensor_board_info(gc2375a_platform_device.name);
    if (gc2375a_board_info) {
        power_gpio    = gc2375a_board_info->gpios.gpio_power;
        reset_gpio    = gc2375a_board_info->gpios.gpio_sensor_rst;
    }
#endif
    ret = tx_isp_clk_set(CONFIG_ISP_CLK);
    if (ret < 0) {
        printk("Cannot set isp clock\n");
        goto err_get_mclk;
    }
    /* request mclk of sensor */
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk("Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }
    clk_set_rate(sensor->mclk, 24000000);
    clk_enable(sensor->mclk);

     /*
        convert sensor-gain into isp-gain,
     */
    gc2375a_attr.max_again      = ANALOG_GAIN_MAX;
    gc2375a_attr.max_dgain      = 0;
    sd = &sensor->sd;
    video = &sensor->video;
    sensor->video.attr          = &gc2375a_attr;
    sensor->video.vi_max_width  = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    v4l2_i2c_subdev_init(sd, client, &gc2375a_ops);
    v4l2_set_subdev_hostdata(sd, sensor);

    pr_debug("probe ok ------->gc2375a\n");

    return 0;

#if 0
    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
#endif
err_get_mclk:
    kfree(sensor);

    return -1;
}

static int gc2375a_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    struct tx_isp_sensor *sensor = v4l2_get_subdev_hostdata(sd);
    if(power_gpio != -1)
        gpio_free(power_gpio);

    if(pwdn_gpio != -1)
        gpio_free(pwdn_gpio);

    if(reset_gpio != -1)
        gpio_free(reset_gpio);

    if(cam_en != -1)
        gpio_free(cam_en);

    clk_disable(sensor->mclk);
    clk_put(sensor->mclk);
    v4l2_device_unregister_subdev(sd);
    kfree(sensor);

    return 0;
}

static const struct i2c_device_id gc2375a_id[] = {
    { "gc2375a", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, gc2375a_id);

static struct i2c_driver gc2375a_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = "gc2375a",
    },
    .probe          = gc2375a_probe,
    .remove         = gc2375a_remove,
    .id_table       = gc2375a_id,
};

static __init int init_gc2375a(void)
{
    printk("%s %d.\n",__func__,__LINE__);
    return i2c_add_driver(&gc2375a_driver);
}

static __exit void exit_gc2375a(void)
{
    i2c_del_driver(&gc2375a_driver);
}

module_init(init_gc2375a);
module_exit(exit_gc2375a);

MODULE_DESCRIPTION("A low-level driver for Gcoreinc gc2375a sensors");
MODULE_LICENSE("GPL");

