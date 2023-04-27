/*
 * ar0144 Camera Driver
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

#define AR0144_CHIP_ID_H               (0x03)
#define AR0144_CHIP_ID_L               (0x56)
#define AR0144_SUPPORT_60FPS_SCLK      (74000000)

#define SENSOR_OUTPUT_MAX_FPS          60
#define SENSOR_OUTPUT_MIN_FPS          5

#define SENSOR_VERSION                 "H20200803"

#define AR0144_WIDTH                   1280
#define AR0144_HEIGHT                  720

struct regval_list {
    unsigned short reg_num;
    unsigned short value;
};

#define AR0144_REG_END                0xffff
#define AR0144_REG_DELAY              0xfffe     //Tell the Init script to insert a delay

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


struct tx_isp_sensor_attribute ar0144_attr;
#ifndef MODULE
static struct sensor_board_info *ar0144_board_info;
#endif

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

    .name = "ar0144",
    .chip_id = 0x0356,
    .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask = V4L2_SBUS_MASK_SAMPLE_16BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device = 0x10,
    .dbus_type = TX_SENSOR_DATA_INTERFACE_MIPI,
    .mipi = {
        .clk = 444,
        .lans = 2,
    },
    .max_again = 262144,
    .max_dgain = 0,
    .min_integration_time = 2,
    .min_integration_time_native = 2,

    .max_integration_time_native = 0x338 - 4,
    .integration_time_limit = 0x338 - 4,
    .total_width = 0x5D0,
    .total_height = 0x338,
    .max_integration_time = 0x338 - 4,
    .integration_time_apply_delay = 2,
    .again_apply_delay = 2,
    .dgain_apply_delay = 2,
    .sensor_ctrl.alloc_again = ar0144_alloc_again,
    .sensor_ctrl.alloc_dgain = ar0144_alloc_dgain,
    //    void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list ar0144_init_regs_mipi_1280_720[] = {

//    { AR0144_REG_DELAY, 200},

    { 0x301A, 0x00D9 }, // RESET_REGISTER
    { AR0144_REG_DELAY, 200},
    { 0x301A, 0x3058 }, // RESET_REGISTER
    { AR0144_REG_DELAY, 100},
    { 0x3F4C, 0x003F }, // PIX_DEF_1D_DDC_LO_DEF
    { 0x3F4E, 0x0018 }, // PIX_DEF_1D_DDC_HI_DEF
    { 0x3F50, 0x17DF }, // PIX_DEF_1D_DDC_EDGE
    { 0x30B0, 0x0028 }, // DIGITAL_TEST
    { AR0144_REG_DELAY, 200},
    { 0x3ED6, 0x3CB5 }, // DAC_LD_10_11
    { 0x3ED8, 0x8765 }, // DAC_LD_12_13
    { 0x3EDA, 0x8888 }, // DAC_LD_14_15
    { 0x3EDC, 0x97FF }, // DAC_LD_16_17
    { 0x3EF8, 0x6522 }, // DAC_LD_44_45
    { 0x3EFA, 0x2222 }, // DAC_LD_46_47
    { 0x3EFC, 0x6666 }, // DAC_LD_48_49
    { 0x3F00, 0xAA05 }, // DAC_LD_52_53
    { 0x3EE2, 0x180E }, // DAC_LD_22_23
    { 0x3EE4, 0x0808 }, // DAC_LD_24_25
    { 0x3EEA, 0x2A09 }, // DAC_LD_30_31
    { 0x3060, 0x0010 }, // ANALOG_GAIN
    { 0x30FE, 0x00A8 }, // NOISE_PEDESTAL
    { 0x3092, 0x00CF }, // ROW_NOISE_CONTROL
    { 0x3268, 0x0030 }, // SEQUENCER_CONTROL
    { 0x3786, 0x0006 }, // DIGITAL_CTRL_1
    { 0x3F4A, 0x0F70 }, // DELTA_DK_PIX_THRES
    { 0x306E, 0x4810 }, // DATAPATH_SELECT
    { 0x3064, 0x1802 }, // SMIA_TEST
    { 0x3EF6, 0x804D }, // DAC_LD_42_43
    { 0x3180, 0xC08F }, // DELTA_DK_CONTROL
    { 0x30BA, 0x7623 }, // DIGITAL_CTRL
    { 0x3176, 0x0480 }, // DELTA_DK_ADJUST_GREENR
    { 0x3178, 0x0480 }, // DELTA_DK_ADJUST_RED
    { 0x317A, 0x0480 }, // DELTA_DK_ADJUST_BLUE
    { 0x317C, 0x0480 }, // DELTA_DK_ADJUST_GREENB

    { 0x302A, 0x0006 }, // VT_PIX_CLK_DIV
    { 0x302C, 0x0001 }, // VT_SYS_CLK_DIV
    { 0x302E, 0x0004 }, // PRE_PLL_CLK_DIV
    { 0x3030, 0x004A }, // PLL_MULTIPLIER
    { 0x3036, 0x000C }, // OP_PIX_CLK_DIV
    { 0x3038, 0x0001 }, // OP_SYS_CLK_DIV
    { 0x30B0, 0x0028 }, // DIGITAL_TEST

    { AR0144_REG_DELAY, 100},
    { 0x31AE, 0x0202 }, // SERIAL_FORMAT
    { 0x31AC, 0x0C0C }, // DATA_FORMAT_BITS

    { 0x31B0, 0x0042 }, // FRAME_PREAMBLE
    { 0x31B2, 0x002E }, // LINE_PREAMBLE

    { 0x31B4, 0x2a33 }, // MIPI_TIMING_0
                // [15:12] hs_prepare 2
                // [11:8] hs_zero 10
                // [7:4] hs_trail 3
                // [3:0] clk_trail 3
    { 0x31B6, 0x210E }, // MIPI_TIMING_1
    { 0x31B8, 0x20C7 }, // MIPI_TIMING_2
#if 0
    { 0x31BA, 0x0105 }, // MIPI_TIMING_3
    { 0x31BC, 0x0004 }, // MIPI_TIMING_4
#else
    { 0x31BA, 0x01a5 }, // MIPI_TIMING_3
    { 0x31BC, 0x8004 }, // MIPI_TIMING_4
#endif
    { 0x3002, 0x0028 }, // Y_ADDR_START    40
    { 0x3004, 0x0004 }, // X_ADDR_START    4
    { 0x3006, 0x02F7 }, // Y_ADDR_END    759
    { 0x3008, 0x0503 }, // X_ADDR_END    1283

    { 0x300A, 0x0338 }, // FRAME_LENGTH_LINES 824
    { 0x300C, 0x05D0 }, // LINE_LENGTH_PCK      1488

//    { 0x3012, 0x0064 }, // COARSE_INTEGRATION_TIME
    { 0x3012, 0x01F3 }, // COARSE_INTEGRATION_TIME
    { 0x30A2, 0x0001 }, // X_ODD_INC
    { 0x30A6, 0x0001 }, // Y_ODD_INC
    { 0x3040, 0x0000 }, // READ_MODE
    { 0x3064, 0x1882 }, // SMIA_TEST
    { 0x3064, 0x1802 }, // SMIA_TEST
    { 0x3028, 0x0010 }, // ROW_SPEED
//    { 0x301A, 0x005C }, // RESET_REGISTER stream_on
    {AR0144_REG_END, 0x00},/* END MARKER */
};

/*
 * the order of the ar0144_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ar0144_win_sizes[] = {
    {
        .width        = AR0144_WIDTH,
        .height       = AR0144_HEIGHT,
        .fps          = SENSOR_OUTPUT_MAX_FPS << 16 | 1,
        .mbus_code    = V4L2_MBUS_FMT_SRGGB12_1X12,
        .colorspace   = V4L2_COLORSPACE_SRGB,
        .regs         = ar0144_init_regs_mipi_1280_720,
    }
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ar0144_stream_on[] = {
    {0x301A,0x005C},
    {AR0144_REG_END,0x0},    /* END MARKER */
};

static struct regval_list ar0144_stream_off[] = {
    {0x301A,0x0058},
    {AR0144_REG_END,0x0},    /* END MARKER */
};


static int ar0144_read(struct tx_isp_subdev *sd, uint16_t reg,
        unsigned char *value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
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
            .len    = 2,
            .buf    = value,
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
        .addr    = client->addr,
        .flags    = 0,
        .len    = 4,
        .buf    = buf,
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
    printk("=======ar0144_read_array=========.\n");
    while (vals->reg_num != AR0144_REG_END) {
        if (vals->reg_num == AR0144_REG_DELAY) {
            private_msleep(vals->value);
        } else{
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
        if (vals->reg_num == AR0144_REG_DELAY) {
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

static int ar0144_set_analog_gain(struct tx_isp_subdev *sd, u16 value)
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
    struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
    struct tx_isp_sensor_win_setting *wsize = &ar0144_win_sizes[0];
    int ret = 0;
//    unsigned char v[2]={0};

    if(!enable)
        return ISP_SUCCESS;

    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;
    ret = ar0144_write_array(sd, wsize->regs);
    if (ret)
        return ret;
//    ret = ar0144_read_array(sd,wsize->regs);

//    ret = ar0144_read(sd, 0x301A, v);
//    printk("read value :0x%x%x.\n",v[0],v[1]);

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
    unsigned int wpclk = 0;
    unsigned short vts = 0;
    unsigned short hts=0;
    unsigned int max_fps = 0;
    unsigned char tmp[2];
    unsigned int newformat = 0; //the format is 24.8
    int ret = 0;

    wpclk = AR0144_SUPPORT_60FPS_SCLK;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk("warn: fps(%x) no in range\n", fps);
        return -1;
    }
    ret += ar0144_read(sd, 0x300C, tmp);
    hts =tmp[0];
    if(ret < 0)
        return -1;
    hts = (hts << 8) + tmp[1];
    vts = wpclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = ar0144_write(sd, 0x300A, vts);
    if(ret < 0)
        return -1;

    sensor->video.fps = fps;
    sensor->video.attr->max_integration_time_native = vts - 4;
    sensor->video.attr->integration_time_limit = vts - 4;
    sensor->video.attr->total_height = vts;
    sensor->video.attr->max_integration_time = vts - 4;
    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return ret;
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

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ar0144_power");
        if (!ret) {
            private_gpio_direction_output(power_gpio, 1);
            private_msleep(50);
        }
        else
            printk("gpio requrest fail %d\n", power_gpio);
    }
    if(pwdn_gpio != -1){
        ret = private_gpio_request(pwdn_gpio,"ar0144_pwdn");
        if(!ret){
            private_gpio_direction_output(pwdn_gpio, 1);
            private_msleep(150);
        }else{
            printk("gpio requrest fail %d\n",pwdn_gpio);
        }
    }
    if(reset_gpio != -1){
        ret = private_gpio_request(reset_gpio,"ar0144_reset");
        if(!ret){
            private_gpio_direction_output(reset_gpio, 0);
            private_msleep(20);
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(20);
            private_gpio_direction_output(reset_gpio, 0);
            private_msleep(86);
        }else{
            printk("gpio requrest fail %d\n",reset_gpio);
        }
    }

    private_msleep(70);
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
                ret = ar0144_set_analog_gain(sd, *(u16*)arg);
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
    ret = ar0144_read(sd, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;

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
    ar0144_write(sd, reg->reg & 0xff, reg->val & 0xff);

    return 0;
}

static struct tx_isp_subdev_core_ops ar0144_core_ops = {
    .g_chip_ident = ar0144_g_chip_ident,
    .reset = ar0144_reset,
    .init = ar0144_init,
    /*.ioctl = ar0144_ops_ioctl,*/
    .g_register = ar0144_g_register,
    .s_register = ar0144_s_register,
};

static struct tx_isp_subdev_video_ops ar0144_video_ops = {
    .s_stream = ar0144_s_stream,
};

static struct tx_isp_subdev_sensor_ops ar0144_sensor_ops = {
    .ioctl    = ar0144_sensor_ops_ioctl,
};

static struct tx_isp_subdev_ops ar0144_ops = {
    .core = &ar0144_core_ops,
    .video = &ar0144_video_ops,
    .sensor = &ar0144_sensor_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device ar0144_platform_device = {
    .name = "ar0144",
    .id = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources = 0,
};

static int ar0144_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct tx_isp_subdev *sd;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &ar0144_win_sizes[0];

    printk("probe ok ----start-w %d h %d -->ar0144\n",AR0144_WIDTH, AR0144_HEIGHT);
    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if(!sensor){
        printk("Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }
    memset(sensor, 0 ,sizeof(*sensor));

#ifndef MODULE
    ar0144_board_info = get_sensor_board_info(ar0144_platform_device.name);
    if (ar0144_board_info) {
        power_gpio    = ar0144_board_info->gpios.gpio_power;
        reset_gpio    = ar0144_board_info->gpios.gpio_sensor_rst;
        i2c_sel1_gpio = ar0144_board_info->gpios.gpio_i2c_sel1;
        pwdn_gpio      = ar0144_board_info->gpios.gpio_sensor_pwdn;
    }
#endif

#if 1
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk("Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }
    private_clk_set_rate(sensor->mclk, 24000000);
    private_clk_enable(sensor->mclk);
#endif
     /*
        convert sensor-gain into isp-gain,
     */
    ar0144_attr.max_again = 262144;
    ar0144_attr.max_dgain = 0;
    sd = &sensor->sd;
    sensor->video.attr = &ar0144_attr;
    sensor->video.vi_max_width = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;
    sensor->video.dc_param = NULL;

    tx_isp_subdev_init(&ar0144_platform_device, sd, &ar0144_ops);

    tx_isp_set_subdevdata(sd, client);
    tx_isp_set_subdev_hostdata(sd, sensor);
    private_i2c_set_clientdata(client, sd);
    pr_debug("probe ok ------->ar0144\n");

    return 0;

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
    if(pwdn_gpio != -1)
        private_gpio_free(pwdn_gpio);
    if(reset_gpio != -1)
        private_gpio_free(reset_gpio);

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
        .owner    = THIS_MODULE,
        .name    = "ar0144",
    },
    .probe        = ar0144_probe,
    .remove        = ar0144_remove,
    .id_table    = ar0144_id,
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

