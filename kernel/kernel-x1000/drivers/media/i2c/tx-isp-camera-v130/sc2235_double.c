/*
 * sc2235.c
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c/i2c-jz.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <soc/gpio.h>

#include <tx-isp/tx-isp-common.h>
#include <tx-isp/sensor-common.h>
#include <linux/sensor_board.h>


#define SC2235_CHIP_ID_H                (0x22)
#define SC2235_CHIP_ID_L                (0x35)
#define SC2235_SUPPORT_PCLK_FPS_30      (81000*1000)
#define SC2235_SUPPORT_PCLK_FPS_15      (45000*1000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5
#define SC2235_MAX_WIDTH                1920
#define SC2235_MAX_HEIGHT               1080
#if defined CONFIG_SC2235_DOUBLE_480X800
#define SC2235_WIDTH                    480
#define SC2235_HEIGHT                   800
#elif defined CONFIG_SC2235_DOUBLE_640X1072
#define SC2235_WIDTH                    640
#define SC2235_HEIGHT                   1072
#elif defined CONFIG_SC2235_DOUBLE_1920X1080
#define SC2235_WIDTH                    1920
#define SC2235_HEIGHT                   1080
#endif
#define SC2235_CAMERA1_RATE             1
#define SC2235_CAMERA2_RATE             1
#define SC2235_CAMERA1_CHN_TYPE         CAMERA_CHN_NORMAL
#define SC2235_CAMERA2_CHN_TYPE         CAMERA_CHN_IR
#define SENSOR_VERSION                  "H20180627a"


static int power_gpio = -1;
module_param(power_gpio, int, S_IRUGO);
MODULE_PARM_DESC(power_gpio, "Power on GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int reset_gpio = -1;
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int i2c_sel1_gpio = -1;
module_param(i2c_sel1_gpio, int, S_IRUGO);
MODULE_PARM_DESC(i2c_sel1_gpio, "i2c select1  GPIO NUM");

static int i2c_sel2_gpio = -1;
module_param(i2c_sel2_gpio, int, S_IRUGO);
MODULE_PARM_DESC(i2c_sel2_gpio, "i2c select2  GPIO NUM");

static int dvp_gpio_func = -1;
module_param(dvp_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(dvp_gpio_func, "Sensor GPIO function");

static int sensor_max_fps = TX_SENSOR_MAX_FPS_25;
module_param(sensor_max_fps, int, S_IRUGO);
MODULE_PARM_DESC(sensor_max_fps, "Sensor Max Fps set interface");


struct tx_isp_sensor_attribute sc2235_attr;
#ifndef MODULE
static struct sensor_board_info *sc2235_board_info;
#endif
struct dual_camera_param sc2235_dc_param;
spinlock_t i2c_poll_mode_lock;


/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
    unsigned int value;
    unsigned int gain;
};

struct again_lut sc2235_again_lut[] = {
    {0x10, 0},
    {0x11, 5731},
    {0x12, 11136},
    {0x13, 16248},
    {0x14, 21097},
    {0x15, 25710},
    {0x16, 30109},
    {0x17, 34312},
    {0x18, 38336},
    {0x19, 42195},
    {0x1a, 45904},
    {0x1b, 49472},
    {0x1c, 52910},
    {0x1d, 56228},
    {0x1e, 59433},
    {0x1f, 62534},
    {0x110, 65536},
    {0x111, 71267},
    {0x112, 76672},
    {0x113, 81784},
    {0x114, 86633},
    {0x115, 91246},
    {0x116, 95645},
    {0x117, 99848},
    {0x118, 103872},
    {0x119, 107731},
    {0x11a, 111440},
    {0x11b, 115008},
    {0x11c, 118446},
    {0x11d, 121764},
    {0x11e, 124969},
    {0x11f, 128070},
    {0x310, 131072},
    {0x311, 136803},
    {0x312, 142208},
    {0x313, 147320},
    {0x314, 152169},
    {0x315, 156782},
    {0x316, 161181},
    {0x317, 165384},
    {0x318, 169408},
    {0x319, 173267},
    {0x31a, 176976},
    {0x31b, 180544},
    {0x31c, 183982},
    {0x31d, 187300},
    {0x31e, 190505},
    {0x31f, 193606},
    {0x710, 196608},
    {0x711, 202339},
    {0x712, 207744},
    {0x713, 212856},
    {0x714, 217705},
    {0x715, 222318},
    {0x716, 226717},
    {0x717, 230920},
    {0x718, 234944},
    {0x719, 238803},
    {0x71a, 242512},
    {0x71b, 246080},
    {0x71c, 249518},
    {0x71d, 252836},
    {0x71e, 256041},
    /* {0x71f, 259142}, */
};



unsigned int sc2235_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = sc2235_again_lut;
    while(lut->gain <= sc2235_attr.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == sc2235_attr.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

unsigned int sc2235_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

struct tx_isp_sensor_attribute sc2235_attr={
    .name = "sc2235",
    .chip_id = 0x2235,
    .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device = 0x30,
    .dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
    .dvp = {
        .mode = SENSOR_DVP_HREF_MODE,
        .blanking = {
            .vblanking = 0,
            .hblanking = 0,
        },
    },
    .max_again = 256041,
    .max_dgain = 0,
    .min_integration_time = 4,
    .min_integration_time_native = 4,
    .max_integration_time_native = 0x5a0 - 4,
    .integration_time_limit = 0x5a0 - 4,
    .total_width = 0x8ca,
    .total_height = 0x5a0,
    .max_integration_time = 0x5a0 - 4,
    .one_line_expr_in_us = 28,
    .integration_time_apply_delay = 2,
    .again_apply_delay = 2,
    .dgain_apply_delay = 2,
    .sensor_ctrl.alloc_again = sc2235_alloc_again,
    .sensor_ctrl.alloc_dgain = sc2235_alloc_dgain,
};


static struct camera_reg_op sc2235_init_regs_1920_1080_25fps[] = {
    {CAMERA_REG_OP_DATA,0x0103,0x01},
    {CAMERA_REG_OP_DATA,0x0100,0x00},
    {CAMERA_REG_OP_DATA,0x3034,0x05},
    {CAMERA_REG_OP_DATA,0x3035,0x2a},
    {CAMERA_REG_OP_DATA,0x3039,0x35},
    {CAMERA_REG_OP_DATA,0x303a,0x2e},
    {CAMERA_REG_OP_DATA,0x320c,0x08},
    {CAMERA_REG_OP_DATA,0x320d,0xca},
    {CAMERA_REG_OP_DATA,0x320e,0x05},
    {CAMERA_REG_OP_DATA,0x320f,0xa0},
    {CAMERA_REG_OP_DATA,0x3222,0x29},
    {CAMERA_REG_OP_DATA,0x3235,0x04},
    {CAMERA_REG_OP_DATA,0x3236,0xae},
    {CAMERA_REG_OP_DATA,0x3237,0x08},
    {CAMERA_REG_OP_DATA,0x3238,0x9a},
    {CAMERA_REG_OP_DATA,0x3301,0x12},
    {CAMERA_REG_OP_DATA,0x3303,0x20},
    {CAMERA_REG_OP_DATA,0x3306,0x44},
    {CAMERA_REG_OP_DATA,0x3309,0xa0},
    {CAMERA_REG_OP_DATA,0x330b,0xca},
    {CAMERA_REG_OP_DATA,0x330e,0x30},
    {CAMERA_REG_OP_DATA,0x3313,0x05},
    {CAMERA_REG_OP_DATA,0x331e,0x0d},
    {CAMERA_REG_OP_DATA,0x331f,0x8d},
    {CAMERA_REG_OP_DATA,0x3320,0x0f},
    {CAMERA_REG_OP_DATA,0x3321,0x8f},
    {CAMERA_REG_OP_DATA,0x3340,0x06},
    {CAMERA_REG_OP_DATA,0x3341,0x50},
    {CAMERA_REG_OP_DATA,0x3342,0x04},
    {CAMERA_REG_OP_DATA,0x3343,0x20},
    {CAMERA_REG_OP_DATA,0x3348,0x07},
    {CAMERA_REG_OP_DATA,0x3349,0x80},
    {CAMERA_REG_OP_DATA,0x334a,0x04},
    {CAMERA_REG_OP_DATA,0x334b,0x20},
    {CAMERA_REG_OP_DATA,0x335e,0x01},
    {CAMERA_REG_OP_DATA,0x335f,0x03},
    {CAMERA_REG_OP_DATA,0x3364,0x05},
    {CAMERA_REG_OP_DATA,0x3366,0x7c},
    {CAMERA_REG_OP_DATA,0x3367,0x08},
    {CAMERA_REG_OP_DATA,0x3368,0x02},
    {CAMERA_REG_OP_DATA,0x3369,0x00},
    {CAMERA_REG_OP_DATA,0x336a,0x00},
    {CAMERA_REG_OP_DATA,0x336b,0x00},
    {CAMERA_REG_OP_DATA,0x337c,0x04},
    {CAMERA_REG_OP_DATA,0x337d,0x06},
    {CAMERA_REG_OP_DATA,0x337f,0x03},
    {CAMERA_REG_OP_DATA,0x3380,0x04},
    {CAMERA_REG_OP_DATA,0x3381,0x0a},
    {CAMERA_REG_OP_DATA,0x33a0,0x05},
    {CAMERA_REG_OP_DATA,0x33b5,0x10},
    {CAMERA_REG_OP_DATA,0x3621,0x28},
    {CAMERA_REG_OP_DATA,0x3622,0xc6},
    {CAMERA_REG_OP_DATA,0x3625,0x02},
    {CAMERA_REG_OP_DATA,0x3630,0x48},
    {CAMERA_REG_OP_DATA,0x3631,0x84},
    {CAMERA_REG_OP_DATA,0x3632,0x88},
    {CAMERA_REG_OP_DATA,0x3633,0x42},
    {CAMERA_REG_OP_DATA,0x3634,0x42},
    {CAMERA_REG_OP_DATA,0x3635,0xc1},
    {CAMERA_REG_OP_DATA,0x3636,0x24},
    {CAMERA_REG_OP_DATA,0x3637,0x14},
    {CAMERA_REG_OP_DATA,0x3638,0x1f},
    {CAMERA_REG_OP_DATA,0x3639,0x09},
    {CAMERA_REG_OP_DATA,0x363b,0x09},
    {CAMERA_REG_OP_DATA,0x363c,0x07},
    {CAMERA_REG_OP_DATA,0x3641,0x01},
    {CAMERA_REG_OP_DATA,0x3210,(SC2235_MAX_WIDTH-SC2235_WIDTH)/2/256}, //col start high
    {CAMERA_REG_OP_DATA,0x3211,8+(SC2235_MAX_WIDTH-SC2235_WIDTH)/2%256}, //col start low
    {CAMERA_REG_OP_DATA,0x3212,(SC2235_MAX_HEIGHT-SC2235_HEIGHT)/2/256}, //row start high
    {CAMERA_REG_OP_DATA,0x3213,8+(SC2235_MAX_HEIGHT-SC2235_HEIGHT)/2%256}, //row start low
    {CAMERA_REG_OP_DATA,0x3208,SC2235_WIDTH/256}, //win width high
    {CAMERA_REG_OP_DATA,0x3209,SC2235_WIDTH%256}, //win width low
    {CAMERA_REG_OP_DATA,0x320a,SC2235_HEIGHT/256}, //win height high
    {CAMERA_REG_OP_DATA,0x320b,SC2235_HEIGHT%256}, //win height low
    {CAMERA_REG_OP_DATA,0x366e,0x08},
    {CAMERA_REG_OP_DATA,0x366f,0x2f},
    {CAMERA_REG_OP_DATA,0x3670,0x00},
    {CAMERA_REG_OP_DATA,0x3677,0x3f},
    {CAMERA_REG_OP_DATA,0x3678,0x42},
    {CAMERA_REG_OP_DATA,0x3679,0x43},
    {CAMERA_REG_OP_DATA,0x367e,0x07},
    {CAMERA_REG_OP_DATA,0x367f,0x0f},
    {CAMERA_REG_OP_DATA,0x3802,0x00},
    {CAMERA_REG_OP_DATA,0x3901,0x02},
    {CAMERA_REG_OP_DATA,0x3908,0x11},
    {CAMERA_REG_OP_DATA,0x391b,0x4d},
    {CAMERA_REG_OP_DATA,0x391e,0x00},
    {CAMERA_REG_OP_DATA,0x3d08,0x01},
    {CAMERA_REG_OP_DATA,0x3e01,0x46},
    {CAMERA_REG_OP_DATA,0x3e03,0x0b},
    {CAMERA_REG_OP_DATA,0x3f00,0x07},
    {CAMERA_REG_OP_DATA,0x3f04,0x08},
    {CAMERA_REG_OP_DATA,0x3f05,0xa6},
    {CAMERA_REG_OP_DATA,0x4500,0x59},
    {CAMERA_REG_OP_DATA,0x5780,0xff},
    {CAMERA_REG_OP_DATA,0x5781,0x04},
    {CAMERA_REG_OP_DATA,0x5785,0x18},

    {CAMERA_REG_OP_END,0x0,0x0},    /* END MARKER */
};
static struct camera_reg_op sc2235_init_regs_1920_1080_15fps[] = {
    {CAMERA_REG_OP_DATA,0x0103,0x01},
    {CAMERA_REG_OP_DATA,0x0100,0x00},
    {CAMERA_REG_OP_DATA,0x3039,0x80},
    {CAMERA_REG_OP_DATA,0x3621,0x28},
    {CAMERA_REG_OP_DATA,0x33b5,0x10},
    {CAMERA_REG_OP_DATA,0x3303,0x20},
    {CAMERA_REG_OP_DATA,0x331e,0x0d},
    {CAMERA_REG_OP_DATA,0x3320,0x0f},
    {CAMERA_REG_OP_DATA,0x3633,0x42},
    {CAMERA_REG_OP_DATA,0x3634,0x42},
    {CAMERA_REG_OP_DATA,0x3364,0x05},
    {CAMERA_REG_OP_DATA,0x4500,0x59},
    {CAMERA_REG_OP_DATA,0x3d08,0x01},
    {CAMERA_REG_OP_DATA,0x3908,0x11},
    {CAMERA_REG_OP_DATA,0x3381,0x0a},
    {CAMERA_REG_OP_DATA,0x3380,0x04},
    {CAMERA_REG_OP_DATA,0x3632,0x88},
    {CAMERA_REG_OP_DATA,0x3309,0xa0},
    {CAMERA_REG_OP_DATA,0x331f,0x8d},
    {CAMERA_REG_OP_DATA,0x3321,0x8f},
    {CAMERA_REG_OP_DATA,0x335e,0x01},
    {CAMERA_REG_OP_DATA,0x335f,0x03},
    {CAMERA_REG_OP_DATA,0x337c,0x04},
    {CAMERA_REG_OP_DATA,0x337d,0x06},
    {CAMERA_REG_OP_DATA,0x33a0,0x05},
    {CAMERA_REG_OP_DATA,0x3301,0x05},
    {CAMERA_REG_OP_DATA,0x3670,0x08},
    {CAMERA_REG_OP_DATA,0x367e,0x07},
    {CAMERA_REG_OP_DATA,0x367f,0x0f},
    {CAMERA_REG_OP_DATA,0x3677,0x2f},
    {CAMERA_REG_OP_DATA,0x3679,0x43},
    {CAMERA_REG_OP_DATA,0x337f,0x03},
    {CAMERA_REG_OP_DATA,0x3368,0x02},
    {CAMERA_REG_OP_DATA,0x3369,0x00},
    {CAMERA_REG_OP_DATA,0x336a,0x00},
    {CAMERA_REG_OP_DATA,0x336b,0x00},
    {CAMERA_REG_OP_DATA,0x3367,0x08},
    {CAMERA_REG_OP_DATA,0x330e,0x30},
    {CAMERA_REG_OP_DATA,0x3366,0x7c},
    {CAMERA_REG_OP_DATA,0x3635,0xc1},
    {CAMERA_REG_OP_DATA,0x363b,0x09},
    {CAMERA_REG_OP_DATA,0x363c,0x07},
    {CAMERA_REG_OP_DATA,0x391e,0x00},
    {CAMERA_REG_OP_DATA,0x3637,0x14},
    {CAMERA_REG_OP_DATA,0x366e,0x08},
    {CAMERA_REG_OP_DATA,0x366f,0x2f},
    {CAMERA_REG_OP_DATA,0x3631,0x84},
    {CAMERA_REG_OP_DATA,0x3630,0x48},
    {CAMERA_REG_OP_DATA,0x3622,0x06},
    {CAMERA_REG_OP_DATA,0x3638,0x1f},
    {CAMERA_REG_OP_DATA,0x3625,0x02},
    {CAMERA_REG_OP_DATA,0x3636,0x24},
    {CAMERA_REG_OP_DATA,0x3e03,0x0b},
    {CAMERA_REG_OP_DATA,0x3802,0x00},
    {CAMERA_REG_OP_DATA,0x391b,0x4d},
    {CAMERA_REG_OP_DATA,0x3222,0x29},
    {CAMERA_REG_OP_DATA,0x3901,0x02},
    {CAMERA_REG_OP_DATA,0x3f00,0x07},
    {CAMERA_REG_OP_DATA,0x3340,0x04},
    {CAMERA_REG_OP_DATA,0x3341,0xec},
    {CAMERA_REG_OP_DATA,0x3342,0x04},
    {CAMERA_REG_OP_DATA,0x3343,0x20},
    {CAMERA_REG_OP_DATA,0x3348,0x04},
    {CAMERA_REG_OP_DATA,0x3349,0xec},
    {CAMERA_REG_OP_DATA,0x334a,0x04},
    {CAMERA_REG_OP_DATA,0x334b,0x20},
    {CAMERA_REG_OP_DATA,0x5780,0xff},
    {CAMERA_REG_OP_DATA,0x5781,0x04},
    {CAMERA_REG_OP_DATA,0x5785,0x18},
    {CAMERA_REG_OP_DATA,0x3313,0x05},
    {CAMERA_REG_OP_DATA,0x3678,0x42},
    {CAMERA_REG_OP_DATA,0x3639,0x09},
    {CAMERA_REG_OP_DATA,0x3208,0x07},
    {CAMERA_REG_OP_DATA,0x3209,0x80},
    {CAMERA_REG_OP_DATA,0x320a,0x04},
    {CAMERA_REG_OP_DATA,0x320b,0x38},
    {CAMERA_REG_OP_DATA,0x3200,0x00},
    {CAMERA_REG_OP_DATA,0x3201,0x00},
    {CAMERA_REG_OP_DATA,0x3202,0x00},
    {CAMERA_REG_OP_DATA,0x3203,0x00},
    {CAMERA_REG_OP_DATA,0x3204,0x07},
    {CAMERA_REG_OP_DATA,0x3205,0x8b},
    {CAMERA_REG_OP_DATA,0x3206,0x04},
    {CAMERA_REG_OP_DATA,0x3207,0x47},
    {CAMERA_REG_OP_DATA,0x3211,0x08},
    {CAMERA_REG_OP_DATA,0x3213,0x08},
    {CAMERA_REG_OP_DATA,0x320c,0x09},
    {CAMERA_REG_OP_DATA,0x320d,0xc4},
    {CAMERA_REG_OP_DATA,0x3f04,0x09},
    {CAMERA_REG_OP_DATA,0x3f05,0xa0},
    {CAMERA_REG_OP_DATA,0x320e,0x04},
    {CAMERA_REG_OP_DATA,0x320f,0xb0},
    {CAMERA_REG_OP_DATA,0x3235,0x04},
    {CAMERA_REG_OP_DATA,0x3236,0xae},
    {CAMERA_REG_OP_DATA,0x3e01,0x4a},
    {CAMERA_REG_OP_DATA,0x3e02,0xa0},
    {CAMERA_REG_OP_DATA,0x3039,0x35},
    {CAMERA_REG_OP_DATA,0x303a,0x8e},
    {CAMERA_REG_OP_DATA,0x3034,0x05},
    {CAMERA_REG_OP_DATA,0x3035,0x8a},
    {CAMERA_REG_OP_DATA,0x3306,0x40},
    {CAMERA_REG_OP_DATA,0x330b,0xc6},
    {CAMERA_REG_OP_DATA,0x3237,0x09},
    {CAMERA_REG_OP_DATA,0x3238,0x94},

    {CAMERA_REG_OP_END,0x0,0x0},    /* END MARKER */
};
/*
 * the order of the sc2235_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting sc2235_win_sizes[] = {
    /* 1920*1080 */
    {
        .width        = SC2235_WIDTH,
        .height        = SC2235_HEIGHT,
        .fps        = 25 << 16 | 1,
        .mbus_code    = V4L2_MBUS_FMT_SBGGR8_1X8,
        .colorspace    = V4L2_COLORSPACE_SRGB,
        .regs         = sc2235_init_regs_1920_1080_25fps,
    }
};

static enum v4l2_mbus_pixelcode sc2235_mbus_code[] = {
    V4L2_MBUS_FMT_SBGGR8_1X8,
    V4L2_MBUS_FMT_SBGGR10_1X10,
    V4L2_MBUS_FMT_SBGGR12_1X12,
};

/*
 * the part of driver was fixed.
 */

static struct camera_reg_op sc2235_stream_on[] = {
    {CAMERA_REG_OP_DATA,0x3001, 0xff},
    {CAMERA_REG_OP_DATA,0x0100, 0x01},
    {CAMERA_REG_OP_END,0x0,0x0},    /* END MARKER */
};

static struct camera_reg_op sc2235_stream_off[] = {
    {CAMERA_REG_OP_DATA,0x0100, 0x00},
    {CAMERA_REG_OP_DATA,0x3001, 0x00},
    {CAMERA_REG_OP_END,0x0,0x0},    /* END MARKER */
};

static int sc2235_read(struct tx_isp_subdev *sd, uint16_t reg, unsigned char *value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned long flag;
    int ret;

    unsigned char buf[2] = {reg >> 8, reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr    = client->addr,
            .flags    = 0,
            .len    = 2,
            .buf    = buf,
        },
        [1] = {
            .addr    = client->addr,
            .flags    = I2C_M_RD,
            .len    = 1,
            .buf    = value,
        }
    };

    spin_lock_irqsave(&i2c_poll_mode_lock, flag);
    ret = private_i2c_transfer(client->adapter, msg, 2);
    spin_unlock_irqrestore(&i2c_poll_mode_lock, flag);
    if (ret > 0)
        ret = 0;

    return ret;
}

static int sc2235_write(struct tx_isp_subdev *sd, uint16_t reg, unsigned char value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned long flag;
    int ret;

    uint8_t buf[3] = {(reg>>8)&0xff, reg&0xff, value};
    struct i2c_msg msg = {
        .addr    = client->addr,
        .flags    = 0,
        .len    = 3,
        .buf    = buf,
    };

    spin_lock_irqsave(&i2c_poll_mode_lock, flag);
    ret = private_i2c_transfer(client->adapter, &msg, 1);
    spin_unlock_irqrestore(&i2c_poll_mode_lock, flag);
    if (ret > 0)
        ret = 0;

    return ret;
}

static inline int sc2235_read_array(struct tx_isp_subdev *sd, struct camera_reg_op *vals)
{
    int ret;
    unsigned char val;
    while (vals->flag != CAMERA_REG_OP_END) {
        if (vals->flag == CAMERA_REG_OP_DATA) {
            ret = sc2235_read(sd, vals->reg, &val);
            if (ret < 0)
                return ret;
            //printk("%x[%x]\n", vals->reg, val);
        } else if (vals->flag == CAMERA_REG_OP_DELAY) {
            private_msleep(vals->val);
        } else {
            pr_debug("%s(%d), error flag: %d\n", __func__, __LINE__, vals->flag);
            return -1;
        }
        vals++;
    }
    return 0;
}

static int sc2235_write_array(struct tx_isp_subdev *sd, struct camera_reg_op *vals)
{
    int ret;
    while (vals->flag != CAMERA_REG_OP_END) {
        if (vals->flag == CAMERA_REG_OP_DATA) {
            ret = sc2235_write(sd, vals->reg, vals->val);
            if (ret < 0)
                return ret;
        } else if (vals->flag == CAMERA_REG_OP_DELAY) {
            private_msleep(vals->val);
        } else {
            pr_debug("%s(%d), error flag: %d\n", __func__, __LINE__, vals->flag);
            return -1;
        }
        vals++;
    }
    return 0;
}

static void dual_camera_i2c_switch(enum camera_chn_type chn)
{
    switch (chn) {
        case SC2235_CAMERA1_CHN_TYPE:
            private_gpio_direction_output(i2c_sel2_gpio, 0);
            private_gpio_direction_output(i2c_sel1_gpio, 1);
            break;

        case SC2235_CAMERA2_CHN_TYPE:
            private_gpio_direction_output(i2c_sel1_gpio, 0);
            private_gpio_direction_output(i2c_sel2_gpio, 1);
            break;

        default:
            printk("dual_camera_switch fail, err chn: %d\n", chn);
    }
}

static void sc2235_switch_camera(struct tx_isp_subdev *sd)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct dual_camera_frame_info *info = &sensor->video.dc_param->frame_info;
    unsigned int last_chn;

    sensor->video.dc_param->frame_cnt++;
    //printk(KERN_DEBUG"sensor: %ld <%d>\n", sensor->video.dc_param->frame_cnt, info->frame_array[info->frame_cursor]);

    if (info->frame_num <= 1) {
        return;
    }

    last_chn = info->frame_array[info->frame_cursor];
    info->frame_cursor++;
    if (info->frame_cursor > info->frame_num-1) {
        info->frame_cursor = 0;
    }
    if (last_chn != info->frame_array[info->frame_cursor]) {
        sc2235_write_array(&sensor->sd, sc2235_stream_off);

        dual_camera_i2c_switch(info->frame_array[info->frame_cursor]);

        sc2235_write_array(&sensor->sd, sc2235_stream_on);
    }
}

static unsigned char sc2235_get_camera_channel(struct tx_isp_subdev *sd)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct dual_camera_frame_info *info = &sensor->video.dc_param->frame_info;

    return info->frame_array[info->frame_cursor];
}

static int sc2235_reset(struct tx_isp_subdev *sd, int val)
{
    return 0;
}

static int sc2235_detect(struct tx_isp_subdev *sd, unsigned int *ident)
{
    int ret;
    unsigned char v;

    ret = sc2235_read(sd, 0x3107, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;
    if (v != SC2235_CHIP_ID_H)
        return -ENODEV;
    *ident = v;

    ret = sc2235_read(sd, 0x3108, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;
    if (v != SC2235_CHIP_ID_L)
        return -ENODEV;
    *ident = (*ident << 8) | v;

    return 0;
}

static int sc2235_set_integration_time(struct tx_isp_subdev *sd, int value)
{
    int ret = 0;

    ret = sc2235_write(sd, 0x3e00, (unsigned char)((value >> 12) & 0x0f));
    ret += sc2235_write(sd, 0x3e01, (unsigned char)((value >> 4) & 0xff));
    ret += sc2235_write(sd, 0x3e02, (unsigned char)((value & 0x0f) << 4));

    /* EXP logic */
    if (value < 0x50) {
        ret = sc2235_write(sd, 0x3314,0x12);
        if (ret < 0)
            return ret;
    }
    else if (value > 0xa0) {
        ret += sc2235_write(sd, 0x3314,0x02);
        if (ret < 0)
            return ret;
    }
    if (ret < 0)
        return ret;

    return 0;
}

static int sc2235_set_analog_gain(struct tx_isp_subdev *sd, int value)
{
    int ret = 0;
    ret = sc2235_write(sd,0x3903,0x84);
    ret += sc2235_write(sd,0x3903,0x04);
    ret += sc2235_write(sd, 0x3e09, (unsigned char)(value & 0xff));
    ret += sc2235_write(sd, 0x3e08, (unsigned char)((value >> 8 << 2) | 0x03));
    if (ret < 0)
        return ret;

    if (value < 0x110) {
        sc2235_write(sd,0x3812,0x00);
        sc2235_write(sd,0x3301,0x12);
        sc2235_write(sd,0x3631,0x84);
        sc2235_write(sd,0x366f,0x2f);
        sc2235_write(sd,0x3622,0xc6);
        sc2235_write(sd,0x5781,0x04);
        sc2235_write(sd,0x5785,0x18);
        sc2235_write(sd,0x3812,0x30);
    }
    else if (value>=0x110&&value<0x710){
        sc2235_write(sd,0x3812,0x00);
        sc2235_write(sd,0x3301,0x15);
        sc2235_write(sd,0x3631,0x88);
        sc2235_write(sd,0x366f,0x2f);
        sc2235_write(sd,0x3622,0xc6);
        sc2235_write(sd,0x5781,0x04);
        sc2235_write(sd,0x5785,0x18);
        sc2235_write(sd,0x3812,0x30);
    }
    else if(value>=0x710&&value<=0x71e){
        sc2235_write(sd,0x3812,0x00);
        sc2235_write(sd,0x3301,0x1e);
        sc2235_write(sd,0x3631,0x88);
        sc2235_write(sd,0x366f,0x2f);
        sc2235_write(sd,0x3622,0xc6);
        sc2235_write(sd,0x5781,0x02);
        sc2235_write(sd,0x5785,0x04);
        sc2235_write(sd,0x3812,0x30);
    }
    else{ //may be flick
        sc2235_write(sd,0x3812,0x00);
        sc2235_write(sd,0x3301,0xff);
        sc2235_write(sd,0x3631,0x88);
        sc2235_write(sd,0x366f,0x3a);
        sc2235_write(sd,0x3622,0x06);
        sc2235_write(sd,0x5781,0x01);
        sc2235_write(sd,0x5785,0x02);
        sc2235_write(sd,0x3812,0x30);
    }

    return 0;
}

static int sc2235_set_digital_gain(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int sc2235_get_black_pedestal(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int sc2235_init(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = &sc2235_win_sizes[0];
    int ret = 0;

    if(!enable)
        return ISP_SUCCESS;

    switch (sensor_max_fps) {
    case TX_SENSOR_MAX_FPS_25:
        wsize->fps = 25 << 16 | 1;
        wsize->regs = sc2235_init_regs_1920_1080_25fps;
        break;
    case TX_SENSOR_MAX_FPS_15:
        wsize->fps = 15 << 16 | 1;
        wsize->regs = sc2235_init_regs_1920_1080_15fps;
        break;
    default:
        printk("Now we do not support this framerate!!!\n");
    }

    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;
    dual_camera_i2c_switch(sensor->video.dc_param->frame_info.frame_array[0]);
    dual_camera_do(ret, sensor, 0, sc2235_write_array, sd, wsize->regs);
    dual_camera_do(ret, sensor, 0, sc2235_write_array, sd, sc2235_stream_off);
    //i2c_sel2_gpio = i2c_sel1_gpio;
    if (ret)
        return ret;
    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    sensor->priv = wsize;

    return 0;
}

static int sc2235_s_stream(struct tx_isp_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = sc2235_write_array(sd, sc2235_stream_on);
        pr_debug("sc2235 stream on\n");
    }
    else {
        ret = sc2235_write_array(sd, sc2235_stream_off);
        pr_debug("sc2235 stream off\n");
    }
    return ret;
}

static int sc2235_set_fps(struct tx_isp_subdev *sd, int fps)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    unsigned int pclk = 0;
    unsigned short hts;
    unsigned short vts = 0;
    unsigned char tmp;
    unsigned int newformat = 0; //the format is 24.8
    unsigned int max_fps = 0; //the format is 24.8
    int ret = 0;

    switch (sensor_max_fps) {
    case TX_SENSOR_MAX_FPS_25:
        pclk = SC2235_SUPPORT_PCLK_FPS_30;
        max_fps = SENSOR_OUTPUT_MAX_FPS;
        break;
    case TX_SENSOR_MAX_FPS_15:
        pclk = SC2235_SUPPORT_PCLK_FPS_15;
        max_fps = TX_SENSOR_MAX_FPS_15;
        break;
    default:
        printk("Now we do not support this framerate!!!\n");
    }

    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk("warn: fps(%d) no in range\n", fps);
        return -1;
    }
    ret = sc2235_read(sd, 0x320c, &tmp);
    hts = tmp;
    ret += sc2235_read(sd, 0x320d, &tmp);
    if(ret < 0)
        return -1;
    hts = ((hts << 8) + tmp);

    vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = sc2235_write(sd, 0x320f, (unsigned char)(vts & 0xff));
    ret += sc2235_write(sd, 0x320e, (unsigned char)(vts >> 8));
    if(ret < 0){
        printk("err: sc2235_write err\n");
        return ret;
    }

    sensor->video.fps = fps;
    sensor->video.attr->max_integration_time_native = vts - 4;
    sensor->video.attr->integration_time_limit = vts - 4;
    sensor->video.attr->total_height = vts;
    sensor->video.attr->max_integration_time = vts - 4;
    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    return ret;
}

static int sc2235_set_mode(struct tx_isp_subdev *sd, int value)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
        wsize = &sc2235_win_sizes[0];
    }else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
        wsize = &sc2235_win_sizes[0];
    }

    if(wsize){
        sensor->video.mbus.width = wsize->width;
        sensor->video.mbus.height = wsize->height;
        sensor->video.mbus.code = wsize->mbus_code;
        sensor->video.mbus.field = V4L2_FIELD_NONE;
        sensor->video.mbus.colorspace = wsize->colorspace;
        sensor->video.fps = wsize->fps;
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    }
    return ret;
}

static int sc2235_set_vflip(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    int ret = 0;
    char val = 0;

    val = enable ? 0x60 : 0;
    ret += sc2235_write(sd, 0x3221, val);
    sensor->video.mbus_change = 0;
    if(!ret)
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return ret;
}

static int sc2235_g_chip_ident(struct tx_isp_subdev *sd,
        struct tx_isp_chip_ident *chip)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "sc2235_power");
        if (!ret) {
            private_gpio_direction_output(power_gpio, 1);
            private_msleep(50);
        }
        else
            printk("gpio requrest fail %d\n", power_gpio);
    }
    if(reset_gpio != -1){
        ret = private_gpio_request(reset_gpio,"sc2235_reset");
        if(!ret){
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(10);
            private_gpio_direction_output(reset_gpio, 0);
            private_msleep(10);
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(1);
        }else{
            printk("gpio requrest fail %d\n",reset_gpio);
        }
    }
    if (pwdn_gpio != -1) {
        ret = private_gpio_request(pwdn_gpio, "sc2235_pwdn");
        if(!ret){
            private_gpio_direction_output(pwdn_gpio, 1);
            private_msleep(50);
            private_gpio_direction_output(pwdn_gpio, 0);
            private_msleep(10);
        }else{
            printk("gpio requrest fail %d\n", pwdn_gpio);
        }
    }
    if (i2c_sel1_gpio != -1) {
        int ret = gpio_request(i2c_sel1_gpio, "sc2235_i2c_sel1");
        if (!ret) {
            private_gpio_direction_output(i2c_sel1_gpio, 1);
        } else {
            printk("gpio requrest fail %d\n", i2c_sel1_gpio);
        }
    }
    if (i2c_sel2_gpio != -1) {
        int ret = gpio_request(i2c_sel2_gpio, "sc2235_i2c_sel2");
        if (!ret) {
            private_gpio_direction_output(i2c_sel2_gpio, 0);
        } else {
            printk("gpio requrest fail %d\n", i2c_sel2_gpio);
        }
    }

    ret = sc2235_detect(sd, &ident);
    if (ret) {
        printk("chip found @ 0x%x (%s) is not an sc2235 chip.\n",
                client->addr, client->adapter->name);
        return ret;
    }
    printk("sc2235 chip found @ 0x%02x (%s)\n", client->addr, client->adapter->name);
    if(chip){
        memcpy(chip->name, "sc2235", sizeof("sc2235"));
        chip->ident = ident;
        chip->revision = SENSOR_VERSION;
    }

    return 0;
}

static int sc2235_sensor_ops_ioctl(struct tx_isp_subdev *sd, unsigned int cmd, void *arg)
{
    long ret = 0;
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);

    if(IS_ERR_OR_NULL(sd)){
        printk("[%d]The pointer is invalid!\n", __LINE__);
        return -EINVAL;
    }
    switch(cmd){
        case TX_ISP_EVENT_SENSOR_INT_TIME:
            if(arg) {
#if 1
                if (sc2235_get_camera_channel(sd) == 0) {
                    ret = sc2235_set_integration_time(sd, *(int*)arg);
                }
#else
                dual_camera_do(ret, sensor, 0, sc2235_set_integration_time, sd, *(int*)arg);
#endif
            }
            break;
        case TX_ISP_EVENT_SENSOR_AGAIN:
            if(arg)
#if 1
                if (sc2235_get_camera_channel(sd) == 0) {
                    ret = sc2235_set_analog_gain(sd, *(int*)arg);
                }
#else
                dual_camera_do(ret, sensor, 0, sc2235_set_analog_gain, sd, *(int*)arg);
#endif
            break;
        case TX_ISP_EVENT_SENSOR_DGAIN:
            if(arg)
                ret = sc2235_set_digital_gain(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_BLACK_LEVEL:
            if(arg)
                ret = sc2235_get_black_pedestal(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_RESIZE:
            if(arg)
                ret = sc2235_set_mode(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_PREPARE_CHANGE:
            dual_camera_do_lock(ret, sensor, 0, sc2235_write_array, sd, sc2235_stream_off);
            break;
        case TX_ISP_EVENT_SENSOR_FINISH_CHANGE:
            dual_camera_do_lock(ret, sensor, 0, sc2235_write_array, sd, sc2235_stream_on);
            break;
        case TX_ISP_EVENT_SENSOR_FPS:
            if(arg)
                dual_camera_do_lock(ret, sensor, 0, sc2235_set_fps, sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_VFLIP:
            if(arg)
                dual_camera_do_lock(ret, sensor, 0, sc2235_set_vflip, sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_DUAL_CAMERA_SWITCH:
            sc2235_switch_camera(sd);
            break;
        default:
            break;;
    }

    return 0;
}

static int sc2235_g_register(struct tx_isp_subdev *sd, struct tx_isp_dbg_register *reg)
{
    unsigned char val = 0;
    int len = 0;
    int ret = 0;

    len = strlen(sd->chip.name);
    if(len && strncmp(sd->chip.name, reg->name, len)){
        return -EINVAL;
    }
    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;
    ret = sc2235_read(sd, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;

    return ret;
}

static int sc2235_s_register(struct tx_isp_subdev *sd, const struct tx_isp_dbg_register *reg)
{
    int len = 0;
    len = strlen(sd->chip.name);
    if(len && strncmp(sd->chip.name, reg->name, len)){
        return -EINVAL;
    }
    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;
    sc2235_write(sd, reg->reg & 0xffff, reg->val & 0xff);

    return 0;
}

static struct tx_isp_subdev_core_ops sc2235_core_ops = {
    .g_chip_ident = sc2235_g_chip_ident,
    .reset = sc2235_reset,
    .init = sc2235_init,
    .g_register = sc2235_g_register,
    .s_register = sc2235_s_register,
};

static struct tx_isp_subdev_video_ops sc2235_video_ops = {
    .s_stream = sc2235_s_stream,
};

static struct tx_isp_subdev_sensor_ops sc2235_sensor_ops = {
    .ioctl    = sc2235_sensor_ops_ioctl,
};

static struct tx_isp_subdev_ops sc2235_ops = {
    .core = &sc2235_core_ops,
    .video = &sc2235_video_ops,
    .sensor = &sc2235_sensor_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device sc2235_platform_device = {
    .name = "sc2235",
    .id = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources = 0,
};


static int sc2235_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    struct tx_isp_subdev *sd;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &sc2235_win_sizes[0];
    enum v4l2_mbus_pixelcode mbus;
    int i = 0;
    int ret;

    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if(!sensor){
        printk("Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }
    memset(sensor, 0 ,sizeof(*sensor));

#ifndef MODULE
    sc2235_board_info = get_sensor_board_info(sc2235_platform_device.name);
    if (sc2235_board_info) {
        power_gpio    = sc2235_board_info->gpios.gpio_power;
        reset_gpio    = sc2235_board_info->gpios.gpio_sensor_rst;
        i2c_sel1_gpio = sc2235_board_info->gpios.gpio_i2c_sel1;
        i2c_sel2_gpio = sc2235_board_info->gpios.gpio_i2c_sel2;

        dvp_gpio_func = sc2235_board_info->dvp_gpio_func;
    }
#endif

    /* request mclk of sensor */
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk("Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }

    clk_set_rate(sensor->mclk, 24000000);
    clk_enable(sensor->mclk);
    ret = set_sensor_gpio_function(dvp_gpio_func);
    if (ret < 0)
        goto err_set_sensor_gpio;

    sc2235_attr.dvp.gpio = dvp_gpio_func;

    switch(dvp_gpio_func){
        case DVP_PA_LOW_8BIT:
        case DVP_PA_HIGH_8BIT:
            mbus = sc2235_mbus_code[0];
            break;
        case DVP_PA_LOW_10BIT:
        case DVP_PA_HIGH_10BIT:
            mbus = sc2235_mbus_code[1];
            break;
        case DVP_PA_12BIT:
            mbus = sc2235_mbus_code[2];
            break;
        default:
            goto err_set_sensor_gpio;
    }

    for(i = 0; i < ARRAY_SIZE(sc2235_win_sizes); i++)
        sc2235_win_sizes[i].mbus_code = mbus;

     /*
        convert sensor-gain into isp-gain,
     */
    switch (sensor_max_fps) {
        case TX_SENSOR_MAX_FPS_25:
            break;
        case TX_SENSOR_MAX_FPS_15:
            sc2235_attr.max_integration_time_native = 1196;
            sc2235_attr.integration_time_limit = 1196;
            sc2235_attr.total_width = 2500;
            sc2235_attr.total_height = 1200;
            sc2235_attr.max_integration_time = 1196;
            break;
        default:
            printk("Now we do not support this framerate!!!\n");
    }
    sc2235_attr.max_again = 256041;
    sc2235_attr.max_dgain = 0; //sc2235_attr.max_dgain;
    sd = &sensor->sd;
    sensor->video.attr = &sc2235_attr;
    sensor->video.mbus_change = 0;
    sensor->video.vi_max_width = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;

    sensor->video.dc_param = &sc2235_dc_param;
    sensor->video.dc_param->frame_info.camera1_frame_rate = SC2235_CAMERA1_RATE;
    sensor->video.dc_param->frame_info.camera2_frame_rate = SC2235_CAMERA2_RATE;
    sensor->video.dc_param->frame_info.camera1_chn_type = SC2235_CAMERA1_CHN_TYPE;
    sensor->video.dc_param->frame_info.camera2_chn_type = SC2235_CAMERA2_CHN_TYPE;
    interpolation_frame_init(&sensor->video.dc_param->frame_info);
    sensor->video.dc_param->frame_cnt = 0;
    sensor->video.dc_param->frame_cnt_comp = 0;
    spin_lock_init(&sensor->video.dc_param->lock);
    spin_lock_init(&i2c_poll_mode_lock);
    i2c_jz_select_poll_mode(client->adapter);

    tx_isp_subdev_init(&sc2235_platform_device, sd, &sc2235_ops);
    tx_isp_set_subdevdata(sd, client);
    tx_isp_set_subdev_hostdata(sd, sensor);
    private_i2c_set_clientdata(client, sd);


    pr_debug("@@@@@@@probe ok ------->sc2235\n");

    return 0;

err_set_sensor_gpio:
    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
err_get_mclk:
    kfree(sensor);

    return -1;
}

static int sc2235_remove(struct i2c_client *client)
{
    struct tx_isp_subdev *sd = private_i2c_get_clientdata(client);
    struct tx_isp_sensor *sensor = tx_isp_get_subdev_hostdata(sd);

    if(power_gpio != -1) {
        private_gpio_direction_output(power_gpio, 0);
        private_gpio_free(power_gpio);
    }
    if(i2c_sel1_gpio != -1)
        private_gpio_free(i2c_sel1_gpio);
    if(i2c_sel2_gpio != -1)
        private_gpio_free(i2c_sel2_gpio);
    if(reset_gpio != -1)
        private_gpio_free(reset_gpio);
    if(pwdn_gpio != -1)
        private_gpio_free(pwdn_gpio);

    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
    tx_isp_subdev_deinit(sd);
    interpolation_frame_deinit(&sc2235_dc_param.frame_info);
    kfree(sensor);

    return 0;
}

static const struct i2c_device_id sc2235_id[] = {
    { "sc2235", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sc2235_id);

static struct i2c_driver sc2235_driver = {
    .driver = {
        .owner    = THIS_MODULE,
        .name    = "sc2235",
    },
    .probe        = sc2235_probe,
    .remove        = sc2235_remove,
    .id_table    = sc2235_id,
};

static __init int init_sc2235(void)
{
    int ret = 0;
    ret = private_driver_get_interface();
    if(ret){
        printk("Failed to init sc2235 dirver.\n");
        return -1;
    }
    return private_i2c_add_driver(&sc2235_driver);
}

static __exit void exit_sc2235(void)
{
    private_i2c_del_driver(&sc2235_driver);
}

module_init(init_sc2235);
module_exit(exit_sc2235);

MODULE_DESCRIPTION("A low-level driver for Smartsenstech sc2235 sensors");
MODULE_LICENSE("GPL");
