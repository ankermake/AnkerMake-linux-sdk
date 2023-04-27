/*
 * tw9912.c
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
#include <linux/delay.h>
#include <tx-isp/sensor-common.h>
#include <tx-isp/apical-isp/apical_math.h>
#include <linux/proc_fs.h>
#include <soc/gpio.h>
#include <linux/sensor_board.h>

#define TW9912_CHIP_ID                  0x60

#define TW9912_REG_END                  0xff
#define TW9912_REG_DELAY                0xfe

#define TW9912_SUPPORT_MCLK             (27*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS           25
#define SENSOR_OUTPUT_MIN_FPS           5
#define DRIVE_CAPABILITY_1

#define TW9912_USE_AGAIN_ONLY



#define WIDTH        720
#define HEIGHT       480

struct regval_list {
    unsigned char reg;
    unsigned char value;
};

static int reset_gpio       = -1;
static int pwdn_gpio        = -1;
static int gpio_i2c_sel1    = -1;
static int gpio_i2c_sel2    = -1;
static int sensor_gpio_func = DVP_PA_LOW_8BIT;

module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

module_param(gpio_i2c_sel1, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "I2C select1  GPIO NUM");

module_param(gpio_i2c_sel2, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "I2C select2  GPIO NUM");

module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");


struct tx_isp_sensor_attribute tw9912_attr;
static struct sensor_board_info *tw9912_board_info;

static uint32_t fix_point_mult2(uint32_t a, uint32_t b)
{
    uint32_t x1,x2,x;
    uint32_t a1,a2,b1,b2;
    uint32_t mask = (((unsigned int)0xffffffff)>>(32-TX_ISP_GAIN_FIXED_POINT));
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

uint32_t tw9912_gainone_to_reg(uint32_t gain_one, uint16_t *regs)
{
    uint32_t gain_one1 = 0;
    uint16_t regsa = 0;
    uint16_t regsd = 0;
    uint32_t max_gain_one = 0;
    uint32_t max_gain_one_a = 0;
    uint32_t max_gain_one_d = 0;
    uint32_t min_gain_one_d = 0;
    uint32_t mask_gain_one_a = 0;
    uint32_t gain_one_a = 0;
    uint32_t gain_one_d = 0;

    uint32_t div_l = 0;
    uint32_t div_l1 = 0;
    uint32_t div_s = 0;
    uint32_t loop = 0;

    max_gain_one_a = 0xf8<<(TX_ISP_GAIN_FIXED_POINT-4);
    max_gain_one_d = 0x1ff<<(TX_ISP_GAIN_FIXED_POINT-8);
    min_gain_one_d = 0x100<<(TX_ISP_GAIN_FIXED_POINT-8);
    max_gain_one = fix_point_mult2(max_gain_one_a, max_gain_one_d);
    mask_gain_one_a = ((~0) >> (TX_ISP_GAIN_FIXED_POINT-4)) << (TX_ISP_GAIN_FIXED_POINT-4);

    if (gain_one >= max_gain_one) {
        gain_one_a = max_gain_one_a;
        gain_one_d = max_gain_one_d;
        regsa = 0xf8; regsd = 0xff;
        goto done;
    }
    if ((gain_one & mask_gain_one_a) < max_gain_one_a) {
        gain_one_a = gain_one & mask_gain_one_a;
        regsa = gain_one_a >>  (TX_ISP_GAIN_FIXED_POINT-4);
    } else {
        gain_one_a = max_gain_one_a;
        regsa = 0xf8;
    }

    div_l = max_gain_one_d;
    div_l1 = max_gain_one_d;
    div_s = min_gain_one_d;
    loop = 0;

    while ((div_l - div_s) > 1) {
        loop++;
        if (loop > 32) goto err_div;
        gain_one_d = (div_s + div_l)/2;
        if (gain_one > fix_point_mult2(gain_one_a, gain_one_d)) {
            div_s = gain_one_d;
        } else {
            div_l = gain_one_d;
        }
    }
    gain_one_d = div_s;
    regsd = 0xff & (gain_one_d >> (TX_ISP_GAIN_FIXED_POINT-8));
    done:

    gain_one1 = fix_point_mult2(gain_one_a, (gain_one_d>>(TX_ISP_GAIN_FIXED_POINT-8))<<(TX_ISP_GAIN_FIXED_POINT-8));
    *regs = ( regsa<<8 ) | regsd;
    //printk("info:  gain_one = 0x%08x, gain_one1 = 0x%08x, sensor_again = 0x%08x\n", gain_one, gain_one1, *regs);
    return gain_one1;
    err_div:

   //printk("err: %s,%s,%d err_div loop = %d\n", __FILE__, __func__, __LINE__, loop);
    gain_one1 = 0x10000;
    *regs = 0x1000;;
    return gain_one1;
}

unsigned int tw9912_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
#ifdef TW9912_USE_AGAIN_ONLY
    unsigned int gain_one = 0;
    unsigned int gain_one1 = 0;
    uint16_t regs = 0;
    unsigned int isp_gain1 = 0;
    uint32_t mask;
    /* low 4 bits are fraction bits */
    gain_one = math_exp2(isp_gain, shift, TX_ISP_GAIN_FIXED_POINT);
    if (gain_one >= (uint32_t)(15.5*(1<<TX_ISP_GAIN_FIXED_POINT)))
        gain_one = (uint32_t)(15.5*(1<<TX_ISP_GAIN_FIXED_POINT));

    regs = gain_one>>(TX_ISP_GAIN_FIXED_POINT-4);
    mask = ~0;
    mask = mask >> (TX_ISP_GAIN_FIXED_POINT-4);
    mask = mask << (TX_ISP_GAIN_FIXED_POINT-4);
    gain_one1 = gain_one&mask;
    isp_gain1 = log2_fixed_to_fixed(gain_one1, TX_ISP_GAIN_FIXED_POINT, shift);
    *sensor_again = regs;
    //printk("info:  isp_gain = 0x%08x, isp_gain1 = 0x%08x, gain_one = 0x%08x, gain_one1 = 0x%08x, sensor_again = 0x%08x\n", isp_gain, isp_gain1, gain_one, gain_one1, regs);
    return isp_gain1;
#else
    unsigned int gain_one = 0;
    unsigned int gain_one1 = 0;
    uint16_t regs = 0;
    unsigned int isp_gain1 = 0;
    /* low 4 bits are fraction bits */
    gain_one = math_exp2(isp_gain, shift, TX_ISP_GAIN_FIXED_POINT);
    if (gain_one >= (uint32_t)(16*(1<<TX_ISP_GAIN_FIXED_POINT)))
        gain_one = (uint32_t)(16*(1<<TX_ISP_GAIN_FIXED_POINT));

    gain_one1 = tw9912_gainone_to_reg(gain_one, &regs);

    isp_gain1 = log2_fixed_to_fixed(gain_one1, TX_ISP_GAIN_FIXED_POINT, shift);
    *sensor_again = regs;
    return isp_gain1;

#endif
}

unsigned int tw9912_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

struct tx_isp_sensor_attribute tw9912_attr={
    .name            = "tw9912",
    .chip_id         = 0x9912,
    .cbus_type       = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask       = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
    .cbus_device     = 0x44,
    .dbus_type       = TX_SENSOR_DATA_INTERFACE_BT656,

    .max_again                      = 0,
    .max_dgain                      = 0,
    .min_integration_time           = 4,
    .min_integration_time_native    = 4,
    .max_integration_time_native    = 0x20d,    // 0x271 0x20d
    .integration_time_limit         = 0x20d,
    .total_width                    = 0x6c0,    // 0x6c0 0x6b4
    .total_height                   = 0x20d,
    .max_integration_time           = 0x20d,
    .integration_time_apply_delay   = 2,
    .again_apply_delay              = 2,
    .dgain_apply_delay              = 2,
    .sensor_ctrl.alloc_again        = tw9912_alloc_again,
    .sensor_ctrl.alloc_dgain        = tw9912_alloc_dgain,
    //    void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list tw9912_init_regs[] = {
    {0x01,0x78},
    {0x02,0x44},
    {0x03,0x20},
    {0x04,0x00},
    {0x05,0x1E},
    {0x06,0x03},
    {0x07,0x02},
    {0x08,0x12},
    {0x09,0xF0},
    {0x0A,0x14},
    {0x0B,0xD0},
    {0x0C,0xCC},
    {0x0D,0x15},
    {0x10,0x00},
    {0x11,0x64},
    {0x12,0x11},
    {0x13,0x80},
    {0x14,0x80},
    {0x15,0x00},
    {0x17,0x30},
    {0x18,0x44},
    {0x1A,0x10},
    {0x1B,0x00},
    {0x1C,0x07},
    {0x1D,0x7F},
    {0x1E,0x08},
    {0x1F,0x00},
    {0x20,0x50},
    {0x21,0x42},
    {0x22,0xF0},
    {0x23,0xD8},
    {0x24,0xBC},
    {0x25,0xB8},
    {0x26,0x44},
    {0x27,0x38},
    {0x28,0x00},
    {0x29,0x00},
    {0x2A,0x78},
    {0x2B,0x44},
    {0x2C,0x30},
    {0x2D,0x14},
    {0x2E,0xA5},
    {0x2F,0x26},
    {0x30,0x00},
    {0x31,0x10},
    {0x32,0xFF},
    {0x33,0x05},
    {0x34,0x1A},
    {0x35,0x00},
    {0x36,0xE2},
    {0x37,0x2D},
    {0x38,0x01},
    {0x40,0x00},
    {0x41,0x80},
    {0x42,0x00},
    {0xC0,0x01},
    {0xC1,0x07},
    {0xC2,0x01},
    {0xC3,0x03},
    {0xC4,0x5A},
    {0xC5,0x00},
    {0xC6,0x20},
    {0xC7,0x04},
    {0xC8,0x00},
    {0xC9,0x06},
    {0xCA,0x06},
    {0xCB,0x30},
    {0xCC,0x00},
    {0xCD,0x54},
    {0xD0,0x00},
    {0xD1,0xF0},
    {0xD2,0xF0},
    {0xD3,0xF0},
    {0xD4,0x00},
    {0xD5,0x00},
    {0xD6,0x10},
    {0xD7,0x70},
    {0xD8,0x00},
    {0xD9,0x04},
    {0xDA,0x80},
    {0xDB,0x80},
    {0xDC,0x20},
    {0xE0,0x00},
    {0xE1,0x49},
    {0xE2,0xD9},
    {0xE3,0x00},
    {0xE4,0x00},
    {0xE5,0x00},
    {0xE6,0x00},
    {0xE7,0x2A},
    {0xE8,0x0F},
    {0xE9,0x61},
    {TW9912_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list tw9912_init_progressive_525[] = {
    {0x01,0x68},
    {0x02,0x40},
    {0x03,0x20},
    {0x04,0x00},
    {0x05,0x1E},
    {0x06,0x03},
    {0x07,0x02},
    {0x08,0x16},
    {0x09,0xF0},
    {0x0A,0x0B},
    {0x0B,0xD0},
    {0x0C,0xCC},
    {0x0D,0x15},
    {0x10,0xE2},
    {0x11,0x82},
    {0x12,0x05},
    {0x13,0x80},
    {0x14,0x80},
    {0x15,0x00},
    {0x17,0x30},
    {0x18,0x44},
    {0x1A,0x10},
    {0x1B,0x00},
    {0x1C,0x0F},
    {0x1D,0x7F},
    {0x1E,0x08},
    {0x1F,0x00},
    {0x20,0x50},
    {0x21,0x42},
    {0x22,0xF0},
    {0x23,0xD8},
    {0x24,0xBC},
    {0x25,0xB8},
    {0x26,0x44},
    {0x27,0x38},
    {0x28,0x00},
    {0x29,0x00},
    {0x2A,0x78},
    {0x2B,0x44},
    {0x2C,0x30},
    {0x2D,0x14},
    {0x2E,0xA5},
    {0x2F,0x26},
    {0x30,0x00},
    {0x31,0x10},
    {0x32,0x00},
    {0x33,0x05},
    {0x34,0x1A},
    {0x35,0x00},
    {0x36,0x00},
    {0x37,0x2D},
    {0x38,0x01},
    {0x40,0x00},
    {0x41,0x80},
    {0x42,0x00},
    {0xC0,0x01},
    {0xC1,0x07},
    {0xC2,0x01},
    {0xC3,0x03},
    {0xC4,0x5A},
    {0xC5,0x00},
    {0xC6,0x20},
    {0xC7,0x04},
    {0xC8,0x00},
    {0xC9,0x06},
    {0xCA,0x06},
    {0xCB,0x30},
    {0xCC,0x00},
    {0xCD,0x54},
    {0xD0,0x00},
    {0xD1,0xF0},
    {0xD2,0xF0},
    {0xD3,0xF0},
    {0xD4,0x00},
    {0xD5,0x00},
    {0xD6,0x10},
    {0xD7,0x70},
    {0xD8,0x00},
    {0xD9,0x04},
    {0xDA,0x80},
    {0xDB,0x80},
    {0xDC,0x20},
    {0xE0,0x00},
    {0xE1,0x49},
    {0xE2,0xD9},
    {0xE3,0x00},
    {0xE4,0x00},
    {0xE5,0x00},
    {0xE6,0x00},
    {0xE7,0x2A},
    {0xE8,0x0F},
    {0xE9,0x61},
    {TW9912_REG_END, 0x00},    /* END MARKER */
};


/*
 * the order of the tw9912_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting tw9912_win_sizes[] = {
    /* 720*576 */
    {
        .width          = WIDTH,
        .height         = HEIGHT,
        .fps            = 25 << 16 | 1,
        .mbus_code      = V4L2_MBUS_FMT_UYVY8_2X8,
        .colorspace     = V4L2_COLORSPACE_SRGB,
        .regs           = tw9912_init_regs,
	//.regs           = tw9912_init_progressive_525,
    }
};

/*
 * the part of driver was fixed.
 */

static struct regval_list tw9912_stream_on[] = {
    {0x03, 0x20}, //bit[0-2]
    {TW9912_REG_END, 0x00},    /* END MARKER */
};


static struct regval_list tw9912_stream_off[] = {
    {0x03, 0x27}, //bit[0-2]
    {TW9912_REG_END, 0x27},    /* END MARKER */
};

static int tw9912_read(struct v4l2_subdev *sd, unsigned char reg, unsigned char *value)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = client->addr,
            .flags  = 0,
            .len    = 1,
            .buf    = &reg,
        },
        [1] = {
            .addr   = client->addr,
            .flags  = I2C_M_RD,
            .len    = 1,
            .buf    = value,
        }
    };

    int ret;
    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

    return ret;
}

static int tw9912_write(struct v4l2_subdev *sd, unsigned char reg, unsigned char value)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr   = client->addr,
        .flags  = 0,
        .len    = 2,
        .buf    = buf,
    };

    int ret;
    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0){
        ret = 0;
    }

    return ret;
}

static int tw9912_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg != TW9912_REG_END) {
        if (vals->reg == TW9912_REG_DELAY) {
            msleep(vals->value);
        }
        else {
            ret = tw9912_read(sd, vals->reg, &val);
            if (ret < 0){
                return ret;
            }
        }

        printk("reg:%#x value: %#x\n",vals->reg, val);
        vals++;
    }
    return 0;
}

static int tw9912_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;
    while(vals->reg != TW9912_REG_END) {
        if (vals->reg == TW9912_REG_DELAY) {
            msleep(vals->value);
        }
        else {
            ret = tw9912_write(sd, vals->reg, vals->value);
            if (ret < 0){
                return ret;
            }
        }
        /* printk("vals->reg:%x, vals->value:%x\n",vals->reg, vals->value); */
        vals++;
    }

    return 0;
}

static int tw9912_reset(struct v4l2_subdev *sd, u32 val)
{
    return 0;
}

static int tw9912_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
    unsigned char v=0;
    int ret;
    ret = tw9912_read(sd, 0x00, &v);
    printk("read value :0x%x\n",v);

    if (ret < 0)
        return ret;

    if (v != TW9912_CHIP_ID)
        return -ENODEV;
    *ident = v;

    return 0;
}

static int tw9912_set_integration_time(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int tw9912_set_analog_gain(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int tw9912_set_digital_gain(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int tw9912_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int tw9912_init(struct v4l2_subdev *sd, u32 enable)
{
    struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
    struct tx_isp_notify_argument arg;
    struct tx_isp_sensor_win_setting *wsize = &tw9912_win_sizes[0];
    int ret = 0;
    if(!enable)
        return ISP_SUCCESS;

    sensor->video.mbus.width    = wsize->width;
    sensor->video.mbus.height   = wsize->height;
    sensor->video.mbus.code     = wsize->mbus_code;
    sensor->video.mbus.field    = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps           = wsize->fps;

    ret = tw9912_write_array(sd, wsize->regs);
    if (ret){
        return ret;
    }

    tw9912_read_array(sd, wsize->regs);

    arg.value = (int)&sensor->video;
    sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
    sensor->priv = wsize;

    return 0;
}

static int tw9912_s_stream(struct v4l2_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = tw9912_write_array(sd, tw9912_stream_on);
        printk("tw9912 stream on\n");
    } else {
        ret = tw9912_write_array(sd, tw9912_stream_off);
        printk("tw9912 stream off\n");
    }

    return ret;
}

static int tw9912_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    return 0;
}

static int tw9912_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    return 0;
}

static int tw9912_set_fps(struct tx_isp_sensor *sensor, int fps)
{
    return 0;
}

static int tw9912_set_mode(struct tx_isp_sensor *sensor, int value)
{
    struct tx_isp_notify_argument arg;
    struct v4l2_subdev *sd = &sensor->sd;
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    wsize = &tw9912_win_sizes[0];

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

static int tw9912_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio,"tw9912_reset");
        if (!ret) {
            gpio_direction_output(reset_gpio, 1);
            msleep(20);
            gpio_direction_output(reset_gpio, 0);
            msleep(20);
            gpio_direction_output(reset_gpio, 1);
            msleep(1);
        } else {
            printk("gpio requrest fail %d\n", reset_gpio);
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio,"tw9912_pwdn");
        if (!ret) {
            gpio_direction_output(pwdn_gpio, 1);
            msleep(150);
            gpio_direction_output(pwdn_gpio, 0);
            msleep(10);
        } else {
            printk("gpio requrest fail %d\n", pwdn_gpio);
        }
    }

    ret = tw9912_detect(sd, &ident);
    if (ret) {
        v4l_err(client,
            "chip found @ 0x%x (%s) is not an tw9912 chip.\n",
            client->addr, client->adapter->name);
        return ret;
    }
    v4l_info(client, "tw9912 chip found @ 0x%02x (%s)\n", client->addr, client->adapter->name);

    return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int tw9912_s_power(struct v4l2_subdev *sd, int on)
{
    return 0;
}

static long tw9912_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
    struct v4l2_subdev *sd = &sensor->sd;
    long ret = 0;

    switch(ctrl->cmd) {
    case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
        ret = tw9912_set_integration_time(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
        ret = tw9912_set_analog_gain(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
        ret = tw9912_set_digital_gain(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
        ret = tw9912_get_black_pedestal(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
        ret = tw9912_set_mode(sensor,ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
        ret = tw9912_write_array(sd, tw9912_stream_off);
        break;
    case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
        ret = tw9912_write_array(sd, tw9912_stream_on);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
        ret = tw9912_set_fps(sensor, ctrl->value);
        break;
    default:
        break;;
    }

    return 0;
}

static long tw9912_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
    struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
    int ret;

    switch (cmd) {
    case VIDIOC_ISP_PRIVATE_IOCTL:
        ret = tw9912_ops_private_ioctl(sensor, arg);
        break;
    default:
        return -1;
        break;
    }

    return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int tw9912_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned char val = 0;
    int ret;

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;

    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    ret = tw9912_read(sd, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 2; // liangyh ???

    return ret;
}

static int tw9912_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;

    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    tw9912_write(sd, reg->reg & 0xff, reg->val & 0xff);

    return 0;
}
#endif

static const struct v4l2_subdev_core_ops tw9912_core_ops = {
    .g_chip_ident   = tw9912_g_chip_ident,
    .reset          = tw9912_reset,
    .init           = tw9912_init,
    .s_power        = tw9912_s_power,
    .ioctl          = tw9912_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
    .g_register     = tw9912_g_register,
    .s_register     = tw9912_s_register,
#endif
};

static const struct v4l2_subdev_video_ops tw9912_video_ops = {
    .s_stream       = tw9912_s_stream,
    .s_parm         = tw9912_s_parm,
    .g_parm         = tw9912_g_parm,
};

static const struct v4l2_subdev_ops tw9912_ops = {
    .core           = &tw9912_core_ops,
    .video          = &tw9912_video_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device tw9912_platform_device = {
    .name = "tw9912",
    .id = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources = 0,
};

static int tw9912_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct v4l2_subdev *sd;
    struct tx_isp_video_in *video;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &tw9912_win_sizes[0];
    int ret = -1;

    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if (!sensor) {
        printk("Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }

#ifndef MODULE
    tw9912_board_info = get_sensor_board_info(tw9912_platform_device.name);
    if (tw9912_board_info) {
        pwdn_gpio       = tw9912_board_info->gpios.gpio_power;
        reset_gpio      = tw9912_board_info->gpios.gpio_sensor_rst;
        gpio_i2c_sel1   = tw9912_board_info->gpios.gpio_i2c_sel1;
        gpio_i2c_sel2   = tw9912_board_info->gpios.gpio_i2c_sel2;
        sensor_gpio_func= tw9912_board_info->dvp_gpio_func;
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

    clk_set_rate(sensor->mclk, 27000000);
    clk_enable(sensor->mclk);
    ret = set_sensor_gpio_function(sensor_gpio_func);
    if (ret < 0)
        goto err_set_sensor_gpio;

    tw9912_attr.dvp.gpio = sensor_gpio_func;
    /*
     *(volatile unsigned int*)(0xb000007c) = 0x20000020;
     while(*(volatile unsigned int*)(0xb000007c) & (1 << 28));
    */

    /*
      convert sensor-gain into isp-gain,
    */
#ifdef TW9912_USE_AGAIN_ONLY
    tw9912_attr.max_again = 0x3f446;
#else
    tw9912_attr.max_again = 0x40000;
#endif
    tw9912_attr.max_dgain = 0;
    sd = &sensor->sd;
    video = &sensor->video;
    sensor->video.attr = &tw9912_attr;
    sensor->video.vi_max_width = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    v4l2_i2c_subdev_init(sd, client, &tw9912_ops);
    v4l2_set_subdev_hostdata(sd, sensor);
    return 0;

err_set_sensor_gpio:
    clk_disable(sensor->mclk);
    clk_put(sensor->mclk);
err_get_mclk:
    kfree(sensor);
    return -1;
}

static int tw9912_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    struct tx_isp_sensor *sensor = v4l2_get_subdev_hostdata(sd);

    if(reset_gpio != -1){
        gpio_free(reset_gpio);
    }

    if(pwdn_gpio != -1){
        gpio_free(pwdn_gpio);
    }

    clk_disable(sensor->mclk);
    clk_put(sensor->mclk);

    v4l2_device_unregister_subdev(sd);
    kfree(sensor);

    return 0;
}

static const struct i2c_device_id tw9912_id[] = {
    { "tw9912", 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, tw9912_id);

static struct i2c_driver tw9912_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = "tw9912",
    },
    .probe          = tw9912_probe,
    .remove         = tw9912_remove,
    .id_table       = tw9912_id,
};

static __init int init_tw9912(void)
{
    return i2c_add_driver(&tw9912_driver);
}

static __exit void exit_tw9912(void)
{
    i2c_del_driver(&tw9912_driver);
}

module_init(init_tw9912);
module_exit(exit_tw9912);

MODULE_DESCRIPTION("A low-level driver for OmniVision tw9912 sensors");
MODULE_LICENSE("GPL");
