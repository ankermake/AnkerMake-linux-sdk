/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * GC5035 mipi driver
 *
 */
#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>

#define GC5035_DEVICE_NAME              "gc5035"
#define GC5035_DEVICE_I2C_ADDR          0x37
#define GC5035_SUPPORT_30FPS_SCLK       (175200000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5


// #define GC5035_DEVICE_WIDTH             1280
// #define GC5035_DEVICE_HEIGHT            960
#define GC5035_DEVICE_WIDTH             2048
#define GC5035_DEVICE_HEIGHT            1536

#define GC5035_CHIP_ID_H                (0x50)
#define GC5035_CHIP_ID_L                (0x35)
#define GC5035_REG_CHIP_ID_HIGH         0xF0
#define GC5035_REG_CHIP_ID_LOW          0xF1

#define GC5035_MAX_WIDTH                2592
#define GC5035_MAX_HEIGHT               1944


#define GC5035_REG_END                  0xffff
#define GC5035_REG_DELAY                0xfffe

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int index;
    unsigned int gain;
};

static int power_gpio   = -1; // GPIO_PB(20);
static int reset_gpio   = -1; // -1
static int pwdn_gpio    = -1; // GPIO_PB(18);
static int i2c_sel_gpio = -1; // -1
static int i2c_bus_num  = 0;  // 5

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param_gpio(i2c_sel_gpio, 0644);

module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static int vic_index = 0;

static struct sensor_attr gc5035_sensor_attr;

static struct again_lut gc5035_again_lut[] = {
    {0x0, 0},
    {0x1, 15616},
    {0x2, 31704},
    {0x3, 47918},
    {0x8, 63668},
    {0x9, 80371},
    {0xa, 97347},
    {0xb, 112881},
    {0xc, 128637},
    {0xd, 146301},
    {0xe, 162907},
    {0xf, 179550},
    {0x10, 194211},
    {0x11, 209810},
    {0x12, 226713},
    {0x13, 242223},
    {0x14, 259755},
};

/*
 * Interface    : MIPI - 2lane
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 594MHz
 * PCLK         : 87.6Mhz
 * resolution   : 1280*720
 * FrameRate    : 30fps
 * HTS          : 1460
 * VTS          : 2008
 * row_time     : 16.66us
 */
#if 0
static struct regval_list gc5035_1280_960_30fps_mipi_init_regs[] = {
    /*SYSTEM*/
    {0xfc,0x01},
    {0xf4,0x40},
    {0xf5,0xe4},
    {0xf6,0x14},
    {0xf8,0x49},
    {0xf9,0x12},
    {0xfa,0x01},
    {0xfc,0x81},
    {0xfe,0x00},
    {0x36,0x01},
    {0xd3,0x87},
    {0x36,0x00},
    {0x33,0x20},
    {0xfe,0x03},
    {0x01,0x87},
    {0xf7,0x11},
    {0xfc,0x8f},
    {0xfc,0x8f},
    {0xfc,0x8e},
    {0xfe,0x00},
    {0xee,0x30},
    {0x87,0x18},
    {0xfe,0x01},
    {0x8c,0x90},
    {0xfe,0x00},
    /*Analog & CISCTL*/
    {0x03,0x02},
    {0x04,0x58},
    {0x41,0x07},
    {0x42,0xcc},
    {0x05,0x02},
    {0x06,0xda},
    {0x9d,0x0c},
    {0x09,0x00},
    {0x0a,0x04},
    {0x0b,0x00},
    {0x0c,0x03},
    {0x0d,0x07},   /* win height */
    {0x0e,0xa8},
    {0x0f,0x0a},   /* win width*/
    {0x10,0x30},
    {0x11,0x02},
    {0x17,0x80},
    {0x19,0x05},
    {0xfe,0x02},
    {0x30,0x03},
    {0x31,0x03},
    {0xfe,0x00},
    {0xd9,0xc0},
    {0x1b,0x20},
    {0x21,0x60},
    {0x28,0x22},
    {0x29,0x30},
    {0x44,0x18},
    {0x4b,0x10},
    {0x4e,0x20},
    {0x50,0x11},
    {0x52,0x33},
    {0x53,0x44},
    {0x55,0x10},
    {0x5b,0x11},
    {0xc5,0x02},
    {0x8c,0x20},
    {0xfe,0x02},
    {0x33,0x05},
    {0x32,0x38},
    {0xfe,0x00},
    {0x91,0x15},
    {0x92,0x3a},
    {0x93,0x20},
    {0x95,0x45},
    {0x96,0x35},
    {0xd5,0xf0},
    {0x97,0x20},
    {0x16,0x0c},
    {0x1a,0x1a},
    {0x1f,0x19},
    {0x20,0x10},
    {0x46,0x83},
    {0x4a,0x04},
    {0x54,0x02},
    {0x62,0x00},
    {0x72,0x8f},
    {0x73,0x89},
    {0x7a,0x05},
    {0x7d,0xcc},
    {0x90,0x00},
    {0xce,0x18},
    {0xd0,0xb3},
    {0xd2,0x40},
    {0xe6,0xe0},
    {0xfe,0x02},
    {0x12,0x01},
    {0x13,0x01},
    {0x14,0x02},
    {0x15,0x00},
    {0x22,0x7c},
    {0xfe,0x00},
    {0xfc,0x88},
    {0xfe,0x10},
    {0xfe,0x00},
    {0xfc,0x8e},
    {0xfe,0x00},
    {0xfe,0x00},
    {0xfe,0x00},
    {0xfc,0x88},
    {0xfe,0x10},
    {0xfe,0x00},
    {0xfc,0x8e},
    /*GAIN*/
    {0xfe,0x00},
    {0xb0,0x6e},
    {0xb1,0x01},
    {0xb2,0x00},
    {0xb3,0x00},
    {0xb4,0x00},
    {0xb6,0x00},
    /*ISP*/
    {0xfe,0x01},
    {0x53,0x00},
    {0x89,0x03},
    {0x60,0x40},
    /*BLK*/
    {0xfe,0x01},
    {0x42,0x21},
    {0x49,0x00},
    {0x4a,0x01},
    {0x4b,0xf8},
    {0x55,0x00},
    /*anti_blooming*/
    {0xfe,0x01},
    {0x41,0x28},
    {0x4c,0x00},
    {0x4d,0x00},
    {0x4e,0x06},
    {0x44,0x02},
    {0x48,0x02},
    /*CROP*/
    {0xfe,0x01},
    {0x91,0x00},
    {0x92,0x0a},
    {0x93,0x00},
    {0x94,0x0b},
    {0x95,0x03},
    {0x96,0xc0},
    {0x97,0x05},
    {0x98,0x00}, //10
    {0x99,0x00},
    /*MIPI*/
    {0xfe,0x03},
    {0x02,0x58},
    {0x03,0xb7},
    {0x15,0x14},
    {0x18,0x0f},
    {0x21,0x22},
    {0x22,0x03},
    {0x23,0x48},
    {0x24,0x12},
    {0x25,0x28},
    {0x26,0x06},
    {0x29,0x03},
    {0x2a,0x58},
    {0x2b,0x06},
    {0xfe,0x01},
    {0x8c,0x10},
    {GC5035_REG_END, 0x0}, /* END MARKER */
};
#else
/*
 * Interface    : MIPI - 2lane
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 594MHz
 * PCLK         : 175.2Mhz
 * resolution   : 2592*1944
 * FrameRate    : 30fps
 * HTS          : 2920
 * VTS          : 2008
 * row_time     : 16.66us
 */
static struct regval_list gc5035_2048_1944_30fps_mipi_init_regs[] = {
    /* SYSTEM */
    {0xfc,0x01},
    {0xf4,0x40},
    {0xf5,0xe9},
    {0xf6,0x14},
    {0xf8,0x49},
    {0xf9,0x82},
    {0xfa,0x00},
    {0xfc,0x81},
    {0xfe,0x00},
    {0x36,0x01},
    {0xd3,0x87},
    {0x36,0x00},
    {0x33,0x00},
    {0xfe,0x03},
    {0x01,0xe7},
    {0xf7,0x01},
    {0xfc,0x8f},
    {0xfc,0x8f},
    {0xfc,0x8e},
    {0xfe,0x00},
    {0xee,0x30},
    {0x87,0x18},
    {0xfe,0x01},
    {0x8c,0x90},
    {0xfe,0x00},
    /* Analog & CISCTL */
    {0x03,0x0b},
    {0x04,0xb8},
    {0x41,0x07},    /* vts 2008 */
    {0x42,0xd8},
    {0x05,0x02},    /* hts 730 */
    {0x06,0xda},
    {0x9d,0x0c},
    {0x09,0x00},    /* row start */
    {0x0a,0x04},
    {0x0b,0x00},    /* col start */
    {0x0c,0x03},
    {0x0d,0x07},    /* win height 1960 */
    {0x0e,0xa8},
    {0x0f,0x0a},    /* win width */
    {0x10,0x30},
    {0x11,0x02},
    {0x17,0x80},
    {0x19,0x05},
    {0xfe,0x02},
    {0x30,0x03},
    {0x31,0x03},
    {0xfe,0x00},
    {0xd9,0xc0},
    {0x1b,0x20},
    {0x21,0x48},
    {0x28,0x22},
    {0x29,0x58},
    {0x44,0x20},
    {0x4b,0x10},
    {0x4e,0x1a},
    {0x50,0x11},
    {0x52,0x33},
    {0x53,0x44},
    {0x55,0x10},
    {0x5b,0x11},
    {0xc5,0x02},
    {0x8c,0x1a},
    {0xfe,0x02},
    {0x33,0x05},
    {0x32,0x38},
    {0xfe,0x00},
    {0x91,0x80},
    {0x92,0x28},
    {0x93,0x20},
    {0x95,0xa0},
    {0x96,0xe0},
    {0xd5,0xfc},
    {0x97,0x28},
    {0x16,0x0c},
    {0x1a,0x1a},
    {0x1f,0x11},
    {0x20,0x10},
    {0x46,0x83},
    {0x4a,0x04},
    {0x54,0x02},
    {0x62,0x00},
    {0x72,0x8f},
    {0x73,0x89},
    {0x7a,0x05},
    {0x7d,0xcc},
    {0x90,0x00},
    {0xce,0x18},
    {0xd0,0xb2},
    {0xd2,0x40},
    {0xe6,0xe0},
    {0xfe,0x02},
    {0x12,0x01},
    {0x13,0x01},
    {0x14,0x01},
    {0x15,0x02},
    {0x22,0x7c},
    {0xfe,0x00},
    {0xfc,0x88},
    {0xfe,0x10},
    {0xfe,0x00},
    {0xfc,0x8e},
    {0xfe,0x00},
    {0xfe,0x00},
    {0xfe,0x00},
    {0xfc,0x88},
    {0xfe,0x10},
    {0xfe,0x00},
    {0xfc,0x8e},
    /* GAIN */
    {0xfe,0x00},
    {0xb0,0x6e},
    {0xb1,0x01},
    {0xb2,0x00},
    {0xb3,0x00},
    {0xb4,0x00},
    {0xb6,0x00},
    /* ISP */
    {0xfe,0x01},
    {0x53,0x00},
    {0x89,0x03},
    {0x60,0x40},
    /* BLK */
    {0xfe,0x01},
    {0x42,0x21},
    {0x49,0x03},
    {0x4a,0xff},
    {0x4b,0xc0},
    {0x55,0x00},
    /* anti_blooming */
    {0xfe,0x01},
    {0x41,0x28},
    {0x4c,0x00},
    {0x4d,0x00},
    {0x4e,0x3c},
    {0x44,0x08},
    {0x48,0x02},
    /* CROP */
    {0xfe,0x01},
    {0x91,0x00},   /* out win y */
    {0x92,0xd0},
    {0x93,0x01},   /* out win x */
    {0x94,0x18},
    {0x95,0x06},   /* out win h */
    {0x96,0x00},
    {0x97,0x08},   /* out win w */
    {0x98,0x00},
    {0x99,0x00},
    /* MIPI */
    {0xfe,0x03},
    {0x02,0x57},
    {0x03,0xb7},
    {0x15,0x14},
    {0x18,0x0f},
    {0x21,0x22},
    {0x22,0x06},
    {0x23,0x48},
    {0x24,0x12},
    {0x25,0x28},
    {0x26,0x08},
    {0x29,0x06},
    {0x2a,0x58},
    {0x2b,0x08},
    {0xfe,0x01},
    {0x8c,0x10},
    {GC5035_REG_END, 0x0}, /* END MARKER */
};
#endif
static struct regval_list gc5035_regs_stream_on[] = {
    {0xfe,0x00},
    {0x3e,0x91},
    {GC5035_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list gc5035_regs_stream_off[] = {
    {0xfe,0x00},
    {0x3e,0x00},
    {GC5035_REG_END, 0x0}, /* END MARKER */
};

static int gc5035_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
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
        printk(KERN_ERR "gc5035: failed to write reg: 0x%x\n", (int)reg);

    return ret;
}

static int gc5035_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
{
    unsigned char buf[1] = {reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = i2c->addr,
            .flags  = 0,
            .len    = 1,
            .buf    = buf,
        },
        [1] = {
            .addr = i2c->addr,
            .flags  = I2C_M_RD,
            .len    = 1,
            .buf    = value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "gc5035(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int gc5035_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != GC5035_REG_END) {
        if (vals->reg_num == GC5035_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc5035_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        // printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static inline int gc5035_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != GC5035_REG_END) {
        if (vals->reg_num == GC5035_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc5035_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n",vals->reg_num, val);
        vals++;
    }

    return 0;
}

static int gc5035_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char l = 1;
    int ret;

    ret = gc5035_read(i2c, GC5035_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != GC5035_CHIP_ID_H) {
        printk(KERN_ERR "gc5035 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = gc5035_read(i2c, GC5035_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;
    if (l != GC5035_CHIP_ID_L) {
        printk(KERN_ERR"gc5035 read chip id high failed:0x%x\n", l);
        return -ENODEV;
    }

    printk(KERN_DEBUG "gc5035 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "gc5035_reset");
        if (ret) {
            printk(KERN_ERR "gc5035: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "gc5035_pwdn");
        if (ret) {
            printk(KERN_ERR "gc5035: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gc5035_power");
        if (ret) {
            printk(KERN_ERR "gc5035: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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
}

static void gc5035_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk(vic_index);
}

static int gc5035_power_on(void)
{
    camera_enable_sensor_mclk(vic_index, 24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 0);
        m_msleep(10);
        gpio_direction_output(power_gpio, 1);
        m_msleep(10);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(10);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(20);
    }

    if (i2c_sel_gpio != -1)
        gpio_direction_output(i2c_sel_gpio, 1);

    int ret, retry;
    for (retry = 0; retry < 10; retry++) {
        ret = gc5035_detect(i2c_dev);
        if (!ret)
            break;
    }
    if (retry >= 10) {
        printk(KERN_ERR "gc5035: failed to detect\n");
        gc5035_power_off();
        return ret;
    }

    ret = gc5035_write_array(i2c_dev, gc5035_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "gc5035: failed to init regs\n");
        gc5035_power_off();
        return ret;
    }

    return 0;
}

static int gc5035_stream_on(void)
{
    int ret = gc5035_write_array(i2c_dev, gc5035_regs_stream_on);

    if (ret)
        printk(KERN_ERR "gc5035: failed to stream on\n");

    return ret;
}

static void gc5035_stream_off(void)
{
    int ret = gc5035_write_array(i2c_dev, gc5035_regs_stream_off);

    if (ret)
        printk(KERN_ERR "gc5035: failed to stream on\n");

}

static int gc5035_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = gc5035_read(i2c_dev, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;
    return ret;
}

static int gc5035_s_register(struct sensor_dbg_register *reg)
{
    return gc5035_write(i2c_dev, reg->reg & 0xff, reg->val & 0xff);
}

static int gc5035_set_integration_time(int value)
{
    int ret = 0;
    unsigned int expo = value;

    expo = expo >> 2;
    expo = expo << 2;   /* 保证为4的倍数 */

    m_msleep(1);
    ret = gc5035_write(i2c_dev, 0xfe, 0x00);
    ret += gc5035_write(i2c_dev, 0x03, (unsigned char)((expo >> 8) & 0x3f));
    ret += gc5035_write(i2c_dev, 0x04, (unsigned char)(expo & 0xff));
    if (ret < 0) {
        printk(KERN_ERR "gc5035: set integration time error. line=%d\n" ,__LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int gc5035_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = gc5035_again_lut;

    while (lut->gain <= gc5035_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = lut[0].index;
            return lut[0].gain;
        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut-1)->index;
            return (lut-1)->gain;
        } else {
            if ((lut->gain == gc5035_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

static int gc5035_set_analog_gain(int value)
{
    int ret = 0;

    ret =  gc5035_write(i2c_dev, 0xfe, 0x00);
    ret += gc5035_write(i2c_dev, 0xb6, value);
    ret += gc5035_write(i2c_dev, 0xb1, (256 >> 8) & 0xf);
    ret += gc5035_write(i2c_dev, 0xb2, 256 & 0xfc);
    if (ret < 0) {
        printk(KERN_ERR "gc5035: set analog gain error  line=%d" ,__LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int gc5035_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{

    return isp_gain;
}

static int gc5035_set_digital_gain(int value)
{
    return 0;
}

static int gc5035_set_fps(int fps)
{
    struct sensor_info *sensor_info = &gc5035_sensor_attr.sensor_info;
    unsigned int wpclk = 0;
    unsigned short vts = 0;
    unsigned short hb=0;
    unsigned short hts=0;
    unsigned int max_fps = 0;
    unsigned char tmp;
    unsigned int newformat = 0;
    int ret = 0;

    wpclk = GC5035_SUPPORT_30FPS_SCLK;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk(KERN_ERR "warn: fps(%x) no in range\n", fps);
        return -1;
    }
    /* H Blanking */
    ret = gc5035_read(i2c_dev, 0x05, &tmp);
    hb = tmp;
    ret += gc5035_read(i2c_dev, 0x06, &tmp);
    if(ret < 0)
        return -1;
    hb = (hb << 8) + tmp;
    hts = hb << 2;

    vts = wpclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = gc5035_write(i2c_dev, 0xfe, 0x00);
    ret = gc5035_write(i2c_dev, 0x41, (unsigned char)(vts >> 8) & 0x3f);
    ret += gc5035_write(i2c_dev, 0x42, (unsigned char)(vts & 0xff));
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts;

    return 0;
}


static struct sensor_attr gc5035_sensor_attr = {
    .device_name                = GC5035_DEVICE_NAME,
    .cbus_addr                  = GC5035_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = gc5035_2048_1944_30fps_mipi_init_regs,

        .width                  = GC5035_DEVICE_WIDTH,
        .height                 = GC5035_DEVICE_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SGRBG10_1X10,

        .fps                    = 30 << 16 | 1,  /* 30/1 */
        .total_width            = 2920,
        .total_height           = 2008,

        .min_integration_time   = 2,
        .max_integration_time   = 2008,
        .max_again              = 259755,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = gc5035_power_on,
        .power_off              = gc5035_power_off,
        .stream_on              = gc5035_stream_on,
        .stream_off             = gc5035_stream_off,
        .get_register           = gc5035_g_register,
        .set_register           = gc5035_s_register,

        .set_integration_time   = gc5035_set_integration_time,
        .alloc_again            = gc5035_alloc_again,
        .set_analog_gain        = gc5035_set_analog_gain,
        .alloc_dgain            = gc5035_alloc_dgain,
        .set_digital_gain       = gc5035_set_digital_gain,
        .set_fps                = gc5035_set_fps,
    },
};

static int gc5035_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(vic_index, &gc5035_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int gc5035_remove(struct i2c_client *client)
{
    camera_unregister_sensor(vic_index, &gc5035_sensor_attr);
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id gc5035_id[] = {
    { GC5035_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, gc5035_id);

static struct i2c_driver gc5035_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = GC5035_DEVICE_NAME,
    },
    .probe          = gc5035_probe,
    .remove         = gc5035_remove,
    .id_table       = gc5035_id,
};

static struct i2c_board_info sensor_gc5035_info = {
    .type           = GC5035_DEVICE_NAME,
    .addr           = GC5035_DEVICE_I2C_ADDR,
};


static __init int init_gc5035(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "gc5035: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&gc5035_driver);
    if (ret) {
        printk(KERN_ERR "gc5035: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_gc5035_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "gc5035: failed to register i2c device\n");
        i2c_del_driver(&gc5035_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_gc5035(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&gc5035_driver);
}

module_init(init_gc5035);
module_exit(exit_gc5035);

MODULE_DESCRIPTION("X2000 GC5035 driver");
MODULE_LICENSE("GPL");
