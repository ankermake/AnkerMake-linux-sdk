/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * JXH62 mipi driver
 *
 */
#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>

#define JXH62_DEVICE_NAME              "jxh62"
#define JXH62_DEVICE_I2C_ADDR          0x30     // 这个地址会被安装驱动时传入的值覆盖

#define JXH62_SUPPORT_SCLK_FPS_25      43200000
#define JXH62_DEVICE_WIDTH             1280
#define JXH62_DEVICE_HEIGHT            720
#define SENSOR_OUTPUT_MAX_FPS          30
#define SENSOR_OUTPUT_MIN_FPS          5

#define JXH62_CHIP_ID_H                (0xa0)
#define JXH62_CHIP_ID_L                (0x62)
#define JXH62_REG_CHIP_ID_HIGH         0x0a
#define JXH62_REG_CHIP_ID_LOW          0x0b

#define JXH62_REG_END                  0xff
#define JXH62_REG_DELAY                0xfe

static int power_gpio    = -1;
static int reset_gpio    = -1; // GPIO_PA(10);
static int pwdn_gpio     = -1; // GPIO_PA(11);
static int i2c_bus_num   = -1;  // 3
static short i2c_addr    = -1;
static int cam_bus_num   = -1;
static char *sensor_name = NULL;
static char *regulator_name = "";

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, short, 0644);
module_param(cam_bus_num, int, 0644);
module_param(sensor_name, charp, 0644);
module_param(regulator_name, charp, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr jxh62_sensor_attr;
static struct regulator *jxh62_regulator = NULL;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

static struct again_lut jxh62_again_lut[] = {
    {0x0, 0},
    {0x1, 5731},
    {0x2, 11136},
    {0x3, 16248},
    {0x4, 21097},
    {0x5, 25710},
    {0x6, 30109},
    {0x7, 34312},
    {0x8, 38336},
    {0x9, 42195},
    {0xa, 45904},
    {0xb, 49472},
    {0xc, 52910},
    {0xd, 56228},
    {0xe, 59433},
    {0xf, 62534},
    {0x10, 65536},
    {0x11, 71267},
    {0x12, 76672},
    {0x13, 81784},
    {0x14, 86633},
    {0x15, 91246},
    {0x16, 95645},
    {0x17, 99848},
    {0x18, 103872},
    {0x19, 107731},
    {0x1a, 111440},
    {0x1b, 115008},
    {0x1c, 118446},
    {0x1d, 121764},
    {0x1e, 124969},
    {0x1f, 128070},
    {0x20, 131072},
    {0x21, 136803},
    {0x22, 142208},
    {0x23, 147320},
    {0x24, 152169},
    {0x25, 156782},
    {0x26, 161181},
    {0x27, 165384},
    {0x28, 169408},
    {0x29, 173267},
    {0x2a, 176976},
    {0x2b, 180544},
    {0x2c, 183982},
    {0x2d, 187300},
    {0x2e, 190505},
    {0x2f, 193606},
    {0x30, 196608},
    {0x31, 202339},
    {0x32, 207744},
    {0x33, 212856},
    {0x34, 217705},
    {0x35, 222318},
    {0x36, 226717},
    {0x37, 230920},
    {0x38, 234944},
    {0x39, 238803},
    {0x3a, 242512},
    {0x3b, 246080},
    {0x3c, 249518},
    {0x3d, 252836},
    {0x3e, 256041},
    {0x3f, 259142},
    {0x40, 262144},
    {0x41, 267875},
    {0x42, 273280},
    {0x43, 278392},
    {0x44, 283241},
    {0x45, 287854},
    {0x46, 292253},
    {0x47, 296456},
    {0x48, 300480},
    {0x49, 304339},
    {0x4a, 308048},
    {0x4b, 311616},
    {0x4c, 315054},
    {0x4d, 318372},
    {0x4e, 321577},
    {0x4f, 324678},
};

/*
 * Interface    : MIPI - 1lane
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 216MHz
 * resolution   : 1280*720
 * FrameRate    : 25fps
 * HTS          : 1920
 * VTS          : 900
 * SysClk       : 43.20000
 */

static struct regval_list jxh62_1280_720_25fps_mipi_init_regs[] = {

    {0x12, 0x40},
    {0x0E, 0x11},
    {0x0F, 0x09},
    {0x10, 0x24},
    {0x11, 0x80},
    {0x19, 0x68},
    {0x20, 0x80},
    {0x21, 0x07},  //frame_w 0x780 1920
    {0x22, 0x84},
    {0x23, 0x03},  //frame_h 0x384 900
    {0x24, 0x00},  //output_w 0x500  1280
    {0x25, 0xD0},  //output_h 0x2d0  720
    {0x26, 0x25},
    {0x27, 0x94},  //w_start 0x294 660
    {0x28, 0x15},  //h_start 0x015 21
    {0x29, 0x02},
    {0x2A, 0x43},
    {0x2B, 0x21},
    {0x2C, 0x08},
    {0x2D, 0x01},
    {0x2E, 0xBB},
    {0x2F, 0xC0},
    {0x41, 0x88},
    {0x42, 0x12},
    {0x39, 0x90},
    {0x1D, 0x00},
    {0x1E, 0x04},
    {0x7A, 0x4C},
    {0x70, 0x89},  //[1]:mipi 10 bits
    {0x71, 0x4A},
    {0x72, 0x48},
    {0x73, 0x43},
    {0x74, 0x52},
    {0x75, 0x2B},
    {0x76, 0x40},
    {0x77, 0x06},
    {0x78, 0x20},
    {0x66, 0x38},
    {0x1F, 0x00},
    {0x30, 0x98},
    {0x31, 0x0E},
    {0x32, 0xF0},
    {0x33, 0x0E},
    {0x34, 0x2C},
    {0x35, 0xA3},
    {0x38, 0x40},
    {0x3A, 0x08},
    {0x56, 0x02},
    {0x60, 0x01},
    {0x0D, 0x50},
    {0x57, 0x80},
    {0x58, 0x33},
    {0x5A, 0x04},
    {0x5B, 0xB6},
    {0x5C, 0x08},
    {0x5D, 0x67},
    {0x5E, 0x04},
    {0x5F, 0x08},
    {0x66, 0x28},
    {0x67, 0xF8},
    {0x68, 0x00},
    {0x69, 0x74},
    {0x6A, 0x1F},
    {0x63, 0x80},
    {0x6C, 0xC0},
    {0x6E, 0x5C},
    {0x82, 0x01},
    {0x0C, 0x00},
    {0x46, 0xC2},
    {0x48, 0x7E},
    {0x62, 0x40},
    {0x7D, 0x57},
    {0x7E, 0x28},
    {0x80, 0x00},
    {0x4A, 0x05},
    {0x49, 0x10},
    {0x13, 0x81},
    {0x59, 0x97},
    {0x12, 0x00},  //no mirror & no flip
    {0x47, 0x47},
    {0x47, 0x44},
    {0x1F, 0x01},
    {0x12, 0x40},
    {JXH62_REG_DELAY, 250 },
    {JXH62_REG_DELAY, 250 },
    {JXH62_REG_END, 0x0},
};

static struct regval_list jxh62_regs_stream_on[] = {
    {0x12, 0x00},
    {JXH62_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list jxh62_regs_stream_off[] = {
    {0x12, 0x40},
    {JXH62_REG_END, 0x0}, /* END MARKER */
};

static int jxh62_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
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
        printk(KERN_ERR "jxh62: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int jxh62_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
{
    unsigned char buf[1] = {reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr  = i2c->addr,
            .flags = 0,
            .len   = 1,
            .buf   = buf,
        },
        [1] = {
            .addr  = i2c->addr,
            .flags = I2C_M_RD,
            .len   = 1,
            .buf   = value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "jxh62(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int jxh62_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != JXH62_REG_END) {
        if (vals->reg_num == JXH62_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = jxh62_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int jxh62_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != JXH62_REG_END) {
        if (vals->reg_num == JXH62_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = jxh62_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n", vals->reg_num, val);
        vals++;
    }

    return 0;
}

static int jxh62_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char l = 1;
    int ret;

    ret = jxh62_read(i2c, JXH62_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != JXH62_CHIP_ID_H) {
        printk(KERN_ERR "jxh62 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = jxh62_read(i2c, JXH62_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;

    if (l != JXH62_CHIP_ID_L) {
        printk(KERN_ERR "jxh62 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "jxh62 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "jxh62_reset");
        if (ret) {
            printk(KERN_ERR "jxh62: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "jxh62_pwdn");
        if (ret) {
            printk(KERN_ERR "jxh62: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "jxh62_power");
        if (ret) {
            printk(KERN_ERR "jxh62: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        jxh62_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(jxh62_regulator)) {
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
    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);

    if (reset_gpio != -1)
        gpio_free(reset_gpio);

    if (power_gpio != -1)
        gpio_free(power_gpio);

    if (jxh62_regulator)
        regulator_put(jxh62_regulator);
}

static void jxh62_power_off(void)
{
    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    camera_disable_sensor_mclk(cam_bus_num);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (jxh62_regulator)
        regulator_disable(jxh62_regulator);
}

static int jxh62_power_on(void)
{
    camera_enable_sensor_mclk(cam_bus_num, 24 * 1000 * 1000);

    if (jxh62_regulator) {
        regulator_enable(jxh62_regulator);
        regulator_disable(jxh62_regulator);
        m_msleep(5);
        regulator_enable(jxh62_regulator);
        m_msleep(10);
    }

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 0);
        m_msleep(10);
        gpio_direction_output(power_gpio, 1);
        m_msleep(10);
    }

    if (reset_gpio != -1) {
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(20);
    }

    if (pwdn_gpio != -1) {
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(10);
    }

    int ret;
    ret = jxh62_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "jxh62: failed to detect\n");
        jxh62_power_off();
        return ret;
    }

    ret = jxh62_write_array(i2c_dev, jxh62_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "jxh62: failed to init regs\n");
        jxh62_power_off();
        return ret;
    }

    return 0;
}

static int jxh62_stream_on(void)
{
    int ret = jxh62_write_array(i2c_dev, jxh62_regs_stream_on);

    if (ret)
        printk(KERN_ERR "jxh62: failed to stream on\n");

    return ret;
}

static void jxh62_stream_off(void)
{
    int ret = jxh62_write_array(i2c_dev, jxh62_regs_stream_off);

    if (ret)
        printk(KERN_ERR "jxh62: failed to stream off\n");
}

static int jxh62_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = jxh62_read(i2c_dev, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;
    return ret;
}

static int jxh62_s_register(struct sensor_dbg_register *reg)
{
    return jxh62_write(i2c_dev, reg->reg & 0xff, reg->val & 0xff);
}

static int jxh62_set_integration_time(int value)
{
    int ret = 0;
    unsigned int expo = value;

    ret = jxh62_write(i2c_dev, 0x01, (unsigned char)(expo & 0xff));
    ret += jxh62_write(i2c_dev, 0x02, (unsigned char)((expo >> 8) & 0xff));
    if (ret < 0) {
        printk(KERN_ERR "jxh62 set integration time error\n");
        return ret;
    }

    return 0;
}

static unsigned int jxh62_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = jxh62_again_lut;

    while (lut->gain <= jxh62_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        }
        else if (isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        } else {
            if ((lut->gain == jxh62_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

static int jxh62_set_analog_gain(int value)
{
    int ret = 0;

     ret = jxh62_write(i2c_dev, 0x00, (unsigned char)(value & 0x7f));
    if (ret < 0) {
        printk(KERN_ERR "jxh62 set analog gain error\n");
        return ret;
    }

    return 0;
}

static unsigned int jxh62_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{

    return isp_gain;
}

static int jxh62_set_digital_gain(int value)
{
    return 0;
}

static int jxh62_set_fps(int fps)
{
    struct sensor_info *sensor_info = &jxh62_sensor_attr.sensor_info;
    unsigned int pclk = JXH62_SUPPORT_SCLK_FPS_25;
    unsigned short hts, vts;
    unsigned int newformat;
    unsigned char tmp;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if (newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "warn: fps(%x) no in range\n", fps);
        return -1;
    }

    ret = jxh62_read(i2c_dev, 0x21, &tmp);
    hts = tmp << 8;
    ret += jxh62_read(i2c_dev, 0x20, &tmp);
    if (ret < 0)
        return -1;
    hts |= tmp;

    vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    jxh62_write(i2c_dev, 0xc0, 0x22);
    jxh62_write(i2c_dev, 0xc1, (unsigned char)(vts & 0xff));
    jxh62_write(i2c_dev, 0xc2, 0x23);
    jxh62_write(i2c_dev, 0xc3, (unsigned char)(vts >> 8));
    ret = jxh62_read(i2c_dev, 0x1f, &tmp);
    if(ret < 0)
        return -1;

    tmp |= (1 << 7); //set bit[7],  register group write function,  auto clean
    jxh62_write(i2c_dev, 0x1f, tmp);

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}


static struct sensor_attr jxh62_sensor_attr = {
    .device_name                = JXH62_DEVICE_NAME,
    .cbus_addr                  = JXH62_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 1,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 300 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = jxh62_1280_720_25fps_mipi_init_regs,

        .width                  = JXH62_DEVICE_WIDTH,
        .height                 = JXH62_DEVICE_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 25 << 16 | 1,
        .total_width            = 1920,
        .total_height           = 900,

        .min_integration_time   = 2,
        .max_integration_time   = 900 - 2,
        .max_again              = 324678,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = jxh62_power_on,
        .power_off              = jxh62_power_off,
        .stream_on              = jxh62_stream_on,
        .stream_off             = jxh62_stream_off,
        .get_register           = jxh62_g_register,
        .set_register           = jxh62_s_register,

        .set_integration_time   = jxh62_set_integration_time,
        .alloc_again            = jxh62_alloc_again,
        .set_analog_gain        = jxh62_set_analog_gain,
        .alloc_dgain            = jxh62_alloc_dgain,
        .set_digital_gain       = jxh62_set_digital_gain,
        .set_fps                = jxh62_set_fps,
    },
};

static int jxh62_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &jxh62_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int jxh62_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &jxh62_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id jxh62_id[] = {
    { JXH62_DEVICE_NAME, 0},
    { }
};
MODULE_DEVICE_TABLE(i2c, jxh62_id);

static struct i2c_driver jxh62_driver = {
    .driver = {
        .owner              = THIS_MODULE,
        .name               = JXH62_DEVICE_NAME,
    },
    .probe                  = jxh62_probe,
    .remove                 = jxh62_remove,
    .id_table               = jxh62_id,
};

static struct i2c_board_info sensor_jxh62_info = {
    .type                   = JXH62_DEVICE_NAME,
    .addr                   = JXH62_DEVICE_I2C_ADDR,
};

static __init int init_jxh62(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "jxh62: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    jxh62_driver.driver.name = sensor_name;
    strcpy(jxh62_id[0].name, sensor_name);
    strcpy(sensor_jxh62_info.type, sensor_name);
    jxh62_sensor_attr.device_name = sensor_name;
    if (i2c_addr != -1)
        sensor_jxh62_info.addr = i2c_addr;

    int ret = i2c_add_driver(&jxh62_driver);
    if (ret) {
        printk(KERN_ERR "jxh62: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_jxh62_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "jxh62: failed to register i2c device\n");
        i2c_del_driver(&jxh62_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_jxh62(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&jxh62_driver);
}

module_init(init_jxh62);
module_exit(exit_jxh62);

MODULE_DESCRIPTION("X2000 JXH62 driver");
MODULE_LICENSE("GPL");
