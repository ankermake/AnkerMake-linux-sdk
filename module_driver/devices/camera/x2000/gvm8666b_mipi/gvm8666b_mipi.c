/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * GVM8666B mipi driver
 *
 */
#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>

#define GVM8666B_DEVICE_NAME              "gvm8666b"
#define GVM8666B_DEVICE_I2C_ADDR          0x5d
#define GVM8666B_SUPPORT_30FPS_SCLK       (175200000)
#define SENSOR_OUTPUT_MAX_FPS             30
#define SENSOR_OUTPUT_MIN_FPS             5


#define GVM8666B_DEVICE_WIDTH             1008
#define GVM8666B_DEVICE_HEIGHT            1488

#define GVM8666B_CHIP_ID                  (0x1402)
#define GVM8666B_REG_CHIP_ID              0x0100

#define GVM8666B_MAX_WIDTH                2592
#define GVM8666B_MAX_HEIGHT               1944


#define GVM8666B_REG_END                  0xffff
#define GVM8666B_REG_DELAY                0xfffe

struct regval_list {
    unsigned short reg_num;
    unsigned short value;
};

struct again_lut {
    unsigned int index;
    unsigned int gain;
};

static int power_gpio   = -1; // GPIO_PB(20);
static int reset_gpio   = -1; // -1
static int pwdn_gpio    = -1; // GPIO_PB(18);
static int i2c_bus_num  = 0;  // 5

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static int vic_index = 0;

static struct sensor_attr gvm8666b_sensor_attr;

static struct again_lut gvm8666b_again_lut[] = {
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
 * PCLK         : 175.2Mhz
 * resolution   : 2592*1944
 * FrameRate    : 30fps
 * HTS          : 2920
 * VTS          : 2008
 * row_time     : 16.66us
 */
static struct regval_list gvm8666b_1088_1488_30fps_mipi_init_regs[] = {
{0x0204, 0x0004},
    {0x0140, 0x0005},
    {0x01CE, 0x9527},
    {0x7918, 0x0001},
    // {0x1016, 0x0001},
    {0x0222, 0x0001},
    {0x7914, 0x0000},
    {0x7918, 0x0000},
    // {0x1016, 0x0000},
    {0x01CE, 0x9527},
    {0x1014, 0xF000},
    {0x01CE, 0x2266},
    {0x03FA, 0x000c},
    {0x0326, 0x004a},
    {0x03F6, 0x0010},
    {0x01A4, 0x0000},
    {0x0108, 0x0004},
    {0x0108, 0x0005},
    {0x0106, 0x0031},
    {0x0140, 0x0005},
    {0x0344, 0x0000},
    {0x0334, 0x000F},
    {0x0204, 0x0007},
    {0x0110, 0x7C7C},
    {0x0338, 0x0001},
    {0x033A, 0x0003},
    {0x0286, 0x1000},
    {0x0282, 0x0014},
    {0x0330, 0x0000},
    {0x0284, 0x0001},
    {0x03E0, 0x0000},
    {0x03D0, 0x0006},
    {0x03D4, 0x0001},
    {0x03D6, 0x0002},
    {0x0296, 0x1000},
    {0x0292, 0x0014},
    {0x03CC, 0x0000},
    {0x0294, 0x0001},
    {0x010A, 0xC523},
    {0x0318, 0x0000},
    {0x010E, 0x020E},
    {0x0140, 0x0005},
    {0x010C, 0x0021},
    {0x0220, 0x0001},
    {0x0418, 0x0000},
    {0x03F8, 0x0000},
    {0x0406, 0x001f},
    {0x040A, 0x0004},
    {0x040C, 0x0000},
    {0x0202, 0x002F},
    {0x0200, 0x0001},
    {0x0104, 0x00C1},
    {0x041A, 0x0007},
    {0x0418, 0x0003},
    {0x0202, 0x002F},
    {0x042C, 0x0000},
    {0x0202, 0x0027},
    {0x0428, 0x0000},
    {0x0202, 0x0025},
    {0x042a, 0x0000},
    {0x0202, 0x0021},
    {0x0412, 0x0005},
    {0x0202, 0x0020},
    {0x0202, 0x0030},
    {0x042E, 0x0009},
    {0x0430, 0x0009},
    {0x0204, 0x0004},
    {0x03EA, 0x0001},
    {0x0116, 0x0010},
    {0x0178, 0xFFFF},
    {0x0170, 0x0001},
    {0x0172, 0x1820},
    {0x480A, 0x0173},
    {0x4004, 0x1000},
    {0x4006, 0x0100},
    {0x4008, 0x0000},
    {0x0340, 0x0000},
    {0x03dc, 0x0001},
    {0x400a, 0x0001},
    {0x400A, 0x0000},
    {0x02A0, 0x0010},
    {0x02A2, 0x0c00},
    {0x035E, 0x0001},
    {0x035A, 0x0001},
    {0x4A4A, 0x0006},
    {0x4A50, 0x1351},
    {0x4802, 0x2231},
    {0x7914, 0x0000},
    {0x7318, 0x0000},
    {0x7418, 0x0000},
    {0x7518, 0x0000},
    {0x0222, 0x0001},
    {0x7200, 0x0001},
    {0x7204, 0x0011},
    {0x723C, 0x0000},
    {0x7230, 0x0400},
    {0x7234, 0x0000},
    {0x7244, 0x0000},
    {0x7224, 0x0071},
    {0x72B4, 0x0000},
    {0x7284, 0x2250},
    {0x7228, 0x0001},
    {0x0222, 0x0000},
    {0x0222, 0x0001},
    {0x0222, 0x0000},
    {0x7210, 0x0110},
    {0x7214, 0x1107},
    {0x72B0, 0x7CF4},
    {0x7218, 0x3110},
    {0x721C, 0x0000},
    {0x720C, 0x0001},
    {0x721C, 0x0301},
    {0x7240, 0x0001},
    {0x730C, 0x0011},
    {0x730C, 0x0010},
    {0x7004, 0x0001},
    {0x70E0, 0x0007},
    {0x70F0, 0x0008},
    {0x7318, 0x0004},
    {0x731C, 0x0200},
    {0x7418, 0x0004},
    {0x741C, 0x0200},
    {0x7518, 0x0004},
    {0x751C, 0x0200},
    {0x73B0, 0x0003},
    {0x73BC, 0x0A00},
    {0x73C0, 0x0B00},
    {0x73B4, 0x001a},
    {0x73B8, 0x0B13},
    {0x73C8, 0x0508},
    {0x73C4, 0x0902},
    {0x74B0, 0x0003},
    {0x74BC, 0x0A00},
    {0x74C0, 0x0B00},
    {0x74B4, 0x001a},
    {0x74B8, 0x0B13},
    {0x74C8, 0x0507},
    {0x74C4, 0x0902},
    {0x75B0, 0x0003},
    {0x75BC, 0x0A00},
    {0x75C0, 0x0B00},
    {0x75B4, 0x001a},
    {0x75B8, 0x0B13},
    {0x75C8, 0x0507},
    {0x75C4, 0x0902},
    {0x72A4, 0x03F0},
    {0x7330, 0x0000},
    {0x7430, 0x0000},
    {0x7530, 0x0000},
    {0x7334, 0x0858},
    {0x7434, 0x0A58},
    {0x7534, 0x0A58},
    {0x0118, 0x0000},
    {0x010A, 0xC523},
    {0x5700, 0x0003},
    {0x4020, 0x07CF},
    {0x4024, 0x0002},
    {0x4026, 0x009C},
    {0x4028, 0x0000},
    {0x4090, 0x0009},
    {0x4010, 0x0000},
    {0x4014, 0x0000},
    {0x4012, 0x0303},
    {0x403C, 0x0000},
    {0x403E, 0x0000},
    {0x4040, 0xFFF0},
    {0x4042, 0xFFF0},
    {0x4044, 0xFFF0},
    {0x4046, 0xFFF0},
    {0x4048, 0xFFF0},
    {0x404A, 0xFFF0},
    {0x4060, 0x0006},
    {0x4062, 0x0006},
    {0x4064, 0x0007},
    {0x4066, 0x0007},
    {0x4068, 0x0000},
    {0x4070, 0x0004},
    {0x4072, 0x000C},
    {0x4074, 0xFFF0},
    {0x4078, 0x0005},
    {0x407A, 0x000C},
    {0x407C, 0xFFF0},
    {0x4080, 0xFFF0},
    {0x4082, 0xFFF0},
    {0x4084, 0xFFF0},
    {0x4088, 0xFFF0},
    {0x408A, 0xFFF0},
    {0x408C, 0xFFF0},
    {0x4098, 0x0000},
    {0x409A, 0x0000},
    {0x011c, 0x0003},
    {0x40A0, 0x0000},
    {0x40A2, 0x0000},
    {0x40A4, 0x0000},
    {0x40A6, 0x0000},
    {0x40A8, 0x0000},
    {0x40AA, 0x0000},
    {0x4160, 0x0008},
    {0x4162, 0x0008},
    {0x4164, 0x0009},
    {0x4166, 0x0009},
    {0x4168, 0x000A},
    {0x416A, 0x000A},
    {0x416C, 0x000B},
    {0x416E, 0x000B},
    {0x4170, 0x000C},
    {0x4172, 0x000C},
    {0x4174, 0x000D},
    {0x4176, 0x000D},
    {0x4178, 0x000E},
    {0x417A, 0x000E},
    {0x417C, 0x000F},
    {0x417E, 0x000F},
    {0x4180, 0x0010},
    {0x4182, 0x0010},
    {0x4184, 0x0011},
    {0x4186, 0x0011},
    {0x4190, 0x0001},
    {0x4192, 0xfff0},
    {0x4194, 0xfff0},
    {0x4196, 0xfff0},
    {0x4198, 0xfff0},
    {0x419A, 0xfff0},
    {0x419C, 0xfff0},
    {0x41A0, 0x0010},
    {0x41B0, 0x0000},
    {0x41B2, 0x07d0},
    {0x41B4, 0x0003},
    {0x41B6, 0x0400},
    {0x41B8, 0x0501},
    {0x40D0, 0x0000},
    {0x40D2, 0x0000},
    {0x40D8, 0x0000},
    {0x40DA, 0x0000},
    {0x40B0, 0x0010},
    {0x40B2, 0x0000},
    {0x40B4, 0x0000},
    {0x03AA, 0x0001},
    {0x5300, 0x0000},
    {0x5306, 0x0000},
    {0x5308, 0x01F6},
    {0x530A, 0xFFFF},
    {0x530C, 0xFFFF},
    {0x530E, 0x0002},
    {0x4822, 0x0200},
    {0x4824, 0x0000},
    {0x4802, 0x2131},
    {0x4826, 0x0172},
    {0x4828, 0x0000},
    {0x482A, 0x0000},
    {0x482C, 0xFFFF},
    {0x482E, 0xFFFF},
    {0x4022, 0x00B9},
    {0x03AE, 0x0000},
    {0x03B0, 0x0000},
    {0x03B2, 0x0000},
    {0x480C, 0x03FF},
    {0x4810, 0x0000},
    {0x5200, 0x0110},
    {0x520A, 0x0000},
    {0x520C, 0x0000},
    {0x5000, 0x0000},
    {0x480A, 0x0172},
    {0x4806, 0x02F0},
    {0x4A0C, 0x0194},
    {0x4A2C, 0x02E0},
    {0x4A0E, 0x0004},
    {0x4A2E, 0x0190},
    {0x4A10, 0x3FFF},
    {0x4A30, 0x3FFF},
    {0x4A12, 0x0004},
    {0x4A32, 0x0068},
    {0x4A08, 0x3FFF},
    {0x4A28, 0x3FFF},
    {0x4A0A, 0x3FFF},
    {0x4A2A, 0x3FFF},
    {0x4A14, 0x3FFF},
    {0x4A34, 0x0064},
    {0x4A16, 0x3FFF},
    {0x4A36, 0x0028},
    {0x4A00, 0x0068},
    {0x4A02, 0x02C0},
    {0x483C, 0x380A},
    {0x4A24, 0x0194},
    {0x4A44, 0x00CC},
    {0x4A26, 0x02C0},
    {0x4A46, 0x01F8},
    {0x4A04, 0x0068},
    {0x4A06, 0x02C0},
    {0x4836, 0x0000},
    {0x4838, 0x0000},
    {0x4A18, 0x00BC},
    {0x4A38, 0x00C4},
    {0x4A1A, 0x01B0},
    {0x4A3A, 0x01B8},
    {0x4A1C, 0x019C},
    {0x4A3C, 0x01A4},
    {0x4A1E, 0x02C8},
    {0x4A3E, 0x02D0},
    {0x4A20, 0x0008},
    {0x4A40, 0x000C},
    {0x4A22, 0x0004},
    {0x4A42, 0x0230},
    {0x5000, 0x0100},
    {0x72AC, 0x002C},
    {0x72A0, 0x0000},
    {0x03BC, 0x0003},
    {0x0388, 0x0001},
    {0x483A, 0x0023},
    {0x03A2, 0x0001},
    {0x03A4, 0x0001},
    {0x03FE, 0x000C},
    {0x039C, 0x0006},
    {0x042C, 0x000C},
    {0x0428, 0x000E},
    {0x042A, 0x000C},
    {0x0412, 0x0005},
    {0x042E, 0x0004},
    {0x0430, 0x000A},
    {0x4016, 0x0001},
    {0x5002, 0x0800},
    {0x5004, 0x0000},
    {0x4000, 0x1002},
    {0x4000, 0x0000},
    {0x4000, 0x0050},
    {0x4500, 0x0000},
    {0x022C, 0x10F4},

    {0x0228, 0x0070},
    {0x01D4, 0x0011},
    {0x01A6, 0x0200},
    {0x4240, 0x0017},
    {0x4242, 0x0020},
    {0x4244, 0x051D},
    {0x4246, 0x002A},
    {0x4248, 0x050C},
    {0x424e, 0x0516},
    {0x4250, 0x0518},
    {0x4252, 0x0522},
    {0x0314, 0x0007},

    {GVM8666B_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list gvm8666b_regs_stream_on[] = {
    // {0x0844, 0x0001},
    // {0x081a, 0x0061},       /* 单帧patten模式 */
    // {0x081a, 0x0042},       /* 单帧深度图像 */
    // {0x7240, 0x00},

    // {0x081a, 0x0042},
    // {0x7240, 0x00},
    // {0x081a, 0x0042},
    {0x081a, 0x0062},    /* 连续深度采图 */
    // {0x081a, 0x0045},
    {GVM8666B_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list gvm8666b_regs_stream_off[] = {
    {0x081a, 0x0040},
    {GVM8666B_REG_END, 0x0}, /* END MARKER */
};

static int gvm8666b_write(struct i2c_client *i2c, unsigned short reg, unsigned short value)
{
    unsigned char buf[4] = {reg >> 8, reg & 0xff, value & 0xff, value >> 8};
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = 4,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "gvm8666b: failed to write reg: 0x%x\n", (int)reg);

    return ret;
}

static int gvm8666b_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
            .addr = i2c->addr,
            .flags  = I2C_M_RD,
            .len    = 2,
            .buf    = value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "gvm8666b(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

#include "gvm8666b_ops.c"

static int gvm8666b_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != GVM8666B_REG_END) {
        if ((vals->reg_num == 0x0284) || (vals->reg_num == 0x0294)
            || (vals->reg_num == 0x7240)) {
                msleep(3);
        } else {
            ret = gvm8666b_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        // printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static inline int gvm8666b_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != GVM8666B_REG_END) {
        if (vals->reg_num == GVM8666B_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gvm8666b_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n",vals->reg_num, val);
        vals++;
    }

    return 0;
}

static int gvm8666b_detect(struct i2c_client *i2c)
{
    unsigned char val[2] = {0};
    unsigned short id = 0;
    int ret;

    ret = gvm8666b_read(i2c, GVM8666B_REG_CHIP_ID, val);
    if (ret < 0)
        return ret;

    id = val[1] << 8 | val[0];
    if (id != GVM8666B_CHIP_ID) {
        printk(KERN_ERR "gvm8666b read chip id high failed:0x%x\n", id);
        return -ENODEV;
    }

    gvm8666b_write(i2c, 0x81a, 0x4000 | 0x3e8);
    gvm8666b_write(i2c, 0x81a, 0x8000 | 0x3e8);

    printk(KERN_DEBUG "gvm8666b get chip id = %02x%02x\n", val[1], val[0]);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "gvm8666b_reset");
        if (ret) {
            printk(KERN_ERR "gvm8666b: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "gvm8666b_pwdn");
        if (ret) {
            printk(KERN_ERR "gvm8666b: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gvm8666b_power");
        if (ret) {
            printk(KERN_ERR "gvm8666b: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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

static void gvm8666b_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk(vic_index);
}

static int gvm8666b_power_on(void)
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
        gpio_direction_output(reset_gpio, 0);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(20);
    }

    int ret, retry;
    for (retry = 0; retry < 10; retry++) {
        ret = gvm8666b_detect(i2c_dev);
        if (!ret)
            break;
    }
    if (retry >= 10) {
        printk(KERN_ERR "gvm8666b: failed to detect\n");
        gvm8666b_power_off();
        return ret;
    }

    ret = gvm8666b_update_config(i2c_dev);
    if (ret) {
        printk(KERN_ERR "gvm8666b update config failed.\n");
        goto err;
    }

    ret = gvm8666b_write_array(i2c_dev, gvm8666b_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "gvm8666b: failed to init regs\n");
        goto err;
    }

    ret = gvm8666b_img_sensor_update_fw(i2c_dev);
    if (ret < 0) {
        printk(KERN_ERR "image sensor update firmware failed.\n");
        goto err;
    }

    ret = gvm8666b_update_phase_shift(i2c_dev);
    if (ret < 0) {
        printk(KERN_ERR "gvm8666b update phase_shift failed.\n");
        goto err;
    }

    return 0;

err:
    gvm8666b_power_off();
    return ret;
}

static int gvm8666b_stream_on(void)
{
    int ret = gvm8666b_write_array(i2c_dev, gvm8666b_regs_stream_on);

    if (ret)
        printk(KERN_ERR "gvm8666b: failed to stream on\n");

    return ret;
}

static void gvm8666b_stream_off(void)
{
    int ret = gvm8666b_write_array(i2c_dev, gvm8666b_regs_stream_off);

    if (ret)
        printk(KERN_ERR "gvm8666b: failed to stream on\n");

}

static int gvm8666b_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val[2];
    int ret;

    if (reg->reg == GT_EEPROM_I2C_READ_ADDR) {
        ret = gvm8666b_read_eeprom_info(i2c_dev);
        if (ret < 0) {
            printk(KERN_ERR "gvm8666b: failed to read eeprom info\n");
            return ret;
        }
    } else {
        ret = gvm8666b_read(i2c_dev, reg->reg & 0xffff, val);
        reg->val = val[1] << 8 | val[0];
    }

    return ret;
}

static int gvm8666b_s_register(struct sensor_dbg_register *reg)
{
    return gvm8666b_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static int gvm8666b_set_integration_time(int value)
{
    int ret = 0;
    unsigned int expo = value;

    expo = expo >> 2;
    expo = expo << 2;   /* 保证为4的倍数 */

    m_msleep(1);
    ret = gvm8666b_write(i2c_dev, 0xfe, 0x00);
    ret += gvm8666b_write(i2c_dev, 0x03, (unsigned char)((expo >> 8) & 0x3f));
    ret += gvm8666b_write(i2c_dev, 0x04, (unsigned char)(expo & 0xff));
    if (ret < 0) {
        printk(KERN_ERR "gvm8666b: set integration time error. line=%d\n" ,__LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int gvm8666b_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = gvm8666b_again_lut;

    while (lut->gain <= gvm8666b_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = lut[0].index;
            return lut[0].gain;
        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut-1)->index;
            return (lut-1)->gain;
        } else {
            if ((lut->gain == gvm8666b_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

static int gvm8666b_set_analog_gain(int value)
{
    int ret = 0;

    ret =  gvm8666b_write(i2c_dev, 0xfe, 0x00);
    ret += gvm8666b_write(i2c_dev, 0xb6, value);
    ret += gvm8666b_write(i2c_dev, 0xb1, (256 >> 8) & 0xf);
    ret += gvm8666b_write(i2c_dev, 0xb2, 256 & 0xfc);
    if (ret < 0) {
        printk(KERN_ERR "gvm8666b: set analog gain error  line=%d" ,__LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int gvm8666b_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{

    return isp_gain;
}

static int gvm8666b_set_digital_gain(int value)
{
    return 0;
}

static int gvm8666b_set_fps(int fps)
{
    struct sensor_info *sensor_info = &gvm8666b_sensor_attr.sensor_info;
    unsigned int wpclk = 0;
    unsigned short vts = 0;
    unsigned short hb=0;
    unsigned short hts=0;
    unsigned int max_fps = 0;
    unsigned char tmp;
    unsigned int newformat = 0;
    int ret = 0;

    wpclk = GVM8666B_SUPPORT_30FPS_SCLK;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk(KERN_ERR "warn: fps(%x) no in range\n", fps);
        return -1;
    }
    /* H Blanking */
    ret = gvm8666b_read(i2c_dev, 0x05, &tmp);
    hb = tmp;
    ret += gvm8666b_read(i2c_dev, 0x06, &tmp);
    if(ret < 0)
        return -1;
    hb = (hb << 8) + tmp;
    hts = hb << 2;

    vts = wpclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = gvm8666b_write(i2c_dev, 0xfe, 0x00);
    ret = gvm8666b_write(i2c_dev, 0x41, (unsigned char)(vts >> 8) & 0x3f);
    ret += gvm8666b_write(i2c_dev, 0x42, (unsigned char)(vts & 0xff));
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts;

    return 0;
}


static struct sensor_attr gvm8666b_sensor_attr = {
    .device_name                = GVM8666B_DEVICE_NAME,
    .cbus_addr                  = GVM8666B_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW12,
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = gvm8666b_1088_1488_30fps_mipi_init_regs,

        .width                  = GVM8666B_DEVICE_WIDTH,
        .height                 = GVM8666B_DEVICE_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SGRBG12_1X12,

        .fps                    = 30 << 16 | 1,  /* 30/1 */
        .total_width            = 2920,
        .total_height           = 2008,

        .min_integration_time   = 2,
        .max_integration_time   = 2008,
        .max_again              = 259755,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = gvm8666b_power_on,
        .power_off              = gvm8666b_power_off,
        .stream_on              = gvm8666b_stream_on,
        .stream_off             = gvm8666b_stream_off,
        .get_register           = gvm8666b_g_register,
        .set_register           = gvm8666b_s_register,

        .set_integration_time   = gvm8666b_set_integration_time,
        .alloc_again            = gvm8666b_alloc_again,
        .set_analog_gain        = gvm8666b_set_analog_gain,
        .alloc_dgain            = gvm8666b_alloc_dgain,
        .set_digital_gain       = gvm8666b_set_digital_gain,
        .set_fps                = gvm8666b_set_fps,
    },
};

static int gvm8666b_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(vic_index, &gvm8666b_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int gvm8666b_remove(struct i2c_client *client)
{
    camera_unregister_sensor(vic_index, &gvm8666b_sensor_attr);
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id gvm8666b_id[] = {
    { GVM8666B_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, gvm8666b_id);

static struct i2c_driver gvm8666b_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = GVM8666B_DEVICE_NAME,
    },
    .probe          = gvm8666b_probe,
    .remove         = gvm8666b_remove,
    .id_table       = gvm8666b_id,
};

static struct i2c_board_info sensor_gvm8666b_info = {
    .type           = GVM8666B_DEVICE_NAME,
    .addr           = GVM8666B_DEVICE_I2C_ADDR,
};


static __init int init_gvm8666b(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "gvm8666b: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&gvm8666b_driver);
    if (ret) {
        printk(KERN_ERR "gvm8666b: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_gvm8666b_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "gvm8666b: failed to register i2c device\n");
        i2c_del_driver(&gvm8666b_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_gvm8666b(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&gvm8666b_driver);
}

module_init(init_gvm8666b);
module_exit(exit_gvm8666b);

MODULE_DESCRIPTION("X2000 GVM8666B driver");
MODULE_LICENSE("GPL");
