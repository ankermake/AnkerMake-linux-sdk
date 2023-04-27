/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * SC035
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <linux/regulator/consumer.h>
#include <camera/hal/camera_sensor.h>
#include <linux/interrupt.h>
#include "pwm.h"


#define CONFIG_SC035_640X480

#define SC035_DEVICE_NAME              "sc035"
#define SC035_DEVICE_I2C_ADDR          0x30

#define SC035_CHIP_ID_H                0x00
#define SC035_CHIP_ID_L                0x31

#define SC035_REG_END                  0xff
#define SC035_REG_DELAY                0xfe
#define SC035_SUPPORT_SCLK             (144000000)
#define SENSOR_OUTPUT_MAX_FPS           60
#define SENSOR_OUTPUT_MIN_FPS           5
#define SC035_MAX_WIDTH                640
#define SC035_MAX_HEIGHT               480
#if defined CONFIG_SC035_640X480
#define SC035_WIDTH                    640
#define SC035_HEIGHT                   480
#endif

static int power_gpio       = -1;
static int reset_gpio       = -1;
static int pwdn_gpio        = -1;   // GPIO_PB(18) //  GPIO_PB(19) //  XSHUTDN
static int i2c_bus_num      = -1;   // 5 // 2
static short i2c_addr       = -1;   // 0x30 // 0x33
static int cam_bus_num      = -1;   // 0 // 1
static char *sensor_name    = NULL; // sc035-ir0    //sc035-ir1
static char *regulator_name = "";
static char *sensor_mode    = NULL; // MASTER_MODE  //SALVE_MODE

static int sensor_light_ctl = -1;   // GPIO_PC(8)
static int sensor_strobe    = -1;   // GPIO_PB(12)

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param_gpio(sensor_light_ctl, 0644);
module_param_gpio(sensor_strobe, 0644);

module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, short, 0644);
module_param(cam_bus_num, int, 0644);
module_param(sensor_name, charp, 0644);
module_param(regulator_name, charp, 0644);
module_param(sensor_mode, charp ,0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr sc035_sensor_attr;
static struct regulator *sc035_regulator = NULL;

struct sc035_light_ctl{
    int strobe_cnt;
    int irq;
};
struct sc035_light_ctl light_ctl;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

struct again_lut sc035_again_lut[] = {
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
    {0x110,65536},
    {0x111,71267},
    {0x112,76672},
    {0x113,81784},
    {0x114,86633},
    {0x115,91246},
    {0x116,95645},
    {0x117,99848},
    {0x118,103872},
    {0x119,107731},
    {0x11a,111440},
    {0x11b,115008},
    {0x11c,118446},
    {0x11d,121764},
    {0x11e,124969},
    {0x11f,128070},
    {0x310,131072},
    {0x311,136803},
    {0x312,142208},
    {0x313,147320},
    {0x314,152169},
    {0x315,156782},
    {0x316,161181},
    {0x317,165384},
    {0x318,169408},
    {0x319,173267},
    {0x31a,176976},
    {0x31b,180544},
    {0x31c,183982},
    {0x31d,187300},
    {0x31e,190505},
    {0x31f,193606},
    {0x710,196608},
    {0x711,202339},
    {0x712,207744},
    {0x713,212856},
    {0x714,217705},
    {0x715,222318},
    {0x716,226717},
    {0x717,230920},
    {0x718,234944},
    {0x719,238803},
    {0x71a,242512},
    {0x71b,246080},
    {0x71c,249518},
    {0x71d,252836},
    {0x71e,256041},
};

/*
 * SensorName RAW_60fps
 * width=640
 * height=480
 * SlaveID=0x60/0x66
 * mclk=24
 * avdd=2.800000
 * dovdd=1.800000
 * dvdd=1.500000
 */
static struct regval_list sc035_init_regs_640_480_60fps_mipi[] = {

    {0x0103,0x01},
    {0x0100,0x00},
    {0x36e9,0x80},
    {0x36f9,0x80},
    {0x3000,0x00},
    {0x3001,0x00},
    {0x300f,0x0f},
    {0x3018,0x33},
    {0x3019,0xfc},
    {0x301c,0x78},
    {0x301f,0x8a},
    {0x3031,0x08},
    {0x3037,0x00},
    {0x303f,0x01},
    {0x320c,0x05},
    {0x320d,0x54},
    {0x320e,0x02},
    {0x320f,0x10},
    {0x3217,0x00},
    {0x3218,0x00},
    {0x3220,0x10},
    {0x3223,0x48},
    {0x3226,0x74},
    {0x3227,0x07},
    {0x323b,0x00},
    {0x3250,0xf0},
    {0x3251,0x02},
    {0x3252,0x02},
    {0x3253,0x08},
    {0x3254,0x02},
    {0x3255,0x07},
    {0x3304,0x48},
    {0x3305,0x00},
    {0x3306,0x98},
    {0x3309,0x50},
    {0x330a,0x01},
    {0x330b,0x18},
    {0x330c,0x18},
    {0x330f,0x40},
    {0x3310,0x10},
    {0x3314,0x1e},
    {0x3315,0x30},
    {0x3316,0x68},
    {0x3317,0x1b},
    {0x3329,0x5c},
    {0x332d,0x5c},
    {0x332f,0x60},
    {0x3335,0x64},
    {0x3344,0x64},
    {0x335b,0x80},
    {0x335f,0x80},
    {0x3366,0x06},
    {0x3385,0x31},
    {0x3387,0x39},
    {0x3389,0x01},
    {0x33b1,0x03},
    {0x33b2,0x06},
    {0x33bd,0xe0},
    {0x33bf,0x10},
    {0x3621,0xa4},
    {0x3622,0x05},
    {0x3624,0x47},
    {0x3630,0x4a},
    {0x3631,0x58},
    {0x3633,0x52},
    {0x3635,0x03},
    {0x3636,0x25},
    {0x3637,0x8a},
    {0x3638,0x0f},
    {0x3639,0x08},
    {0x363a,0x00},
    {0x363b,0x48},
    {0x363c,0x86},
    {0x363d,0x10},
    {0x363e,0xf8},
    {0x3640,0x00},
    {0x3641,0x01},
    {0x36ea,0x2f},
    {0x36eb,0x0d},
    {0x36ec,0x1d},
    {0x36ed,0x20},
    {0x36fa,0x3b},
    {0x36fb,0x10},
    {0x36fc,0x02},
    {0x36fd,0x00},
    {0x3908,0x91},
    {0x391b,0x81},
    {0x3d08,0x01},
    {0x3e01,0x30},
    {0x3e02,0x00}, // 0x300
    {0x3e03,0x2b},
    {0x3e06,0x0c},
    {0x3e08,0x04}, // Coarse gain 0x00-1x 0x04-2x 0x0c-4x 0x1c-8x
    {0x3e09,0x1a}, // Fine gain 1.0～1.9375    --> total gain 3.25x
    {0x3f04,0x03},
    {0x3f05,0x80},
    {0x4500,0x59},
    {0x4501,0xc4},
    {0x4603,0x00},
    {0x4800,0x64},
    {0x4809,0x01},
    {0x4810,0x00},
    {0x4811,0x01},
    {0x4837,0x2f},
    {0x5011,0x00},
    {0x5988,0x02},
    {0x598e,0x05},
    {0x598f,0x17},

    //60fps
    {0x320e,0x04},
    {0x320f,0x20},

    {0x36e9,0x40},
    {0x36f9,0x24},

    // {0x0100,0x01},

    //Delay 10ms
    {0x4418,0x0a},
    {0x4419,0x80},

    //[gain<2]
    {0x3314,0x1e},
    {0x3317,0x1b},//0826
    {0x3631,0x58},
    {0x3630,0x4a},

    //[4>gain>=2]
    {0x3314,0x6f},
    {0x3317,0x10},//0823
    {0x3631,0x48},
    {0x3630,0x4c},

    //[gain>=4]
    {0x3314,0x76},//0823
    {0x3317,0x15},//0823
    {0x3631,0x48},
    {0x3630,0x4c},
    {SC035_REG_DELAY,10},
    {SC035_REG_END, 0x00},    /* END MARKER */
};

 struct regval_list sc035_regs_salve[] = {

    {0x3222,0x02},
    {0x3223,0x4c},
    {0x3226,0x06},
    {0x3227,0x06},
    {0x3228,0x04},
    {0x3229,0x1a},
    {0x322b,0x0b},
    {0x3225,0x04},
    {0x3231,0x2a},

    {SC035_REG_DELAY,10},
    {SC035_REG_END, 0x00},     /* END MARKER */
};

 struct regval_list sc035_regs_master[] = {

    {SC035_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list sc035_regs_stream_on[] = {

    {0x0100, 0x01},             /* RESET_REGISTER */
    {SC035_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list sc035_regs_stream_off[] = {

    {0x0100, 0x00},             /* RESET_REGISTER */
    {SC035_REG_END, 0x00},     /* END MARKER */
};

static int sc035_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
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
        printk(KERN_ERR "sc035: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int sc035_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
        printk(KERN_ERR "sc035(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int sc035_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != SC035_REG_END) {
        if (vals->reg_num == SC035_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = sc035_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int sc035_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != SC035_REG_END) {
        if (vals->reg_num == SC035_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = sc035_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        printk("reg = 0x%02x, val = 0x%02x\n",vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int sc035_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = sc035_read(i2c, 0x3107, &h);
    if (ret < 0)
        return ret;
    if (h != SC035_CHIP_ID_H) {
        printk(KERN_ERR "sc035 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = sc035_read(i2c, 0x3108, &l);
    if (ret < 0)
        return ret;

    if (l != SC035_CHIP_ID_L) {
        printk(KERN_ERR "sc035 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "sc035 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "sc035_reset");
        if (ret) {
            printk(KERN_ERR "sc035: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "sc035_pwdn");
        if (ret) {
            printk(KERN_ERR "sc035: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "sc035_power");
        if (ret) {
            printk(KERN_ERR "sc035: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        sc035_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(sc035_regulator)) {
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

    if (sc035_regulator)
        regulator_put(sc035_regulator);
}

static void sc035_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (sc035_regulator)
        regulator_disable(sc035_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int sc035_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 24 * 1000 * 1000);

    if (sc035_regulator)
        regulator_enable(sc035_regulator);

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

    ret = sc035_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "sc035: failed to detect\n");
       sc035_power_off();
        return ret;
    }

    ret = sc035_write_array(i2c_dev, sc035_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "sc035: failed to init regs\n");
        sc035_power_off();
        return ret;
    }

    return 0;
}

static int sc035_stream_on(void)
{
    int ret;

    if(0==strcmp("MASTER_MODE",sensor_mode)){
        enable_irq(light_ctl.irq);
        ret = sc035_write_array(i2c_dev, sc035_regs_master);
    }else if(0==strcmp("SALVE_MODE",sensor_mode)){
         ret = sc035_write_array(i2c_dev, sc035_regs_salve);
    }

    light_ctl.strobe_cnt = 0;
    ret = sc035_write_array(i2c_dev, sc035_regs_stream_on);
    if (ret)
        printk(KERN_ERR "sc035: failed to stream on\n");

    return ret;
}

static void sc035_stream_off(void)
{
    if(0==strcmp("MASTER_MODE",sensor_mode) )
        disable_irq(light_ctl.irq);

    int ret = sc035_write_array(i2c_dev, sc035_regs_stream_off);
    if (ret)
        printk(KERN_ERR "sc035: failed to stream on\n");
}

static int sc035_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = sc035_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int sc035_s_register(struct sensor_dbg_register *reg)
{
    int ret;
    ret = sc035_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);

    return ret;
}

static int sc035_set_integration_time(int value)
{
    int ret = 0;
    ret = sc035_write(i2c_dev, 0x3e01, (unsigned char)((value >> 4) & 0xff));
    ret += sc035_write(i2c_dev, 0x3e02, (unsigned char)((value & 0x0f) << 4));
    if (ret < 0)
        return ret;

    return 0;

}

static unsigned int sc035_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

    struct again_lut *lut = sc035_again_lut;
    while(lut->gain <= sc035_sensor_attr.sensor_info.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == sc035_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static int sc035_set_analog_gain(int value)
{
    int ret = 0;

    ret = sc035_write(i2c_dev,0x3903,0x0b);
    ret += sc035_write(i2c_dev, 0x3e09, (unsigned char)(value & 0xff));
    ret += sc035_write(i2c_dev, 0x3e08, (unsigned char)((value >> 8 << 2) & 0x1a));
    if (ret < 0)
        return ret;

    if (value < 0x310) {
        sc035_write(i2c_dev,0x3314,0x65);
        sc035_write(i2c_dev,0x3317,0x10);
    } else if(value>=0x310&&value<=0x71f){
        sc035_write(i2c_dev,0x3314,0x60);
        sc035_write(i2c_dev,0x3317,0x0e);
    } else{
       return -1;
    }

    return 0;

}

unsigned int sc035_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int sc035_set_digital_gain(int value)
{
    return 0;
}

static int sc035_set_fps(int fps)
{
    struct sensor_info *sensor_info = &sc035_sensor_attr.sensor_info;
    unsigned int sclk = 0;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp = 0;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "sc035 fps(%d) no in range\n", fps);
        return -1;
    }
    sclk = SC035_SUPPORT_SCLK;

    ret = sc035_read(i2c_dev, 0x320c, &tmp);
    hts = tmp;
    ret += sc035_read(i2c_dev, 0x320d, &tmp);
    if(ret < 0)
        return -1;
    hts = (hts << 8) + tmp;

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = sc035_write(i2c_dev, 0x320f, (unsigned char)(vts & 0xff));
    ret += sc035_write(i2c_dev, 0x320e, (unsigned char)(vts >> 8));
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}

static int sc035_get_frame_start(void)
{
    return light_ctl.strobe_cnt;
}

static struct sensor_attr sc035_sensor_attr = {
    .device_name                = SC035_DEVICE_NAME,
    .cbus_addr                  = SC035_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW8,
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = sc035_init_regs_640_480_60fps_mipi,
        .width                  = SC035_WIDTH,
        .height                 = SC035_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SRGGB8_1X8,

        .fps                    = 60 << 16 | 1,
        .total_width            = 0x0554,
        .total_height           = 0x0420,

        .min_integration_time   = 6,
        .max_integration_time   = 0x0420 - 6,
        .one_line_expr_in_us    = 15,
        .max_again              = 256041,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = sc035_power_on,
        .power_off              = sc035_power_off,
        .stream_on              = sc035_stream_on,
        .stream_off             = sc035_stream_off,
        .get_register           = sc035_g_register,
        .set_register           = sc035_s_register,

        .set_integration_time   = sc035_set_integration_time,
        .alloc_again            = sc035_alloc_again,
        .set_analog_gain        = sc035_set_analog_gain,
        .alloc_dgain            = sc035_alloc_dgain,
        .set_digital_gain       = sc035_set_digital_gain,
        .set_fps                = sc035_set_fps,
        .frame_start_callback    = sc035_get_frame_start,
    },
};

static irqreturn_t strobe_irq_handler(int irq, void *data)
{
    struct sc035_light_ctl *_light_ctl = data;
    _light_ctl->strobe_cnt++;
    if ((_light_ctl->strobe_cnt % 2) == 0)
        gpio_direction_output(sensor_light_ctl, 1);
    else
        gpio_direction_output(sensor_light_ctl, 0);

    return IRQ_HANDLED;
}

static int init_interrupt(void)
{
    int ret;
    char gpio_str[10];

    if (sensor_light_ctl == -1){
        printk(KERN_ERR "sc035: Please sensor_light_ctl gpio setting \n");
        return -1;
    }
    ret = gpio_request(sensor_light_ctl, "sensor_light_ctl");
    if (ret) {
        printk(KERN_ERR "sc035: failed to request sensor_light_ctl pin: %s\n", gpio_to_str(sensor_light_ctl, gpio_str));
        goto err_sensor_light_ctl;
    }
    gpio_direction_output(sensor_light_ctl, 0);

    if (sensor_strobe == -1){
        printk(KERN_ERR "sc035: Please sensor_light_ctl gpio setting \n");
        goto err_sensor_strobe;
    }
    ret = gpio_request(sensor_strobe, "sensor_strobe");
    if (ret) {
        printk(KERN_ERR "sc035: failed to request sensor_strobe pin: %s\n", gpio_to_str(sensor_strobe, gpio_str));
        goto err_sensor_strobe;
    }
    gpio_direction_input(sensor_strobe);

    light_ctl.irq = gpio_to_irq(sensor_strobe);
    ret = request_irq(light_ctl.irq, strobe_irq_handler, IRQF_TRIGGER_FALLING, "sensor_strobe", (void *)&light_ctl);
    if (ret != 0){
        printk(KERN_ERR "sc035: failed to request_irq  %d\n", light_ctl.irq);
        goto err_request_irq;
    }

    disable_irq(light_ctl.irq);

    return ret;

err_request_irq:
    gpio_free(sensor_strobe);
err_sensor_strobe:
    gpio_free(sensor_light_ctl);
err_sensor_light_ctl:

    return ret;

}

static int sc035_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int ret;

    if (0==strcmp("MASTER_MODE", sensor_mode)) {
        ret = init_interrupt();
        if(ret != 0)
            return ret;
    }

    ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &sc035_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sc035_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &sc035_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id sc035_id[] = {
    { SC035_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sc035_id);

static struct i2c_driver sc035_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = SC035_DEVICE_NAME,
    },
    .probe              = sc035_probe,
    .remove             = sc035_remove,
    .id_table           = sc035_id,
};

static struct i2c_board_info sensor_sc035_info = {
    .type               = SC035_DEVICE_NAME,
    .addr               = SC035_DEVICE_I2C_ADDR,
};

static __init int init_sc035(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "sc035: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    sc035_driver.driver.name = sensor_name;
    strcpy(sc035_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_sc035_info.addr = i2c_addr;
    strcpy(sensor_sc035_info.type, sensor_name);
    sc035_sensor_attr.device_name = sensor_name;

    int ret = i2c_add_driver(&sc035_driver);
    if (ret) {
        printk(KERN_ERR "sc035: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_sc035_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "sc035: failed to register i2c device\n");
        i2c_del_driver(&sc035_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_sc035(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sc035_driver);
}

module_init(init_sc035);
module_exit(exit_sc035);

MODULE_DESCRIPTION("x2000 sc035 driver");
MODULE_LICENSE("GPL");
