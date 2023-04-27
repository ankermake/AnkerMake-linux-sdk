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
#include <linux/regulator/consumer.h>
#include <camera/hal/camera_sensor.h>
#include <linux/interrupt.h>
#include "pwm.h"


#define SC031_DEVICE_NAME              "sc031"
#define SC031_DEVICE_I2C_ADDR          0x30

#define SC031_CHIP_ID_H                0x00
#define SC031_CHIP_ID_L                0x31

#define SC031_REG_END                  0xff
#define SC031_REG_DELAY                0xfe
#define SC031_SUPPORT_SCLK             (72000000)
#define SENSOR_OUTPUT_MAX_FPS          60
#define SENSOR_OUTPUT_MIN_FPS          5
#define SC031_MAX_WIDTH                640
#define SC031_MAX_HEIGHT               480
#define SC031_WIDTH                    640
#define SC031_HEIGHT                   480

//#define LIGHT_WITH_PWM
#define SC031_SET_LED_LEVEL            0xa55aa55a

static int power_gpio       = -1;
static int reset_gpio       = -1;
static int pwdn_gpio        = -1;   // GPIO_PB(18) //  GPIO_PB(19) //  XSHUTDN
static int i2c_bus_num      = -1;   // 5 // 2
static short i2c_addr       = -1;   // 0x30 // 0x33
static int cam_bus_num      = -1;   // 0 // 1
static char *sensor_name    = NULL; // sc031-ir0    //sc031-ir1
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
static struct sensor_attr sc031_sensor_attr;
static struct regulator *sc031_regulator = NULL;


struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

static struct again_lut sc031_again_lut[] = {
    {0x10, 0},
    {0x11, 5954},
    {0x12, 11136},
    {0x13, 16247},
    {0x14, 11136},
    {0x15, 11659},
    {0x16, 30108},
    {0x17, 34311},
    {0x18, 38335},
    {0x19, 42195},
    {0x1a, 45903},
    {0x1b, 49471},
    {0x1c, 52910},
    {0x1d, 56227},
    {0x1e, 59433},
    {0x1f, 62533},
    {0x410, 65535},
    {0x411, 71266},
    {0x412, 76671},
    {0x413, 81782},
    {0x414, 86632},
    {0x415, 91245},
    {0x416, 95643},
    {0x417, 99846},
    {0x418, 103870},
    {0x419, 107730},
    {0x41a, 111438},
    {0x41b, 115006},
    {0x41c, 118445},
    {0x41d, 121762},
    {0x41e, 124968},
    {0x41f, 128068},
    {0xc10, 131070},
    {0xc11, 136801},
    {0xc12, 142206},
    {0xc13, 147317},
    {0xc14, 152167},
    {0xc15, 156780},
    {0xc16, 161178},
    {0xc17, 165381},
    {0xc18, 169405},
    {0xc19, 173265},
    {0xc1a, 176973},
    {0xc1b, 180541},
    {0xc1c, 183980},
    {0xc1d, 187297},
    {0xc1e, 190503},
    {0xc1f, 193603},
    {0x1c10, 196605},
    {0x1c11, 202336},
    {0x1c12, 207741},
    {0x1c13, 212852},
    {0x1c14, 217702},
    {0x1c15, 222315},
    {0x1c16, 226713},
    {0x1c17, 230916},
    {0x1c18, 234940},
    {0x1c19, 238800},
    {0x1c1a, 242508},
    {0x1c1b, 246076},
    {0x1c1c, 249515},
    {0x1c1d, 252832},
    {0x1c1e, 256038},
    {0x1c1f, 259138},
};

/*
 * SensorName RAW_30fps
 * width=640
 * height=480
 * SlaveID=0x60/0x66
 * mclk=24
 * avdd=2.800000
 * dovdd=1.800000
 * dvdd=1.500000
 */
static struct regval_list sc031_init_regs_640_480_30fps_mipi[] = {

    {0x0103, 0x01},
    {0x0100, 0x00},
    {0x36e9, 0x80},
    {0x36f9, 0x80},
    {0x3001, 0x00},
    {0x3000, 0x00},
    {0x300f, 0x0f},
    {0x3018, 0x33},
    {0x3019, 0xfc},
    {0x301c, 0x78},
    {0x301f, 0x0e},
    {0x3031, 0x0a},
    {0x3037, 0x20},
    {0x303f, 0x01},
    {0x320c, 0x04},
    {0x320d, 0xb0},
    {0x320e, 0x07},
    {0x320f, 0xd0},
    {0x3220, 0x10},
    {0x3221, 0x06},

    {0x3250, 0xc0},
    {0x3251, 0x02},
    {0x3252, 0x02},
    {0x3253, 0xa6},
    {0x3254, 0x02},
    {0x3255, 0x07},
    {0x3304, 0x48},
    {0x3306, 0x38},
    {0x3309, 0x68},
    {0x330b, 0xe0},
    {0x330c, 0x18},
    {0x330f, 0x20},
    {0x3310, 0x10},
    {0x3314, 0x1e},
    {0x3315, 0x38},
    {0x3316, 0x40},
    {0x3317, 0x10},
    {0x3329, 0x34},
    {0x332d, 0x34},
    {0x332f, 0x38},
    {0x3335, 0x3c},
    {0x3344, 0x3c},
    {0x335b, 0x80},
    {0x335f, 0x80},
    {0x3366, 0x06},
    {0x3385, 0x31},
    {0x3387, 0x51},
    {0x3389, 0x01},
    {0x33b1, 0x03},
    {0x33b2, 0x06},
    {0x3621, 0xa4},
    {0x3622, 0x05},
    {0x3624, 0x47},
    {0x3630, 0x46},
    {0x3631, 0x48},
    {0x3633, 0x52},
    {0x3635, 0x18},
    {0x3636, 0x25},
    {0x3637, 0x89},
    {0x3638, 0x0f},
    {0x3639, 0x08},
    {0x363a, 0x00},
    {0x363b, 0x48},
    {0x363c, 0x06},
    {0x363d, 0x00},
    {0x363e, 0xf8},
    {0x3640, 0x00},
    {0x3641, 0x01},
    {0x36ea, 0x3b},
    {0x36eb, 0x0e},
    {0x36ec, 0x1e},
    {0x36ed, 0x33},
    {0x36fa, 0x3a},
    {0x36fc, 0x01},
    {0x3908, 0x91},
    {0x3d08, 0x01},
    {0x3e01, 0x30},
    {0x3e02, 0x00}, // 0x300
    {0x3e06, 0x0c},
    {0x3e08, 0x0c}, // Coarse gain 0x00-1x 0x04-2x 0x0c-4x 0x1c-8x
    {0x3e09, 0x10}, // Fine gain 1.0～1.9375    --> total gain 4.5x
    {0x4500, 0x59},
    {0x4501, 0xc4},
    {0x4603, 0x00},
    {0x4809, 0x01},
    {0x4837, 0x38},
    {0x5011, 0x00},
    {0x36e9, 0x00},
    {0x36f9, 0x00},
    //{0x0100, 0x01},
    //Delay  10ms}
    {0x4418, 0x08},
    {0x4419, 0x8e},

    //[gain<2]
    {0x3314, 0x1e},//0222
    {0x3317, 0x10},
    //[4>gain>=2]
    {0x3314, 0x4f}, //0222
    {0x3317, 0x0f}, //0625
    //[gain>=4]
    {0x3314, 0x4f}, //0222
    {0x3317, 0x0f}, //0625
    {SC031_REG_DELAY,10},
    {SC031_REG_END, 0x00},    /* END MARKER */
};

 struct regval_list sc031_regs_salve[] = {

    {0x3222, 0x02},
    {0x3223, 0xc4},
    {0x3226, 0x02},
    {0x3227, 0x02},
    {0x3228, 0x07},
    {0x3229, 0xce},
    {0x322b, 0x0b},
    {0x3225, 0x04},
    {0x3231, 0x2a},

    {SC031_REG_DELAY,10},
    {SC031_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list sc031_regs_stream_on[] = {

    {0x0100, 0x01},             /* RESET_REGISTER */
    {SC031_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list sc031_regs_stream_off[] = {

    {0x0100, 0x00},             /* RESET_REGISTER */
    {SC031_REG_END, 0x00},     /* END MARKER */
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

        printk("reg = 0x%02x, val = 0x%02x\n",vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int sc031_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char v;

    ret = sc031_read(i2c, 0x3107, &v);
    if (ret < 0)
        return ret;

    if (v != SC031_CHIP_ID_H) {
        printk(KERN_ERR "sc031 read chip id high failed:0x%x\n", v);
        return -ENODEV;
    }

    ret = sc031_read(i2c, 0x3108, &v);
    if (ret < 0)
        return ret;

    if (v != SC031_CHIP_ID_L) {
        printk(KERN_ERR "sc031 read chip id low failed:0x%x\n", v);
        return -ENODEV;
    }

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "sc031_power");
        if (ret) {
            printk(KERN_ERR "sc031: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }
    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        sc031_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(sc031_regulator)) {
            ret = -1;
            printk(KERN_ERR "regulator_get error!\n");
            goto err_regulator;
        }
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


    return 0;

err_pwdn_gpio:
    if (reset_gpio != -1)
        gpio_free(reset_gpio);
err_reset_gpio:
    if (sc031_regulator)
        regulator_put(sc031_regulator);
err_regulator:
    if (power_gpio != -1)
        gpio_free(power_gpio);
err_power_gpio:
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

    if (sc031_regulator)
        regulator_put(sc031_regulator);
}

static void sc031_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (sc031_regulator)
        regulator_disable(sc031_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int sc031_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(10);
    }

    if (sc031_regulator) {
        regulator_enable(sc031_regulator);
        m_msleep(10);
    }

    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 1);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    m_msleep(4);

    ret = sc031_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "sc031: failed to detect\n");
        goto err;
    }

    ret = sc031_write_array(i2c_dev, sc031_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "sc031: failed to init regs\n");
        goto err;
    }

    return 0;
err:
    sc031_power_off();
    deinit_gpio();
    return ret;
}

static int sc031_stream_on(void)
{
    int ret;

    ret = sc031_write_array(i2c_dev, sc031_regs_salve);
    if (ret)
        printk(KERN_ERR "sc031: failed to sc031_regs_slave\n");

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

static int sc031_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = sc031_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 1;
    return ret;
}

static int sc031_s_register(struct sensor_dbg_register *reg)
{
    int ret = 0;
    if (reg->reg != SC031_SET_LED_LEVEL)
        ret = sc031_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xff);

    return ret;
}

static int sc031_set_integration_time(int value)
{
    int ret = 0;

    value = value << 4;
    ret = sc031_write(i2c_dev, 0x3e01, (unsigned char)((value >> 8) & 0xff));
    ret += sc031_write(i2c_dev, 0x3e02, (unsigned char)value);
    if (ret < 0)
        return ret;

    return 0;
}

static unsigned int sc031_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

    struct again_lut *lut = sc031_again_lut;
    while(lut->gain <= sc031_sensor_attr.sensor_info.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == sc031_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static int sc031_set_analog_gain(int value)
{
    int ret;

    ret = sc031_write(i2c_dev, 0x3e08, (unsigned char)(value >> 8));
    ret += sc031_write(i2c_dev, 0x3e09, (unsigned char)(value & 0xff));
    if (ret < 0)
        return ret;

    return 0;
}

static unsigned int sc031_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int sc031_set_digital_gain(int value)
{
    return 0;
}

static int sc031_set_fps(int fps)
{
    struct sensor_info *sensor_info = &sc031_sensor_attr.sensor_info;
    unsigned int sclk = 0;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp = 0;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "sc031 fps(%d) no in range\n", fps);
        return -1;
    }
    sclk = SC031_SUPPORT_SCLK;

    ret = sc031_read(i2c_dev, 0x320c, &tmp);
    hts = tmp;
    ret += sc031_read(i2c_dev, 0x320d, &tmp);
    if(ret < 0)
        return -1;
    hts = (hts << 8) + tmp;

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = sc031_write(i2c_dev, 0x320f, (unsigned char)(vts & 0xff));
    ret += sc031_write(i2c_dev, 0x320e, (unsigned char)(vts >> 8));
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}

extern int sc031_frame_start_callback(void);

static struct sensor_attr sc031_sensor_attr = {
    .device_name                = SC031_DEVICE_NAME,
    .cbus_addr                  = SC031_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = sc031_init_regs_640_480_30fps_mipi,
        .width                  = SC031_WIDTH,
        .height                 = SC031_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 30 << 16 | 1,
        .total_width            = 0x04b0,
        .total_height           = 0x07d0,

        .min_integration_time   = 3,
        .max_integration_time   = 0x07d0 - 6,
        .max_again              = 259138,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = sc031_power_on,
        .power_off              = sc031_power_off,
        .stream_on              = sc031_stream_on,
        .stream_off             = sc031_stream_off,
        .get_register           = sc031_g_register,
        .set_register           = sc031_s_register,

        .set_integration_time   = sc031_set_integration_time,
        .alloc_again            = sc031_alloc_again,
        .set_analog_gain        = sc031_set_analog_gain,
        .alloc_dgain            = sc031_alloc_dgain,
        .set_digital_gain       = sc031_set_digital_gain,
        .set_fps                = sc031_set_fps,

        .frame_start_callback   = sc031_frame_start_callback,
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
    sc031_driver.driver.name = sensor_name;
    strcpy(sc031_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_sc031_info.addr = i2c_addr;
    strcpy(sensor_sc031_info.type, sensor_name);
    sc031_sensor_attr.device_name = sensor_name;

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

MODULE_DESCRIPTION("x2000 sc031 driver");
MODULE_LICENSE("GPL");
