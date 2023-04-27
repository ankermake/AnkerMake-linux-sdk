/*
 * ar0144 Camera Driver
 *
 * Copyright (C) 2019, Ingenic Semiconductor Inc.
 *
 * Authors: Jeff <jifu.liu@ingenic.com>
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
#include <tx-isp/tx-isp-common.h>
#include <tx-isp/sensor-common.h>
#include <linux/sensor_board.h>

#include <soc/gpio.h>

#define AR0144_CHIP_ID_H                 (0x03)
#define AR0144_CHIP_ID_L                 (0x56)
#define AR0144_CHIP_ID                   (0x0356)
#define AR0144_CHIP_ID_REG               (0x3000)

#define AR0144_REG_END                   0xffff
#define AR0144_DELAY_MS                  0xfffe


#define SENSOR_OUTPUT_MAX_FPS            30
#define SENSOR_OUTPUT_MIN_FPS            5
#define AR0144_WIDTH                    1280
#define AR0144_HEIGHT                   800

#define ar0144_SUPPORT_PCLK             (48*1000*1000)
#define SENSOR_VERSION                  "H20191106"

struct regval_list {
    unsigned short reg_num;
    unsigned short value;
};

static struct sensor_board_info *ar0144_board_info;

static int reset_gpio = -1;
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int power_gpio = -1;
module_param(power_gpio, int, S_IRUGO);
MODULE_PARM_DESC(power_gpio, "power_gpio GPIO NUM");

static int i2c_sel1_gpio = -1;
module_param(i2c_sel1_gpio, int, S_IRUGO);
MODULE_PARM_DESC(i2c_sel1_gpio, "i2c_sel1_gpio GPIO NUM");

static int sensor_gpio_func = -1;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
    unsigned char value;
    unsigned int gain;
};

struct again_lut ar0144_again_lut[] = {
    { 0x0, 0 },
    { 0x1, 2794 },
    { 0x2, 6397 },
    { 0x3, 9011 },
    { 0x4, 12388 },
    { 0x5, 16447 },
    { 0x6, 19572 },
    { 0x7, 23340 },
    { 0x8, 26963 },
    { 0x9, 31135 },
    { 0xa, 35780 },
    { 0xb, 39588 },
    { 0xc, 44438 },
    { 0xd, 49051 },
    { 0xe, 54517 },
    { 0xf, 59685 },
    { 0x10, 65536 },
    { 0x12, 71490 },
    { 0x14, 78338 },
    { 0x16, 85108 },
    { 0x18, 92854 },
    { 0x1a, 100992 },
    { 0x1c, 109974 },
    { 0x1e, 120053 },
    { 0x20, 131072 },
    { 0x22, 137247 },
    { 0x24, 143667 },
    { 0x26, 150644 },
    { 0x28, 158212 },
    { 0x2a, 166528 },
    { 0x2c, 175510 },
    { 0x2e, 185457 },
    { 0x30, 196608 },
    { 0x32, 202783 },
    { 0x34, 209203 },
    { 0x36, 216276 },
    { 0x38, 223748 },
    { 0x3a, 232064 },
    { 0x3c, 241046 },
    { 0x3e, 250993 },
    { 0x40, 262144 },
};

struct tx_isp_sensor_attribute ar0144_attr;

static unsigned int ar0144_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = ar0144_again_lut;

    while(lut->gain <= ar0144_attr.max_again) {
        if(isp_gain == 0) {
            *sensor_again = 0;
            return 0;

        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;

        } else {
            if((lut->gain == ar0144_attr.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

unsigned int ar0144_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

struct tx_isp_sensor_attribute ar0144_attr={
    .name                                 = "ar0144",
    .chip_id                              = 0x0356,
    .cbus_type                            = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask                            = V4L2_SBUS_MASK_SAMPLE_16BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device                          = 0x18,
    .dbus_type                            = TX_SENSOR_DATA_INTERFACE_DVP,
    .dvp = {
        .mode = SENSOR_DVP_HREF_MODE,
        .blanking = {
            .vblanking = 0,
            .hblanking = 0,
        },
    },
    .max_again                            = 262144,
    .max_dgain                            = 0,
    .min_integration_time                 = 1,
    .min_integration_time_native          = 1,
    .max_integration_time_native          = 0x033B,
    .integration_time_limit               = 0x033B,
    .total_width                          = 0x078E,
    .total_height                         = 0x033B,
    .max_integration_time                 = 0x033B,
    .integration_time_apply_delay         = 2,
    .again_apply_delay                    = 1,
    .dgain_apply_delay                    = 1,
    .sensor_ctrl.alloc_again              = ar0144_alloc_again,
    .sensor_ctrl.alloc_dgain              = ar0144_alloc_dgain,
    //    void priv; /* point to struct tx_isp_sensor_board_info */
};


static struct regval_list ar0144_init_regs_1280_800_30fps[] = {
    {0x301A, 0x00D9},
    {AR0144_DELAY_MS, 100},
    {0x301A, 0x30D8},
    {0x3088, 0x8000},
    {0x3086, 0x327F},
    {0x3086, 0x5780},
    {0x3086, 0x2730},
    {0x3086, 0x7E13},
    {0x3086, 0x8000},
    {0x3086, 0x157E},
    {0x3086, 0x1380},
    {0x3086, 0x000F},
    {0x3086, 0x8190},
    {0x3086, 0x1643},
    {0x3086, 0x163E},
    {0x3086, 0x4522},
    {0x3086, 0x0937},
    {0x3086, 0x8190},
    {0x3086, 0x1643},
    {0x3086, 0x167F},
    {0x3086, 0x9080},
    {0x3086, 0x0038},
    {0x3086, 0x7F13},
    {0x3086, 0x8023},
    {0x3086, 0x3B7F},
    {0x3086, 0x9345},
    {0x3086, 0x0280},
    {0x3086, 0x007F},
    {0x3086, 0xB08D},
    {0x3086, 0x667F},
    {0x3086, 0x9081},
    {0x3086, 0x923C},
    {0x3086, 0x1635},
    {0x3086, 0x7F93},
    {0x3086, 0x4502},
    {0x3086, 0x8000},
    {0x3086, 0x7FB0},
    {0x3086, 0x8D66},
    {0x3086, 0x7F90},
    {0x3086, 0x8182},
    {0x3086, 0x3745},
    {0x3086, 0x0236},
    {0x3086, 0x8180},
    {0x3086, 0x4416},
    {0x3086, 0x3143},
    {0x3086, 0x7416},
    {0x3086, 0x787B},
    {0x3086, 0x7D45},
    {0x3086, 0x023D},
    {0x3086, 0x6445},
    {0x3086, 0x0A3D},
    {0x3086, 0x647E},
    {0x3086, 0x1281},
    {0x3086, 0x8037},
    {0x3086, 0x7F10},
    {0x3086, 0x450A},
    {0x3086, 0x3F74},
    {0x3086, 0x7E10},
    {0x3086, 0x7E12},
    {0x3086, 0x0F3D},
    {0x3086, 0xD27F},
    {0x3086, 0xD480},
    {0x3086, 0x2482},
    {0x3086, 0x9C03},
    {0x3086, 0x430D},
    {0x3086, 0x2D46},
    {0x3086, 0x4316},
    {0x3086, 0x5F16},
    {0x3086, 0x532D},
    {0x3086, 0x1660},
    {0x3086, 0x404C},
    {0x3086, 0x2904},
    {0x3086, 0x2984},
    {0x3086, 0x81E7},
    {0x3086, 0x816F},
    {0x3086, 0x170A},
    {0x3086, 0x81E7},
    {0x3086, 0x7F81},
    {0x3086, 0x5C0D},
    {0x3086, 0x5749},
    {0x3086, 0x5F53},
    {0x3086, 0x2553},
    {0x3086, 0x274D},
    {0x3086, 0x2BF8},
    {0x3086, 0x1016},
    {0x3086, 0x4C09},
    {0x3086, 0x2BB8},
    {0x3086, 0x2B98},
    {0x3086, 0x4E11},
    {0x3086, 0x5367},
    {0x3086, 0x4001},
    {0x3086, 0x605C},
    {0x3086, 0x095C},
    {0x3086, 0x1B40},
    {0x3086, 0x0245},
    {0x3086, 0x0045},
    {0x3086, 0x8029},
    {0x3086, 0xB67F},
    {0x3086, 0x8040},
    {0x3086, 0x047F},
    {0x3086, 0x8841},
    {0x3086, 0x095C},
    {0x3086, 0x0B29},
    {0x3086, 0xB241},
    {0x3086, 0x0C40},
    {0x3086, 0x0340},
    {0x3086, 0x135C},
    {0x3086, 0x0341},
    {0x3086, 0x1117},
    {0x3086, 0x125F},
    {0x3086, 0x2B90},
    {0x3086, 0x2B80},
    {0x3086, 0x816F},
    {0x3086, 0x4010},
    {0x3086, 0x4101},
    {0x3086, 0x5327},
    {0x3086, 0x4001},
    {0x3086, 0x6029},
    {0x3086, 0xA35F},
    {0x3086, 0x4D1C},
    {0x3086, 0x1702},
    {0x3086, 0x81E7},
    {0x3086, 0x2983},
    {0x3086, 0x4588},
    {0x3086, 0x4021},
    {0x3086, 0x7F8A},
    {0x3086, 0x4039},
    {0x3086, 0x4580},
    {0x3086, 0x2440},
    {0x3086, 0x087F},
    {0x3086, 0x885D},
    {0x3086, 0x5367},
    {0x3086, 0x2992},
    {0x3086, 0x8810},
    {0x3086, 0x2B04},
    {0x3086, 0x8916},
    {0x3086, 0x5C43},
    {0x3086, 0x8617},
    {0x3086, 0x0B5C},
    {0x3086, 0x038A},
    {0x3086, 0x484D},
    {0x3086, 0x4E2B},
    {0x3086, 0x804C},
    {0x3086, 0x0B41},
    {0x3086, 0x9F81},
    {0x3086, 0x6F41},
    {0x3086, 0x1040},
    {0x3086, 0x0153},
    {0x3086, 0x2740},
    {0x3086, 0x0160},
    {0x3086, 0x2983},
    {0x3086, 0x2943},
    {0x3086, 0x5C05},
    {0x3086, 0x5F4D},
    {0x3086, 0x1C81},
    {0x3086, 0xE745},
    {0x3086, 0x0281},
    {0x3086, 0x807F},
    {0x3086, 0x8041},
    {0x3086, 0x0A91},
    {0x3086, 0x4416},
    {0x3086, 0x092F},
    {0x3086, 0x7E37},
    {0x3086, 0x8020},
    {0x3086, 0x307E},
    {0x3086, 0x3780},
    {0x3086, 0x2015},
    {0x3086, 0x7E37},
    {0x3086, 0x8020},
    {0x3086, 0x0343},
    {0x3086, 0x164A},
    {0x3086, 0x0A43},
    {0x3086, 0x160B},
    {0x3086, 0x4316},
    {0x3086, 0x8F43},
    {0x3086, 0x1690},
    {0x3086, 0x4316},
    {0x3086, 0x7F81},
    {0x3086, 0x450A},
    {0x3086, 0x4130},
    {0x3086, 0x7F83},
    {0x3086, 0x5D29},
    {0x3086, 0x4488},
    {0x3086, 0x102B},
    {0x3086, 0x0453},
    {0x3086, 0x2D40},
    {0x3086, 0x3045},
    {0x3086, 0x0240},
    {0x3086, 0x087F},
    {0x3086, 0x8053},
    {0x3086, 0x2D89},
    {0x3086, 0x165C},
    {0x3086, 0x4586},
    {0x3086, 0x170B},
    {0x3086, 0x5C05},
    {0x3086, 0x8A60},
    {0x3086, 0x4B91},
    {0x3086, 0x4416},
    {0x3086, 0x0915},
    {0x3086, 0x3DFF},
    {0x3086, 0x3D87},
    {0x3086, 0x7E3D},
    {0x3086, 0x7E19},
    {0x3086, 0x8000},
    {0x3086, 0x8B1F},
    {0x3086, 0x2A1F},
    {0x3086, 0x83A2},
    {0x3086, 0x7E11},
    {0x3086, 0x7516},
    {0x3086, 0x3345},
    {0x3086, 0x0A7F},
    {0x3086, 0x5380},
    {0x3086, 0x238C},
    {0x3086, 0x667F},
    {0x3086, 0x1381},
    {0x3086, 0x8414},
    {0x3086, 0x8180},
    {0x3086, 0x313D},
    {0x3086, 0x6445},
    {0x3086, 0x2A3D},
    {0x3086, 0xD27F},
    {0x3086, 0x4480},
    {0x3086, 0x2494},
    {0x3086, 0x3DFF},
    {0x3086, 0x3D4D},
    {0x3086, 0x4502},
    {0x3086, 0x7FD0},
    {0x3086, 0x8000},
    {0x3086, 0x8C66},
    {0x3086, 0x7F90},
    {0x3086, 0x8194},
    {0x3086, 0x3F44},
    {0x3086, 0x1681},
    {0x3086, 0x8416},
    {0x3086, 0x2C2C},
    {0x3086, 0x2C2C},
    {0x3F00, 0x0005},
    {0x3ED6, 0x3CB1},
    {0x3EDA, 0xBADE},
    {0x3EDA, 0xBAEE},
    {0x3ED6, 0x3CB5},
    {0x3F00, 0x0A05},
    {0x3F00, 0xAA05},
    {0x3F00, 0xAA05},
    {0x3EDA, 0xBCEE},
    {0x3EDA, 0xCCEE},
    {0x3EF8, 0x6542},
    {0x3EF8, 0x6522},
    {0x3EFA, 0x4442},
    {0x3EFA, 0x4422},
    {0x3EFA, 0x4222},
    {0x3EFA, 0x2222},
    {0x3EFC, 0x4446},
    {0x3EFC, 0x4466},
    {0x3EFC, 0x4666},
    {0x3EFC, 0x6666},
    {0x3EEA, 0xAA09},
    {0x3EE2, 0x180E},
    {0x3EE4, 0x0808},
    {0x3060, 0x000E},
    {0x3EEA, 0x2A09},
    {AR0144_DELAY_MS, 100},
    {0x3268, 0x0037},
    {0x3092, 0x00CF},
    {0x3786, 0x0006},
    {0x3F4A, 0x0F70},
    {0x3092, 0x00CF},
    {0x3786, 0x0006},
    {0x3268, 0x0036},
    {0x3268, 0x0034},
    {0x3268, 0x0030},
    {0x3064, 0x1802},
    {0x306E, 0x5010},
    {0x306E, 0x4810},
    {0x3EF6, 0x8001},
    {0x3EF6, 0x8041},
    {0x3180, 0xC08F},
    {0x302A, 0x0006},
    {0x302C, 0x0002},
    {0x302E, 0x0001},
    {0x3030, 0x000C},//24M  0x0018
    {0x3036, 0x000C},
    {0x3038, 0x0001},
    {0x30B0, 0x4000},
    {AR0144_DELAY_MS, 100},
    {0x3002, 0x0000},
    {0x3004, 0x0004},
    {0x3006, 0x031F},
    {0x3008, 0x0503},
    {0x300A, 0x033B},//vts
    {0x300C, 0x078E},// 0x078E
    {0x3012, 0x00F8},
    {0x31AC, 0x0C0C},
    {0x31C6, 0x8000},
    {0x306E, 0x9010},
    {0x30A2, 0x0001},
    {0x30A6, 0x0001},
    {0x3082, 0x0003},
    {0x3040, 0xC000},
    {0x31AE, 0x0200},
    //{ AR0144_FLASH,0x0100U },//flash_out
    //{0x301D, 0x0003},//FLIP
    {0x3028, 0x0010},
    //{0x301A, 0x19D8},
    {0x301A, 0x19C8},
    {AR0144_REG_END, 0x00},
};

/*
 * the order of the ar0144_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ar0144_win_sizes[] = {
    /* 1280*800 */
    {
        .width                     = AR0144_WIDTH,
        .height                    = AR0144_HEIGHT,
        .fps                       = SENSOR_OUTPUT_MAX_FPS << 16 | 1, /* 12.5 fps */
        .mbus_code                 = V4L2_MBUS_FMT_SGRBG8_1X8,
        .colorspace                = V4L2_COLORSPACE_SRGB,
        .regs                      = ar0144_init_regs_1280_800_30fps,
    }
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ar0144_stream_on[] = {
    {0x301A, 0x19CC},
    {AR0144_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list ar0144_stream_off[] = {
    {0x301A, 0x19C8},
    {AR0144_REG_END, 0x00},    /* END MARKER */
};


static int ar0144_read(struct tx_isp_subdev *sd, uint16_t reg,
        unsigned char *value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned char buf[2] = {reg >> 8, reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr    = client->addr,
            .flags   = 0,
            .len     = 2,
            .buf     = buf,
        },
        [1] = {
            .addr    = client->addr,
            .flags   = I2C_M_RD,
            .len     = 2,
            .buf     = value,
        }
    };

    int ret;
    ret = private_i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

    return ret;

}

static int ar0144_write(struct tx_isp_subdev *sd, uint16_t reg,
        unsigned short value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    uint8_t buf[4] = {reg >> 8, reg & 0xff, value >> 8, value & 0xff};
    struct i2c_msg msg = {
        .addr     = client->addr,
        .flags    = 0,
        .len      = 4,
        .buf      = buf,
    };

    int ret;
    ret = private_i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;

}

static inline int ar0144_read_array(struct tx_isp_subdev *sd, struct regval_list *vals)
{
    int ret;
    unsigned char val[2];
    uint16_t value;

    while (vals->reg_num != AR0144_REG_END) {
        if (vals->reg_num == AR0144_DELAY_MS) {
            private_msleep(vals->value);
        } else {
            ret = ar0144_read(sd, vals->reg_num, val);
            if (ret < 0)
                return ret;
        }

        value=(val[0]<<8)|val[1];
        printk("reg_num:0x%x, val:0x%x.\n",vals->reg_num,value);
        vals++;
    }
    return 0;
}


static int ar0144_write_array(struct tx_isp_subdev *sd, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != AR0144_REG_END) {
        if (vals->reg_num == AR0144_DELAY_MS) {
             private_msleep(vals->value);
             printk("msleep %d.\n",vals->value);
        } else {
            ret = ar0144_write(sd, vals->reg_num, vals->value);
            printk("reg_num 0x%x,value 0x%x.\n",vals->reg_num,vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }
    return 0;
}


static int ar0144_reset(struct tx_isp_subdev *sd, int val)
{
    return 0;
}

static int ar0144_detect(struct tx_isp_subdev *sd, unsigned int *ident)
{
    unsigned char v[2]={0};
    int ret;

    ret = ar0144_read(sd, 0x3000, v);

    if (ret < 0)
        return ret;

    if (v[0] != AR0144_CHIP_ID_H)
        return -ENODEV;

    if (v[1] != AR0144_CHIP_ID_L)
        return -ENODEV;

    return 0;

}

static int ar0144_set_integration_time(struct tx_isp_subdev *sd, int value)
{
    int ret;

    ret = ar0144_write(sd,0x3012, value);
    if (ret < 0)
        return ret;

    return 0;
}

static int ar0144_set_analog_gain(struct tx_isp_subdev *sd, int value)
{
    int ret;

    ret = ar0144_write(sd,0x3060, value);
    if (ret < 0)
        return ret;

    return 0;
}

static int ar0144_set_digital_gain(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int ar0144_get_black_pedestal(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int ar0144_init(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor                 = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize     = &ar0144_win_sizes[0];
    int ret = 0;

    if(!enable)
        return ISP_SUCCESS;

    sensor->video.mbus.width                      = wsize->width;
    sensor->video.mbus.height                     = wsize->height;
    sensor->video.mbus.code                       = wsize->mbus_code;
    sensor->video.mbus.field                      = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace                 = wsize->colorspace;
    sensor->video.fps                             = wsize->fps;

    ret = ar0144_write_array(sd, wsize->regs);
    ret= ar0144_read_array(sd,wsize->regs);
    if (ret)
        return ret;

    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    sensor->priv = wsize;

    return 0;
}

static int ar0144_s_stream(struct tx_isp_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = ar0144_write_array(sd, ar0144_stream_on);
        printk("ar0144 stream on\n");
    }
    else {
        ret = ar0144_write_array(sd, ar0144_stream_off);
        printk("ar0144 stream off\n");
    }

    return ret;
}

static int ar0144_set_fps(struct tx_isp_subdev *sd, int fps)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    unsigned int pclk = ar0144_SUPPORT_PCLK;
    unsigned short hts=0;
    unsigned short vts = 0;
    unsigned char tmp[2];
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
        return -1;
    ret += ar0144_read(sd, 0x300C, tmp);
    hts=tmp[0];
    if(ret < 0)
        return -1;

    hts = (hts << 8) + tmp[1];
    /*vts = (pclk << 4) / (hts * (newformat >> 4));*/
    vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = ar0144_write(sd, 0x300A, vts);
    if(ret < 0)
        return -1;

    sensor->video.fps = fps;
    sensor->video.attr->max_integration_time_native = vts - 4;
    sensor->video.attr->integration_time_limit = vts - 4;
    sensor->video.attr->total_height = vts;
    sensor->video.attr->max_integration_time = vts - 4;
    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return 0;
}

static int ar0144_set_mode(struct tx_isp_subdev *sd, int value)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
        wsize = &ar0144_win_sizes[0];
    }else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
        wsize = &ar0144_win_sizes[0];
    }

    if(wsize){
        sensor->video.mbus.width                 = wsize->width;
        sensor->video.mbus.height                = wsize->height;
        sensor->video.mbus.code                  = wsize->mbus_code;
        sensor->video.mbus.field                 = V4L2_FIELD_NONE;
        sensor->video.mbus.colorspace            = wsize->colorspace;
        sensor->video.fps = wsize->fps;
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
        if(sensor->priv != wsize){
            ret = ar0144_write_array(sd, wsize->regs);
            if(!ret)
                sensor->priv = wsize;
        }
        sensor->video.fps = wsize->fps;
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    }

    return ret;
}


static int ar0144_set_vflip(struct tx_isp_subdev *sd, int enable)
{
    return 0;
}

static int ar0144_g_chip_ident(struct tx_isp_subdev *sd,
        struct tx_isp_chip_ident *chip)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    printk(KERN_ERR "[ar0144]Enter ar0144_g_chip_ident for ar0144.\n");

    if(reset_gpio != -1){
        ret = private_gpio_request(reset_gpio,"ar0144_reset");
        if(!ret){
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(20);
            private_gpio_direction_output(reset_gpio, 0);
            private_msleep(20);
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(1);
        }else{
            printk("gpio requrest fail %d\n",reset_gpio);
        }
    }
    if (pwdn_gpio != -1) {
        ret = private_gpio_request(pwdn_gpio, "ar0144_pwdn");
        if(!ret){
            private_gpio_direction_output(pwdn_gpio, 1);
            private_msleep(150);
            //private_gpio_direction_output(pwdn_gpio, 0);
            //private_msleep(10);
        }else{
            printk("gpio requrest fail %d\n", pwdn_gpio);
        }
    }

    if (power_gpio != -1) {
        ret = private_gpio_request(power_gpio, "ar0144_power");
        if(!ret){
            private_gpio_direction_output(power_gpio, 1);
            private_msleep(150);
        }else{
            printk("gpio requrest fail %d\n", power_gpio);
        }
    }

    ret = ar0144_detect(sd, &ident);
    if (ret) {
        printk("chip found @ 0x%x (%s) is not an ar0144 chip.\n",
                client->addr, client->adapter->name);

        return ret;
    }

    printk("ar0144 chip found @ 0x%02x (%s)\n", client->addr, client->adapter->name);
    if(chip){
        memcpy(chip->name, "ar0144", sizeof("ar0144"));
        chip->ident = ident;
        chip->revision = SENSOR_VERSION;
    }

    return 0;
}

static int ar0144_sensor_ops_ioctl(struct tx_isp_subdev *sd, unsigned int cmd, void *arg)
{
    long ret = 0;

    if(IS_ERR_OR_NULL(sd)){
        printk("[%d]The pointer is invalid!\n", __LINE__);

        return -EINVAL;
    }

    switch(cmd){
        case TX_ISP_EVENT_SENSOR_INT_TIME:
            if(arg)
                ret = ar0144_set_integration_time(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_AGAIN:
            if(arg)
                ret = ar0144_set_analog_gain(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_DGAIN:
            if(arg)
                ret = ar0144_set_digital_gain(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_BLACK_LEVEL:
            if(arg)
                ret = ar0144_get_black_pedestal(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_RESIZE:
            if(arg)
                ret = ar0144_set_mode(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_PREPARE_CHANGE:
            ret = ar0144_write_array(sd, ar0144_stream_off);
            break;
        case TX_ISP_EVENT_SENSOR_FINISH_CHANGE:
            ret = ar0144_write_array(sd, ar0144_stream_on);
            break;
        case TX_ISP_EVENT_SENSOR_FPS:
            if(arg)
                ret = ar0144_set_fps(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_VFLIP:
            if(arg)
                ret = ar0144_set_vflip(sd, *(int*)arg);
            break;
        default:
            break;;
    }

    return 0;
}

static int ar0144_g_register(struct tx_isp_subdev *sd, struct tx_isp_dbg_register *reg)
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

    ret = ar0144_read(sd, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;

    return ret;
}

static int ar0144_s_register(struct tx_isp_subdev *sd, const struct tx_isp_dbg_register *reg)
{
    int len = 0;

    len = strlen(sd->chip.name);
    if(len && strncmp(sd->chip.name, reg->name, len)){
        return -EINVAL;
    }

    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;

    ar0144_write(sd, reg->reg & 0xffff, reg->val & 0xff);

    return 0;
}

static struct tx_isp_subdev_core_ops ar0144_core_ops = {
    .g_chip_ident              = ar0144_g_chip_ident,
    .reset                     = ar0144_reset,
    .init                      = ar0144_init,
    .g_register                = ar0144_g_register,
    .s_register                = ar0144_s_register,
};


static struct tx_isp_subdev_video_ops ar0144_video_ops = {
    .s_stream                  = ar0144_s_stream,
};

static struct tx_isp_subdev_sensor_ops    ar0144_sensor_ops = {
    .ioctl                     = ar0144_sensor_ops_ioctl,
};

static struct tx_isp_subdev_ops ar0144_ops = {
    .core                      = &ar0144_core_ops,
    .video                     = &ar0144_video_ops,
    .sensor                    = &ar0144_sensor_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;

struct platform_device ar0144_platform_device = {
    .name                      = "ar0144",
    .id                        = -1,
    .dev = {
        .dma_mask              = &tx_isp_module_dma_mask,
        .coherent_dma_mask     = 0xffffffff,
        .platform_data         = NULL,
    },
    .num_resources             = 0,
};

static int ar0144_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    struct tx_isp_subdev *sd;
    struct tx_isp_video_in *video;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &ar0144_win_sizes[0];
    int ret;

    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if(!sensor){
        printk(KERN_ERR "[ar0144]Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }

#ifndef MODULE
    ar0144_board_info = get_sensor_board_info(ar0144_platform_device.name);
    if (ar0144_board_info) {
        power_gpio          = ar0144_board_info->gpios.gpio_power;
        reset_gpio          = ar0144_board_info->gpios.gpio_sensor_rst;
        i2c_sel1_gpio       = ar0144_board_info->gpios.gpio_i2c_sel1;
        pwdn_gpio           = ar0144_board_info->gpios.gpio_sensor_pwdn;
        sensor_gpio_func    = ar0144_board_info->dvp_gpio_func;
    }
#endif

    memset(sensor, 0 ,sizeof(*sensor));
    /* request mclk of sensor */
    sensor->mclk = clk_get(NULL, "cgu_cim");
    ret = set_sensor_gpio_function(sensor_gpio_func);
    if (ret < 0)
    {
        printk(KERN_ERR "set_sensor_gpio_function failed.\n");
        goto err_set_sensor_gpio;
    }

    ar0144_attr.dvp.gpio = sensor_gpio_func;

    if (IS_ERR(sensor->mclk)) {
        printk(KERN_ERR "[ar0144]Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }

    clk_set_rate(sensor->mclk, 24000000);
    clk_enable(sensor->mclk);

/*
 *  convert sensor-gain into isp-gain,
 */

    ar0144_attr.max_again                  = 262144;
    ar0144_attr.max_dgain                  = 0;
    sd                                     = &sensor->sd;
    video                                  = &sensor->video;
    sensor->video.attr                     = &ar0144_attr;
    sensor->video.mbus_change              = 0;
    sensor->video.vi_max_width             = wsize->width;
    sensor->video.vi_max_height            = wsize->height;
    sensor->video.mbus.width               = wsize->width;
    sensor->video.mbus.height              = wsize->height;
    sensor->video.mbus.code                = wsize->mbus_code;
    sensor->video.mbus.field               = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace          = wsize->colorspace;
    sensor->video.fps                      = wsize->fps;

    tx_isp_subdev_init(&ar0144_platform_device, sd, &ar0144_ops);
    tx_isp_set_subdevdata(sd, client);
    tx_isp_set_subdev_hostdata(sd, sensor);
    private_i2c_set_clientdata(client, sd);
    printk(KERN_ERR "[ar0144]@@@@@@@probe ok ------->ar0144.\n");

    return 0;

err_set_sensor_gpio:
    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
err_get_mclk:
    kfree(sensor);

    return -1;
}

static int ar0144_remove(struct i2c_client *client)
{
    struct tx_isp_subdev *sd = private_i2c_get_clientdata(client);
    struct tx_isp_sensor *sensor = tx_isp_get_subdev_hostdata(sd);

    if(power_gpio != -1) {
        private_gpio_direction_output(power_gpio, 0);
        private_gpio_free(power_gpio);
    }
    if(i2c_sel1_gpio != -1)
        private_gpio_free(i2c_sel1_gpio);

    if(reset_gpio != -1)
        private_gpio_free(reset_gpio);
    if(pwdn_gpio != -1)
        private_gpio_free(pwdn_gpio);

    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
    tx_isp_subdev_deinit(sd);
    kfree(sensor);

    return 0;
}

static const struct i2c_device_id ar0144_id[] = {
    { "ar0144", 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, ar0144_id);

static struct i2c_driver ar0144_driver = {
    .driver = {
        .owner        = THIS_MODULE,
        .name         = "ar0144",
    },
    .probe            = ar0144_probe,
    .remove           = ar0144_remove,
    .id_table         = ar0144_id,
};

static __init int init_ar0144(void)
{
    int ret = 0;
    ret = private_driver_get_interface();
    if(ret){
        printk("Failed to init ar0144 dirver.\n");
        return -1;
    }

    return private_i2c_add_driver(&ar0144_driver);
}

static __exit void exit_ar0144(void)
{
    private_i2c_del_driver(&ar0144_driver);
}

module_init(init_ar0144);
module_exit(exit_ar0144);

MODULE_DESCRIPTION("A low-level driver for Onsemi ar0144 sensors");
MODULE_LICENSE("GPL");
