/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * PAG7930 mipi driver
 *
 */
#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>

#define PAG7930_DEVICE_NAME              "pag7930"
#define PAG7930_DEVICE_I2C_ADDR          0x40     // 这个地址会被安装驱动时传入的值覆盖

#define PAG7930_SUPPORT_SCLK_FPS_60      70000000
#define PAG7930_DEVICE_WIDTH             1280
#define PAG7930_DEVICE_HEIGHT            800
#define SENSOR_OUTPUT_MAX_FPS            60
#define SENSOR_OUTPUT_MIN_FPS            5

#define PAG7930_CHIP_ID_H                (0x78)
#define PAG7930_CHIP_ID_L                (0x30)
#define PAG7930_REG_CHIP_ID_HIGH         0x01
#define PAG7930_REG_CHIP_ID_LOW          0x00

#define PAG7930_REG_END                  0xff
#define PAG7930_REG_DELAY                0xfe

#define PAG7930_RAW8                     (0)  //设置raw8格式
#define PAG7930_TEST_CHIP                (0)
#define PAG7930_27M                      (1)

static int power_gpio    = -1;
static int reset_gpio    = -1;
static int i2c_bus_num   = -1;  // 3
static short i2c_addr    = -1;
static int cam_bus_num   = -1;
static char *sensor_name = NULL;
static char *regulator_name = "";

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);

module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, short, 0644);
module_param(cam_bus_num, int, 0644);
module_param(sensor_name, charp, 0644);
module_param(regulator_name, charp, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr pag7930_sensor_attr;
static struct regulator *pag7930_regulator = NULL;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int index;
    unsigned char reg_51;
    unsigned char reg_52;
    unsigned int gain;
};

static struct again_lut pag7930_again_lut[] = {
    {0, 0x0, 0x1d, 0},
    {1, 0x0, 0x1e, 3708},
    {2, 0x0, 0x1f, 6396},
    {3, 0x0, 0x20, 9011},
    {4, 0x0, 0x22, 14844},
    {5, 0x0, 0x25, 20338},
    {6, 0x0, 0x27, 25530},
    {7, 0x0, 0x29, 30452},
    {8, 0x0, 0x2b, 35130},
    {9, 0x0, 0x2d, 39587},
    {10, 0x0, 0x2f, 43844},
    {11, 0x0, 0x31, 47918},
    {12, 0x0, 0x33, 51823},
    {13, 0x0, 0x35, 55573},
    {14, 0x0, 0x37, 58673},
    {15, 0x0, 0x39, 62166},
    {16, 0x0, 0x3b, 65535},
    {17, 0x0, 0x3d, 68787},
    {18, 0x0, 0x3f, 71931},
    {19, 0x0, 0x41, 74975},
    {20, 0x0, 0x45, 80782},
    {21, 0x0, 0x49, 86253},
    {22, 0x0, 0x4d, 91065},
    {23, 0x0, 0x51, 95987},
    {24, 0x0, 0x55, 100665},
    {25, 0x0, 0x59, 105122},
    {26, 0x0, 0x5d, 109379},
    {27, 0x0, 0x61, 113167},
    {28, 0x0, 0x65, 116258},
    {29, 0x0, 0x69, 120845},
    {30, 0x0, 0x6d, 124462},
    {31, 0x0, 0x71, 127946},
    {32, 0x0, 0x75, 131306},
    {33, 0x0, 0x79, 134322},
    {34, 0x0, 0x7d, 137466},
    {35, 0x0, 0x85, 140510},
    {36, 0x0, 0x8d, 146317},
    {37, 0x0, 0x95, 151598},
    {38, 0x0, 0x9d, 156780},
    {39, 0x0, 0xa5, 161522},
    {40, 0x0, 0xad, 166200},
    {41, 0x0, 0xb5, 170657},
    {42, 0x0, 0xbd, 174765},
    {43, 0x0, 0xc5, 178845},
    {44, 0x0, 0xcd, 182756},
    {45, 0x0, 0xd5, 186380},
    {46, 0x0, 0xdd, 189997},
    {47, 0x0, 0xe5, 193358},
    {48, 0x0, 0xed, 196723},
    {49, 0x0, 0xf5, 199971},
    {50, 0x0, 0xfd, 203001},
    {51, 0x1, 0x04, 206045},
    {52, 0x1, 0x18, 211751},
    {53, 0x1, 0x2c, 217133},
    {54, 0x1, 0x36, 222315},
    {55, 0x1, 0x4a, 227142},
    {56, 0x1, 0x5e, 231735},
    {57, 0x1, 0x68, 236115},
    {58, 0x1, 0x7c, 240375},
    {59, 0x1, 0x86, 244380},
    {60, 0x1, 0x9a, 248223},
    {61, 0x1, 0xae, 251915},
    {62, 0x1, 0xb8, 255532},
    {63, 0x1, 0xc2, 258955},
    {64, 0x1, 0xd6, 262258},
};


/*
 * Interface    : MIPI - 2lane
 * MCLK         : 27Mhz
 * resolution   : 1280*800
 * FrameRate    : 60fps
 * p_xclk       : 70MHz
 */
static struct regval_list pag7930_1280_800_60fps_mipi_init_regs[] = {
    {0xEF, 0x00}, /* bank num */
    {0x32, 0xFF},
    {0x33, 0x0F},
    {0x78, 0x01},
    {0x64, 0x00},
    {0x4C, 0x4A},
    {0x4D, 0xCD},
    {0x4E, 0x11},
    {0x4F, 0x00},  //FrameTime 0x4f[0:3]~0x4c(0x11cd4a=1166666), fps = pclk/frame_time = 60 Hz
    {0x2B, 0x00},
    {0x27, 0xC3},
#if (PAG7930_27M)
    {0x29, 0x9F},
#else           // 24M
    {0x29, 0xA3},
#endif
    {0x45, 0x00},
#if (PAG7930_RAW8)
    {0xCD, 0xFF},
    {0xCE, 0x11},
#endif
    {0xEF, 0x01},
    //{0xC4, 0x04},
    //{0xC5, 0x00},
    {0xEF, 0x02},
    {0x11, 0x03},
    {0x02, 0xFF},
    {0x03, 0x08},
    {0xEF, 0x03},
    {0xB0, 0x01},
    {0x3B, 0x00},
    {0x3E, 0x01},
    {0x0F, 0x01},
#if (PAG7930_RAW8)
    {0x06, 0x03},   //mipi_data_format, [0:2]: 011(3): raw8
#else
    {0x06, 0x04},   //mipi_data_format, [0:2]: 100(4): raw10
#endif
    {0x10, 0x02},
    {0x11, 0x01},
    {0x43, 0x0E},
    {0x49, 0x06},
    {0x50, 0x07},
    {0x51, 0x00},
    {0x52, 0x14},
    {0x53, 0x00},
    {0x17, 0x07},
    {0x18, 0x05},
    {0x1B, 0x05},
    {0x1C, 0x06},
    {0x4F, 0x01},
    // {0x07, 0x10},
    // {0x08, 0x05},
    // {0x04, 0x30},
    // {0x05, 0x03},
    {0xED, 0x01},
    {0xEF, 0x04},
    {0x13, 0x01},
    {0x16, 0xF2},
    {0x17, 0xE4},
    {0x18, 0xD5},
    {0x19, 0xC5},
    {0x1A, 0xB9},
    {0x1B, 0xB4},
    {0x1C, 0xBE},
    {0x1D, 0xE5},
    {0x1E, 0x3E},
    {0x1F, 0x29},
    {0x20, 0x15},
    {0x21, 0x46},
    {0x22, 0x54},
    {0x23, 0x00},
    {0x30, 0x33},  //Expo mode ctrl, [1:0]: 01: auto expo; 11: manual axpo
    {0x31, 0x64},
    {0x35, 0x07},
    {0x36, 0x0B},
    {0x3C, 0x4B},
    {0x3D, 0x01},
    /* Auto expo */
    {0x42, 0x00},  //[0-3]: MinExpo[27:24]; [4-7]: MaxExpo[27:24]
    {0x43, 0x1D},  //MinGain_l8, 0x1d = 29
    {0x44, 0x00},  //MinGain_h3
    // max gain [1, 16]
    {0x45, 0xD6},  // 0x1d   0xe8  0x104  0x12c 0x160  0x1d6(max)
    {0x46, 0x01},  //  1.0   8.0    8.8     10    12     16
    {0x47, 0xD0},
    {0x48, 0x07},
    {0x49, 0x00},  //Min_Expo, 0x49~0x47, 0x0007d0 = 2000
    {0x4A, 0xF2},
    {0x4B, 0xB1},
    {0x4C, 0x11},  //Max_Expo, 0x4C~0x4A, 0x11b1f2 = 1159666, Max_Expo < FrameTime
    /* Manual expo */
    {0x51, 0xAE},
    {0x52, 0x00},  //manual gain
    {0x53, 0xA0},
    {0x54, 0x68},
    {0x55, 0x06},  //manual expo
    {0x2A, 0x00},
    {0xEF, 0x00},
    {0x16, 0x3C},
    {0x17, 0x78},
    {0x18, 0x02},
    {0x2F, 0x68},
    {0x37, 0x04},
    {0x38, 0x17},
    {0x39, 0x77},
    {0x3B, 0x1D},
    {0x3F, 0x01},
    {0x66, 0x01},
    {0xCD, 0x24},
    {0xCE, 0x10},
    {0xEF, 0x01},
    {0x06, 0x00},
    {0x07, 0x3C},
    {0x0A, 0x00},
    {0x0B, 0xBF},
    {0x0F, 0x54},
    {0x11, 0x50},
    {0x13, 0x55},
    {0x0D, 0x00},
    {0x0E, 0x00},
    {0x10, 0x00},
    {0x12, 0x00},
    {0x16, 0x50},
    {0x18, 0x96},
    {0x19, 0x00},
    {0x1E, 0x81},
    {0x20, 0xC2},
    {0x21, 0x11},
    {0x31, 0x0A},
    {0x39, 0x0A},
    {0x3C, 0x00},
    {0x3D, 0x05},
    {0x3F, 0x30},
    {0x40, 0x0C},
    {0x42, 0x00},
    {0x4D, 0x02},
    {0x4E, 0x00},
    {0x4F, 0x21},
    {0x50, 0x00},
    {0x51, 0x21},
    {0x84, 0x5A},
    {0x85, 0x00},
    {0x86, 0x46},
    {0x87, 0x64},
    {0x88, 0x00},
    {0x89, 0x5A},
    {0x8A, 0x6E},
    {0x8B, 0x00},
    {0x8C, 0x5A},
    {0x8D, 0x8C},
    {0x8E, 0x00},
    {0x8F, 0x82},
    {0x9D, 0x3C},
    {0xD1, 0x92},
    {0xD2, 0x02},
    {0xEF, 0x02},
    {0x13, 0x00},
    {0x44, 0x00},
    {0x45, 0xFF},
    {0x46, 0x00},
    {0x47, 0xFF},
    {0x4A, 0x00},
    {0x4B, 0x04},
#if (PAG7930_TEST_CHIP)
    {0x5B, 0x00},
#else
    {0x5B, 0x01},
#endif
    {0x83, 0x08},
    {0x84, 0x07},
    {0x87, 0x08},
    {0x88, 0x07},
    {0x92, 0x11},
    {0x93, 0x01},
    {0x94, 0x60},
    {0x95, 0x17},
    {0xB9, 0x08},
    {0xBA, 0x0B},
    {0xBF, 0x02},
    {0xC0, 0x03},
    {0xD1, 0x39},
    {0xE3, 0xAD},
    {0xE4, 0x02},
    {0xE7, 0x70},
    {0xE8, 0x03},
    {0xEF, 0x03},
    {0x07, 0x00},
    {0x08, 0x05}, /* w_h8 0x500 1280 */
    {0x04, 0x20},
    {0x05, 0x03}, /* h_h8 0x320 800 */
    {0x09, 0x12},
    {0X1B, 0X06}, /* mipi clk pre */
    {0X1E, 0X1D}, /* mipi clk zero */
    {0xED, 0x01},
    {0xEF, 0x01},
    {0xC6, 0x80},
    {0xC7, 0x02},
    {0xC8, 0x20},
    {0xC9, 0x03},
    {0xC4, 0x08},
    {0xC2, 0x08},
    {0xCB, 0x0A}, //v_start, l8
    {0xEF, 0x02},
    {0x23, 0x20},
    {0x24, 0x03},
    {0x21, 0x80},
    {0x22, 0x02},
    {0x19, 0x2E},
    {0x1A, 0x03},
    {0x2D, 0x20},
    {0x2E, 0x03},
    {0x2F, 0x90},
    {0x30, 0x01},
    {0xEF, 0x04},
    {0x02, 0x80},
    {0x03, 0x02},
    {0x04, 0x20},
    {0x05, 0x03},
    {0xEF, 0x00},
    {0x06, 0x00},  // led strobe
    {0x0D, 0xC2},  // led strobe
    {0xE6, 0x10},  // vsync timing
    {0xEB, 0x80},  // reg values update flag
    {0xEF, 0x02},
    {0x7C, 0x01},  //R expo LED polarity
    {0xEF, 0x00},
    {PAG7930_REG_DELAY, 3},
    {PAG7930_REG_END, 0x00},  /* END MARKER */
};


static struct regval_list pag7930_regs_stream_on[] = {
    {0xEF, 0x00},
    {0x36, 0x81},
    {0x3C, 0x08},
    {0x37, 0x65},
    {0x66, 0x29},
    {0x64, 0x00},
    {0x33, 0x0F},
    {0x30, 0x01},
    {0x78, 0x01},
    {0xEB, 0x80},
    {PAG7930_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list pag7930_regs_stream_off[] = {
    {0xEF, 0x00},
    {0x78, 0x00},
    {0x30, 0x00},
    {PAG7930_REG_DELAY, 30},
    {0x64, 0x11},
    {0x33, 0x0B},
    {0xEE, 0x16},
    {0x66, 0x15},
    {0x37, 0x9A},
    {0x3C, 0x04},
    {0x36, 0x42},
    {0xEB, 0x80},
    {PAG7930_REG_END, 0x00},    /* END MARKER */
};

static int pag7930_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
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
        printk(KERN_ERR "pag7930: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int pag7930_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
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
        printk(KERN_ERR "pag7930(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int pag7930_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != PAG7930_REG_END) {
        if (vals->reg_num == PAG7930_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = pag7930_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int pag7930_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != PAG7930_REG_END) {
        if (vals->reg_num == PAG7930_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = pag7930_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n", vals->reg_num, val);
        vals++;
    }

    return 0;
}

static int pag7930_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char l = 1;
    int ret;

    ret = pag7930_read(i2c, PAG7930_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != PAG7930_CHIP_ID_H) {
        printk(KERN_ERR "pag7930 read chip id high failed: 0x%x\n", h);
        return -ENODEV;
    }

    ret = pag7930_read(i2c, PAG7930_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;
    if (l != PAG7930_CHIP_ID_L) {
        printk(KERN_ERR "pag7930 read chip id low failed: 0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "pag7930 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "pag7930_reset");
        if (ret) {
            printk(KERN_ERR "pag7930: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "pag7930_power");
        if (ret) {
            printk(KERN_ERR "pag7930: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        pag7930_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(pag7930_regulator)) {
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
    if (reset_gpio != -1)
        gpio_free(reset_gpio);
err_reset_gpio:
    return ret;
}

static void deinit_gpio(void)
{
    if (reset_gpio != -1)
        gpio_free(reset_gpio);

    if (power_gpio != -1)
        gpio_free(power_gpio);

    if (pag7930_regulator)
        regulator_put(pag7930_regulator);
}

static void pag7930_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (pag7930_regulator)
        regulator_disable(pag7930_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int pag7930_power_on(void)
{
    camera_enable_sensor_mclk(cam_bus_num, 27 * 1000 * 1000);

    if (pag7930_regulator) {
        regulator_enable(pag7930_regulator);
        regulator_disable(pag7930_regulator);
        m_msleep(5);
        regulator_enable(pag7930_regulator);
        m_msleep(10);
    }

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 0);
        m_msleep(5);
        gpio_direction_output(power_gpio, 1);
        m_msleep(10);
    }

    if (reset_gpio != -1) {
        gpio_direction_output(reset_gpio, 0);
        m_msleep(5);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
    }

    int ret;
    ret = pag7930_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "pag7930: failed to detect\n");
        pag7930_power_off();
        return ret;
    }

    ret = pag7930_write_array(i2c_dev, pag7930_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "pag7930: failed to init regs\n");
        pag7930_power_off();
        return ret;
    }

    return 0;
}

static int pag7930_stream_on(void)
{
    int ret = pag7930_write_array(i2c_dev, pag7930_regs_stream_on);
    if (ret)
        printk(KERN_ERR "pag7930: failed to stream on\n");

    return ret;
}

static void pag7930_stream_off(void)
{
    int ret = pag7930_write_array(i2c_dev, pag7930_regs_stream_off);
    if (ret)
        printk(KERN_ERR "pag7930: failed to stream off\n");
}

static int pag7930_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = pag7930_read(i2c_dev, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;
    return ret;
}

static int pag7930_s_register(struct sensor_dbg_register *reg)
{
    int ret = 0;

    ret  = pag7930_write(i2c_dev, reg->reg & 0xff, reg->val & 0xff);
    ret += pag7930_write(i2c_dev, 0xeb, 0x80);
    return ret;
}

static int pag7930_set_integration_time(int value)
{
    int ret = 0;
    unsigned int expo = value;

    ret  = pag7930_write(i2c_dev, 0xef, 0x04);
    ret += pag7930_write(i2c_dev, 0x30, 0x32); //R_ae_Enh=0 to disable auto expo
    ret += pag7930_write(i2c_dev, 0x53, (unsigned char)(expo & 0xff));
    ret += pag7930_write(i2c_dev, 0x54, (unsigned char)((expo >> 8) & 0xff));
    ret += pag7930_write(i2c_dev, 0x55, (unsigned char)((expo >> 16) & 0xff));
    ret += pag7930_write(i2c_dev, 0x5b, (unsigned char)((expo >> 24) & 0x0f));
    ret += pag7930_write(i2c_dev, 0x30, 0x33); //R_ae_manual=1, R_ae_Enh=1 to enable manual expo
    ret += pag7930_write(i2c_dev, 0xeb, 0x80); //update flag
    if (ret < 0) {
        printk(KERN_ERR "pag7930: failed to set integration time\n");
        return ret;
    }

    return 0;
}

static unsigned int pag7930_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = pag7930_again_lut;

    while (lut->gain <= pag7930_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = lut[0].index;
            return lut[0].gain;
        }
        else if (isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->index;
            return (lut - 1)->gain;
        } else {
            if ((lut->gain == pag7930_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static int pag7930_set_analog_gain(int value)
{
    int ret = 0;
    struct again_lut *val_lut = pag7930_again_lut;

    ret  = pag7930_write(i2c_dev, 0xef, 0x04);
    ret += pag7930_write(i2c_dev, 0x30, 0x32); //R_ae_Enh=0 to disable auto gain
    ret += pag7930_write(i2c_dev, 0x51, val_lut[value].reg_51);
    ret += pag7930_write(i2c_dev, 0x52, val_lut[value].reg_52);
    ret += pag7930_write(i2c_dev, 0x30, 0x33); //R_ae_manual=1, R_ae_Enh=1 to enable manual gain
    ret += pag7930_write(i2c_dev, 0xeb, 0x80); //update flag
    if (ret < 0) {
        printk(KERN_ERR "pag7930: failed to set analog gain\n");
        return ret;
    }

    return 0;
}

static unsigned int pag7930_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int pag7930_set_digital_gain(int value)
{
    return 0;
}

static int pag7930_set_fps(int fps)
{
    struct sensor_info *sensor_info = &pag7930_sensor_attr.sensor_info;
    unsigned int pclk = PAG7930_SUPPORT_SCLK_FPS_60;
    unsigned int frm_time;
    unsigned int newformat;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if (newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "warn: fps(%x) no in range\n", fps);
        return -1;
    }

    frm_time = pclk * (fps & 0xffff) / ((fps & 0xffff0000) >> 16);

    ret  = pag7930_write(i2c_dev, 0xef, 0x00);
    ret += pag7930_write(i2c_dev, 0x4c, (unsigned char)(frm_time & 0xff));
    ret += pag7930_write(i2c_dev, 0x4d, (unsigned char)((frm_time >> 8) & 0xff));
    ret += pag7930_write(i2c_dev, 0x4e, (unsigned char)((frm_time >> 16) & 0xff));
    ret += pag7930_write(i2c_dev, 0x4f, (unsigned char)((frm_time >> 24) & 0x0f));
    ret += pag7930_write(i2c_dev, 0xeb, 0x80);
    if(ret < 0) {
        printk(KERN_ERR "pag7930: failed to set fps \n");
        return ret;
    }

    sensor_info->fps = fps;
    sensor_info->total_height = frm_time/sensor_info->total_width;
    sensor_info->max_integration_time = (frm_time-7000)/sensor_info->total_width - 4;

    return 0;
}

static struct sensor_attr pag7930_sensor_attr = {
    .device_name                = PAG7930_DEVICE_NAME,
    .cbus_addr                  = PAG7930_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
#if (PAG7930_RAW8)
        .data_fmt               = MIPI_RAW8,
#else
        .data_fmt               = MIPI_RAW10,
#endif
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 300 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = pag7930_1280_800_60fps_mipi_init_regs,

        .width                  = PAG7930_DEVICE_WIDTH,
        .height                 = PAG7930_DEVICE_HEIGHT,
#if (PAG7930_RAW8)
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR8_1X8,
#else
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,
#endif
        .fps                    = 60 << 16 | 1,
        .total_width            = 1305,
        .total_height           = 894,

        .min_integration_time   = 2,
        .max_integration_time   = 894 - 2,
        .max_again              = 262258,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = pag7930_power_on,
        .power_off              = pag7930_power_off,
        .stream_on              = pag7930_stream_on,
        .stream_off             = pag7930_stream_off,
        .get_register           = pag7930_g_register,
        .set_register           = pag7930_s_register,

        .set_integration_time   = pag7930_set_integration_time,
        .alloc_again            = pag7930_alloc_again,
        .set_analog_gain        = pag7930_set_analog_gain,
        .alloc_dgain            = pag7930_alloc_dgain,
        .set_digital_gain       = pag7930_set_digital_gain,
        .set_fps                = pag7930_set_fps,
    },
};

static int pag7930_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &pag7930_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int pag7930_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &pag7930_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id pag7930_id[] = {
    { PAG7930_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, pag7930_id);

static struct i2c_driver pag7930_driver = {
    .driver = {
        .owner              = THIS_MODULE,
        .name               = PAG7930_DEVICE_NAME,
    },
    .probe                  = pag7930_probe,
    .remove                 = pag7930_remove,
    .id_table               = pag7930_id,
};

static struct i2c_board_info sensor_pag7930_info = {
    .type                   = PAG7930_DEVICE_NAME,
    .addr                   = PAG7930_DEVICE_I2C_ADDR,
};

static __init int init_pag7930(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "pag7930: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    pag7930_driver.driver.name = sensor_name;
    strcpy(pag7930_id[0].name, sensor_name);
    strcpy(sensor_pag7930_info.type, sensor_name);
    pag7930_sensor_attr.device_name = sensor_name;
    if (i2c_addr != -1)
        sensor_pag7930_info.addr = i2c_addr;

    int ret = i2c_add_driver(&pag7930_driver);
    if (ret) {
        printk(KERN_ERR "pag7930: failed to add i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_pag7930_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "pag7930: failed to register i2c device\n");
        i2c_del_driver(&pag7930_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_pag7930(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&pag7930_driver);
}

module_init(init_pag7930);
module_exit(exit_pag7930);

MODULE_DESCRIPTION("x2000 PAG7930 mipi driver");
MODULE_LICENSE("GPL");