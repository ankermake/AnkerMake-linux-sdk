/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * SC031
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>

#define SC031_DEVICE_NAME              "sc031-dvp"
#define SC031_DEVICE_I2C_ADDR          0x30

#define SC031_CHIP_ID_H                0x00
#define SC031_CHIP_ID_L                0x31

#define SC031_REG_END                  0xffff
#define SC031_REG_DELAY                0xfffe

static int power_gpio       = -1;   // -1
static int reset_gpio       = -1;   // GPIO_PA(11)
static int pwdn_gpio        = -1;   // GPIO_PA(10)
static int i2c_bus_num      = -1;   // 3(CLK/DA:PA(17/16))
static int cam_bus_num      = -1;   // 0(VIC0)

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param(i2c_bus_num, int, 0644);
module_param(cam_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr sc031_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

/*
 * Interface    : DVP
 * MCLK         : 24Mhz,
 * resolution   : 640*480
 * FrameRate    : 80fps
 */
static struct regval_list sc031_dvp_init_regs_640_480_80fps[] = {
    {0x0103,0x01},  /* software reset */

    {0x0100,0x00},  /* stream off */
    //{0x3000,0x0f},  /* default:DVP */
    //{0x3031,0x0c},  /* default:8-bits */

    {0x3018,0x1f},
    {0x3019,0xff},
    {0x301c,0xb4},
    {0x363c,0x08},
    {0x3630,0x82},
    {0x3638,0x0f},
    {0x3639,0x08},
    {0x335b,0x80},
    {0x3636,0x25},
    {0x3640,0x02},
    {0x3306,0x38},
    {0x3304,0x48},
    {0x3389,0x01},
    {0x3385,0x31},
    {0x330c,0x18},
    {0x3315,0x38},
    {0x3306,0x28},
    {0x3309,0x68},
    {0x3387,0x51},
    {0x3306,0x48},
    {0x3366,0x04},
    {0x335f,0x80},
    {0x363a,0x00},
    {0x3622,0x01},
    {0x3633,0x62},
    {0x36f9,0x20},
    {0x3637,0x80},
    {0x363d,0x04},
    {0x3e06,0x00},
    {0x363c,0x48},
    {0x320c,0x03},
    {0x320e,0x0e},
    {0x320f,0xa8},
    {0x3306,0x38},
    {0x330b,0xb6},
    {0x36f9,0x24},
    {0x363b,0x4a},
    {0x3366,0x02},
    {0x3316,0x78},
    {0x3344,0x74},
    {0x3335,0x74},
    {0x332f,0x70},
    {0x332d,0x6c},
    {0x3329,0x6c},
    {0x363c,0x08},
    {0x3630,0x81},
    {0x3366,0x06},
    {0x3314,0x3a},
    {0x3317,0x28},
    {0x3622,0x05},
    {0x363d,0x00},
    {0x3637,0x86},
    {0x3e01,0x62},
    {0x3633,0x52},
    {0x3630,0x86},
    {0x3306,0x4c},
    {0x330b,0xa0},
    {0x3631,0x48},
    {0x33b1,0x03},
    {0x33b2,0x06},
    {0x320c,0x02},
    {0x320e,0x02},
    {0x320f,0x0d},
    {0x3e01,0x20},
    {0x3e02,0x20},
    {0x3316,0x68},
    {0x3344,0x64},
    {0x3335,0x64},
    {0x332f,0x60},
    {0x332d,0x5c},
    {0x3329,0x5c},
    {0x3310,0x10},
    {0x3637,0x87},
    {0x363e,0xf8},
    {0x3254,0x02},
    {0x3255,0x07},
    {0x3252,0x02},
    {0x3253,0xa6},
    {0x3250,0xf0},
    {0x3251,0x02},
    {0x330f,0x50},
    {0x3630,0x46},
    {0x3621,0xa2},
    {0x3621,0xa0},
    {0x4500,0x59},
    {0x3637,0x88},
    {0x3908,0x81},
    {0x3640,0x00},
    {0x3641,0x02},
    {0x363c,0x05},
    {0x363b,0x4c},
    {0x36e9,0x40},
    {0x36ea,0x36},
    {0x36ed,0x13},
    {0x36f9,0x04},
    {0x36fa,0x38},
    {0x330b,0x80},
    {0x3640,0x00},
    {0x3641,0x01},
    {0x3d08,0x00},  /* PCLK / LREF / FSYNC polarity */
    {0x3306,0x48},
    {0x3621,0xa4},
    {0x300f,0x0f},
    {0x4837,0x1b},
    {0x4809,0x01},
    {0x363b,0x48},
    {0x363c,0x06},
    {0x36e9,0x00},
    {0x36ea,0x3b},
    {0x36eb,0x1A},
    {0x36ec,0x0A},
    {0x36ed,0x33},
    {0x36f9,0x00},
    {0x36fa,0x3a},
    {0x36fc,0x01},
    {0x320c,0x03},
    {0x320d,0x6e},
    {0x320e,0x02},
    {0x320f,0xab},
    {0x330b,0x80},
    {0x330f,0x50},
    {0x3637,0x89},
    {0x3641,0x01},
    {0x4501,0xc4},
    {0x5011,0x01},
    {0x3908,0x21},
    {0x3e01,0x3e},
    {0x3e02,0x80}, // 1000
    {0x3306,0x38},
    {0x3e08,0x00}, // Coarse gain 0x00-1x 0x04-2x 0x0c-4x 0x1c-8x
    {0x3e09,0x18}, // Fine gain 1.0～1.9375    --> total gain 3.25x
    {0x330b,0xe0},
    {0x330f,0x20},
    {0x3d08,0x01},  /* PCLK / LREF / FSYNC polarity */
    {0x3314,0x65},
    {0x5011,0x00},
    {0x3e06,0x0c},
    {0x3908,0x91},
    {0x3624,0x47},
    {0x3220,0x10},
    {0x3635,0x18},
    {0x3223,0x50},
    {0x301f,0x01},
    {0x3028,0x82},
    {0x0100,0x01},

    // {0x4501,0x08},

    //Delay 10ms
    {0x4418,0x08},
    {0x4419,0x80},
    {0x363d,0x10},
    {0x3630,0x48},
    //[gain<2]
    {0x3314,0x65},
    {0x3317,0x10},
    //[4>gain>=2]
    {0x3314,0x65},
    {0x3317,0x10},
    //[gain>=4]
    {0x3314,0x60},
    {0x3317,0x0e},

    {SC031_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list sc031_regs_stream_on[] = {
    {0x0100, 0x01},
    {SC031_REG_DELAY, 0x17},
    {SC031_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list sc031_regs_stream_off[] = {
    {0x0100, 0x00},
    {SC031_REG_END, 0x00},  /* END MARKER */
};

static int sc031_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
{
    unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = 3,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "sc031: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int sc031_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
{
    unsigned char buf[2] = {reg >> 8, reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = i2c->addr,
            .flags  = 0,
            .len    = 2,
            .buf    = buf,
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
        printk(KERN_ERR "sc031(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int sc031_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != SC031_REG_END) {
        if (vals->reg_num == SC031_REG_DELAY) {
            m_msleep(vals->value);
        } else {

            ret = sc031_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int sc031_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != SC031_REG_END) {
        if (vals->reg_num == SC031_REG_DELAY) {
            m_msleep(vals->value);
        } else {

            ret = sc031_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        if (val == vals->value)
            printk(KERN_ERR "reg = 0x%02x, val = 0x%02x  == 0x%02x\n",vals->reg_num, val, vals->value);
        else
            printk(KERN_ERR "reg = 0x%02x, val = 0x%02x  != 0x%02x\n",vals->reg_num, val, vals->value);

        vals++;
    }

    return 0;
}

static int sc031_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = sc031_read(i2c, 0x3107, &h);
    if (ret < 0)
        return ret;
    if (h != SC031_CHIP_ID_H) {
        printk(KERN_ERR "sc031 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = sc031_read(i2c, 0x3108, &l);
    if (ret < 0)
        return ret;

    if (l != SC031_CHIP_ID_L) {
        printk(KERN_ERR "sc031 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "sc031 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    ret = dvp_init_select_gpio(&sc031_sensor_attr.dvp,sc031_sensor_attr.dvp.gpio_mode);
    if (ret) {
        printk(KERN_ERR "sc031: failed to init dvp pins\n");
        return ret;
    }

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "sc031_reset");
        if (ret) {
            printk(KERN_ERR "sc031: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "sc031_pwdn");
        if (ret) {
            printk(KERN_ERR "sc031: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "sc031_power");
        if (ret) {
            printk(KERN_ERR "sc031: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    return 0;

err_power_gpio:
    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);
err_pwdn_gpio:
    if (reset_gpio != -1)
        gpio_free(reset_gpio);
err_reset_gpio:
    dvp_deinit_gpio();

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

    dvp_deinit_gpio();
}

static void sc031_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int sc031_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(50);
        gpio_direction_output(pwdn_gpio, 1);
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

    ret = sc031_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "sc031: failed to detect\n");
       sc031_power_off();
        return ret;
    }

    ret = sc031_write_array(i2c_dev, sc031_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "sc031: failed to init regs\n");
        sc031_power_off();
        return ret;
    }

    return 0;
}

static int sc031_stream_on(void)
{
    int ret;

    ret = sc031_write_array(i2c_dev, sc031_regs_stream_on);
    if (ret)
        printk(KERN_ERR "sc031: failed to stream on\n");

    return ret;
}

static void sc031_stream_off(void)
{
    int ret = sc031_write_array(i2c_dev, sc031_regs_stream_off);
    if (ret)
        printk(KERN_ERR "sc031: failed to stream on\n");
}

static struct sensor_attr sc031_sensor_attr = {
    .device_name                = SC031_DEVICE_NAME,
    .cbus_addr                  = SC031_DEVICE_I2C_ADDR,

    .dma_mode                   = SENSOR_DATA_DMA_MODE_GREY, /* 是否单独提取Y数据 */
    .dbus_type                  = SENSOR_DATA_BUS_DVP,
    .dvp = {
        .hsync_polarity         = POLARITY_HIGH_ACTIVE,
        .vsync_polarity         = POLARITY_LOW_ACTIVE,
        .pclk_polarity          = POLARITY_SAMPLE_FALLING,
        .img_scan_mode          = DVP_IMG_SCAN_PROGRESS,
    },

    .sensor_info = {
        .private_init_setting   = sc031_dvp_init_regs_640_480_80fps,
        .width                  = 640,
        .height                 = 480,
        .fmt                    = SENSOR_PIXEL_FMT_Y8_1X8,

        .fps                    = 80 << 16 | 1,
    },

    .ops = {
        .power_on               = sc031_power_on,
        .power_off              = sc031_power_off,
        .stream_on              = sc031_stream_on,
        .stream_off             = sc031_stream_off,
    },
};

static int sc031_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int ret;

    ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &sc031_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sc031_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &sc031_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id sc031_id[] = {
    { SC031_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sc031_id);

static struct i2c_driver sc031_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = SC031_DEVICE_NAME,
    },
    .probe              = sc031_probe,
    .remove             = sc031_remove,
    .id_table           = sc031_id,
};

static struct i2c_board_info sensor_sc031_info = {
    .type               = SC031_DEVICE_NAME,
    .addr               = SC031_DEVICE_I2C_ADDR,
};

static __init int init_sc031(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "sc031: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&sc031_driver);
    if (ret) {
        printk(KERN_ERR "sc031: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_sc031_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "sc031: failed to register i2c device\n");
        i2c_del_driver(&sc031_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_sc031(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sc031_driver);
}

module_init(init_sc031);
module_exit(exit_sc031);

MODULE_DESCRIPTION("x2000 sc031 dvp driver");
MODULE_LICENSE("GPL");
