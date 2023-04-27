/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * JXF37 mipi driver
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>


#define JXF37_DEVICE_NAME               "jxf37"
#define JXF37_DEVICE_I2C_ADDR           0x40

#define JXF37_SUPPORT_SCLK              (37125000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5

#define JXF37_DEVICE_WIDTH              1920
#define JXF37_DEVICE_HEIGHT             1080

#define JXF37_CHIP_ID_H                 (0x0f)
#define JXF37_CHIP_ID_L                 (0x37)
#define JXF37_REG_CHIP_ID_HIGH          0x0A
#define JXF37_REG_CHIP_ID_LOW           0x0B

#define JXF37_REG_END                   0xffff
#define JXF37_REG_DELAY                 0xfffe


static int power_gpio   = -1; // GPIO_PC(21);
static int reset_gpio   = -1; // GPIO_PA(11)
static int pwdn_gpio    = -1; // -1
static int i2c_sel_gpio = -1; // -1
static int i2c_bus_num  = 0;  // 5 (SCL:PD4, SDA:PD5)

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param_gpio(i2c_sel_gpio, 0644);

module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static int vic_index = 0;

static struct sensor_attr jxf37_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

struct again_lut jxf37_again_lut[] = {
    {0x0,  0 },
    {0x1,  5731 },
    {0x2,  11136},
    {0x3,  16248},
    {0x4,  21097},
    {0x5,  25710},
    {0x6,  30109},
    {0x7,  34312},
    {0x8,  38336},
    {0x9,  42195},
    {0xa,  45904},
    {0xb,  49472},
    {0xc,  52910},
    {0xd,  56228},
    {0xe,  59433},
    {0xf,  62534},
    {0x10,  65536},
    {0x11,  71267},
    {0x12,  76672},
    {0x13,  81784},
    {0x14,  86633},
    {0x15,  91246},
    {0x16,  95645},
    {0x17,  99848},
    {0x18,  103872},
    {0x19,  107731},
    {0x1a,  111440},
    {0x1b,  115008},
    {0x1c,  118446},
    {0x1d,  121764},
    {0x1e,  124969},
    {0x1f,  128070},
    {0x20,  131072},
    {0x21,  136803},
    {0x22,  142208},
    {0x23,  147320},
    {0x24,  152169},
    {0x25,  156782},
    {0x26,  161181},
    {0x27,  165384},
    {0x28,  169408},
    {0x29,  173267},
    {0x2a,  176976},
    {0x2b,  180544},
    {0x2c,  183982},
    {0x2d,  187300},
    {0x2e,  190505},
    {0x2f,  193606},
    {0x30,  196608},
    {0x31,  202339},
    {0x32,  207744},
    {0x33,  212856},
    {0x34,  217705},
    {0x35,  222318},
    {0x36,  226717},
    {0x37,  230920},
    {0x38,  234944},
    {0x39,  238803},
    {0x3a,  242512},
    {0x3b,  246080},
    {0x3c,  249518},
    {0x3d,  252836},
    {0x3e,  256041},
    {0x3f,  259142},
/*
    {0x40,  262144},
    {0x41,  267875},
    {0x42,  273280},
    {0x43,  278392},
    {0x44,  283241},
    {0x45,  287854},
    {0x46,  292253},
    {0x47,  296456},
    {0x48,  300480},
    {0x49,  304339},
    {0x4a,  308048},
    {0x4b,  311616},
    {0x4c,  315054},
    {0x4d,  318372},
    {0x4e,  321577},
    {0x4f,  324678},
*/
};

/*
 * Interface    : MIPI - 2lane
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 648MHz
 * resolution   : 1920*1080
 * FrameRate    : 25fps
 */
static struct regval_list jxf37_1920_1080_25fps_mipi_init_regs[] = {
    {0x12,0x40},
    {0x48,0x8A},
    {0x48,0x0A},
    {0x0E,0x11},
    {0x0F,0x14},
    {0x10,0x36},
    {0x11,0x80},
    {0x0D,0xF0},
    {0x5F,0x41},
    {0x60,0x20},
    {0x58,0x12},
    {0x57,0x60},
    {0x9D,0x00},
    {0x20,0x80},
    {0x21,0x07},
    {0x22,0x46},
    {0x23,0x05},
    {0x24,0xC0},
    {0x25,0x38},
    {0x26,0x43},

    {0x27,0x1A},
    {0x28,0x15},
    {0x29,0x07},
    {0x2A,0x0A},
    {0x2B,0x17},
    {0x2C,0x00},
    {0x2D,0x00},
    {0x2E,0x14},
    {0x2F,0x44},
    {0x41,0xC8},
    {0x42,0x3B},
    {0x47,0x42},
    {0x76,0x60},
    {0x77,0x09},
    {0x1D,0x00},
    {0x1E,0x04},
    {0x6B,0xCC},
    {0x6C,0x40},    /* second lane disable */
    {0x6E,0x2C},

    {0x70,0xD0},    /* bit[4:2] clk-prepare */
    {0x71,0x94},    /* bit[4:0] clk-zero */
                    /* bit[7:5] HS-zero */
    {0x72,0x94},    /* bit[7:5] hs-prepare */
    {0x73,0x58},
    {0x74,0x02},
    {0x78,0x96},
    {0x89,0x01},
    {0x6B,0x20},

    {0x86,0x40},
    {0x31,0x0C},
    {0x32,0x38},
    {0x33,0x6C},
    {0x34,0x88},
    {0x35,0x88},
    {0x3A,0xAF},
    {0x3B,0x00},
    {0x3C,0x57},
    {0x3D,0x78},
    {0x3E,0xFF},
    {0x3F,0xF8},
    {0x40,0xFF},
    {0x56,0xB2},
    {0x59,0xE8},
    {0x5A,0x04},
    {0x85,0x70},
    {0x8A,0x04},
    {0x91,0x13},
    {0x9B,0x03},
    {0x9C,0xE1},
    {0xA9,0x78},
    {0x5B,0xB0},
    {0x5C,0x71},
    {0x5D,0xF6},
    {0x5E,0x14},
    {0x62,0x01},
    {0x63,0x0F},
    {0x64,0xC0},
    {0x65,0x02},
    {0x67,0x65},
    {0x66,0x04},
    {0x68,0x00},
    {0x69,0x7C},
    {0x6A,0x12},
    {0x7A,0x80},
    {0x82,0x21},
    {0x8F,0x91},
    {0xAE,0x30},
    {0x13,0x81},
    {0x96,0x04},
    {0x4A,0x05},
    {0x7E,0xCD},
    {0x50,0x02},
    {0x49,0x10},
    {0xAF,0x12},
    {0x80,0x41},
    {0x7B,0x4A},
    {0x7C,0x08},
    {0x7F,0x57},
    {0x90,0x00},
    {0x8C,0xFF},
    {0x8D,0xC7},
    {0x8E,0x00},
    {0x8B,0x01},
    {0x0C,0x00},
    {0x81,0x74},
    {0x19,0x20},
    {0x46,0x00},
    {0x48,0x8A},
    {0x48,0x0A},
    {JXF37_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list jxf37_regs_stream_on[] = {
    {0x12,0x20},
    {JXF37_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list jxf37_regs_stream_off[] = {
    {0x12,0x60},
    {JXF37_REG_END, 0x0}, /* END MARKER */
};


static int jxf37_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
{
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr = i2c->addr,
        .flags  = 0,
        .len    = 2,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "jxf37: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int jxf37_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
{
    unsigned char buf[1] = {reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr = i2c->addr,
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
        printk(KERN_ERR "jxf37(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int jxf37_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != JXF37_REG_END) {
        if (vals->reg_num == JXF37_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = jxf37_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        // printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static inline int jxf37_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != JXF37_REG_END) {
        if (vals->reg_num == JXF37_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = jxf37_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n",vals->reg_num, val);
        vals++;
    }

    return 0;
}

static int jxf37_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char l = 1;
    int ret;

    ret = jxf37_read(i2c, JXF37_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != JXF37_CHIP_ID_H) {
        printk(KERN_ERR "jxf37 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = jxf37_read(i2c, JXF37_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;

    if (l != JXF37_CHIP_ID_L) {
        printk(KERN_ERR"jxf37 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }

    printk(KERN_DEBUG "jxf37 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "jxf37_reset");
        if (ret) {
            printk(KERN_ERR "jxf37: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "jxf37_pwdn");
        if (ret) {
            printk(KERN_ERR "jxf37: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "jxf37_power");
        if (ret) {
            printk(KERN_ERR "jxf37: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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

static void jxf37_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk(vic_index);
}


static int jxf37_power_on(void)
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
        m_msleep(10);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
    }

    if (i2c_sel_gpio != -1)
        gpio_direction_output(i2c_sel_gpio, 1);

    int ret;
    ret = jxf37_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "jxf37: failed to detect\n");
        jxf37_power_off();
        return ret;
    }

    ret = jxf37_write_array(i2c_dev, jxf37_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "jxf37: failed to init regs\n");
        jxf37_power_off();
        return ret;
    }

    return 0;
}

static int jxf37_stream_on(void)
{
    int ret = jxf37_write_array(i2c_dev, jxf37_regs_stream_on);

    if (ret)
        printk(KERN_ERR "jxf37: failed to stream on\n");

    return ret;
}

static void jxf37_stream_off(void)
{
    int ret = jxf37_write_array(i2c_dev, jxf37_regs_stream_off);

    if (ret)
        printk(KERN_ERR "jxf37: failed to stream on\n");
}

static int jxf37_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = jxf37_read(i2c_dev, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;
    return ret;
}

static int jxf37_s_register(struct sensor_dbg_register *reg)
{
    return jxf37_write(i2c_dev, reg->reg & 0xff, reg->val & 0xff);
}

static int jxf37_set_integration_time(int value)
{
    int ret = 0;
    unsigned int expo = value;

    ret  = jxf37_write(i2c_dev, 0x01, (unsigned char)(expo & 0xff));
    ret += jxf37_write(i2c_dev, 0x02, (unsigned char)((expo >> 8) & 0xff)) ;

    if (ret < 0)
        return ret;

    return 0;
}

static unsigned int jxf37_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = jxf37_again_lut;

    while (lut->gain <= jxf37_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        }
        else if (isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        } else {
            if ((lut->gain == jxf37_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

static unsigned char val_99 = 0x0F;
static unsigned char val_9b = 0x0F;
static int jxf37_set_analog_gain(int value)
{
    unsigned char tmp1;
    unsigned char tmp2;
    unsigned char tmp99;
    unsigned char tmp9b;
    int ret;

    if (value < 0x10) {
        tmp1 = 0x00;
        tmp2 = 0x04;
    } else if (value >= 0x10 && value < 0x30) {
        tmp1 = 0x00;
        tmp2 = 0x24;
    } else {
        tmp1 = 0x40;
        tmp2 = 0x24;
    }

    ret = jxf37_write(i2c_dev, 0x00, (unsigned char)(value & 0x7f));

    /* complement the steak */
    if (value <= 0x40){
        tmp99 = (unsigned char)(val_99 & 0x0f);
        tmp9b = (unsigned char)(val_9b & 0x0f);
    } else {
        tmp99 = (unsigned char)(val_99);
        tmp9b = (unsigned char)(val_9b);
    }

    ret += jxf37_write(i2c_dev, 0x0c, tmp1);
    ret += jxf37_write(i2c_dev, 0x66, tmp2);
    ret += jxf37_write(i2c_dev, 0x99, tmp99);
    ret += jxf37_write(i2c_dev, 0x9b, tmp9b);
    if (ret < 0)
        return ret;

    return 0;
}

static unsigned int jxf37_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{

    return isp_gain;
}

static int jxf37_set_digital_gain(int value)
{
    return 0;
}

static int jxf37_set_fps(int fps)
{
    struct sensor_info *sensor_info = &jxf37_sensor_attr.sensor_info;
    unsigned int sclk = 0;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char val = 0;
    unsigned int newformat = 0;
    unsigned int max_fps = 0;
    int ret = 0;

    sclk = JXF37_SUPPORT_SCLK;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if (newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "warn: fps(%x) no in range\n", fps);
        return -1;
    }

    val = 0;
    ret += jxf37_read(i2c_dev, 0x21, &val);
    hts = val << 8;
    val = 0;
    ret += jxf37_read(i2c_dev, 0x20, &val);
    hts |= val;
    hts *= 2;
    if (ret < 0) {
        printk(KERN_ERR "err: jxf37 read err\n");
        return ret;
    }

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    jxf37_write(i2c_dev, 0xc0, 0x22);
    jxf37_write(i2c_dev, 0xc1, (unsigned char)(vts & 0xff));
    jxf37_write(i2c_dev, 0xc2, 0x23);
    jxf37_write(i2c_dev, 0xc3, (unsigned char)(vts >> 8));
    ret = jxf37_read(i2c_dev, 0x1f, &val);
    if(ret < 0)
        return -1;

    val |= (1 << 7); //set bit[7],  register group write function,  auto clean
    ret = jxf37_write(i2c_dev, 0x1f, val);
    if (0 != ret) {
        printk(KERN_ERR "err: jxf37_write err\n");
        return ret;
    }

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}


static struct sensor_attr jxf37_sensor_attr = {
    .device_name                = JXF37_DEVICE_NAME,
    .cbus_addr                  = JXF37_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = jxf37_1920_1080_25fps_mipi_init_regs,

        .width                  = JXF37_DEVICE_WIDTH,
        .height                 = JXF37_DEVICE_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 25 << 16 | 1,  /* 25/1 */
        .total_width            = 2560,
        .total_height           = 1350,

        .min_integration_time   = 2,
        .max_integration_time   = 1350 - 4,
        .max_again              = 259142,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = jxf37_power_on,
        .power_off              = jxf37_power_off,
        .stream_on              = jxf37_stream_on,
        .stream_off             = jxf37_stream_off,
        .get_register           = jxf37_g_register,
        .set_register           = jxf37_s_register,

        .set_integration_time   = jxf37_set_integration_time,
        .alloc_again            = jxf37_alloc_again,
        .set_analog_gain        = jxf37_set_analog_gain,
        .alloc_dgain            = jxf37_alloc_dgain,
        .set_digital_gain       = jxf37_set_digital_gain,
        .set_fps                = jxf37_set_fps,
    },
};

static int jxf37_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(vic_index, &jxf37_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int jxf37_remove(struct i2c_client *client)
{
    camera_unregister_sensor(vic_index, &jxf37_sensor_attr);
    deinit_gpio();

    return 0;
}

static const struct i2c_device_id jxf37_id[] = {
    { JXF37_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, jxf37_id);

static struct i2c_driver jxf37_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = JXF37_DEVICE_NAME,
    },
    .probe          = jxf37_probe,
    .remove         = jxf37_remove,
    .id_table       = jxf37_id,
};

static struct i2c_board_info sensor_jxf37_info = {
    .type           = JXF37_DEVICE_NAME,
    .addr           = JXF37_DEVICE_I2C_ADDR,
};

static __init int init_jxf37(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "jxf37: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&jxf37_driver);
    if (ret) {
        printk(KERN_ERR "jxf37: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_jxf37_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "jxf37: failed to register i2c device\n");
        i2c_del_driver(&jxf37_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_jxf37(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&jxf37_driver);
}

module_init(init_jxf37);
module_exit(exit_jxf37);


MODULE_DESCRIPTION("X2000 JXF37 driver");
MODULE_LICENSE("GPL");
