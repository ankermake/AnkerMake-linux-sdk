/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * SC2355
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <linux/regulator/consumer.h>
#include <camera/hal/camera_sensor.h>
#include <linux/interrupt.h>


#define SC2355_DEVICE_NAME              "sc2355"
#define SC2355_DEVICE_I2C_ADDR          0x30

#define SC2355_CHIP_ID_H                0xeb
#define SC2355_CHIP_ID_L                0x2c

#define SC2355_REG_END                  0xff
#define SC2355_REG_DELAY                0xfe
#define SC2355_SUPPORT_SCLK             (72000000)
#define SENSOR_OUTPUT_MAX_FPS           60
#define SENSOR_OUTPUT_MIN_FPS           5
#define SENSOR_OUTPUT_FPS               30
#define SC2355_MAX_WIDTH                640
#define SC2355_MAX_HEIGHT               360
#define SC2355_WIDTH                    640
#define SC2355_HEIGHT                   360

#define SC2355_SET_LED_LEVEL            0xa55aa55a

static int power_gpio       = -1;
static int reset_gpio       = -1;
static int pwdn_gpio        = -1;   // GPIO_PB(18) //  GPIO_PB(19) //  XSHUTDN
static int i2c_bus_num      = -1;   // 5 // 2
static short i2c_addr       = -1;   // 0x30 // 0x33
static int cam_bus_num      = -1;   // 0 // 1
static char *sensor_name    = NULL; // sc2355-ir0    //sc2355-ir1
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
static struct sensor_attr sc2355_sensor_attr;
static struct regulator *sc2355_regulator = NULL;


struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int index;
    unsigned int analog_value;
    unsigned int digital_coarse_value;
    unsigned int digital_fine_coarse;
    unsigned int gain;
};

static struct again_lut sc2355_again_lut[] = {
    {0x0, 0x0, 0x0, 0x80,  0},   // 1x  again
    {0x1, 0x0, 0x0, 0x84,  2886},
    {0x2, 0x0, 0x0, 0x88,  5776},
    {0x3, 0x0, 0x0, 0x8c,  8494},
    {0x4, 0x0, 0x0, 0x90,  11136},
    {0x5, 0x0, 0x0, 0x94,  13706},
    {0x6, 0x0, 0x0, 0x98,  16287},
    {0x7, 0x0, 0x0, 0x9c,  18723},
    {0x8, 0x0, 0x0, 0xa0,  21097},
    {0x9, 0x0, 0x0, 0xa4, 23413},
    {0xa, 0x0, 0x0, 0xa8, 25746},
    {0xb, 0x0, 0x0, 0xac, 27952},
    {0xc, 0x0, 0x0, 0xb0, 30108},
    {0xd, 0x0, 0x0, 0xb4, 32216},
    {0xe, 0x0, 0x0, 0xb8, 34344},
    {0xf, 0x0, 0x0, 0xbc, 36361},
    {0x10, 0x0, 0x0, 0xc0, 38335},
    {0x11, 0x0, 0x0, 0xc4, 40269},
    {0x12, 0x0, 0x0, 0xc8, 42225},
    {0x13, 0x0, 0x0, 0xcc, 44082},
    {0x14, 0x0, 0x0, 0xd0, 45903},
    {0x15, 0x0, 0x0, 0xd4, 47689},
    {0x16, 0x0, 0x0, 0xd8, 49499},
    {0x17, 0x0, 0x0, 0xdc, 51220},
    {0x18, 0x0, 0x0, 0xe0, 52910},
    {0x19, 0x0, 0x0, 0xe4, 54570},
    {0x1a, 0x0, 0x0, 0xe8, 56253},
    {0x1b, 0x0, 0x0, 0xec, 57856},
    {0x1c, 0x0, 0x0, 0xf0, 59433},
    {0x1d, 0x0, 0x0, 0xf4, 60983},
    {0x1e, 0x0, 0x0, 0xf8, 62557},
    {0x1f, 0x0, 0x0, 0xfc, 64058},
    {0x20, 0x1, 0x0, 0x80, 65535},  // 2x again
    {0x21, 0x0, 0x1, 0x86, 69877},
    {0x22, 0x0, 0x1, 0x8c, 74029},
    {0x23, 0x0, 0x1, 0x92, 77964},
    {0x24, 0x0, 0x1, 0x98, 81782},
    {0x25, 0x0, 0x1, 0x9e, 85452},
    {0x26, 0x0, 0x1, 0xa4, 88985},
    {0x27, 0x0, 0x1, 0xaa, 92355},
    {0x28, 0x0, 0x1, 0xb0, 95643},
    {0x29, 0x0, 0x1, 0xb6, 98821},
    {0x2a, 0x0, 0x1, 0xbc, 101896},
    {0x2b, 0x0, 0x1, 0xc2, 104842},
    {0x2c, 0x0, 0x1, 0xc8, 107730},
    {0x2d, 0x0, 0x1, 0xce, 110532},
    {0x2e, 0x0, 0x1, 0xd4, 113253},
    {0x2f, 0x0, 0x1, 0xda, 115871},
    {0x30, 0x0, 0x1, 0xe0, 118445},
    {0x31, 0x0, 0x1, 0xe6, 120950},
    {0x32, 0x0, 0x1, 0xec, 123391},
    {0x33, 0x0, 0x1, 0xf2, 125746},
    {0x34, 0x0, 0x1, 0xf8, 128068},
    {0x35, 0x3, 0x0, 0x80, 131070},  // 4x  again
    {0x36, 0x0, 0x03, 0x88,136801},
    {0x37, 0x0, 0x03, 0x90,142206},
    {0x38, 0x0, 0x03, 0x98,147317},
    {0x39, 0x0, 0x03, 0xa0,152167},
    {0x3a, 0x0, 0x03, 0xa8,156780},
    {0x3b, 0x0, 0x03, 0xb0,161178},
    {0x3c, 0x0, 0x03, 0xb8,165381},
    {0x3d, 0x0, 0x03, 0xc0,169405},
    {0x3e, 0x0, 0x03, 0xc8,173265},
    {0x3f, 0x0, 0x03, 0xd0,176973},
    {0x40, 0x0, 0x03, 0xd8,180541},
    {0x41, 0x0, 0x03, 0xe0,183980},
    {0x42, 0x0, 0x03, 0xe8,187297},
    {0x43, 0x0, 0x03, 0xf0,190503},
    {0x44, 0x0, 0x03, 0xf8,193603},
    {0x45, 0x7, 0x0, 0x80, 196605},  // 8x again
    {0x46, 0x7, 0x0, 0x88, 202381},  // 8x again * (1~2 dgain)
    {0x47, 0x7, 0x0, 0x92, 209076},
    {0x48, 0x7, 0x0, 0x9c, 215328},
    {0x49, 0x7, 0x0, 0xa6, 221192},
    {0x4a, 0x7, 0x0, 0xb0, 226713},
    {0x4b, 0x7, 0x0, 0xba, 231930},
    {0x4c, 0x7, 0x0, 0xc4, 236874},
    {0x4d, 0x7, 0x0, 0xce, 241572},
    {0x4e, 0x7, 0x0, 0xd8, 246104},
    {0x4f, 0x7, 0x0, 0xe2, 250375},
    {0x50, 0x7, 0x0, 0xec, 254461},
    {0x51, 0x7, 0x0, 0xf6, 258378},
    {0x52, 0xf, 0x0, 0x80, 262140},  // 16x again
};

/*
 * SensorName RAW_30fps
 * width=640
 * height=360
 * SlaveID=0x60/0x66
 * mclk=24
 * avdd=2.800000
 * dovdd=1.800000
 * dvdd=1.500000
 */
static struct regval_list sc2355_init_regs_640_360_60fps_mipi[] = {
    {0x0103, 0x01},
    {0x0100, 0x00},
    {0x36e9, 0x80},
    {0x36ea, 0x0f},
    {0x36eb, 0x24},
    {0x36ed, 0x14},
    {0x36e9, 0x01},
    {0x301f, 0x07},
    {0x303f, 0x82},
    {0x3200, 0x00},
    {0x3201, 0x9c},
    {0x3202, 0x00},
    {0x3203, 0xec},
    {0x3204, 0x05},
    {0x3205, 0xab},
    {0x3206, 0x03},
    {0x3207, 0xcb},
    {0x3208, 0x02},
    {0x3209, 0x80},
    {0x320a, 0x01},
    {0x320b, 0x68},
    {0x320e, 0x02},
    {0x320f, 0x71},
    {0x3210, 0x00},
    {0x3211, 0x08},
    {0x3212, 0x00},
    {0x3213, 0x08},
    {0x3215, 0x31},
    {0x3220, 0x01},
    {0x3248, 0x02},
    {0x3253, 0x0a},
    {0x3301, 0xff},
    {0x3302, 0xff},
    {0x3303, 0x10},
    {0x3306, 0x28},
    {0x3307, 0x02},
    {0x330a, 0x00},
    {0x330b, 0xb0},
    {0x3318, 0x02},
    {0x3320, 0x06},
    {0x3321, 0x02},
    {0x3326, 0x12},
    {0x3327, 0x0e},
    {0x3328, 0x03},
    {0x3329, 0x0f},
    {0x3364, 0x4f},
    {0x33b3, 0x40},
    {0x33f9, 0x2c},
    {0x33fb, 0x38},
    {0x33fc, 0x0f},
    {0x33fd, 0x1f},
    {0x349f, 0x03},
    {0x34a6, 0x01},
    {0x34a7, 0x1f},
    {0x34a8, 0x40},
    {0x34a9, 0x30},
    {0x34ab, 0xa6},
    {0x34ad, 0xa6},
    {0x3622, 0x60},
    {0x3623, 0x40},
    {0x3624, 0x61},
    {0x3625, 0x08},
    {0x3626, 0x03},
    {0x3630, 0xa8},
    {0x3631, 0x84},
    {0x3632, 0x90},
    {0x3633, 0x43},
    {0x3634, 0x09},
    {0x3635, 0x82},
    {0x3636, 0x48},
    {0x3637, 0xe4},
    {0x3641, 0x22},
    {0x3670, 0x0f},
    {0x3674, 0xc0},
    {0x3675, 0xc0},
    {0x3676, 0xc0},
    {0x3677, 0x86},
    {0x3678, 0x88},
    {0x3679, 0x8c},
    {0x367c, 0x01},
    {0x367d, 0x0f},
    {0x367e, 0x01},
    {0x367f, 0x0f},
    {0x3690, 0x63},
    {0x3691, 0x63},
    {0x3692, 0x73},
    {0x369c, 0x01},
    {0x369d, 0x1f},
    {0x369e, 0x8a},
    {0x369f, 0x9e},
    {0x36a0, 0xda},
    {0x36a1, 0x01},
    {0x36a2, 0x03},
    {0x3900, 0x0d},
    {0x3904, 0x04},
    {0x3905, 0x98},
    {0x391b, 0x81},
    {0x391c, 0x10},
    {0x391d, 0x19},
    {0x3933, 0x01},
    {0x3934, 0x82},
    {0x3940, 0x5d},
    {0x3942, 0x01},
    {0x3943, 0x82},
    {0x3949, 0xc8},
    {0x394b, 0x64},
    {0x3952, 0x02},
    {0x3e00, 0x00},
    {0x3e01, 0x26},
    {0x3e02, 0xb0},
    {0x4502, 0x34},
    {0x4509, 0x30},
    {0x450a, 0x71},
    {0x4819, 0x05},
    {0x481b, 0x03},
    {0x481d, 0x0a},
    {0x481f, 0x02},
    {0x4821, 0x08},
    {0x4823, 0x03},
    {0x4825, 0x02},
    {0x4827, 0x03},
    {0x4829, 0x04},
    {0x5000, 0x46},
    {0x5900, 0x01},
    {0x5901, 0x04},

    //vbin_QC
    {0x3215,0x22},
    {0x337e,0xa0},
    {0x3905,0x9c},
    {0x3032,0x60},
    //hbin_QC
    {0x5900,0xf5},

    //{0x0100, 0x01},
    {SC2355_REG_DELAY,10},
    {SC2355_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list sc2355_regs_salve[] = {
    //slave
#if (SENSOR_OUTPUT_FPS == 30)
    {0x320e, 0x04},
    {0x320f, 0xe2},
    {0x322e, 0x04},    //vts-4
    {0x322f, 0xde},
#elif (SENSOR_OUTPUT_FPS == 60)
    {0x320e, 0x02},
    {0x320f, 0x71},
    {0x322e, 0x02},
    {0x322f, 0x6d},
#endif
    {0x3222, 0x02},
    {0x3224, 0x82},
    {0x300b, 0x40},
    {0x3253, 0x0e},

    {SC2355_REG_DELAY,10},
    {SC2355_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list sc2355_regs_stream_on[] = {
    {0x0100, 0x01},             /* RESET_REGISTER */
    {SC2355_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list sc2355_regs_stream_off[] = {
    {0x0100, 0x00},             /* RESET_REGISTER */
    {SC2355_REG_END, 0x00},     /* END MARKER */
};

static int sc2355_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
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
        printk(KERN_ERR "sc2355: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int sc2355_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
        printk(KERN_ERR "sc2355(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int sc2355_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != SC2355_REG_END) {
        if (vals->reg_num == SC2355_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = sc2355_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int sc2355_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != SC2355_REG_END) {
        if (vals->reg_num == SC2355_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = sc2355_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        printk("reg = 0x%02x, val = 0x%02x\n",vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int sc2355_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = sc2355_read(i2c, 0x3107, &h);
    if (ret < 0)
        return ret;
    if (h != SC2355_CHIP_ID_H) {
        printk(KERN_ERR "sc2355 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = sc2355_read(i2c, 0x3108, &l);
    if (ret < 0)
        return ret;

    if (l != SC2355_CHIP_ID_L) {
        printk(KERN_ERR "sc2355 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "sc2355 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "sc2355_reset");
        if (ret) {
            printk(KERN_ERR "sc2355: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            reset_gpio = -1;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "sc2355_pwdn");
        if (ret) {
            printk(KERN_ERR "sc2355: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "sc2355_power");
        if (ret) {
            printk(KERN_ERR "sc2355: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        sc2355_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(sc2355_regulator)) {
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

    if (sc2355_regulator)
        regulator_put(sc2355_regulator);
}

static void sc2355_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (sc2355_regulator)
        regulator_disable(sc2355_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int sc2355_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 24 * 1000 * 1000);

    if (sc2355_regulator) {
        regulator_enable(sc2355_regulator);
        m_msleep(10);
    }
    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(10);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(50);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
    }

    if (reset_gpio != -1){
        // gpio_direction_output(reset_gpio, 1);
        // m_msleep(5);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(30);
    }

    ret = sc2355_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "sc2355: failed to detect\n");
        goto fail;
    }

    ret = sc2355_write_array(i2c_dev, sc2355_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "sc2355: failed to init regs\n");
        goto fail;
    }


    return 0;

fail:
    sc2355_power_off();
    return ret;
}

static int sc2355_stream_on(void)
{
    int ret;

    ret = sc2355_write_array(i2c_dev, sc2355_regs_salve);
    if (ret)
        printk(KERN_ERR "sc2355: failed to write sc2355 mode\n");

    ret = sc2355_write_array(i2c_dev, sc2355_regs_stream_on);
    if (ret)
        printk(KERN_ERR "sc2355: failed to stream on\n");

    return ret;
}

static void sc2355_stream_off(void)
{
    int ret;

    ret = sc2355_write_array(i2c_dev, sc2355_regs_stream_off);
    if (ret)
        printk(KERN_ERR "sc2355: failed to stream on\n");
}

static int sc2355_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = sc2355_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 1;
    return ret;
}

static int sc2355_s_register(struct sensor_dbg_register *reg)
{
    int ret = 0;

    if (reg->reg != SC2355_SET_LED_LEVEL)
        ret = sc2355_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xff);

    return ret;
}

static int sc2355_set_integration_time(int value)
{
    int ret = 0;

    //ret = sc2355_write(i2c_dev, 0x3e00, (unsigned char)((value >> 12) & 0xf));
    ret += sc2355_write(i2c_dev, 0x3e01, (unsigned char)((value >> 4) & 0xff));
    ret += sc2355_write(i2c_dev, 0x3e02, (unsigned char)((value & 0x0f) << 4));
    if (ret < 0)
        return ret;

    return 0;
}

static unsigned int sc2355_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = sc2355_again_lut;
    while(lut->gain <= sc2355_sensor_attr.sensor_info.max_again) {
        if(isp_gain <= 0) {
            *sensor_again = lut[0].index;
            return lut[0].gain;
        } else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->index;
            return (lut - 1)->gain;
        } else {
            if((lut->gain == sc2355_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static int sc2355_set_analog_gain(int value)
{
    int ret = 0;
    struct again_lut *val_lut = sc2355_again_lut;

    ret = sc2355_write(i2c_dev, 0x3e09, val_lut[value].analog_value);
    ret += sc2355_write(i2c_dev, 0x3e06, val_lut[value].digital_coarse_value);
    ret += sc2355_write(i2c_dev, 0x3e07, val_lut[value].digital_fine_coarse);
    if (ret < 0)
        return ret;

    return 0;
}

static unsigned int sc2355_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int sc2355_set_digital_gain(int value)
{
    return 0;
}

static int sc2355_set_fps(int fps)
{
    struct sensor_info *sensor_info = &sc2355_sensor_attr.sensor_info;
    unsigned int sclk = 0;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp = 0;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "sc2355 fps(%d) no in range\n", fps);
        return -1;
    }
    sclk = SC2355_SUPPORT_SCLK;

    ret = sc2355_read(i2c_dev, 0x320c, &tmp);
    hts = tmp;
    ret += sc2355_read(i2c_dev, 0x320d, &tmp);
    if(ret < 0)
        return -1;
    hts = (hts << 8) + tmp;

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = sc2355_write(i2c_dev, 0x320f, (unsigned char)(vts & 0xff));
    ret += sc2355_write(i2c_dev, 0x320e, (unsigned char)(vts >> 8));
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}

extern int sc2355_frame_start_callback(void);

static struct sensor_attr sc2355_sensor_attr = {
    .device_name                = SC2355_DEVICE_NAME,
    .cbus_addr                  = SC2355_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 1,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = sc2355_init_regs_640_360_60fps_mipi,
        .width                  = SC2355_WIDTH,
        .height                 = SC2355_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 60 << 16 | 1,
        .total_width            = 0x780,
        .total_height           = 0x271,

        .min_integration_time   = 6,
        .max_integration_time   = 0x271 - 4,
        .max_again              = 262140,
        .max_dgain              = 0,

#if (SENSOR_OUTPUT_FPS == 30)
        .fps                    = 30 << 16 | 1,
        .total_height           = 0x4e2,
        .max_integration_time   = 0x4e2 - 4,
#elif (SENSOR_OUTPUT_FPS == 60)
        .fps                    = 60 << 16 | 1,
        .total_height           = 0x271,
        .max_integration_time   = 0x271 - 4,
#endif
    },

    .ops = {
        .power_on               = sc2355_power_on,
        .power_off              = sc2355_power_off,
        .stream_on              = sc2355_stream_on,
        .stream_off             = sc2355_stream_off,
        .get_register           = sc2355_g_register,
        .set_register           = sc2355_s_register,

        .set_integration_time   = sc2355_set_integration_time,
        .alloc_again            = sc2355_alloc_again,
        .set_analog_gain        = sc2355_set_analog_gain,
        .alloc_dgain            = sc2355_alloc_dgain,
        .set_digital_gain       = sc2355_set_digital_gain,
        .set_fps                = sc2355_set_fps,

        .frame_start_callback   = sc2355_frame_start_callback,
    },
};


static int sc2355_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int ret;

    ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &sc2355_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sc2355_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &sc2355_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id sc2355_id[] = {
    { SC2355_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sc2355_id);

static struct i2c_driver sc2355_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = SC2355_DEVICE_NAME,
    },
    .probe              = sc2355_probe,
    .remove             = sc2355_remove,
    .id_table           = sc2355_id,
};

static struct i2c_board_info sensor_sc2355_info = {
    .type               = SC2355_DEVICE_NAME,
    .addr               = SC2355_DEVICE_I2C_ADDR,
};

static __init int init_sc2355(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "sc2355: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    sc2355_driver.driver.name = sensor_name;
    strcpy(sc2355_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_sc2355_info.addr = i2c_addr;
    strcpy(sensor_sc2355_info.type, sensor_name);
    sc2355_sensor_attr.device_name = sensor_name;

    int ret = i2c_add_driver(&sc2355_driver);
    if (ret) {
        printk(KERN_ERR "sc2355: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_sc2355_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "sc2355: failed to register i2c device\n");
        i2c_del_driver(&sc2355_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_sc2355(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sc2355_driver);
}

module_init(init_sc2355);
module_exit(exit_sc2355);

MODULE_DESCRIPTION("x2000 sc2355 driver");
MODULE_LICENSE("GPL");
