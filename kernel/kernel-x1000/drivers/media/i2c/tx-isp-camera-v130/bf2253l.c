/*
 * bf2253l Camera Driver
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

#define BF2253L_CHIP_ID_H               (0x22)
#define BF2253L_CHIP_ID_L               (0x53)
#define BF2253L_SUPPORT_30FPS_SCLK      (66000000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5

#define BF2253L_WIDTH                   1600
#define BF2253L_HEIGHT                  1200


#define SENSOR_VERSION                  "H20180612a"


struct again_lut {
    unsigned int value;
    unsigned int gain;
};


static int power_gpio = -1; // -1
static int reset_gpio = -1; // GPIO_PA(19)
static int pwdn_gpio = -1; // GPIO_PA(20)
static int i2c_sel1_gpio = -1; // -1

module_param(power_gpio, int, S_IRUGO);
MODULE_PARM_DESC(power_gpio, "Power on GPIO NUM");

module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

module_param(i2c_sel1_gpio, int, S_IRUGO);
MODULE_PARM_DESC(i2c_sel1_gpio, "i2c select1  GPIO NUM");

static struct tx_isp_sensor_attribute bf2253l_attr;

#ifndef MODULE
static struct sensor_board_info *bf2253l_board_info;
#endif


static struct again_lut bf2253l_again_lut[] = {
    {0xf, 0},
    {0x10, 5731},
    {0x11, 10967},
    {0x12, 16088},
    {0x13, 20870},
    {0x14, 25566},
    {0x15, 29771},
    {0x16, 34048},
    {0x17, 37779},
    {0x18, 41837},
    {0x19, 45214},
    {0x1a, 48808},
    {0x1b, 52003},
    {0x1c, 55352},
    {0x1d, 58291},
    {0x1e, 61379},
    {0x1f, 64369},
    {0x20, 69909},
    {0x21, 75389},
    {0x22, 80330},
    {0x23, 85103},
    {0x24, 89577},
    {0x25, 93912},
    {0x26, 97929},
    {0x27, 101815},
    {0x28, 105668},
    {0x29, 109165},
    {0x2a, 112706},
    {0x2b, 116012},
    {0x2c, 119125},
    {0x2d, 122218},
    {0x2e, 125114},
    {0x2f, 127924},
    {0x30, 133575},
    {0x31, 138715},
    {0x32, 143630},
    {0x33, 148130},
    {0x34, 152644},
    {0x35, 156692},
    {0x36, 160656},
    {0x37, 164461},
    {0x38, 167883},
    {0x39, 171158},
    {0x3a, 174628},
    {0x3b, 177778},
    {0x3c, 180759},
    {0x3d, 183728},
    {0x3e, 186466},
    {0x3f, 189063},
    {0x40, 191834},
    {0x41, 194362},
    {0x42, 196881},
    {0x43, 199413},
    {0x44, 201726},
    {0x45, 204005},
    {0x46, 206178},
    {0x47, 208395},
    {0x48, 210430},
    {0x49, 212237},
    {0x4a, 214423},
    {0x4b, 216147},
    {0x4c, 218006},
    {0x4d, 219838},
    {0x4e, 221528},
    {0x4f, 223225},
};


static unsigned int bf2253l_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
     struct again_lut *lut = bf2253l_again_lut;

    while(lut->gain <= bf2253l_attr.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut->value;
            return 0;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == bf2253l_attr.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static unsigned int bf2253l_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}


static struct tx_isp_sensor_attribute bf2253l_attr={
    .name                                   = "bf2253l",
    .chip_id                                = 0x2253,
    .cbus_type                              = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask                              = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
    .cbus_device                            = 0x6e,
    .dbus_type                              = TX_SENSOR_DATA_INTERFACE_MIPI,
    .mipi = {
        .clk = 660,
        .lans = 1,
    },
    .max_again                              = 223225,
    .max_dgain                              = 0,
    .min_integration_time                   = 2,
    .min_integration_time_native            = 2,
    .max_integration_time_native            = 1236 - 4,
    .integration_time_limit                 = 1236 - 4,
    .total_width                            = 1780,
    .total_height                           = 1236,
    .max_integration_time                   = 1236 - 4,
    .integration_time_apply_delay           = 2,
    .again_apply_delay                      = 2,
    .dgain_apply_delay                      = 0,
    .sensor_ctrl.alloc_again                = bf2253l_alloc_again,
    .sensor_ctrl.alloc_dgain                = bf2253l_alloc_dgain,
    //    void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct camera_reg_op bf2253l_init_regs_1600_1200_30fps_mipi[] = {
    {CAMERA_REG_OP_DATA, 0xe1, 0x06},
    {CAMERA_REG_OP_DATA, 0xe2, 0x06},
    {CAMERA_REG_OP_DATA, 0xe3, 0x0e},
    {CAMERA_REG_OP_DATA, 0xe4, 0x40},
    {CAMERA_REG_OP_DATA, 0xe5, 0x67},
    {CAMERA_REG_OP_DATA, 0xe6, 0x02},
    {CAMERA_REG_OP_DATA, 0xe8, 0x84},
    {CAMERA_REG_OP_DATA, 0x01, 0x14},
    {CAMERA_REG_OP_DATA, 0x03, 0x98},
    {CAMERA_REG_OP_DATA, 0x27, 0x21},
    {CAMERA_REG_OP_DATA, 0x29, 0x20},
    {CAMERA_REG_OP_DATA, 0x59, 0x10},
    {CAMERA_REG_OP_DATA, 0x5a, 0x10},   //blc ob
    {CAMERA_REG_OP_DATA, 0x5c, 0x11},
    {CAMERA_REG_OP_DATA, 0x5d, 0x73},
    {CAMERA_REG_OP_DATA, 0x6a, 0x2f},
    {CAMERA_REG_OP_DATA, 0x6b, 0x0e},
    {CAMERA_REG_OP_DATA, 0x6c, 0x7e},
    {CAMERA_REG_OP_DATA, 0x6f, 0x10},
    {CAMERA_REG_OP_DATA, 0x70, 0x08},
    {CAMERA_REG_OP_DATA, 0x71, 0x05},
    {CAMERA_REG_OP_DATA, 0x72, 0x10},
    {CAMERA_REG_OP_DATA, 0x73, 0x08},
    {CAMERA_REG_OP_DATA, 0x74, 0x05},
    {CAMERA_REG_OP_DATA, 0x75, 0x06},
    {CAMERA_REG_OP_DATA, 0x76, 0x20},
    {CAMERA_REG_OP_DATA, 0x77, 0x03},
    {CAMERA_REG_OP_DATA, 0x78, 0x0e},
    {CAMERA_REG_OP_DATA, 0x79, 0x08},
    {CAMERA_REG_OP_DATA, 0x00, 0x2c},
    {CAMERA_REG_OP_END, 0x00, 0x00},
};


/*
 * the order of the bf2253l_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting bf2253l_win_sizes[] = {
    {
        .width                      = BF2253L_WIDTH,
        .height                     = BF2253L_HEIGHT,
        .fps                        = 30 << 16 | 1,
        .mbus_code                  = V4L2_MBUS_FMT_SBGGR10_1X10,
        .colorspace                 = V4L2_COLORSPACE_SRGB,
        .regs                       = bf2253l_init_regs_1600_1200_30fps_mipi,
    }
};

/*
 * the part of driver was fixed.
 */

static struct camera_reg_op bf2253l_stream_on[] = {
    {CAMERA_REG_OP_DATA, 0xe0, 0x00},
    {CAMERA_REG_OP_END, 0x00, 0x00},
};

static struct camera_reg_op bf2253l_stream_off[] = {
    {CAMERA_REG_OP_DATA, 0xe0, 0x01},
    {CAMERA_REG_OP_END, 0x00, 0x00},
};


static int bf2253l_read(struct tx_isp_subdev *sd, unsigned char reg,
        unsigned char *value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    struct i2c_msg msg[2] = {
        [0] = {
            .addr     = client->addr,
            .flags    = 0,
            .len      = 1,
            .buf      = &reg,
        },
        [1] = {
            .addr     = client->addr,
            .flags    = I2C_M_RD,
            .len      = 1,
            .buf      = value,
        }
    };

    int ret;
    ret = private_i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

    return ret;
}

static int bf2253l_write(struct tx_isp_subdev *sd, unsigned char reg,
        unsigned char value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr     = client->addr,
        .flags    = 0,
        .len      = 2,
        .buf      = buf,
    };

    int ret;
    ret = private_i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;
}

static inline int bf2253l_read_array(struct tx_isp_subdev *sd, struct camera_reg_op *vals)
{
    int ret;
    unsigned char val;

    while (vals->flag != CAMERA_REG_OP_END) {
        if (vals->flag == CAMERA_REG_OP_DATA) {
            if (vals->reg == 0xfe) {
                ret = bf2253l_write(sd, vals->reg, vals->val);
                if (ret < 0)
                    return ret;
                // printk(KERN_ERR "bf2253l: %x[%x]\n", vals->reg, vals->val);
            } else {
                ret = bf2253l_read(sd, vals->reg, &val);
                if (ret < 0)
                    return ret;
                // printk(KERN_ERR "bf2253l: %x[%x]\n", vals->reg, val);
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

static int bf2253l_write_array(struct tx_isp_subdev *sd, struct camera_reg_op *vals)
{
    int ret;

    while (vals->flag != CAMERA_REG_OP_END) {
        if (vals->flag == CAMERA_REG_OP_DATA) {
            ret = bf2253l_write(sd, vals->reg, vals->val);
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

static int bf2253l_reset(struct tx_isp_subdev *sd, int val)
{
    return 0;
}

static int bf2253l_detect(struct tx_isp_subdev *sd, unsigned int *ident)
{
    unsigned char v;
    int ret;

    ret = bf2253l_read(sd, 0xfc, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != BF2253L_CHIP_ID_H)
        return -ENODEV;

    *ident = v;

    ret = bf2253l_read(sd, 0xfd, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != BF2253L_CHIP_ID_L)
        return -ENODEV;

    *ident = (*ident << 8) | v;

    return 0;
}

static int bf2253l_set_integration_time(struct tx_isp_subdev *sd, int value)
{
    int ret = 0;
    unsigned int expo = value;

    ret += bf2253l_write(sd, 0x6c, (unsigned char)(expo & 0xff));
    ret += bf2253l_write(sd, 0x6b, (unsigned char)((expo >> 8) & 0xff));
    if (ret < 0) {
        printk(KERN_ERR "bf2253l: bf2253l_write error  %d\n" ,__LINE__ );
        return ret;
    }

    return 0;
}


static int bf2253l_set_analog_gain(struct tx_isp_subdev *sd, u16 gain)
{
    int ret = 0;

    ret = bf2253l_write(sd, 0x6a, (gain) & 0xff);
    if (ret < 0) {
        printk(KERN_ERR "bf2253l_write error  %d" ,__LINE__ );
        return ret;
    }

    return 0;
}

static int bf2253l_set_digital_gain(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int bf2253l_get_black_pedestal(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int bf2253l_init(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = &bf2253l_win_sizes[0];
    int ret = 0;

    if (!enable)
        return ISP_SUCCESS;

    sensor->video.mbus.width                = wsize->width;
    sensor->video.mbus.height               = wsize->height;
    sensor->video.mbus.code                 = wsize->mbus_code;
    sensor->video.mbus.field                = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace           = wsize->colorspace;
    sensor->video.fps                       = wsize->fps;

    ret = bf2253l_write_array(sd, wsize->regs);
    if (ret)
        return ret;

    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    sensor->priv = wsize;

    return 0;
}

static int bf2253l_s_stream(struct tx_isp_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = bf2253l_write_array(sd, bf2253l_stream_on);
        pr_debug("bf2253l stream on\n");
    } else {
        ret = bf2253l_write_array(sd, bf2253l_stream_off);
        pr_debug("bf2253l stream off\n");
    }

    return ret;
}

static int bf2253l_set_fps(struct tx_isp_subdev *sd, int fps)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    unsigned int pclk = BF2253L_SUPPORT_30FPS_SCLK;
    unsigned short hts, vts;
    unsigned int newformat; //the format is 24.8
    int ret = 0;
    unsigned char tmp;
    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if (newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
        return -1;

    ret += bf2253l_read(sd, 0x26, &tmp);
    hts = tmp;

    ret += bf2253l_read(sd, 0x25, &tmp);
    hts = hts << 8 | tmp;
    if (ret < 0)
        return -1;

    vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = bf2253l_write(sd, 0x22, vts & 0xff);
    ret += bf2253l_write(sd, 0x23, (vts >> 8) & 0xff);
    if (ret < 0)
        return -1;

    sensor->video.fps = fps;
    sensor->video.attr->total_height = vts;

    sensor->video.attr->max_integration_time_native     = vts - 4;
    sensor->video.attr->integration_time_limit          = vts - 4;
    sensor->video.attr->max_integration_time            = vts - 4;

    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return ret;
}

static int bf2253l_set_mode(struct tx_isp_subdev *sd, int value)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    if (value == TX_ISP_SENSOR_FULL_RES_MAX_FPS) {
        wsize = &bf2253l_win_sizes[0];
    } else if (value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS) {
        wsize = &bf2253l_win_sizes[0];
    }

    if (wsize) {
        sensor->video.mbus.width            = wsize->width;
        sensor->video.mbus.height           = wsize->height;
        sensor->video.mbus.code             = wsize->mbus_code;
        sensor->video.mbus.field            = V4L2_FIELD_NONE;
        sensor->video.mbus.colorspace       = wsize->colorspace;
        sensor->video.fps                   = wsize->fps;
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    }

    return ret;
}

static int bf2253l_set_vflip(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    int ret = 0;

    ret = bf2253l_write(sd, 0x00, 0x22);

    if (!ret)
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return 0;
}

static int bf2253l_g_chip_ident(struct tx_isp_subdev *sd,
        struct tx_isp_chip_ident *chip)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "bf2253l_power");
        if (!ret) {
            private_gpio_direction_output(power_gpio, 1);
            private_msleep(1);
        } else {
            printk(KERN_ERR "bf2253l: gpio requrest fail %d\n", power_gpio);
            return -1;
        }
    }

    if (i2c_sel1_gpio != -1) {
        int ret = gpio_request(i2c_sel1_gpio, "bf2253l_i2c_sel1");
        if (!ret) {
            private_gpio_direction_output(i2c_sel1_gpio, 1);
        } else {
            printk(KERN_ERR "bf2253l: gpio requrest fail %d\n", i2c_sel1_gpio);
            return -1;
        }
    }

    if (pwdn_gpio != -1) {
        ret = private_gpio_request(pwdn_gpio,"bf2253l_pwdn");
        if (!ret) {
            private_gpio_direction_output(pwdn_gpio, 1);
            usleep_range(200, 200);
            private_gpio_direction_output(pwdn_gpio, 0);
            usleep_range(200, 200);
        } else {
            printk(KERN_ERR "bf2253l: gpio requrest fail %d\n",pwdn_gpio);
            return -1;
        }
    }

    if (reset_gpio != -1) {
        ret = private_gpio_request(reset_gpio,"bf2253l_reset");
        if (!ret) {
            private_gpio_direction_output(reset_gpio, 0);
            usleep_range(200, 200);
            private_gpio_direction_output(reset_gpio, 1);
            usleep_range(200, 200);
        } else {
            printk(KERN_ERR "bf2253l: gpio requrest fail %d\n",reset_gpio);
            return -1;
        }
    }

    ret = bf2253l_detect(sd, &ident);
    if (ret) {
        printk(KERN_ERR "bf2253l: chip found @ 0x%x (%s) is not an bf2253l chip.\n",
                client->addr, client->adapter->name);
        return ret;
    }

    printk(KERN_ERR "bf2253l: bf2253l chip found @ 0x%02x (%s)\n", client->addr, client->adapter->name);
    if (chip) {
        memcpy(chip->name, "bf2253l", sizeof("bf2253l"));
        chip->ident = ident;
        chip->revision = SENSOR_VERSION;
    }

    return 0;
}

static int bf2253l_sensor_ops_ioctl(struct tx_isp_subdev *sd, unsigned int cmd, void *arg)
{
    long ret = 0;

    if (IS_ERR_OR_NULL(sd)) {
        printk(KERN_ERR "bf2253l: [%d]The pointer is invalid!\n", __LINE__);
        return -EINVAL;
    }

    switch (cmd) {
        case TX_ISP_EVENT_SENSOR_INT_TIME:
            if (arg)
                ret = bf2253l_set_integration_time(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_AGAIN:
            if (arg)
                ret = bf2253l_set_analog_gain(sd, *(u16*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_DGAIN:
            if (arg)
                ret = bf2253l_set_digital_gain(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_BLACK_LEVEL:
            if (arg)
                ret = bf2253l_get_black_pedestal(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_RESIZE:
            if (arg)
                ret = bf2253l_set_mode(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_PREPARE_CHANGE:
            ret = bf2253l_write_array(sd, bf2253l_stream_off);
            break;
        case TX_ISP_EVENT_SENSOR_FINISH_CHANGE:
            ret = bf2253l_write_array(sd, bf2253l_stream_on);
            break;
        case TX_ISP_EVENT_SENSOR_FPS:
            if (arg)
                ret = bf2253l_set_fps(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_VFLIP:
            if (arg)
                ret = bf2253l_set_vflip(sd, *(int*)arg);
            break;
        default:
            break;;
    }

    return 0;
}

static int bf2253l_g_register(struct tx_isp_subdev *sd, struct tx_isp_dbg_register *reg)
{
    unsigned char val = 0;
    int len = 0;
    int ret = 0;

    len = strlen(sd->chip.name);
    if (len && strncmp(sd->chip.name, reg->name, len)) {
        return -EINVAL;
    }

    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;

    ret = bf2253l_read(sd, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;

    return ret;
}

static int bf2253l_s_register(struct tx_isp_subdev *sd, const struct tx_isp_dbg_register *reg)
{
    int len = 0;

    len = strlen(sd->chip.name);
    if (len && strncmp(sd->chip.name, reg->name, len)) {
        return -EINVAL;
    }

    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;

    bf2253l_write(sd, reg->reg & 0xff, reg->val & 0xff);

    return 0;
}

static struct tx_isp_subdev_core_ops bf2253l_core_ops = {
    .g_chip_ident               = bf2253l_g_chip_ident,
    .reset                      = bf2253l_reset,
    .init                       = bf2253l_init,
    /*.ioctl                    = bf2253l_ops_ioctl,*/
    .g_register                 = bf2253l_g_register,
    .s_register                 = bf2253l_s_register,
};

static struct tx_isp_subdev_video_ops bf2253l_video_ops = {
    .s_stream                   = bf2253l_s_stream,
};

static struct tx_isp_subdev_sensor_ops bf2253l_sensor_ops = {
    .ioctl                      = bf2253l_sensor_ops_ioctl,
};

static struct tx_isp_subdev_ops bf2253l_ops = {
    .core                       = &bf2253l_core_ops,
    .video                      = &bf2253l_video_ops,
    .sensor                     = &bf2253l_sensor_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device bf2253l_platform_device = {
    .name                       = "bf2253l",
    .id                         = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources              = 0,
};

static int bf2253l_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct tx_isp_subdev *sd;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &bf2253l_win_sizes[0];
    int ret = 0;

    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if (!sensor) {
        printk(KERN_ERR "bf2253l: Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }


#ifndef MODULE
    bf2253l_board_info = get_sensor_board_info(bf2253l_platform_device.name);
    if (bf2253l_board_info) {
        power_gpio    = bf2253l_board_info->gpios.gpio_power;
        reset_gpio    = bf2253l_board_info->gpios.gpio_sensor_rst;
        pwdn_gpio     = bf2253l_board_info->gpios.gpio_sensor_pwdn;
        i2c_sel1_gpio = bf2253l_board_info->gpios.gpio_i2c_sel1;
    }
#endif


    ret = tx_isp_clk_set(CONFIG_ISP_CLK);
    if (ret < 0) {
        printk(KERN_ERR "bf2253l: Cannot set isp clock\n");
        goto err_get_mclk;
    }

    /* request mclk of sensor */
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk(KERN_ERR "bf2253l: Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }

    private_clk_set_rate(sensor->mclk, 24000000);
    private_clk_enable(sensor->mclk);

     /*
        convert sensor-gain into isp-gain,
     */
    bf2253l_attr.max_again = 223225;
    bf2253l_attr.max_dgain = 0;
    sd = &sensor->sd;

    sensor->video.attr                  = &bf2253l_attr;
    sensor->video.vi_max_width          = wsize->width;
    sensor->video.vi_max_height         = wsize->height;
    sensor->video.mbus.width            = wsize->width;
    sensor->video.mbus.height           = wsize->height;
    sensor->video.mbus.code             = wsize->mbus_code;
    sensor->video.mbus.field            = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace       = wsize->colorspace;
    sensor->video.fps                   = wsize->fps;
    sensor->video.dc_param              = NULL;

    tx_isp_subdev_init(&bf2253l_platform_device, sd, &bf2253l_ops);
    tx_isp_set_subdevdata(sd, client);
    tx_isp_set_subdev_hostdata(sd, sensor);
    private_i2c_set_clientdata(client, sd);

    pr_debug("probe ok ------->bf2253l\n");

    return 0;
err_get_mclk:
    kfree(sensor);

    return -1;
}

static int bf2253l_remove(struct i2c_client *client)
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

static const struct i2c_device_id bf2253l_id[] = {
    { "bf2253l", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, bf2253l_id);

static struct i2c_driver bf2253l_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = "bf2253l",
    },
    .probe              = bf2253l_probe,
    .remove             = bf2253l_remove,
    .id_table           = bf2253l_id,
};

static __init int init_bf2253l(void)
{
    int ret = 0;

    ret = private_driver_get_interface();
    if (ret) {
        printk(KERN_ERR "bf2253l: Failed to init bf2253l dirver.\n");
        return -1;
    }

    return private_i2c_add_driver(&bf2253l_driver);
}

static __exit void exit_bf2253l(void)
{
    private_i2c_del_driver(&bf2253l_driver);
}

module_init(init_bf2253l);
module_exit(exit_bf2253l);

MODULE_DESCRIPTION("x1830 bf2253l driver depend on isp");
MODULE_LICENSE("GPL");
