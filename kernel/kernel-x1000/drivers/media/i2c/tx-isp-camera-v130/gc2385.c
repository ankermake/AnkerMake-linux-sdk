/*
 * gc2385 Camera Driver
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
#include <tx-isp/tx-isp-common.h>
#include <tx-isp/sensor-common.h>
#include <tx-isp/apical-isp/apical_math.h>

#define GC2385_CHIP_ID_H              (0x23)
#define GC2385_CHIP_ID_L              (0x85)
#define GC2385_SUPPORT_30FPS_SCLK     (78000000)
#define GC2385_SUPPORT_15FPS_SCLK     (38000000)
#define SENSOR_OUTPUT_MAX_FPS         30
#define SENSOR_OUTPUT_MIN_FPS         5
#define GC2385_MAX_WIDTH              1600
#define GC2385_MAX_HEIGHT             1200

#if defined CONFIG_GC2385_480X800
#define GC2385_WIDTH                  480
#define GC2385_HEIGHT                 800
#elif defined CONFIG_GC2385_720X1200
#define GC2385_WIDTH                  720
#define GC2385_HEIGHT                 1200
#elif defined CONFIG_GC2385_1600X1200
#define GC2385_WIDTH                  1600
#define GC2385_HEIGHT                 1200
#endif

/* SENSOR MIRROR FLIP INFO */
#define GC2385_MIRROR_FLIP_ENABLE    0
#if GC2385_MIRROR_FLIP_ENABLE
#define GC2385_MIRROR        0xd7
#define GC2385_STARTY        0x04
#define GC2385_STARTX        0x06
#define GC2385_BLK_Select_H  0x00
#define GC2385_BLK_Select_L  0x3c
#else
#define GC2385_MIRROR        0xd4
#define GC2385_STARTY        0x03
#define GC2385_STARTX        0x05
#define GC2385_BLK_Select_H  0x3c
#define GC2385_BLK_Select_L  0x00
#endif

#define SENSOR_VERSION                "H20200726"


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


struct tx_isp_sensor_attribute gc2385_attr;
#ifndef MODULE
static struct sensor_board_info *gc2385_board_info;
#endif

static unsigned int gc2385_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

    return 0;
}

static unsigned int gc2385_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

struct tx_isp_sensor_attribute gc2385_attr = {
    .name = "gc2385",
    .chip_id = 0x2385,
    .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
    .cbus_device = 0x37,
    .dbus_type = TX_SENSOR_DATA_INTERFACE_MIPI,
    .mipi = {
        .clk = 624,
        .lans = 1,
    },
    .max_again = 263489,
    .max_dgain = 0,
    .min_integration_time = 2,
    .min_integration_time_native = 2,
    .max_integration_time_native = 1240 - 4,
    .integration_time_limit = 1240 -4,
    .total_width = 2080,
    .total_height = 1240,
    .max_integration_time = 1240 - 4,
    .integration_time_apply_delay = 2,
    .again_apply_delay = 2,
    .dgain_apply_delay = 0,
    .sensor_ctrl.alloc_again = gc2385_alloc_again,
    .sensor_ctrl.alloc_dgain = gc2385_alloc_dgain,
};

/*
 * Actual_window_size=1600*1200,MIPI 1lane
 * MCLK=24Mhz,MIPI_clcok=624MHz,row_time=26.66us
 * Pixel_line=2080 line_frame=1240,frame_rate=30fps
 */
static struct camera_reg_op gc2385_init_regs_1600_1200_30fps_mipi[] = {
    /* system */
    {CAMERA_REG_OP_DATA, 0xfe, 0x00},
    {CAMERA_REG_OP_DATA, 0xfe, 0x00},
    {CAMERA_REG_OP_DATA, 0xfe, 0x00},
    {CAMERA_REG_OP_DATA, 0xf2, 0x02},
    {CAMERA_REG_OP_DATA, 0xf4, 0x03},
    {CAMERA_REG_OP_DATA, 0xf7, 0x01},
    {CAMERA_REG_OP_DATA, 0xf8, 0x28},
    {CAMERA_REG_OP_DATA, 0xf9, 0x02},
    {CAMERA_REG_OP_DATA, 0xfa, 0x08},
    {CAMERA_REG_OP_DATA, 0xfc, 0x8e},
    {CAMERA_REG_OP_DATA, 0xe7, 0xcc},
    {CAMERA_REG_OP_DATA, 0x88, 0x03},

    /* analog */
    {CAMERA_REG_OP_DATA, 0x03, 0x04}, //exp_in_h :1152
    {CAMERA_REG_OP_DATA, 0x04, 0x80}, //exp_in_l
    {CAMERA_REG_OP_DATA, 0x05, 0x02}, //HB_h :646
    {CAMERA_REG_OP_DATA, 0x06, 0x86}, //HB_l
    {CAMERA_REG_OP_DATA, 0x07, 0x00}, //VB_h :16
    {CAMERA_REG_OP_DATA, 0x08, 0x10}, //VB_l
    {CAMERA_REG_OP_DATA, 0x09, 0x00}, //row_start_h :4
    {CAMERA_REG_OP_DATA, 0x0a, 0x04}, //row_start_l
    {CAMERA_REG_OP_DATA, 0x0b, 0x00}, //col_start_h :2
    {CAMERA_REG_OP_DATA, 0x0c, 0x02}, //col_start_l
    {CAMERA_REG_OP_DATA, 0x17, GC2385_MIRROR}, //mirror & flip
    {CAMERA_REG_OP_DATA, 0x18, 0x02},
    {CAMERA_REG_OP_DATA, 0x19, 0x17},
    {CAMERA_REG_OP_DATA, 0x1c, 0x18},
    {CAMERA_REG_OP_DATA, 0x20, 0x73},
    {CAMERA_REG_OP_DATA, 0x21, 0x38},
    {CAMERA_REG_OP_DATA, 0x22, 0xa2},
    {CAMERA_REG_OP_DATA, 0x29, 0x20},
    {CAMERA_REG_OP_DATA, 0x2f, 0x14},
    {CAMERA_REG_OP_DATA, 0x3f, 0x40},
    {CAMERA_REG_OP_DATA, 0xcd, 0x94},
    {CAMERA_REG_OP_DATA, 0xce, 0x45},
    {CAMERA_REG_OP_DATA, 0xd1, 0x0c},
    {CAMERA_REG_OP_DATA, 0xd7, 0x9b},
    {CAMERA_REG_OP_DATA, 0xd8, 0x99},
    {CAMERA_REG_OP_DATA, 0xda, 0x3b},
    {CAMERA_REG_OP_DATA, 0xd9, 0xb5},
    {CAMERA_REG_OP_DATA, 0xdb, 0x75},
    {CAMERA_REG_OP_DATA, 0xe3, 0x1b},
    {CAMERA_REG_OP_DATA, 0xe4, 0xf8},

    /* BLk */
    {CAMERA_REG_OP_DATA, 0x40, 0x22},
    {CAMERA_REG_OP_DATA, 0x43, 0x07},
    {CAMERA_REG_OP_DATA, 0x4e, GC2385_BLK_Select_H},
    {CAMERA_REG_OP_DATA, 0x4f, GC2385_BLK_Select_L},
    {CAMERA_REG_OP_DATA, 0x68, 0x00},

    /* gain */
    {CAMERA_REG_OP_DATA, 0xb0, 0x46}, //global gain :88
    {CAMERA_REG_OP_DATA, 0xb1, 0x01}, //auto_pregain_h[3:0] :64
    {CAMERA_REG_OP_DATA, 0xb2, 0x00}, //auto_pregain_l[7:2]
    {CAMERA_REG_OP_DATA, 0xb6, 0x00}, //gain code[3:0]

    /* crop */
    {CAMERA_REG_OP_DATA, 0x90, 0x01}, //crop mode en
    {CAMERA_REG_OP_DATA, 0x91, (GC2385_MAX_HEIGHT-GC2385_HEIGHT)/2/256}, //win_y_h
    {CAMERA_REG_OP_DATA, 0x92, 8+(GC2385_MAX_HEIGHT-GC2385_HEIGHT)/2%256}, //win_y_l
    {CAMERA_REG_OP_DATA, 0x93, (GC2385_MAX_WIDTH-GC2385_WIDTH)/2/256}, //win_x_h
    {CAMERA_REG_OP_DATA, 0x94, 8+(GC2385_MAX_WIDTH-GC2385_WIDTH)/2%256}, //win_x_l
    {CAMERA_REG_OP_DATA, 0x95, GC2385_HEIGHT/256}, //win_height_h
    {CAMERA_REG_OP_DATA, 0x96, GC2385_HEIGHT%256}, //win_height_l
    {CAMERA_REG_OP_DATA, 0x97, GC2385_WIDTH/256}, //win_width_h
    {CAMERA_REG_OP_DATA, 0x98, GC2385_WIDTH%256}, //win_width_l

    /* mipi */
    {CAMERA_REG_OP_DATA, 0xfe, 0x00},
    {CAMERA_REG_OP_DATA, 0xed, 0x00},
    {CAMERA_REG_OP_DATA, 0xfe, 0x03},
    {CAMERA_REG_OP_DATA, 0x01, 0x03}, //phy lane0 en, phy clk en
    {CAMERA_REG_OP_DATA, 0x02, 0x82},
    {CAMERA_REG_OP_DATA, 0x03, 0xd0},
    {CAMERA_REG_OP_DATA, 0x04, 0x04},
    {CAMERA_REG_OP_DATA, 0x05, 0x00},
    {CAMERA_REG_OP_DATA, 0x06, 0x80},
    {CAMERA_REG_OP_DATA, 0x11, 0x2b}, //RAW8:0x2a RAW10:0x2b
    {CAMERA_REG_OP_DATA, 0x12, (GC2385_WIDTH*5/4)%256}, //LWC_set_l :RAW10:win_width*5/4
    {CAMERA_REG_OP_DATA, 0x13, (GC2385_WIDTH*5/4)/256}, //LWC_set_h
    {CAMERA_REG_OP_DATA, 0x15, 0x00},
    {CAMERA_REG_OP_DATA, 0x42, GC2385_WIDTH%256}, //buf_win_width_l
    {CAMERA_REG_OP_DATA, 0x43, GC2385_WIDTH/256}, //buf_win_width_h

    {CAMERA_REG_OP_DATA, 0x1b, 0x10},
    {CAMERA_REG_OP_DATA, 0x1c, 0x10},
    {CAMERA_REG_OP_DATA, 0x21, 0x08},
    {CAMERA_REG_OP_DATA, 0x22, 0x05},
    {CAMERA_REG_OP_DATA, 0x23, 0x13},
    {CAMERA_REG_OP_DATA, 0x24, 0x02},
    {CAMERA_REG_OP_DATA, 0x25, 0x13},
    {CAMERA_REG_OP_DATA, 0x26, 0x06},
    {CAMERA_REG_OP_DATA, 0x29, 0x06},
    {CAMERA_REG_OP_DATA, 0x2a, 0x08},
    {CAMERA_REG_OP_DATA, 0x2b, 0x06},
    {CAMERA_REG_OP_DATA, 0xfe, 0x00},
    {CAMERA_REG_OP_END, 0x0, 0x0},    /* END MARKER */
};

/*
 * the order of the gc2385_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting gc2385_win_sizes[] = {
    {
        .width        = GC2385_WIDTH,
        .height       = GC2385_HEIGHT,
        .fps          = 30 << 16 | 1,
        .mbus_code    = V4L2_MBUS_FMT_SBGGR10_1X10,
        .colorspace   = V4L2_COLORSPACE_SRGB,
        .regs         = gc2385_init_regs_1600_1200_30fps_mipi,
    }
};

static struct camera_reg_op gc2385_stream_on[] = {
    {CAMERA_REG_OP_DATA, 0xfe, 0x00},
    {CAMERA_REG_OP_DATA, 0xed, 0x90},
    {CAMERA_REG_OP_END,0x0,0x0},    /* END MARKER */
};

static struct camera_reg_op gc2385_stream_off[] = {
    {CAMERA_REG_OP_DATA, 0xfe, 0x00},
    {CAMERA_REG_OP_DATA, 0xef, 0x00},
    {CAMERA_REG_OP_END,0x0,0x0},    /* END MARKER */
};

static int gc2385_read(struct tx_isp_subdev *sd, unsigned char reg, unsigned char *value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    struct i2c_msg msg[2] = {
        [0] = {
            .addr  = client->addr,
            .flags = 0,
            .len   = 1,
            .buf   = &reg,
        },
        [1] = {
            .addr  = client->addr,
            .flags = I2C_M_RD,
            .len   = 1,
            .buf   = value,
        }
    };
    int ret;
    ret = private_i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;
    return ret;
}

static int gc2385_write(struct tx_isp_subdev *sd, unsigned char reg, unsigned char value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr  = client->addr,
        .flags = 0,
        .len   = 2,
        .buf   = buf,
    };
    int ret;
    ret = private_i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;
    return ret;
}

static inline int gc2385_read_array(struct tx_isp_subdev *sd, struct camera_reg_op *vals)
{
    int ret;
    unsigned char val;
    while (vals->flag != CAMERA_REG_OP_END) {
        if (vals->flag == CAMERA_REG_OP_DATA) {
            if (vals->reg == 0xfe) {
                ret = gc2385_write(sd, vals->reg, vals->val);
                if (ret < 0)
                    return ret;
                printk("%x[%x]\n", vals->reg, vals->val);
            } else {
                ret = gc2385_read(sd, vals->reg, &val);
                if (ret < 0)
                    return ret;
                printk("%x[%x]\n", vals->reg, val);
            }
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

static int gc2385_write_array(struct tx_isp_subdev *sd, struct camera_reg_op *vals)
{
    int ret;
    while (vals->flag != CAMERA_REG_OP_END) {
        if (vals->flag == CAMERA_REG_OP_DATA) {
            ret = gc2385_write(sd, vals->reg, vals->val);
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


static int gc2385_set_integation_time(struct tx_isp_subdev *sd, int value)
{

    return 0;
}

#define ANALOG_GAIN_1 64   /* 1.00x */
#define ANALOG_GAIN_2 92   /* 1.43x */
#define ANALOG_GAIN_3 127  /* 1.99x */
#define ANALOG_GAIN_4 183  /* 2.86x */
#define ANALOG_GAIN_5 257  /* 4.01x */
#define ANALOG_GAIN_6 369  /* 5.76x */
#define ANALOG_GAIN_7 531  /* 8.30x */
#define ANALOG_GAIN_8 750  /* 11.72x */
#define ANALOG_GAIN_9 1092 /* 17.06x */

static int write_gain_reg(struct tx_isp_subdev *sd, u16 val)
{
    gc2385_write(sd, 0xfe, 0x00);
    gc2385_write(sd, 0x20, 0x73);
    gc2385_write(sd, 0x22, 0xa2);
    // analog gain
    gc2385_write(sd, 0xb6, 0x00);
    gc2385_write(sd, 0xb1, val >> 8);
    gc2385_write(sd, 0xb2, val & 0xff);
}

static int gc2385_set_analog_gain(struct tx_isp_subdev *sd, u16 gain)
{
    u16 val, tmp;

    val = gain;
    if (val < 0x40)
        val = 0x40;
    if (val >= ANALOG_GAIN_1 && val < ANALOG_GAIN_2) {
        tmp = 256 * val / ANALOG_GAIN_1;
        write_gain_reg(sd, tmp);
    } else if (val >= ANALOG_GAIN_2 && val < ANALOG_GAIN_3) {
        tmp = 256 * val / ANALOG_GAIN_2;
        write_gain_reg(sd, tmp);
    } else if (val >= ANALOG_GAIN_3 && val < ANALOG_GAIN_4) {
        tmp = 256 * val / ANALOG_GAIN_3;
        write_gain_reg(sd, tmp);
    } else if (val >= ANALOG_GAIN_4 && val < ANALOG_GAIN_5) {
        tmp = 256 * val / ANALOG_GAIN_4;
        write_gain_reg(sd, tmp);
    } else if (val >= ANALOG_GAIN_5 && val < ANALOG_GAIN_6) {
        tmp = 256 * val / ANALOG_GAIN_5;
        write_gain_reg(sd, tmp);
    } else if (val >= ANALOG_GAIN_6 && val < ANALOG_GAIN_7) {
        tmp = 256 * val / ANALOG_GAIN_6;
        write_gain_reg(sd, tmp);
    } else if (val >= ANALOG_GAIN_7 && val < ANALOG_GAIN_8) {
        tmp = 256 * val / ANALOG_GAIN_7;
        write_gain_reg(sd, tmp);
    } else if (val >= ANALOG_GAIN_8 && val < ANALOG_GAIN_9) {
        tmp = 256 * val / ANALOG_GAIN_8;
        write_gain_reg(sd, tmp);
    } else {
        tmp = 256 * val / ANALOG_GAIN_9;
        write_gain_reg(sd, tmp);
    }

    return 0;
}

static int gc2385_set_digital_gain(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int gc2385_get_black_pedestal(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int gc2385_set_mode(struct tx_isp_subdev *sd, int value)
{

    return 0;
}

static int gc2385_set_fps(struct tx_isp_subdev *sd, int fps)
{

    return 0;
}

static int gc2385_set_vflip(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    int ret = 0;
    unsigned char val = 0xc0;

    ret = gc2385_write(sd, 0xfe, 0x00);
    if (enable) {
        val &= ~0x03;
        val |= 0x02;
        sensor->video.mbus.code = V4L2_MBUS_FMT_SGBRG10_1X10;
    } else {
        val &= 0xfd;
        sensor->video.mbus.code = V4L2_MBUS_FMT_SRGGB10_1X10;
    }
    ret += gc2385_write(sd, 0x17, val);

    if (!ret)
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return ret;
}

static int gc2385_sensor_ops_ioctl(struct tx_isp_subdev *sd, unsigned int cmd, void *arg)
{
    long ret = 0;

    if (IS_ERR_OR_NULL(sd)) {
        printk("%d the pointer is invaild.\n", __LINE__);
        return -EINVAL;
    }

    switch (cmd) {
        case TX_ISP_EVENT_SENSOR_INT_TIME:
            if (arg)
                ret = gc2385_set_integation_time(sd, *(int *)arg);
            break;
        case TX_ISP_EVENT_SENSOR_AGAIN:
            if (arg)
                ret = gc2385_set_analog_gain(sd, *(u16 *)arg);
            break;
        case TX_ISP_EVENT_SENSOR_DGAIN:
            if (arg)
                ret = gc2385_set_digital_gain(sd, *(int *)arg);
            break;
        case TX_ISP_EVENT_SENSOR_BLACK_LEVEL:
            if (arg)
                ret = gc2385_get_black_pedestal(sd, *(int *)arg);
            break;
        case TX_ISP_EVENT_SENSOR_RESIZE:
            if (arg)
                ret = gc2385_set_mode(sd, *(int *)arg);
            break;
        case TX_ISP_EVENT_SENSOR_PREPARE_CHANGE:
            ret = gc2385_write_array(sd, gc2385_stream_off);
            break;
        case TX_ISP_EVENT_SENSOR_FINISH_CHANGE:
            ret = gc2385_write_array(sd, gc2385_stream_on);
            break;
        case TX_ISP_EVENT_SENSOR_FPS:
            if (arg)
                ret = gc2385_set_fps(sd, *(int *)arg);
            break;
        case TX_ISP_EVENT_SENSOR_VFLIP:
            if (arg)
                ret = gc2385_set_vflip(sd, *(int *)arg);
            break;
        default: break;
    }
    return 0;
}

static int gc2385_detect(struct tx_isp_subdev *sd, unsigned int *ident)
{
    unsigned char v;
    int ret;

    ret = gc2385_read(sd, 0xf0, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;
    if (v != GC2385_CHIP_ID_H)
        return -ENODEV;
    *ident = v;

    ret = gc2385_read(sd, 0xf1, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;
    if (v != GC2385_CHIP_ID_L)
        return -ENODEV;

    *ident = (*ident << 8) | v;

    return 0;
}

static int gc2385_g_chip_ident(struct tx_isp_subdev *sd, struct tx_isp_chip_ident *chip)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gc2385_power");
        if (!ret) {
            private_gpio_direction_output(power_gpio, 1);
            private_msleep(50);
        } else {
            printk("gpio request fail %d\n", power_gpio);
        }
    }

    if (i2c_sel1_gpio != -1) {
        int ret = gpio_request(i2c_sel1_gpio, "gc2385_i2c_sel1");
        if (!ret) {
            private_gpio_direction_output(i2c_sel1_gpio, 1);
        } else {
            printk("gpio request fail %d\n", i2c_sel1_gpio);
        }
    }

    if (pwdn_gpio != -1) {
        ret = private_gpio_request(pwdn_gpio, "gc2385_pwdn");
        if (!ret) {
            private_gpio_direction_output(pwdn_gpio, 0);
            private_msleep(10);
            private_gpio_direction_output(pwdn_gpio, 1);
            private_msleep(20);

        } else {
            printk("gpio request fail %d\n", pwdn_gpio);
        }
    }

    if (reset_gpio != -1) {
        ret = private_gpio_request(reset_gpio, "gc2385_reset");
        if (!ret) {
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(5);
            private_gpio_direction_output(reset_gpio, 0);
            private_msleep(30);
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(35);
        } else {
            printk("gpio request fail %d\n", reset_gpio);
        }
    }
    ret = gc2385_detect(sd, &ident);
    if (ret) {
        printk("chip found @ 0x%x (%s) is not an gc2385 chip.\n",
                client->addr, client->adapter->name);
        return ret;
    }
    printk("gc2385 chip found @ 0x%02x (%s)\n", client->addr, client->adapter->name);
    if (chip) {
        memcpy(chip->name, "gc2385", sizeof("gc2385"));
        chip->ident = ident;
        chip->revision = SENSOR_VERSION;
    }

    return 0;
}

static int gc2385_reset(struct tx_isp_subdev *sd, int val)
{
    return 0;
}

static int gc2385_init(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = &gc2385_win_sizes[0];
    int ret = 0;

    if (!enable)
        return ISP_SUCCESS;

    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;
    ret = gc2385_write_array(sd, wsize->regs);
    if (ret)
        return ret;

    //gc2385_read_array(sd, wsize->regs);
    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    sensor->priv = wsize;

    return 0;
}

static int gc2385_g_register(struct tx_isp_subdev *sd, struct tx_isp_dbg_register *reg)
{
    unsigned char val = 0;
    int len = 0;
    int ret = 0;

    len = strlen(sd->chip.name);
    if (len && strncmp(sd->chip.name, reg->name, len)) {
        return -EINVAL;
    }
    if (!private_capable(CAP_SYS_ADMIN));
        return -EPERM;
    ret = gc2385_read(sd, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;

    return ret;
}

static int gc2385_s_register(struct tx_isp_subdev *sd, struct tx_isp_dbg_register *reg)
{
    int len = 0;

    len = strlen(sd->chip.name);
    if(len && strncmp(sd->chip.name, reg->name, len)){
        return -EINVAL;
    }
    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;
    gc2385_write(sd, reg->reg & 0xff, reg->val & 0xff);

    return 0;
}

static int gc2385_s_stream(struct tx_isp_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = gc2385_write_array(sd, gc2385_stream_on);
        pr_debug("gc2385 stream on.\n");
    } else {
        ret = gc2385_write_array(sd, gc2385_stream_off);
        pr_debug("gc2385 stream off.\n");
    }
    return ret;
}

static struct tx_isp_subdev_core_ops gc2385_core_ops = {
    .g_chip_ident = gc2385_g_chip_ident,
    .reset        = gc2385_reset,
    .init         = gc2385_init,
    .g_register   = gc2385_g_register,
    .s_register   = gc2385_s_register,
};

static struct tx_isp_subdev_video_ops gc2385_video_ops = {
    .s_stream = gc2385_s_stream,
};

static struct tx_isp_subdev_sensor_ops gc2385_sensor_ops = {
    .ioctl = gc2385_sensor_ops_ioctl,
};

static struct tx_isp_subdev_ops gc2385_ops = {
    .core   = &gc2385_core_ops,
    .video  = &gc2385_video_ops,
    .sensor = &gc2385_sensor_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device gc2385_platform_device = {
    .name = "gc2385",
    .id = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources = 0,
};

static int gc2385_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct tx_isp_subdev *sd;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &gc2385_win_sizes[0];

    printk("gc2385_probe......\n");
    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if (!sensor) {
        printk("failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }
    memset(sensor, 0, sizeof(*sensor));

#ifndef MODULE
    gc2385_board_info = get_sensor_board_info(gc2385_platform_device.name);
    if (gc2385_board_info) {
        power_gpio    = gc2385_board_info->gpios.gpio_power;
        reset_gpio    = gc2385_board_info->gpios.gpio_sensor_rst;
        pwdn_gpio     = gc2385_board_info->gpios.gpio_sensor_pwdn;
        i2c_sel1_gpio = gc2385_board_info->gpios.gpio_i2c_sel1;
    }
#endif

    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk("cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }
    private_clk_set_rate(sensor->mclk, 24000000);
    private_clk_enable(sensor->mclk);

    // convert sensor-gain into isp-gain
    gc2385_attr.max_again = 0;
    gc2385_attr.max_dgain = 0;
    sd = &sensor->sd;
    sensor->video.attr = &gc2385_attr;
    sensor->video.vi_max_width = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;
    sensor->video.dc_param = NULL;
    tx_isp_subdev_init(&gc2385_platform_device, sd, &gc2385_ops);
    tx_isp_set_subdevdata(sd, client);
    tx_isp_set_subdev_hostdata(sd, sensor);
    private_i2c_set_clientdata(client, sd);

    return 0;
err_get_mclk:
    kfree(sensor);
    return -1;
}

static int gc2385_remove(struct i2c_client *client)
{
    struct tx_isp_subdev *sd = private_i2c_get_clientdata(client);
    struct tx_isp_sensor *sensor = tx_isp_get_subdev_hostdata(sd);

    if (power_gpio != -1) {
        private_gpio_direction_output(power_gpio, 0);
        private_gpio_free(power_gpio);
    }
    if (i2c_sel1_gpio != -1)
        private_gpio_free(i2c_sel1_gpio);
    if (pwdn_gpio != -1)
        private_gpio_free(pwdn_gpio);
    if (reset_gpio != -1)
        private_gpio_free(reset_gpio);

    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
    tx_isp_subdev_deinit(sd);
    kfree(sensor);

    return 0;
}

static const struct i2c_device_id gc2385_id[] = {
    { "gc2385", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, gc2385_id);

static struct i2c_driver gc2385_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name  = "gc2385",
    },
    .probe    = gc2385_probe,
    .remove   = gc2385_remove,
    .id_table = gc2385_id,
};

static __init int init_gc2385(void)
{
    int ret = 0;

    ret = private_driver_get_interface();
    if (ret) {
        printk("failed to init gc2385 driver.\n");
        return -1;
    }

    return private_i2c_add_driver(&gc2385_driver);
}

static __exit void exit_gc2385(void)
{
    private_i2c_del_driver(&gc2385_driver);
}

module_init(init_gc2385);
module_exit(exit_gc2385);

MODULE_DESCRIPTION("A low-level driver for Gcoreinc gc2375a sensors");
MODULE_LICENSE("GPL");