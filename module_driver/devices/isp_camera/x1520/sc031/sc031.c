/*
 * sc031.c
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
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <dvp_gpio_func.h>

#include <tx-isp/sensor-common.h>
#include <tx-isp/apical-isp/apical_math.h>
#include <linux/proc_fs.h>
#include <utils/gpio.h>

#define SC031_CHIP_ID_H                (0x00)
#define SC031_CHIP_ID_L                (0x31)
#define SC031_REG_END                  0xffff
#define SC031_REG_DELAY                0xfffe

#ifdef MD_X1520_SC031_120FPS_SUPPORT
#define SC031_SUPPORT_PCLK_FPS     (72000*1000)
#define SENSOR_OUTPUT_MAX_FPS           120
#else
#define SC031_SUPPORT_PCLK_FPS     (24000*1000)
#define SENSOR_OUTPUT_MAX_FPS           60
#endif

#define SENSOR_OUTPUT_MIN_FPS           5
#define DRIVE_CAPABILITY_1

static int reset_gpio       = -1;//GPIO_PA(10)
static int pwdn_gpio        = -1;

module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

static char *dvp_gpio_func_str = "DVP_PA_LOW_8BIT";
module_param(dvp_gpio_func_str, charp, 0644);
MODULE_PARM_DESC(dvp_gpio_func_str, "Sensor GPIO function");

static int dvp_gpio_func = -1;

struct tx_isp_sensor_attribute sc031_attr;

struct regval_list {
    uint16_t reg_num;
    unsigned char value;
};

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
    unsigned int value;
    unsigned int gain;
};

struct again_lut sc031_again_lut[] = {
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
    {0x110,65536},
    {0x111,71267},
    {0x112,76672},
    {0x113,81784},
    {0x114,86633},
    {0x115,91246},
    {0x116,95645},
    {0x117,99848},
    {0x118,103872},
    {0x119,107731},
    {0x11a,111440},
    {0x11b,115008},
    {0x11c,118446},
    {0x11d,121764},
    {0x11e,124969},
    {0x11f,128070},
    {0x310,131072},
    {0x311,136803},
    {0x312,142208},
    {0x313,147320},
    {0x314,152169},
    {0x315,156782},
    {0x316,161181},
    {0x317,165384},
    {0x318,169408},
    {0x319,173267},
    {0x31a,176976},
    {0x31b,180544},
    {0x31c,183982},
    {0x31d,187300},
    {0x31e,190505},
    {0x31f,193606},
    {0x710,196608},
    {0x711,202339},
    {0x712,207744},
    {0x713,212856},
    {0x714,217705},
    {0x715,222318},
    {0x716,226717},
    {0x717,230920},
    {0x718,234944},
    {0x719,238803},
    {0x71a,242512},
    {0x71b,246080},
    {0x71c,249518},
    {0x71d,252836},
    {0x71e,256041},
};


unsigned int sc031_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = sc031_again_lut;

    while(lut->gain <= sc031_attr.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == sc031_attr.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

unsigned int sc031_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

struct tx_isp_sensor_attribute sc031_attr={
    .name                               = "sc031",
    .chip_id                            = 0x0031,
    .cbus_type                          = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask                          = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device                        = 0x30,
    .dbus_type                          = TX_SENSOR_DATA_INTERFACE_DVP,
    .dvp = {
        .mode = SENSOR_DVP_HREF_MODE,
        .blanking = {
            .vblanking = 0,
            .hblanking = 0,
        },
    },
    .max_again                          = 256041,
    .max_dgain                          = 0,
    .min_integration_time               = 4,
    .min_integration_time_native        = 4,
#ifdef MD_X1520_SC031_120FPS_SUPPORT
    .max_integration_time_native        = 0x2ab - 4,
    .integration_time_limit             = 0x2ab - 4,
    .total_width                        = 0x36e,
    .total_height                       = 0x2ab,
    .max_integration_time               = 0x2ab - 4,
    .one_line_expr_in_us                = 12,//0x36e/72M
    //0x148 (0x3e01 0x14 0x3e02 0x80) 一行的总曝光时间 0x36e/72M*0x148
#else
    .max_integration_time_native        = 0x215 - 4,
    .integration_time_limit             = 0x215 - 4,
    .total_width                        = 0x2ee,
    .total_height                       = 0x215,
    .max_integration_time               = 0x215 - 4,
    .one_line_expr_in_us                = 31,//0x2ee/24M
#endif
    .integration_time_apply_delay       = 2,
    .again_apply_delay                  = 2,
    .dgain_apply_delay                  = 2,
    .sensor_ctrl.alloc_again            = sc031_alloc_again,
    .sensor_ctrl.alloc_dgain            = sc031_alloc_dgain,
};



static struct regval_list sc031_init_regs_640_480_60fps[] = {
    {0x0103,0x01},
    {0x0100,0x00},
    {0x300f,0x0f},
    {0x3018,0x1f},
    {0x3019,0xff},
    {0x301c,0xb4},
    {0x3028,0x82},
    {0x320c,0x02},//hts
    {0x320d,0xee},
    {0x320e,0x02},//vts
    {0x320f,0x15},
    {0x3220,0x10},
    {0x3250,0xf0},
    {0x3251,0x02},
    {0x3252,0x02},
    {0x3253,0x08},
    {0x3254,0x02},
    {0x3255,0x07},
    {0x3304,0x24},
    {0x3306,0x24},
    {0x3309,0x34},
    {0x330b,0x80},
    {0x330c,0x0c},
    {0x330f,0x20},
    {0x3310,0x10},
    {0x3314,0x1d},
    {0x3315,0x1c},
    {0x3316,0x24},
    {0x3317,0x10},
    {0x3329,0x1e},
    {0x332d,0x1e},
    {0x332f,0x20},
    {0x3335,0x22},
    {0x3344,0x22},
    {0x335b,0x80},
    {0x335f,0x80},
    {0x3366,0x06},
    {0x3385,0x1f},
    {0x3387,0x2f},
    {0x3389,0x01},
    {0x33b1,0x03},
    {0x33b2,0x06},
    {0x3621,0xa4},
    {0x3622,0x05},
    {0x3624,0x47},
    {0x3630,0x46},
    {0x3631,0x48},
    {0x3633,0x52},
    {0x3635,0x18},
    {0x3636,0x25},
    {0x3637,0x89},
    {0x3638,0x2f},
    {0x3639,0x08},
    {0x363a,0x00},
    {0x363b,0x48},
    {0x363c,0x06},
    {0x363d,0x00},
    {0x363e,0xf8},
    {0x3640,0x00},
    {0x3641,0x00},
    {0x36e9,0x04},
    {0x36ea,0x18},
    {0x36eb,0x1e},
    {0x36ec,0x0e},
    {0x36ed,0x03},
    {0x36f9,0x04},
    {0x36fa,0x10},
    {0x36fb,0x10},
    {0x36fc,0x00},
    {0x36fd,0x03},
    {0x3908,0x91},
    {0x3d08,0x01},
    {0x3e01,0x08},
    {0x3e02,0x00},
    {0x3e06,0x0c},
    {0x4500,0x59},
    {0x4501,0xc4},
    {0x5011,0x00},
    {0x0100,0x01},
    //{0x4501,0xAC},
    //{0x5011,0x01},
    //{0x0100,0x01},

    {SC031_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list sc031_init_regs_640_480_120fps[] = {
    {0x0103,0x01},
    {0x0100,0x00},

    {0x300f,0x0f},
    {0x3018,0x1f},
    {0x3019,0xff},
    {0x301c,0xb4},
    {0x301f,0x01},
    {0x320c,0x03},//hts
    {0x320d,0x6e},
    {0x320e,0x02},//vts
    {0x320f,0xab},
    {0x3220,0x10},
    {0x3223,0x50},
    {0x3250,0xf0},
    {0x3251,0x02},
    {0x3252,0x02},
    {0x3253,0xa6},
    {0x3254,0x02},
    {0x3255,0x07},
    {0x3304,0x48},
    {0x3306,0x38},
    {0x3309,0x68},
    {0x330b,0xe0},
    {0x330c,0x18},
    {0x330f,0x20},
    {0x3310,0x10},
    {0x3314,0x65},
    {0x3315,0x38},
    {0x3316,0x68},
    {0x3317,0x10},
    {0x3329,0x5c},
    {0x332d,0x5c},
    {0x332f,0x60},
    {0x3335,0x64},
    {0x3344,0x64},
    {0x335b,0x80},
    {0x335f,0x80},
    {0x3366,0x06},
    {0x3385,0x31},
    {0x3387,0x51},
    {0x3389,0x01},
    {0x33b1,0x03},
    {0x33b2,0x06},
    {0x3621,0xa4},
    {0x3622,0x05},
    {0x3624,0x47},
    {0x3631,0x48},
    {0x3633,0x52},
    {0x3635,0x18},
    {0x3636,0x25},
    {0x3637,0x89},
    {0x3638,0x0f},
    {0x3639,0x08},
    {0x363a,0x00},
    {0x363b,0x48},
    {0x363c,0x06},
    {0x363e,0xf8},
    {0x3640,0x00},
    {0x3641,0x01},
    {0x36e9,0x00},
    {0x36ea,0x3b},
    {0x36eb,0x1a},
    {0x36ec,0x0a},
    {0x36ed,0x33},
    {0x36f9,0x00},
    {0x36fa,0x3a},
    {0x36fc,0x01},
    {0x3908,0x91},
    {0x3d08,0x01},
    {0x3e01,0x14},
    {0x3e02,0x80},
    {0x3e06,0x0c},
    {0x4500,0x59},
    {0x4501,0xc4},
    {0x5011,0x00},
    //{0x4501,0xAC},
    //{0x5011,0x01},
    //{0x0100,0x01},

    {SC031_REG_END, 0x00},    /* END MARKER */
};

/*
 * the order of the sc031_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting sc031_win_sizes[] = {
    /* 640*480 */
    {
        .width                  = 640,
        .height                 = 480,
        .fps                    = 120 << 16 | 1,
        .mbus_code              = V4L2_MBUS_FMT_SBGGR8_1X8,
        .colorspace             = V4L2_COLORSPACE_SRGB,
        .regs                   = sc031_init_regs_640_480_120fps,
    },

    {
        .width                  = 640,
        .height                 = 480,
        .fps                    = 60 << 16 | 1,
        .mbus_code              = V4L2_MBUS_FMT_SBGGR8_1X8,
        .colorspace             = V4L2_COLORSPACE_SRGB,
        .regs                   = sc031_init_regs_640_480_60fps,
    }
};

/*
 * the part of driver was fixed.
 */
#ifdef MD_X1520_SC031_120FPS_SUPPORT
static struct regval_list sc031_stream_on[] = {
    {0x0100, 0x01},
    {SC031_REG_DELAY, 0x0a},
    {0x4418,0x08},
    {0x4419,0x80},
    {0x363d,0x10},
    {0x3630,0x48},
    {SC031_REG_END, 0x00},    /* END MARKER */
};
#else
static struct regval_list sc031_stream_on[] = {
    {0x0100, 0x01},
    {SC031_REG_DELAY, 0x17},
    {0x4418,0x08},
    {0x4419,0x8e},
    {SC031_REG_END, 0x00},    /* END MARKER */
};
#endif
static struct regval_list sc031_stream_off[] = {
    {0x0100, 0x00},
    {SC031_REG_END, 0x00},    /* END MARKER */
};

static int sc031_read(struct v4l2_subdev *sd, uint16_t reg, unsigned char *value)
{
    int ret;
    unsigned char buf[2] = {reg >> 8, reg & 0xff};
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct i2c_msg msg[2] = {
        [0] = {
            .addr       = client->addr,
            .flags      = 0,
            .len        = 2,
            .buf        = buf,
        },
        [1] = {
            .addr       = client->addr,
            .flags      = I2C_M_RD,
            .len        = 1,
            .buf        = value,
        }
    };

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

    return ret;
}

static int sc031_write(struct v4l2_subdev *sd, uint16_t reg, unsigned char value)
{
    int ret;
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    uint8_t buf[3] = {(reg>>8)&0xff, reg&0xff, value};
    struct i2c_msg msg = {
        .addr           = client->addr,
        .flags          = 0,
        .len            = 3,
        .buf            = buf,
    };

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;
}

static inline int sc031_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != SC031_REG_END) {

        if (vals->reg_num == SC031_REG_DELAY) {
            msleep(vals->value);
        } else {
            ret = sc031_read(sd, vals->reg_num, &val);
            if (ret < 0)
                return ret;
            printk("0x%x 0x%x\n",vals->reg_num,val);
        }
        vals++;
    }

    return 0;
}

static int sc031_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != SC031_REG_END) {
        if (vals->reg_num == SC031_REG_DELAY) {
            msleep(vals->value);
        } else {
            ret = sc031_write(sd, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int sc031_reset(struct v4l2_subdev *sd, unsigned int val)
{
    return 0;
}

static int sc031_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
    int ret;
    unsigned char v;

    ret = sc031_read(sd, 0x3107, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != SC031_CHIP_ID_H)
        return -ENODEV;
    *ident = v;

    ret = sc031_read(sd, 0x3108, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != SC031_CHIP_ID_L)
        return -ENODEV;

    *ident = (*ident << 8) | v;

    return 0;
}

static int sc031_set_integration_time(struct v4l2_subdev *sd, int value)
{
    int ret = 0;
    ret = sc031_write(sd, 0x3e01, (unsigned char)((value >> 4) & 0xff));
    ret += sc031_write(sd, 0x3e02, (unsigned char)((value & 0x0f) << 4));
    if (ret < 0)
        return ret;

    return 0;
}

static int sc031_set_analog_gain(struct v4l2_subdev *sd, int value)
{
    int ret = 0;

    ret = sc031_write(sd,0x3903,0x0b);
    ret += sc031_write(sd, 0x3e09, (unsigned char)(value & 0xff));
    ret += sc031_write(sd, 0x3e08, (unsigned char)((value >> 8 << 2) | 0x03));
    if (ret < 0)
        return ret;

    if (value < 0x310) {
        sc031_write(sd,0x3314,0x65);
        sc031_write(sd,0x3317,0x10);
    } else if(value>=0x310&&value<=0x71f){
        sc031_write(sd,0x3314,0x60);
        sc031_write(sd,0x3317,0x0e);
    } else{
       return -1;
    }

    return 0;
}

static int sc031_set_digital_gain(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int sc031_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int sc031_init(struct v4l2_subdev *sd, unsigned int enable)
{
    struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
    struct tx_isp_notify_argument arg;
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = 0;
    // unsigned char val=0;
    if(!enable)
        return ISP_SUCCESS;
#ifdef MD_X1520_SC031_120FPS_SUPPORT
        wsize = &sc031_win_sizes[0];
#else
        wsize = &sc031_win_sizes[1];
#endif

    sensor->video.mbus.width    = wsize->width;
    sensor->video.mbus.height   = wsize->height;
    sensor->video.mbus.code     = wsize->mbus_code;
    sensor->video.mbus.field    = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps           = wsize->fps;

    ret = sc031_write_array(sd, wsize->regs);
    if (ret)
        return ret;
   /* sc031_read_array(sd, wsize->regs);
   sc031_read(sd, 0x320e, &val);
    printk("0x320e %x\n",val);
    sc031_read(sd, 0x3902, &val);
    printk("0x3902 %x\n",val);

        sc031_read(sd, 0x3e03, &val);
    printk("0x3e03 %x\n",val);
        sc031_read(sd, 0x3e08, &val);
    printk("0x3e08 %x\n",val);
        sc031_read(sd, 0x3e09, &val);
    printk("0x3e09 %x\n",val);
        sc031_read(sd, 0x3e06, &val);
    printk("0x3e06 %x\n",val);
        sc031_read(sd, 0x3e07, &val);
    printk("0x3e07 %x\n",val);*/
    arg.value = (int)&sensor->video;
    sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
    sensor->priv = wsize;

    return 0;
}

static int sc031_s_stream(struct v4l2_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = sc031_write_array(sd, sc031_stream_on);
        pr_debug("sc031 stream on\n");
    } else {
        ret = sc031_write_array(sd, sc031_stream_off);
        pr_debug("sc031 stream off\n");
    }

    return ret;
}

static int sc031_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    return 0;
}

static int sc031_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    return 0;
}

static int sc031_set_fps(struct tx_isp_sensor *sensor, int fps)
{

    struct v4l2_subdev *sd = &sensor->sd;
    struct tx_isp_notify_argument arg;
    unsigned int pclk = 0;
    unsigned short hts;
    unsigned short vts = 0;
    unsigned char tmp;
    unsigned int newformat = 0; //the format is 24.8
    unsigned int max_fps = 0; //the format is 24.8
    int ret = 0;

    pclk = SC031_SUPPORT_PCLK_FPS;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk(KERN_ERR "sc031: warn: fps(%d) no in range\n", fps);
        return -1;
    }

    ret = sc031_read(sd, 0x320c, &tmp);
    hts = tmp;
    ret += sc031_read(sd, 0x320d, &tmp);
    if(ret < 0)
        return -1;

    hts = ((hts << 8) + tmp);

    vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = sc031_write(sd, 0x320f, (unsigned char)(vts & 0xff));
    ret += sc031_write(sd, 0x320e, (unsigned char)(vts >> 8));
    if(ret < 0){
        printk("err: sc031_write err\n");
        return ret;
    }

    sensor->video.fps = fps;
    sensor->video.attr->max_integration_time_native = vts - 4;
    sensor->video.attr->integration_time_limit      = vts - 4;
    sensor->video.attr->total_height                = vts;
    sensor->video.attr->max_integration_time        = vts - 4;

    arg.value = (int)&sensor->video;
    sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);

    return ret;
}

static int sc031_set_mode(struct tx_isp_sensor *sensor, int value)
{
    struct tx_isp_notify_argument arg;
    struct v4l2_subdev *sd = &sensor->sd;
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    #ifdef MD_X1520_SC031_120FPS_SUPPORT
    wsize = &sc031_win_sizes[0];
    #else
    wsize = &sc031_win_sizes[1];
    #endif
    if (wsize) {
        sensor->video.mbus.width = wsize->width;
        sensor->video.mbus.height = wsize->height;
        sensor->video.mbus.code = wsize->mbus_code;
        sensor->video.mbus.field = V4L2_FIELD_NONE;
        sensor->video.mbus.colorspace = wsize->colorspace;
        sensor->video.fps = wsize->fps;
        arg.value = (int)&sensor->video;
        sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
    }

    return ret;
}

static int sc031_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio,"sc031_reset");
        if (!ret) {
            gpio_direction_output(reset_gpio, 1);
            msleep(10);
            gpio_direction_output(reset_gpio, 0);
            msleep(10);
            gpio_direction_output(reset_gpio, 1);
            msleep(1);
        } else {
            printk(KERN_ERR "sc031: gpio requrest fail %d\n",reset_gpio);
            return -1;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "sc031_pwdn");
        if (!ret) {
            gpio_direction_output(pwdn_gpio, 1);
            msleep(50);
            gpio_direction_output(pwdn_gpio, 0);
            msleep(10);
        } else {
            printk(KERN_ERR "sc031: gpio requrest fail %d\n", pwdn_gpio);
            return -1;
        }
    }
    ret = sc031_detect(sd, &ident);
    if (ret) {
        v4l_err(client,
            "sc031: chip found @ 0x%x (%s) is not an sc031 chip.\n",
            client->addr, client->adapter->name);
        return ret;
    }
    v4l_info(client, "sc031 chip found @ 0x%02x (%s)\n",
         client->addr, client->adapter->name);

    return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int sc031_s_power(struct v4l2_subdev *sd, int on)
{
    return 0;
}

static long sc031_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
    struct v4l2_subdev *sd = &sensor->sd;
    long ret = 0;

    switch(ctrl->cmd){
        case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
            //printk("sc031_set_integration_time %x \n",ctrl->value);
            ret = sc031_set_integration_time(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
            //printk("sc031_set_analog_gain %x \n",ctrl->value);
            ret = sc031_set_analog_gain(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
            //printk("%d %d\n",ctrl->value,__LINE__);
            ret = sc031_set_digital_gain(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
            //printk("%d %d\n",ctrl->value,__LINE__);
            ret = sc031_get_black_pedestal(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
            //printk("%d %d\n",ctrl->value,__LINE__);
            ret = sc031_set_mode(sensor,ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
            ret = sc031_write_array(sd, sc031_stream_off);
            break;
        case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
            ret = sc031_write_array(sd, sc031_stream_on);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
            //printk("%d %d\n",ctrl->value,__LINE__);
            ret = sc031_set_fps(sensor, ctrl->value);
            break;
        default:
            pr_debug("do not support ctrl->cmd ====%d\n",ctrl->cmd);
            break;;
    }

    return ret;
}

static long sc031_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
    struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
    int ret;

    switch(cmd){
        case VIDIOC_ISP_PRIVATE_IOCTL:
            ret = sc031_ops_private_ioctl(sensor, arg);
            break;
        default:
            return -1;
    }

    return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sc031_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned char val = 0;
    int ret;

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;
    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    ret = sc031_read(sd, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    //printk("%d %s %llx %llx\n",__LINE__,__func__, reg->reg, reg->val);
    return ret;
}

static int sc031_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;
    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    sc031_write(sd, reg->reg & 0xffff, reg->val & 0xff);
    //printk("%d %s %llx %llx\n",__LINE__,__func__, reg->reg, reg->val);

    return 0;
}
#endif

static const struct v4l2_subdev_core_ops sc031_core_ops = {
    .g_chip_ident   = sc031_g_chip_ident,
    .reset          = sc031_reset,
    .init           = sc031_init,
    .s_power        = sc031_s_power,
    .ioctl          = sc031_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
    .g_register     = sc031_g_register,
    .s_register     = sc031_s_register,
#endif
};

static const struct v4l2_subdev_video_ops sc031_video_ops = {
    .s_stream       = sc031_s_stream,
    .s_parm         = sc031_s_parm,
    .g_parm         = sc031_g_parm,
};

static const struct v4l2_subdev_ops sc031_ops = {
    .core           = &sc031_core_ops,
    .video          = &sc031_video_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device sc031_platform_device = {
    .name = "sc031",
    .id = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources = 0,
};

static int sc031_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct v4l2_subdev *sd;
    struct tx_isp_video_in *video;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int i, ret;

    #ifdef MD_X1520_SC031_120FPS_SUPPORT
        wsize = &sc031_win_sizes[0];
    #else
        wsize = &sc031_win_sizes[1];
    #endif

    for (i = 0; i < ARRAY_SIZE(dvp_gpio_func_array); i++) {
        if (!strcmp(dvp_gpio_func_str, dvp_gpio_func_array[i])) {
            dvp_gpio_func = i;
            break;
        }
    }

    if (i == ARRAY_SIZE(dvp_gpio_func_array))
        printk(KERN_ERR "sensor dvp_gpio_func set error!\n");

    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if(!sensor){
        printk(KERN_ERR "sc031: Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }

    ret = tx_isp_clk_set(MD_X1520_SC031_ISPCLK);
    if (ret < 0) {
        printk(KERN_ERR "sc031: Cannot set isp clock\n");
        goto err_get_mclk;
    }
    /* request mclk of sensor */
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk(KERN_ERR "sc031: Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }
#ifdef MD_X1520_SC031_120FPS_SUPPORT
    clk_set_rate(sensor->mclk, 24000000);
#else
    clk_set_rate(sensor->mclk, 6000000);
#endif
    clk_enable(sensor->mclk);

    ret = set_sensor_gpio_function(dvp_gpio_func);
    if (ret < 0)
        goto err_set_sensor_gpio;

    sc031_attr.dvp.gpio = dvp_gpio_func;


    /*
      convert sensor-gain into isp-gain,
    */

    sc031_attr.max_again       = 256041;
    sc031_attr.max_dgain       = 0; //sc031_attr.max_dgain;
    sd                          = &sensor->sd;
    video                       = &sensor->video;
    sensor->video.attr          = &sc031_attr;
    sensor->video.vi_max_width  = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    v4l2_i2c_subdev_init(sd, client, &sc031_ops);
    v4l2_set_subdev_hostdata(sd, sensor);

    pr_debug("@@@@@@@probe ok ------->sc031\n");
    return 0;

err_set_sensor_gpio:
    clk_disable(sensor->mclk);
    clk_put(sensor->mclk);
err_get_mclk:
    kfree(sensor);

    return -1;
}

static int sc031_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    struct tx_isp_sensor *sensor = v4l2_get_subdev_hostdata(sd);

    if(reset_gpio != -1)
        gpio_free(reset_gpio);

    if(pwdn_gpio != -1)
        gpio_free(pwdn_gpio);

    clk_disable(sensor->mclk);
    clk_put(sensor->mclk);

    v4l2_device_unregister_subdev(sd);
    kfree(sensor);

    return 0;
}

static const struct i2c_device_id sc031_id[] = {
    { "sc031", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sc031_id);

static struct i2c_driver sc031_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = "sc031",
    },
    .probe          = sc031_probe,
    .remove         = sc031_remove,
    .id_table       = sc031_id,
};

static __init int init_sc031(void)
{
    return i2c_add_driver(&sc031_driver);
}

static __exit void exit_sc031(void)
{
    i2c_del_driver(&sc031_driver);
}

module_init(init_sc031);
module_exit(exit_sc031);

MODULE_DESCRIPTION("x1520 sc031 driver depend on isp");
MODULE_LICENSE("GPL");
