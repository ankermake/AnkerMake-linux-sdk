/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * OV2735 mipi driver
 *
 */
#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>


#define OV2735_DEVICE_NAME             "ov2735"
#define OV2735_DEVICE_I2C_ADDR         0x3c

#define OV2735_SUPPORT_SCLK_FPS_30     (42000000)
#define SENSOR_OUTPUT_MAX_FPS          30
#define SENSOR_OUTPUT_MIN_FPS          5

#define OV2735_MAX_WIDTH               1920
#define OV2735_MAX_HEIGHT              1080
#define OV2735_WIDTH                   1920
#define OV2735_HEIGHT                  1080

#define OV2735_CHIP_ID_H               (0x27)
#define OV2735_CHIP_ID_L               (0x35)
#define OV2735_REG_CHIP_ID_HIGH        0x02
#define OV2735_REG_CHIP_ID_LOW         0x03

#define OV2735_REG_DELAY               0xfffe
#define OV2735_REG_END                 0Xffff

static int power_gpio = -1;  // -1;
static int reset_gpio = -1;  // GPIO_PA(10);
static int pwdn_gpio = -1;   // -1;
static int i2c_bus_num = -1; // 3;
static int cam_bus_num = -1; //0;
static int i2c_addr = -1;    //0x3d;
static char *sensor_name = NULL;
static char *regulator_name = "";

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(i2c_addr, int, 0644);
module_param(cam_bus_num, int, 0644);
module_param(i2c_bus_num, int, 0644);
module_param(sensor_name, charp, 0644);
module_param(regulator_name, charp, 0644);


static struct sensor_attr ov2735_sensor_attr;
static struct i2c_client *i2c_dev;
static struct regulator *ov2735_regulator = NULL;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int index;
    unsigned int gain;
};

static struct again_lut ov2735_again_lut[] = {
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
    {0x20, 65536},
    {0x22, 71267},
    {0x24, 76672},
    {0x26, 81784},
    {0x28, 86633},
    {0x2a, 91246},
    {0x2c, 95645},
    {0x2e, 99848},
    {0x30, 103872},
    {0x32, 107731},
    {0x34, 111440},
    {0x36, 115008},
    {0x38, 118446},
    {0x3a, 121764},
    {0x3c, 124969},
    {0x3e, 128070},
    {0x40, 131072},
    {0x44, 136803},
    {0x48, 142208},
    {0x4c, 147320},
    {0x50, 152169},
    {0x54, 156782},
    {0x58, 161181},
    {0x5c, 165384},
    {0x60, 169408},
    {0x64, 173267},
    {0x68, 176976},
    {0x6c, 180544},
    {0x70, 183982},
    {0x74, 187300},
    {0x78, 190505},
    {0x7c, 193606},
    {0x80, 196608},
    {0x88, 202339},
    {0x90, 207744},
    {0x98, 212856},
    {0xa0, 217705},
    {0xa8, 222318},
    {0xb0, 226717},
    {0xb8, 230920},
    {0xc0, 234944},
    {0xc8, 238803},
    {0xd0, 242512},
    {0xd8, 246080},
    {0xe0, 249518},
    {0xe8, 252836},
    {0xf0, 256041},
    {0xf8, 259142},
};


static struct regval_list ov2735_init_regs_1920_1080_30fps_mipi[] = {
    {0xfd, 0x00},
    {0x2f, 0x10},
    {0x34, 0x00},
    {0x30, 0x15},
    {0x33, 0x01},
    {0x35, 0x20},

    {0xfd, 0x01},
    {0x0d, 0x00},
    {0x30, 0x00},
    {0x03, 0x05},
    {0x04, 0x24},
    {0x01, 0x01},
    {0x09, 0x00},
    {0x0a, 0x20},
    {0x06, 0x0a},
    {0x24, 0x10},
    {0x01, 0x01},
    {0xfb, 0x73},
    {0x01, 0x01},

    {0xfd, 0x01},
    {0x1a, 0x6b},
    {0x1c, 0xea},
    {0x16, 0x0c},
    {0x21, 0x00},
    {0x11, 0x63},
    {0x19, 0xc3},
    {0x26, 0xda},
    {0x29, 0x01},
    {0x33, 0x6f},
    {0x2a, 0xd2},
    {0x2c, 0x40},
    {0xd0, 0x02},
    {0xd1, 0x01},
    {0xd2, 0x20},
    {0xd3, 0x04},
    {0xd4, 0x2a},
    {0x50, 0x00},
    {0x51, 0x2c},
    {0x52, 0x29},
    {0x53, 0x00},
    {0x55, 0x44},
    {0x58, 0x29},
    {0x5a, 0x00},
    {0x5b, 0x00},
    {0x5d, 0x00},
    {0x64, 0x2f},
    {0x66, 0x62},
    {0x68, 0x5b},
    {0x75, 0x46},
    {0x76, 0xf0},
    {0x77, 0x4f},
    {0x78, 0xef},
    {0x72, 0xcf},
    {0x73, 0x36},
    {0x7d, 0x0d},
    {0x7e, 0x0d},
    {0x8a, 0x22}, //77
    {0x8b, 0x22}, //77
    {0xfd, 0x01},
    {0xb1, 0x83}, //DPHY enable 8b
    {0xb3, 0x0b}, //0b;09;1d
    {0xb4, 0x14}, //MIPI PLL enable;14;35;36
    {0x9d, 0x40}, //mipi hs dc level 40/03/55
    {0xa1, 0x05}, //speed/03
    {0x94, 0x44}, //dphy time
    {0x95, 0x33}, //dphy time
    {0x96, 0x1f}, //dphy time
    {0x98, 0x45}, //dphy time
    {0x9c, 0x10}, //dphy time
    {0xb5, 0x70}, //30
    //{0xa0, 0x01}, //mipi enable
    {0x25, 0xe0},
    {0x20, 0x7b},
    {0x8f, 0x88},
    {0x91, 0x40},

    {0xfd, 0x02},
    {0x5e, 0x03},
    {0xa1, 0x04},
    {0xa3, 0x40},
    {0xa5, 0x02},
    {0xa7, 0xc4},
    {0xfd, 0x01},
    {0x86, 0x78},
    {0x89, 0x78},
    {0x87, 0x6b},
    {0x88, 0x6b},
    {0xfc, 0xe0},
    {0xfe, 0xe0},
    {0xf0, 0x40},
    {0xf1, 0x40},
    {0xf2, 0x40},
    {0xf3, 0x40},

    {0xfd, 0x02},
    {0xa0, 0x00}, //v_start_h3
    {0xa1, 0x08}, //v_start_l8
    {0xa2, 0x04}, //v_size_h3
    {0xa3, 0x38}, //v_size_l8
    {0xa4, 0x00}, //h_start_half_h3
    {0xa5, 0x04}, //h_start_half_l8, keep center
    {0xa6, 0x03}, //h_size_half_h3
    {0xa7, 0xc0}, //h_size_half_l8

    {0xfd, 0x01},
    {0x8e, 0x07},
    {0x8f, 0x80}, //MIPI column number
    {0x90, 0x04}, //MIPI row number
    {0x91, 0x38},

    {0xfd, 0x03},
    {0xc0, 0x01}, //enable transfer OTP BP information

    {0xfd, 0x04},
    {0x21, 0x14},
    {0x22, 0x14},
    {0x23, 0x14}, //enhance normal and dummy BPC

    {0xfd, 0x01},
#if 0
    {0x0d, 0x00},
    {0x06, 0xe0}, //insert dummy line , the frame rate is 30.01.
#else
    {0x0d, 0x10},
    {0x8c, 0x04}, //hts_h5
    {0x8d, 0x1d}, //hts_l8
    {0x0e, 0x05}, //vts_h8
    {0x0f, 0x31}, //vts_l8
    {0x01, 0x01},
#endif
    //{0xa0, 0x01}, //MIPI enable, stream on

    {OV2735_REG_END, 0x00},
};

static struct regval_list ov2735_regs_stream_on[] = {
    {0xfd, 0x01},
    {0xa0, 0x01},
    {OV2735_REG_END, 0x0},
};

static struct regval_list ov2735_regs_stream_off[] = {
    {0xfd, 0x01},
    {0xa0, 0x00},
    {OV2735_REG_END, 0x0},
};

static int ov2735_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
{
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = 2,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "ov2735: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int ov2735_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
{
    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = i2c->addr,
            .flags  = 0,
            .len    = 1,
            .buf    = &reg,
        },
        [1] = {
            .addr   = i2c->addr,
            .flags  = I2C_M_RD,
            .len    = 1,
            .buf    = value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "ov2735(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int ov2735_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != OV2735_REG_END) {
        if (vals->reg_num == OV2735_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = ov2735_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int ov2735_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char l = 1;
    int ret;

    ret = ov2735_read(i2c, OV2735_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != OV2735_CHIP_ID_H) {
        printk(KERN_ERR "ov2735 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = ov2735_read(i2c, OV2735_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;
    if (l != OV2735_CHIP_ID_L) {
        printk(KERN_ERR "ov2735 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }

    printk(KERN_DEBUG "ov2735 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "ov2735_reset");
        if (ret) {
            printk(KERN_ERR "ov2735: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "ov2735_pwdn");
        if (ret) {
            printk(KERN_ERR "ov2735: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ov2735_power");
        if (ret) {
            printk(KERN_ERR "ov2735: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        ov2735_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(ov2735_regulator)) {
            printk(KERN_ERR "regulator_get error!\n");
            ret = -1;
            goto err_regulator;
        }
    }
    return 0;

err_regulator:
    if (power_gpio != -1)
        gpio_free(power_gpio);
err_power_gpio:
    if (pwdn_gpio != 1)
        gpio_free(pwdn_gpio);
err_pwdn_gpio:
    if (reset_gpio != -1)
        gpio_free(reset_gpio);
err_reset_gpio:
    return ret;
}

static void deinit_gpio(void)
{
    if (power_gpio != -1)
        gpio_free(power_gpio);

    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);

    if (reset_gpio != -1)
        gpio_free(reset_gpio);

    if (ov2735_regulator)
        regulator_put(ov2735_regulator);
}

static void ov2735_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (ov2735_regulator)
        regulator_disable(ov2735_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int ov2735_power_on(void)
{
    int ret, retry;

    camera_enable_sensor_mclk(cam_bus_num, 24 * 1000 * 1000);

    if (ov2735_regulator)
        regulator_enable(ov2735_regulator);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(50);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(50);
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(10);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 1);
        m_msleep(5);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(30);
    }

    for (retry = 0; retry < 10; retry++) {
        ret = ov2735_detect(i2c_dev);
        if (!ret)
            break;
    }
    if (retry >= 10) {
        printk(KERN_ERR "ov2735: failed to detect\n");
        ov2735_power_off();
        return ret;
    }

    ret = ov2735_write_array(i2c_dev, ov2735_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "ov2735: failed to init regs\n");
        ov2735_power_off();
        return ret;
    }

    return 0;
}

static int ov2735_stream_on(void)
{
    int ret = ov2735_write_array(i2c_dev, ov2735_regs_stream_on);
    if (ret)
        printk(KERN_ERR "ov2735: failed to stream on\n");

    return ret;
}

static void ov2735_stream_off(void)
{
    int ret = ov2735_write_array(i2c_dev, ov2735_regs_stream_off);
    if (ret)
        printk(KERN_ERR "ov2735: failed to stream off\n");
}


static int ov2735_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = ov2735_read(i2c_dev, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;
    return ret;
}

static int ov2735_s_register(struct sensor_dbg_register *reg)
{
    return ov2735_write(i2c_dev, reg->reg & 0xff, reg->val & 0xff);
}

static int ov2735_set_integration_time(int value)
{
    int ret = 0;

    ret = ov2735_write(i2c_dev, 0xfd, 0x01);
    ret += ov2735_write(i2c_dev, 0x4, (unsigned char)(value & 0xff));
    ret += ov2735_write(i2c_dev, 0x3, (unsigned char)((value & 0xff00) >> 8));
    ret += ov2735_write(i2c_dev, 0x01, 0x01);
    if (ret < 0) {
        printk(KERN_ERR "ov2735: set integration time error. line=%d\n" ,__LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int ov2735_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = ov2735_again_lut;

    while (lut->gain <= ov2735_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = lut[0].index;
            return lut[0].gain;
        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut-1)->index;
            return (lut-1)->gain;
        } else {
            if ((lut->gain == ov2735_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

static int ov2735_set_analog_gain(int value)
{
    int ret = 0;

    ret = ov2735_write(i2c_dev, 0xfd, 0x01);
    ret += ov2735_write(i2c_dev, 0x24, (unsigned char)value);
    ret += ov2735_write(i2c_dev, 0x01, 0x01);
    if (ret < 0) {
        printk(KERN_ERR "ov2735: set analog gain error  line=%d" ,__LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int ov2735_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int ov2735_set_digital_gain(int value)
{
    return 0;
}

static int ov2735_set_fps(int fps)
{
    struct sensor_info *sensor_info = &ov2735_sensor_attr.sensor_info;
    unsigned int sclk = OV2735_SUPPORT_SCLK_FPS_30;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "warn: fps(0x%x) no in range\n", fps);
        return -1;
    }

    ret = ov2735_write(i2c_dev, 0xfd, 0x01);
    if(ret < 0)
        return ret;
    ret = ov2735_read(i2c_dev, 0x8c, &tmp);
    hts = tmp;
    ret += ov2735_read(i2c_dev, 0x8d, &tmp);
    if (ret < 0) {
        printk(KERN_ERR "err: ov2735_read err\n");
        return ret;
    }
    hts = (hts << 8) + tmp;

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = ov2735_write(i2c_dev, 0xfd, 0x01);
    //ret += ov2735_write(i2c_dev, 0x0d, 0x10);//frame_exp_seperate_en
    ret += ov2735_write(i2c_dev, 0x0e, (vts >> 8) & 0xff);
    ret += ov2735_write(i2c_dev, 0x0f, vts & 0xff);
    ret += ov2735_write(i2c_dev, 0x01, 0x01);
    if (ret < 0) {
        printk(KERN_ERR "err: ov2735_write err\n");
        return ret;
    }
    printk(KERN_ERR "hts: %x, vts: %x\n", hts, vts);

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}

static struct sensor_attr ov2735_sensor_attr = {
    .device_name                = OV2735_DEVICE_NAME,
    .cbus_addr                  = OV2735_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = ov2735_init_regs_1920_1080_30fps_mipi,

        .width                  = OV2735_WIDTH,
        .height                 = OV2735_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 30 << 16 | 1,
        .total_width            = 0x41d,
        .total_height           = 0x531,

        .min_integration_time   = 2,
        .max_integration_time   = 0x531-4,
        .max_again              = 259142,
        .max_dgain              = 0,
    },

    .ops  = {
        .power_on               = ov2735_power_on,
        .power_off              = ov2735_power_off,
        .stream_on              = ov2735_stream_on,
        .stream_off             = ov2735_stream_off,
        .get_register           = ov2735_g_register,
        .set_register           = ov2735_s_register,

        .set_integration_time   = ov2735_set_integration_time,
        .alloc_again            = ov2735_alloc_again,
        .set_analog_gain        = ov2735_set_analog_gain,
        .alloc_dgain            = ov2735_alloc_dgain,
        .set_digital_gain       = ov2735_set_digital_gain,
        .set_fps                = ov2735_set_fps,
    },
};

static int sensor_ov2735_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ov2735_sensor_attr.cbus_addr = i2c_addr,
    ret = camera_register_sensor(cam_bus_num, &ov2735_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sensor_ov2735_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &ov2735_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id ov2735_id_table[] = {
    { OV2735_DEVICE_NAME, 0 },
    {},
};

static struct i2c_driver ov2735_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = OV2735_DEVICE_NAME,
    },
    .probe          = sensor_ov2735_probe,
    .remove         = sensor_ov2735_remove,
    .id_table       = ov2735_id_table,
};

static struct i2c_board_info sensor_ov2735_info = {
    .type = OV2735_DEVICE_NAME,
    .addr = OV2735_DEVICE_I2C_ADDR,
};

static __init int ov2735_sensor_init(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "ov2735: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    ov2735_driver.driver.name = sensor_name;
    strcpy(ov2735_id_table[0].name, sensor_name);
    strcpy(sensor_ov2735_info.type, sensor_name);
    ov2735_sensor_attr.device_name = sensor_name;
    if (i2c_addr != -1)
        sensor_ov2735_info.addr = i2c_addr;

    int ret = i2c_add_driver(&ov2735_driver);
    if (ret) {
        printk(KERN_ERR "ov2735: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_ov2735_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "ov2735: failed to register i2c device\n");
        i2c_del_driver(&ov2735_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void ov2735_sensor_exit(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&ov2735_driver);
}

module_init(ov2735_sensor_init);
module_exit(ov2735_sensor_exit);

MODULE_DESCRIPTION("x2000 ov2735 driver");
MODULE_LICENSE("GPL");
