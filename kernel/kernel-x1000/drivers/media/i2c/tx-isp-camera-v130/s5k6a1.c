/*
 * s5k6a1.c
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
#include <soc/gpio.h>

#include <tx-isp/tx-isp-common.h>
#include <tx-isp/sensor-common.h>
#include <linux/sensor_board.h>


#define S5K6A1_CHIP_ID_H             (0x6A)
#define S5K6A1_CHIP_ID_L             (0x10)


#define S5K6A1_REG_END                0xffff
#define S5K6A1_REG_DELAY              0xfffe

#define S5K6A1_SUPPORT_30FPS_SCLK    (48000000)
#define SENSOR_OUTPUT_MAX_FPS        30
#define SENSOR_OUTPUT_MIN_FPS        5
#define S5K6A1_WIDTH                 1296
#define S5K6A1_HEIGHT                1042

#define SENSOR_VERSION               "H20201120"


static int power_gpio = -1;
module_param(power_gpio, int, S_IRUGO);
MODULE_PARM_DESC(power_gpio, "Power on GPIO NUM");

static int reset_gpio = -1;
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int i2c_sel1_gpio = -1;
module_param(i2c_sel1_gpio, int, S_IRUGO);
MODULE_PARM_DESC(i2c_sel1_gpio, "i2c select1  GPIO NUM");


struct tx_isp_sensor_attribute s5k6a1_attr;
static struct sensor_board_info *s5k6a1_board_info;


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

/* static unsigned char again_mode = LCG; */
struct again_lut s5k6a1_again_lut[] = {

    {0x20, 0},
    {0x22, 5731},
    {0x24, 11136},
    {0x26, 16247},
    {0x28, 21097},
    {0x2a, 25710},
    {0x2c, 30108},
    {0x2e, 34311},
    {0x30, 38335},
    {0x32, 42195},
    {0x34, 45903},
    {0x36, 49471},
    {0x38, 52910},
    {0x3a, 56227},
    {0x3c, 59433},
    {0x3e, 62533},
    {0x40, 65535},
    {0x44, 71266},
    {0x48, 76671},
    {0x4c, 81782},
    {0x50, 86632},
    {0x54, 91245},
    {0x58, 95643},
    {0x5c, 99846},
    {0x60, 103870},
    {0x64, 107730},
    {0x68, 111438},
    {0x6c, 115006},
    {0x70, 118445},
    {0x74, 121762},
    {0x78, 124968},
    {0x7c, 128068},
    {0x80, 131070},
    {0x90, 142206},
    {0xa0, 152167},
    {0xb0, 161178},
    {0xc0, 169405},
    {0xd0, 176973},
    {0xe0, 183980},
    {0xf0, 190503},
    {0x100, 196605},
};

unsigned int s5k6a1_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = s5k6a1_again_lut;

    while(lut->gain <= s5k6a1_attr.max_again) {
        if(isp_gain == 0) {
            *sensor_again = 0x20;
            return 0;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == s5k6a1_attr.max_again) && (isp_gain >= lut->gain)) {
            *sensor_again = lut->value;
            return lut->gain;
        }
    }
    lut++;
    }

    return isp_gain;

}

unsigned int s5k6a1_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

struct tx_isp_sensor_attribute s5k6a1_attr={
    .name = "s5k6a1",
    .chip_id = 0x6A10,
    .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device = 0x10,
    .dbus_type = TX_SENSOR_DATA_INTERFACE_MIPI,
    .mipi = {
        .mode = SENSOR_MIPI_SONY_MODE,
        .clk = 300,
        .lans = 1,
    },

    .max_again = 196605,
    .max_dgain = 0,
    .min_integration_time = 2,
    .min_integration_time_native = 2,
    .max_integration_time_native = 1058 - 6,
    .integration_time_limit = 1058 - 6,
    .total_width = 1486,
    .total_height = 1058,
    .max_integration_time = 1058 - 6,
    .integration_time_apply_delay = 6,
    .again_apply_delay = 2,
    .dgain_apply_delay = 0,
    .sensor_ctrl.alloc_again = s5k6a1_alloc_again,
    .sensor_ctrl.alloc_dgain = s5k6a1_alloc_dgain,
};

/*
 *  VendorName=Samsung
 *  ChipName=S5K6A1
 *  MipiLanes=1
 *  MipiFreq=800
 *  Mclk=12
 *  PixelWidth=1296
 *  PixelHeight=1040
 *  I2CAddress=0x20
 */
static struct regval_list s5k6a1_init_regs_1296_1040_30fps[] = {

    {0x0100,0x00},  // mode_select
    {S5K6A1_REG_DELAY,33},
    {0x0103,0x01},  // software_reset
    {0x0101,0x03},  // image_orientation
    {0x301C,0x35},
    {0x3016,0x05},
    {0x3034,0x73},
    {0x3037,0x01},
    {0x3035,0x05},
    {0x301E,0x09},
    {0x301B,0xC0},
    {0x3013,0x28},
//    {0x0601,0x02},
    {0x0100,0x00},
    {S5K6A1_REG_END, 0x00},  /* END MARKER */
};

/*
 * the order of the s5k6a1_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting s5k6a1_win_sizes[] = {
    {
        .width          = S5K6A1_WIDTH,
        .height         = S5K6A1_HEIGHT,
        .fps            = 30 << 16 | 1,
        .mbus_code      = V4L2_MBUS_FMT_SGBRG10_1X10,
        .colorspace     = V4L2_COLORSPACE_SRGB,
        .regs           = s5k6a1_init_regs_1296_1040_30fps,
    }
};

/*
 * the part of driver was fixed.
 */

static struct regval_list s5k6a1_stream_on[] = {
    {0x0100, 0x01},
    {S5K6A1_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list s5k6a1_stream_off[] = {
    {0x0100, 0x00},
    {S5K6A1_REG_END, 0x00},    /* END MARKER */
};

static int s5k6a1_read(struct tx_isp_subdev *sd, uint16_t reg,
        unsigned char *value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    uint8_t buf[2] = {(reg >> 8)&0xff, reg&0xff};

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

    int ret;

    ret = private_i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

    return ret;
}

static int s5k6a1_write(struct tx_isp_subdev *sd, uint16_t reg,
        unsigned char value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);

    uint8_t buf[3] = {(reg >> 8)&0xff, reg&0xff,value};
    struct i2c_msg msg = {
        .addr    = client->addr,
        .flags    = 0,
        .len    = 3,
        .buf    = buf,
    };
    int ret;

    ret = private_i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;
}

static inline int s5k6a1_read_array(struct tx_isp_subdev *sd, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != S5K6A1_REG_END) {
        if (vals->reg_num == S5K6A1_REG_DELAY) {
                private_msleep(vals->value);
                printk("private_msleep %d\n",vals->value);
        } else {
            ret = s5k6a1_read(sd, vals->reg_num, &val);
            printk("reg_num 0x%2x,value 0x%x\n",vals->reg_num,val);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int s5k6a1_write_array(struct tx_isp_subdev *sd, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != S5K6A1_REG_END) {
        if (vals->reg_num == S5K6A1_REG_DELAY) {
                private_msleep(vals->value);
        } else {
            ret = s5k6a1_write(sd, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int s5k6a1_reset(struct tx_isp_subdev *sd, int val)
{
    return 0;
}

static int s5k6a1_detect(struct tx_isp_subdev *sd, unsigned int *ident)
{
    unsigned char val = 0;
    int ret;

    ret = s5k6a1_read(sd, 0x0000, &val);
    if (ret < 0)
        return ret;
    if (val != S5K6A1_CHIP_ID_H)
        return -ENODEV;

    *ident = val;

    ret = s5k6a1_read(sd, 0x0001, &val);
    if (ret < 0)
        return ret;
    if (val != S5K6A1_CHIP_ID_L)
        return -ENODEV;

    *ident = (*ident << 8) | val;

    printk("read chip ID 0x%02x\n",*ident);
    return 0;
}

static int s5k6a1_set_integration_time(struct tx_isp_subdev *sd, int value)
{
    int ret = 0;
    unsigned int expo = value;

    ret += s5k6a1_write(sd, 0x0202, (unsigned char)((expo >> 8) & 0xff));
    ret = s5k6a1_write(sd,  0x0203, (unsigned char)(expo & 0xff));

    if (ret < 0)
        return ret;

    return 0;
}

static int s5k6a1_set_analog_gain(struct tx_isp_subdev *sd, int value)
{
    int ret = 0;

    ret = s5k6a1_write(sd, 0x0204, (unsigned char)((value >> 8)& 0xff));
    ret = s5k6a1_write(sd, 0x0205, (unsigned char)(value & 0xff));

    if (ret < 0)
     return ret;

    return 0;
}

static int s5k6a1_set_digital_gain(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int s5k6a1_get_black_pedestal(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int s5k6a1_init(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = &s5k6a1_win_sizes[0];
    int ret = 0;

    if(!enable)
        return ISP_SUCCESS;
    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;
    ret = s5k6a1_write_array(sd, wsize->regs);

    if (ret)
        return ret;

    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    sensor->priv = wsize;
    return 0;
}


static int s5k6a1_s_stream(struct tx_isp_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = s5k6a1_write_array(sd, s5k6a1_stream_on);
        pr_debug("s5k6a1 stream on\n");
    }
    else {
        ret = s5k6a1_write_array(sd, s5k6a1_stream_off);
        pr_debug("s5k6a1 stream off\n");
    }
    return ret;
}

static int s5k6a1_set_fps(struct tx_isp_subdev *sd, int fps)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    unsigned int pclk = S5K6A1_SUPPORT_30FPS_SCLK;
    unsigned short hts, vts;
    unsigned char tmp;
    unsigned int newformat;
    int ret = 0;

    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
        return -1;

    ret = s5k6a1_read(sd, 0x0342, &tmp);
    hts = tmp;
    ret += s5k6a1_read(sd, 0x0343, &tmp);
    if(ret < 0)
        return -1;
    hts = ((hts << 8) + tmp);    //HTS

    vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = s5k6a1_write(sd, 0x0340, (unsigned char)(vts >> 8)&&0xff);
    ret += s5k6a1_write(sd, 0x0341, (unsigned char)(vts&&0xff));
    if(ret < 0)
        return -1;

    sensor->video.fps = fps;
    sensor->video.attr->total_height = vts;
    sensor->video.attr->max_integration_time_native = vts - 4;
    sensor->video.attr->integration_time_limit = vts - 4;
    sensor->video.attr->max_integration_time = vts - 4;
    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return ret;

}

static int s5k6a1_set_mode(struct tx_isp_subdev *sd, int value)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
        wsize = &s5k6a1_win_sizes[0];
    }else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
        wsize = &s5k6a1_win_sizes[0];
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

static int s5k6a1_g_chip_ident(struct tx_isp_subdev *sd,
        struct tx_isp_chip_ident *chip)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if(pwdn_gpio != -1){
        ret = private_gpio_request(pwdn_gpio,"s5k6a1_pwdn");
        if(!ret){
            private_gpio_direction_output(pwdn_gpio, 1);
            private_msleep(50);
        }else{
            printk("gpio requrest fail %d\n",pwdn_gpio);
        }
    }
    if(reset_gpio != -1){
        ret = private_gpio_request(reset_gpio,"s5k6a1_reset");
        if(!ret){
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(5);
            private_gpio_direction_output(reset_gpio, 0);
            private_msleep(20);
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(5);
        }else{
            printk("gpio requrest fail %d\n",reset_gpio);
        }
    }
    if (i2c_sel1_gpio != -1) {
        int ret = gpio_request(i2c_sel1_gpio, "s5k6a1_i2c_sel1");
        if (!ret) {
            private_gpio_direction_output(i2c_sel1_gpio, 1);
        } else {
            printk("gpio requrest fail %d\n", i2c_sel1_gpio);
        }
    }

    ret = s5k6a1_detect(sd, &ident);
    if (ret) {
        printk("chip found @ 0x%x (%s) is not an s5k6a1 chip.\n",
                client->addr, client->adapter->name);
        return ret;
    }
    printk("s5k6a1 chip found @ 0x%02x (%s)\n", client->addr, client->adapter->name);
    if(chip){
        memcpy(chip->name, "s5k6a1", sizeof("s5k6a1"));
        chip->ident = ident;
        chip->revision = SENSOR_VERSION;
    }
    return 0;
}

static int s5k6a1_sensor_ops_ioctl(struct tx_isp_subdev *sd, unsigned int cmd, void *arg)
{
    long ret = 0;
    if(IS_ERR_OR_NULL(sd)){
        printk("[%d]The pointer is invalid!\n", __LINE__);
        return -EINVAL;
    }
    switch(cmd){
        case TX_ISP_EVENT_SENSOR_INT_TIME:
            if(arg)
                ret = s5k6a1_set_integration_time(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_AGAIN:
            if(arg)
                ret = s5k6a1_set_analog_gain(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_DGAIN:
            if(arg)
                ret = s5k6a1_set_digital_gain(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_BLACK_LEVEL:
            if(arg)
                ret = s5k6a1_get_black_pedestal(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_RESIZE:
            if(arg)
                ret = s5k6a1_set_mode(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_PREPARE_CHANGE:
            ret = s5k6a1_write_array(sd, s5k6a1_stream_off);
            break;
        case TX_ISP_EVENT_SENSOR_FINISH_CHANGE:
            ret = s5k6a1_write_array(sd, s5k6a1_stream_on);
            break;
        case TX_ISP_EVENT_SENSOR_FPS:
            if(arg)
                ret = s5k6a1_set_fps(sd, *(int*)arg);
            break;
//        case TX_ISP_EVENT_SENSOR_VFLIP:
//            if(arg)
//                ret = s5k6a1_set_vflip(sd, *(int*)arg);
//            break;
        default:
            break;;
    }
    return 0;
}

static int s5k6a1_g_register(struct tx_isp_subdev *sd, struct tx_isp_dbg_register *reg)
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
    ret = s5k6a1_read(sd, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int s5k6a1_s_register(struct tx_isp_subdev *sd, const struct tx_isp_dbg_register *reg)
{
    int len = 0;

    len = strlen(sd->chip.name);
    if(len && strncmp(sd->chip.name, reg->name, len)){
        return -EINVAL;
    }
    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;
    s5k6a1_write(sd, reg->reg & 0xffff, reg->val & 0xff);
    return 0;
}

static struct tx_isp_subdev_core_ops s5k6a1_core_ops = {
    .g_chip_ident = s5k6a1_g_chip_ident,
    .reset = s5k6a1_reset,
    .init = s5k6a1_init,
    .g_register = s5k6a1_g_register,
    .s_register = s5k6a1_s_register,
};

static struct tx_isp_subdev_video_ops s5k6a1_video_ops = {
    .s_stream = s5k6a1_s_stream,
};

static struct tx_isp_subdev_sensor_ops    s5k6a1_sensor_ops = {
    .ioctl = s5k6a1_sensor_ops_ioctl,
};

static struct tx_isp_subdev_ops s5k6a1_ops = {
    .core = &s5k6a1_core_ops,
    .video = &s5k6a1_video_ops,
    .sensor = &s5k6a1_sensor_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device s5k6a1_platform_device = {
    .name = "s5k6a1",
    .id = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources = 0,
};


static int s5k6a1_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    struct tx_isp_subdev *sd;
    struct tx_isp_video_in *video;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &s5k6a1_win_sizes[0];
    int ret;

    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if(!sensor){
        printk("Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }
    memset(sensor, 0 ,sizeof(*sensor));

    s5k6a1_board_info = get_sensor_board_info(s5k6a1_platform_device.name);
    if (s5k6a1_board_info) {
        power_gpio          = s5k6a1_board_info->gpios.gpio_power;
        reset_gpio          = s5k6a1_board_info->gpios.gpio_sensor_rst;
        pwdn_gpio           = s5k6a1_board_info->gpios.gpio_sensor_pwdn;
    }

    /* request mclk of sensor */
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk("Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }
    clk_set_rate(sensor->mclk, 12000000);
    clk_enable(sensor->mclk);

     /*
        convert sensor-gain into isp-gain,
     */
    s5k6a1_attr.max_again = 196605;
    s5k6a1_attr.max_dgain = 0;
    sd = &sensor->sd;

    video = &sensor->video;
    sensor->video.attr = &s5k6a1_attr;
    sensor->video.mbus_change = 0;
    sensor->video.vi_max_width = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;
    sensor->video.dc_param = NULL;
    tx_isp_subdev_init(&s5k6a1_platform_device, sd, &s5k6a1_ops);
    tx_isp_set_subdevdata(sd, client);
    tx_isp_set_subdev_hostdata(sd, sensor);
    private_i2c_set_clientdata(client, sd);

    pr_debug("probe ok ------->s5k6a1\n");
    return 0;

err_get_mclk:
    kfree(sensor);

    return -1;
}

static int s5k6a1_remove(struct i2c_client *client)
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

static const struct i2c_device_id s5k6a1_id[] = {
    { "s5k6a1", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, s5k6a1_id);

static struct i2c_driver s5k6a1_driver = {
    .driver = {
        .owner    = THIS_MODULE,
        .name    = "s5k6a1",
    },
    .probe        = s5k6a1_probe,
    .remove        = s5k6a1_remove,
    .id_table    = s5k6a1_id,
};

static __init int init_s5k6a1(void)
{
    int ret = 0;
    ret = private_driver_get_interface();
    if(ret){
        printk("Failed to init s5k6a1 dirver.\n");
        return -1;
    }
    return private_i2c_add_driver(&s5k6a1_driver);
}

static __exit void exit_s5k6a1(void)
{
    private_i2c_del_driver(&s5k6a1_driver);
}

module_init(init_s5k6a1);
module_exit(exit_s5k6a1);

MODULE_DESCRIPTION("A low-level driver for samsung s5k6a1 sensors");
MODULE_LICENSE("GPL");
