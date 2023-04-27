/*
 * sc132gs.c
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
#include <linux/proc_fs.h>
#include <utils/gpio.h>

#include <tx-isp/tx-isp-common.h>
#include <tx-isp/sensor-common.h>


#define SC132GS_CHIP_ID_H                (0x01)
#define SC132GS_CHIP_ID_L                (0x32)

#define SC132GS_REG_END                  0xffff
#define SC132GS_REG_DELAY                0xfffe

#define SC132GS_SUPPORT_PCLK_FPS     (108*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS           30

#define SENSOR_OUTPUT_MIN_FPS           5
#define DRIVE_CAPABILITY_1

static int reset_gpio       = -1;//GPIO_PA(8)
static int pwdn_gpio        = -1;

struct tx_isp_sensor_attribute sc132gs_attr;


module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);


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

struct again_lut sc132gs_again_lut[] = {
    {0x320, 0},
    {0x321, 2886},
    {0x322, 5776},
    {0x323, 8494},
    {0x324, 11136},
    {0x325, 13706},
    {0x326, 16287},
    {0x327, 18723},
    {0x328, 21097},
    {0x329, 23413},
    {0x32a, 25746},
    {0x32b, 27952},
    {0x32c, 30108},
    {0x32d, 32216},
    {0x32e, 34344},
    {0x32f, 36361},
    {0x330, 38335},
    {0x331, 40269},
    {0x332, 42225},
    {0x333, 44082},
    {0x334, 45903},
    {0x335, 47689},
    {0x336, 49499},
    {0x337, 51220},
    {0x338, 52910},
    {0x339, 54570},
    {0x2320, 56253},
    {0x2321, 59130},
    {0x2322, 61970},
    {0x2323, 64680},
    {0x2324, 67360},
    {0x2325, 69967},
    {0x2326, 72460},
    {0x2327, 74932},
    {0x2328, 77340},
    {0x2329, 79649},
    {0x232a, 81942},
    {0x232b, 84180},
    {0x232c, 86329},
    {0x232d, 88467},
    {0x232e, 90522},
    {0x232f, 92568},
    {0x2330, 94571},
    {0x2331, 96499},
    {0x2332, 98421},
    {0x2333, 100305},
    {0x2334, 102121},
    {0x2335, 103933},
    {0x2336, 105711},
    {0x2337, 107427},
    {0x2338, 109141},
    {0x2339, 110825},
    {0x233a, 112451},
    {0x233b, 114077},
    {0x233c, 115648},
    {0x233d, 117221},
    {0x233e, 118768},
    {0x233f, 120264},
    {0x2720, 121762},
    {0x2721, 124665},
    {0x2722, 127505},
    {0x2723, 130239},
    {0x2724, 132895},
    {0x2725, 135480},
    {0x2726, 138017},
    {0x2727, 140467},
    {0x2728, 142855},
    {0x2729, 145204},
    {0x272a, 147477},
    {0x272b, 149696},
    {0x272c, 151864},
    {0x272d, 154002},
    {0x272e, 156075},
    {0x272f, 158103},
    {0x2730, 160106},
    {0x2731, 162051},
    {0x2732, 163956},
    {0x2733, 165824},
    {0x2734, 167672},
    {0x2735, 169468},
    {0x2736, 171231},
    {0x2737, 172962},
    {0x2738, 174676},
    {0x2739, 176345},
    {0x273a, 177986},
    {0x273b, 179612},
    {0x273c, 181197},
    {0x273d, 182756},
    {0x273e, 184290},
    {0x273f, 185812},
    {0x2f20, 187297},
    {0x2f21, 190212},
    {0x2f22, 193028},
    {0x2f23, 195774},
    {0x2f24, 198430},
    {0x2f25, 201026},
    {0x2f26, 203541},
    {0x2f27, 206002},
    {0x2f28, 208400},
    {0x2f29, 210729},
    {0x2f2a, 213012},
    {0x2f2b, 215231},
    {0x2f2c, 217409},
    {0x2f2d, 219528},
    {0x2f2e, 221610},
    {0x2f2f, 223638},
    {0x2f30, 225633},
    {0x2f31, 227586},
    {0x2f32, 229491},
    {0x2f33, 231367},
    {0x2f34, 233199},
    {0x2f35, 235003},
    {0x2f36, 236766},
    {0x2f37, 238504},
    {0x2f38, 240211},
    {0x2f39, 241880},
    {0x2f3a, 243528},
    {0x2f3b, 245140},
    {0x2f3c, 246732},
    {0x2f3d, 248291},
    {0x2f3e, 249831},
    {0x2f3f, 251340},
    {0x3f20, 252832},
    {0x3f21, 255741},
    {0x3f22, 258563},
    {0x3f23, 261303},
};


unsigned int sc132gs_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = sc132gs_again_lut;

    while(lut->gain <= sc132gs_attr.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == sc132gs_attr.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

unsigned int sc132gs_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

//important modify every time
struct tx_isp_sensor_attribute sc132gs_attr={
    .name                               = "sc132gs",
    .chip_id                            = 0x0132,
    .cbus_type                          = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask                          = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device                        = 0x30,
    .dbus_type                          = TX_SENSOR_DATA_INTERFACE_MIPI,
    .mipi = {
        .clk  = 480, //Todo - PLL Need Calc
        .lans = 2,
    },
    .max_again                          = 261303,
    .max_dgain                          = 0,
    .min_integration_time               = 0,
    .min_integration_time_native        = 0,
    .max_integration_time_native        = 0x960 - 8,
    .integration_time_limit             = 0x960 - 8,
    .total_width                        = 0x5dc,
    .total_height                       = 0x960,
    .max_integration_time               = 0x960 - 8,
    .one_line_expr_in_us                = 12,
    .integration_time_apply_delay       = 2,
    .again_apply_delay                  = 2,
    .dgain_apply_delay                  = 2,
    .sensor_ctrl.alloc_again            = sc132gs_alloc_again,
    .sensor_ctrl.alloc_dgain            = sc132gs_alloc_dgain,
};



static struct regval_list sc132gs_init_regs_1072_1280_30fps[] = {

    {0x0103,0x01},
    {0x0100,0x00},

    //PLL bypass
    {0x36e9,0x80},
    {0x36f9,0x80},

    {0x3018,0x32},
    {0x3019,0x0c},
    {0x301a,0xb4},
    {0x3031,0x0a},
    {0x3032,0x60},
    {0x3038,0x44},
    {0x3207,0x17},
    {0x3208,0x04},
    {0x3209,0x30},
    {0x320c,0x05},    //hts
    {0x320d,0xdc},
    {0x320e,0x09},    //vts
    {0x320f,0x60},
    {0x3250,0xcc},
    {0x3251,0x02},
    {0x3252,0x09},
    {0x3253,0x5b},
    {0x3254,0x05},
    {0x3255,0x3b},
    {0x3306,0x78},
    {0x330a,0x00},
    {0x330b,0xc8},
    {0x330f,0x24},
    {0x3314,0x80},
    {0x3315,0x40},
    {0x3317,0xf0},
    {0x331f,0x12},
    {0x3364,0x00},
    {0x3385,0x41},
    {0x3387,0x41},
    {0x3389,0x09},
    {0x33ab,0x00},
    {0x33ac,0x00},
    {0x33b1,0x03},
    {0x33b2,0x12},
    {0x33f8,0x02},
    {0x33fa,0x01},
    {0x3409,0x08},
    {0x34f0,0xc0},
    {0x34f1,0x20},
    {0x34f2,0x03},
    {0x3622,0xf5},
    {0x3630,0x5c},
    {0x3631,0x80},
    {0x3632,0xc8},
    {0x3633,0x32},
    {0x3638,0x2a},
    {0x3639,0x07},
    {0x363b,0x48},
    {0x363c,0x83},
    {0x363d,0x10},
    {0x36ea,0x38},
    {0x36fa,0x25},
    {0x36fb,0x05},
    {0x36fd,0x04},
    {0x3900,0x11},
    {0x3901,0x05},
    {0x3902,0xc5},
    {0x3904,0x04},
    {0x3908,0x91},
    {0x391e,0x00},
    {0x3e01,0x53},
    {0x3e02,0xe0},
    {0x3e03,0x0b},    //again mode
    {0x3e09,0x20},
    {0x3e0e,0xd2},
    {0x3e14,0xb0},
    {0x3e1e,0x7c},
    {0x3e26,0x20},
    {0x4418,0x38},
    {0x4503,0x10},
    {0x4837,0x21},
//    {0x4800,0x20},// mipi clk is'not series
    {0x5000,0x0e},
    {0x540c,0x51},
    {0x550f,0x38},
    {0x5780,0x67},
    {0x5784,0x10},
    {0x5785,0x06},
    {0x5787,0x02},
    {0x5788,0x00},
    {0x5789,0x00},
    {0x578a,0x02},
    {0x578b,0x00},
    {0x578c,0x00},
    {0x5790,0x00},
    {0x5791,0x00},
    {0x5792,0x00},
    {0x5793,0x00},
    {0x5794,0x00},
    {0x5795,0x00},
    {0x5799,0x04},

    //PLL set
    {0x36e9,0x20},
    {0x36f9,0x24},

//    [gain<2]
    {0x33fa,0x01},
    {0x3317,0xf0},

//    [gain>=2]
    {0x33fa,0x02},
    {0x3317,0x0a},

//    {0x0100,0x01},        //stream on
//    {SC132GS_REG_DELAY,0x17},    //delay
    {0x0100,0x00},        //stream off
    {SC132GS_REG_END, 0x0}, /* END MARKER */
};

/*
 * the order of the sc132gs_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting sc132gs_win_sizes[] = {
    {
        .width                  = 1072,
        .height                 = 1280,
        .fps                    = 30 << 16 | 1,
        .mbus_code              = V4L2_MBUS_FMT_SBGGR10_1X10,
        .colorspace             = V4L2_COLORSPACE_SRGB,
        .regs                   = sc132gs_init_regs_1072_1280_30fps,
    },
};

/*
 *     stream  switch
 */
static struct regval_list sc132gs_stream_on[] = {
    {0x0100, 0x01},
    {SC132GS_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list sc132gs_stream_off[] = {
    {0x0100, 0x00},
    {SC132GS_REG_END, 0x00},    /* END MARKER */
};

static int sc132gs_read(struct v4l2_subdev *sd, uint16_t reg, unsigned char *value)
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

static int sc132gs_write(struct v4l2_subdev *sd, uint16_t reg, unsigned char value)
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

static inline int sc132gs_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != SC132GS_REG_END) {

        if (vals->reg_num == SC132GS_REG_DELAY) {
            msleep(vals->value);
        } else {
            ret = sc132gs_read(sd, vals->reg_num, &val);
            if (ret < 0)
                return ret;
            printk("0x%x 0x%x\n",vals->reg_num,val);
        }
        vals++;
    }

    return 0;
}

static int sc132gs_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != SC132GS_REG_END) {
        if (vals->reg_num == SC132GS_REG_DELAY) {
            msleep(vals->value);
        } else {
            ret = sc132gs_write(sd, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int sc132gs_reset(struct v4l2_subdev *sd, unsigned int val)
{
    return 0;
}

static int sc132gs_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
    int ret;
    unsigned char v;

    ret = sc132gs_read(sd, 0x3107, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != SC132GS_CHIP_ID_H)
        return -ENODEV;
    *ident = v;

    ret = sc132gs_read(sd, 0x3108, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != SC132GS_CHIP_ID_L)
        return -ENODEV;

    *ident = (*ident << 8) | v;

    return 0;
}

static int sc132gs_set_integration_time(struct v4l2_subdev *sd, int value)
{
    int ret = 0;

    ret = sc132gs_write(sd, 0x3e02, (unsigned char)(value & 0xff));
    if (ret < 0)
        return ret;

    ret = sc132gs_write(sd, 0x3e01, (unsigned char)((value >> 8 ) &0xff));
    if (ret < 0)
        return ret;

    ret = sc132gs_write(sd, 0x3e00, (unsigned char)((value >> 16) & 0x0f));

    if (ret < 0)
        return ret;

    return 0;
}

static int sc132gs_set_analog_gain(struct v4l2_subdev *sd, int value)
{
    int ret = 0;

    ret = sc132gs_write(sd, 0x3e09, (unsigned char)(value & 0xff));
    if (ret < 0)
        return ret;

    ret = sc132gs_write(sd, 0x3e08, (unsigned char)(value >> 8));
    if (ret < 0)
        return ret;

//	printk("------(%x): 0x3e09[%x] 0x3e08[%x]\n", value, (unsigned char)(value & 0xff), (unsigned char)(value >> 8));

    return 0;
}

static int sc132gs_set_digital_gain(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int sc132gs_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int sc132gs_init(struct v4l2_subdev *sd, unsigned int enable)
{
    struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
    struct tx_isp_notify_argument arg;
    struct tx_isp_sensor_win_setting *wsize = sc132gs_win_sizes;
    int ret = 0;

    if(!enable)
        return ISP_SUCCESS;

    sensor->video.mbus.width    = wsize->width;
    sensor->video.mbus.height   = wsize->height;
    sensor->video.mbus.code     = wsize->mbus_code;
    sensor->video.mbus.field    = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps           = wsize->fps;

    ret = sc132gs_write_array(sd, wsize->regs);
//    sc132gs_read_array(sd, wsize->regs);

    if (ret)
        return ret;

    arg.value = (int)&sensor->video;
    sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
    sensor->priv = wsize;

    return 0;
}

static int sc132gs_s_stream(struct v4l2_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = sc132gs_write_array(sd, sc132gs_stream_on);
        pr_debug("sc132gs stream on\n");
    } else {
        ret = sc132gs_write_array(sd, sc132gs_stream_off);
        pr_debug("sc132gs stream off\n");
    }

    return ret;
}

static int sc132gs_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    return 0;
}

static int sc132gs_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    return 0;
}

static int sc132gs_set_fps(struct tx_isp_sensor *sensor, int fps)
{

    struct v4l2_subdev *sd = &sensor->sd;
    struct tx_isp_notify_argument arg;
    unsigned int pclk = 0;
    unsigned short hts;
    unsigned short vts = 0;
    unsigned char tmp;
    unsigned int newformat = 0;
    unsigned int max_fps = 0;
    int ret = 0;

    pclk = SC132GS_SUPPORT_PCLK_FPS;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk(KERN_ERR "sc132: fps(%d) no in range\n", fps);
        return -1;
    }

    ret = sc132gs_read(sd, 0x320c, &tmp);
    hts = tmp;
    ret += sc132gs_read(sd, 0x320d, &tmp);
    if(ret < 0)
        return -1;
    hts = ((hts << 8) + tmp);    //HTS

    vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = sc132gs_write(sd, 0x320e, (unsigned char)(vts >> 8));
    ret += sc132gs_write(sd, 0x320f, (unsigned char)(vts & 0xff));    //VTS
    if(ret < 0){
        printk("err: sc132gs_write err\n");
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

static int sc132gs_set_mode(struct tx_isp_sensor *sensor, int value)
{
    struct tx_isp_notify_argument arg;
    struct v4l2_subdev *sd = &sensor->sd;
    struct tx_isp_sensor_win_setting *wsize = sc132gs_win_sizes;
    int ret = ISP_SUCCESS;

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

static int sc132gs_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio,"sc132gs_reset");
        if (!ret) {
            gpio_direction_output(reset_gpio, 1);
            msleep(10);
            gpio_direction_output(reset_gpio, 0);
            msleep(10);
            gpio_direction_output(reset_gpio, 1);
            msleep(1);
        } else {
            printk(KERN_ERR "sc132: gpio requrest fail %d\n",reset_gpio);
            return -1;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "sc132gs_pwdn");
        if (!ret) {
            gpio_direction_output(pwdn_gpio, 0);
            msleep(50);
            gpio_direction_output(pwdn_gpio, 1);
            msleep(10);
        } else {
            printk(KERN_ERR "sc132: gpio requrest fail %d\n", pwdn_gpio);
            return -1;
        }
    }
    ret = sc132gs_detect(sd, &ident);
    if (ret) {
        v4l_err(client,
            "sc132: chip found @ 0x%x (%s) is not an sc132gs chip.\n",
            client->addr, client->adapter->name);
        return ret;
    }
    v4l_info(client, "sc132gs chip found @ 0x%02x (%s)\n",
         client->addr, client->adapter->name);

    return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int sc132gs_s_power(struct v4l2_subdev *sd, int on)
{
    return 0;
}

static long sc132gs_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
    struct v4l2_subdev *sd = &sensor->sd;
    long ret = 0;

    switch(ctrl->cmd){
        case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
            //printk("sc132gs_set_integration_time %x \n",ctrl->value);
            ret = sc132gs_set_integration_time(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
            //printk("sc132gs_set_analog_gain %x \n",ctrl->value);
            ret = sc132gs_set_analog_gain(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
            //printk("%d %d\n",ctrl->value,__LINE__);
            ret = sc132gs_set_digital_gain(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
            //printk("%d %d\n",ctrl->value,__LINE__);
            ret = sc132gs_get_black_pedestal(sd, ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
            //printk("%d %d\n",ctrl->value,__LINE__);
            ret = sc132gs_set_mode(sensor,ctrl->value);
            break;
        case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
            ret = sc132gs_write_array(sd, sc132gs_stream_off);
            break;
        case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
            ret = sc132gs_write_array(sd, sc132gs_stream_on);
            break;
        case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
            //printk("%d %d\n",ctrl->value,__LINE__);
            ret = sc132gs_set_fps(sensor, ctrl->value);
            break;
        default:
            pr_debug("do not support ctrl->cmd ====%d\n",ctrl->cmd);
            break;;
    }

    return ret;
}

static long sc132gs_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
    struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
    int ret;

    switch(cmd){
        case VIDIOC_ISP_PRIVATE_IOCTL:
            ret = sc132gs_ops_private_ioctl(sensor, arg);
            break;
        default:
            return -1;
    }

    return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sc132gs_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned char val = 0;
    int ret;

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;
    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    ret = sc132gs_read(sd, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    //printk("%d %s %llx %llx\n",__LINE__,__func__, reg->reg, reg->val);
    return ret;
}

static int sc132gs_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;
    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    sc132gs_write(sd, reg->reg & 0xffff, reg->val & 0xff);
    //printk("%d %s %llx %llx\n",__LINE__,__func__, reg->reg, reg->val);

    return 0;
}
#endif

static const struct v4l2_subdev_core_ops sc132gs_core_ops = {
    .g_chip_ident   = sc132gs_g_chip_ident,
    .reset          = sc132gs_reset,
    .init           = sc132gs_init,
    .s_power        = sc132gs_s_power,
    .ioctl          = sc132gs_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
    .g_register     = sc132gs_g_register,
    .s_register     = sc132gs_s_register,
#endif
};

static const struct v4l2_subdev_video_ops sc132gs_video_ops = {
    .s_stream       = sc132gs_s_stream,
    .s_parm         = sc132gs_s_parm,
    .g_parm         = sc132gs_g_parm,
};

static const struct v4l2_subdev_ops sc132gs_ops = {
    .core           = &sc132gs_core_ops,
    .video          = &sc132gs_video_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device sc132gs_platform_device = {
    .name = "sc132gs",
    .id = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources = 0,
};

static int sc132gs_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct v4l2_subdev *sd;
    struct tx_isp_video_in *video;
    struct tx_isp_sensor *sensor;
    struct clk *isp_clk=NULL;
    struct tx_isp_sensor_win_setting *wsize = sc132gs_win_sizes;
    int ret = 0;

    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if(!sensor){
        printk(KERN_ERR "sc132: Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }

    ret = tx_isp_clk_set(MD_X1520_SC132_ISPCLK);
    if (ret < 0) {
        printk(KERN_ERR "sc132: Cannot set isp clock\n");
        goto err_get_mclk;
    }
    /* request mclk of sensor */
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk(KERN_ERR "sc132: Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }
    clk_set_rate(sensor->mclk, 24000000);
    clk_enable(sensor->mclk);

    /* request isp clk */
    isp_clk = clk_get(NULL, "cgu_isp");
    if (IS_ERR(isp_clk)) {
        printk(KERN_ERR "sc132: Cannot get input clock cgu_isp\n");
        goto err_get_isp_clk;
    }
    clk_set_rate(isp_clk, 200*1000000);    //200M

    /*
      convert sensor-gain into isp-gain,
    */
    sc132gs_attr.max_again       = 261303;
    sc132gs_attr.max_dgain       = 0; //sc132gs_attr.max_dgain;
    sd                          = &sensor->sd;
    video                       = &sensor->video;
    sensor->video.attr          = &sc132gs_attr;
    sensor->video.vi_max_width  = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    v4l2_i2c_subdev_init(sd, client, &sc132gs_ops);
    v4l2_set_subdev_hostdata(sd, sensor);

    pr_debug("@@@@@@@probe ok ------->sc132gs\n");
    return 0;

err_get_mclk:
    kfree(sensor);
err_get_isp_clk:
    kfree(sensor);

    return -1;
}

static int sc132gs_remove(struct i2c_client *client)
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

static const struct i2c_device_id sc132gs_id[] = {
    { "sc132gs", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sc132gs_id);

static struct i2c_driver sc132gs_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = "sc132gs",
    },
    .probe          = sc132gs_probe,
    .remove         = sc132gs_remove,
    .id_table       = sc132gs_id,
};

static __init int init_sc132gs(void)
{
    return i2c_add_driver(&sc132gs_driver);
}

static __exit void exit_sc132gs(void)
{
    i2c_del_driver(&sc132gs_driver);
}

module_init(init_sc132gs);
module_exit(exit_sc132gs);

MODULE_DESCRIPTION("x1520 sc132 driver depend on isp");
MODULE_LICENSE("GPL");
