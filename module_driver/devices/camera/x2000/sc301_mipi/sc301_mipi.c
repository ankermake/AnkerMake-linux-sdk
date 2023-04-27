/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * SC301
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <linux/regulator/consumer.h>
#include <camera/hal/camera_sensor.h>

#define SC301_DEVICE_NAME              "sc301"
#define SC301_DEVICE_I2C_ADDR          0x30

#define SC301_CHIP_ID_H                0xcc
#define SC301_CHIP_ID_L                0x40

#define SC301_REG_END                  0xff
#define SC301_REG_DELAY                0xfe
#define SC301_SUPPORT_SCLK             (54000000)
#define SENSOR_OUTPUT_MAX_FPS          30
#define SENSOR_OUTPUT_MIN_FPS          5
#define SC301_MAX_WIDTH                2048
#define SC301_MAX_HEIGHT               1536
#define SC301_WIDTH                    2048
#define SC301_HEIGHT                   1536

static int power_gpio       = -1;
static int reset_gpio       = -1;
static int pwdn_gpio        = -1;   // GPIO_PD(2)
static int i2c_bus_num      = -1;   // 5
static short i2c_addr       = -1;   // 0x30
static int cam_bus_num      = -1;   // 5
static char *sensor_name    = NULL; // sc301-vis
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
static struct sensor_attr sc301_sensor_attr;
static struct regulator *sc301_regulator = NULL;

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

struct again_lut sc301_again_lut[] = {
    { 0, 0x0,  0x0,  0x80,  0},   // 1x = ananlog
    { 1, 0x0,  0x0,  0x84, 2886},
    { 2, 0x0,  0x0,  0x88, 5776},
    { 3, 0x0,  0x0,  0x8c, 8494},
    { 4, 0x0,  0x0,  0x90, 11136},
    { 5, 0x0,  0x0,  0x94, 13706},
    { 6, 0x0,  0x0,  0x98, 16287},
    { 7, 0x0,  0x0,  0x9c, 18723},
    { 8, 0x0,  0x0,  0xa0, 21097},
    { 9, 0x0,  0x0,  0xa4, 23413},
    { 10, 0x0,  0x0,  0xa8, 25746},
    { 11, 0x0,  0x0,  0xac, 27952},
    { 12, 0x0,  0x0,  0xb0, 30108},
    { 13, 0x0,  0x0,  0xb4, 32216},
    { 14, 0x0,  0x0,  0xb8, 34344},
    { 15, 0x0,  0x0,  0xbc, 36361},
    { 16, 0x0,  0x0,  0xc0, 38335},
    { 17, 0x0,  0x0,  0xc4, 40269},
    { 18, 0x0,  0x0,  0xc8, 42225},
    { 19, 0x40, 0x0,  0x80, 42587},   // 1.569x = ananlog
    { 20, 0x0,  0x0,  0xcc, 44082},
    { 21, 0x0,  0x0,  0xd0, 45903},
    { 22, 0x0,  0x0,  0xd4, 47689},
    { 23, 0x0,  0x0,  0xd8, 49499},
    { 24, 0x0,  0x0,  0xdc, 51220},
    { 25, 0x0,  0x0,  0xe0, 52910},
    { 26, 0x0,  0x0,  0xe4, 54570},
    { 27, 0x0,  0x0,  0xe8, 56253},
    { 28, 0x0,  0x0,  0xec, 57856},
    { 29, 0x0,  0x0,  0xf0, 59433},
    { 30, 0x0,  0x0,  0xf4, 60983},
    { 31, 0x0,  0x0,  0xf8, 62557},
    { 32, 0x0,  0x0,  0xfc, 64058},
    { 33, 0x0,  0x01, 0x80, 65535},
    { 34, 0x0,  0x01, 0x84, 68467},
    { 35, 0x0,  0x01, 0x88, 71266},
    { 36, 0x0,  0x01, 0x8c, 74029},
    { 37, 0x0,  0x01, 0x90, 76671},
    { 38, 0x0,  0x01, 0x94, 79281},
    { 39, 0x0,  0x01, 0x98, 81782},
    { 40, 0x0,  0x01, 0x9c, 84258},
    { 41, 0x0,  0x01, 0xa0, 86632},
    { 42, 0x0,  0x01, 0xa4, 88985},
    { 43, 0x0,  0x01, 0xa8, 91245},
    { 44, 0x0,  0x01, 0xac, 93487},
    { 45, 0x0,  0x01, 0xb0, 95643},
    { 46, 0x0,  0x01, 0xb4, 97785},
    { 47, 0x0,  0x01, 0xb8, 99846},
    { 48, 0x0,  0x01, 0xbc, 101896},
    { 49, 0x0,  0x01, 0xc0, 103870},
    { 50, 0x0,  0x01, 0xc4, 105835},
    { 51, 0x0,  0x01, 0xc8, 107730},
    { 52, 0x48, 0x0,  0x80, 108122},   // 3.138x = ananlog
    { 53, 0x0,  0x01, 0xcc, 109617},
    { 54, 0x0,  0x01, 0xd0, 111438},
    { 55, 0x0,  0x01, 0xd4, 113253},
    { 56, 0x0,  0x01, 0xd8, 115006},
    { 57, 0x0,  0x01, 0xdc, 116755},
    { 58, 0x0,  0x01, 0xe0, 118445},
    { 59, 0x0,  0x01, 0xe8, 121762},
    { 60, 0x0,  0x01, 0xec, 123391},
    { 61, 0x0,  0x01, 0xf0, 124968},
    { 62, 0x0,  0x01, 0xf4, 126543},
    { 63, 0x0,  0x01, 0xf8, 128068},
    { 64, 0x0,  0x01, 0xfc, 129593},
    { 65, 0x0,  0x03, 0x80, 131070},
    { 66, 0x0,  0x03, 0x84, 133979},
    { 67, 0x0,  0x03, 0x88, 136801},
    { 68, 0x0,  0x03, 0x8c, 139542},
    { 69, 0x0,  0x03, 0x90, 142206},
    { 70, 0x0,  0x03, 0x94, 144796},
    { 71, 0x0,  0x03, 0x98, 147317},
    { 72, 0x0,  0x03, 0x9c, 149773},
    { 73, 0x0,  0x03, 0xa0, 152167},
    { 74, 0x0,  0x03, 0xa4, 154502},
    { 75, 0x0,  0x03, 0xa8, 156780},
    { 76, 0x0,  0x03, 0xac, 159005},
    { 77, 0x0,  0x03, 0xb0, 161178},
    { 78, 0x0,  0x03, 0xb4, 163303},
    { 79, 0x0,  0x03, 0xb8, 165381},
    { 80, 0x0,  0x03, 0xbc, 167414},
    { 81, 0x0,  0x03, 0xc0, 169405},
    { 82, 0x0,  0x03, 0xc4, 171355},
    { 83, 0x0,  0x03, 0xc8, 173265},
    { 84, 0x0,  0x03, 0xcc, 175137},
    { 85, 0x49, 0x0,  0x80, 173657},   // 6.276x = ananlog
    { 86, 0x0,  0x03, 0xd0, 176973},
    { 87, 0x0,  0x03, 0xd4, 178774},
    { 88, 0x0,  0x03, 0xd8, 180541},
    { 89, 0x0,  0x03, 0xdc, 182276},
    { 90, 0x0,  0x03, 0xe0, 183980},
    { 91, 0x0,  0x03, 0xe4, 185653},
    { 92, 0x0,  0x03, 0xe8, 187297},
    { 93, 0x0,  0x03, 0xec, 188914},
    { 94, 0x0,  0x03, 0xf0, 190503},
    { 95, 0x0,  0x03, 0xf4, 192065},
    { 96, 0x0,  0x03, 0xf8, 193603},
    { 97, 0x0,  0x03, 0xfc, 195116},
    { 98, 0x0,  0x07, 0x80, 196605},
    { 99, 0x0,  0x07, 0x84, 199514},
    { 100, 0x0,  0x07, 0x88, 202336},
    { 101, 0x0,  0x07, 0x8c, 205077},
    { 102, 0x0,  0x07, 0x90, 207741},
    { 103, 0x0,  0x07, 0x94, 210331},
    { 104, 0x0,  0x07, 0x98, 212852},
    { 105, 0x0,  0x07, 0x9c, 215308},
    { 106, 0x0,  0x07, 0xa0, 217702},
    { 107, 0x0,  0x07, 0xa4, 220037},
    { 108, 0x0,  0x07, 0xa8, 222315},
    { 109, 0x0,  0x07, 0xac, 224540},
    { 110, 0x0,  0x07, 0xb0, 226713},
    { 111, 0x0,  0x07, 0xb4, 228838},
    { 112, 0x0,  0x07, 0xb8, 230916},
    { 113, 0x0,  0x07, 0xbc, 232949},
    { 114, 0x0,  0x07, 0xc0, 234940},
    { 115, 0x0,  0x07, 0xc4, 236890},
    { 116, 0x0,  0x07, 0xc8, 238800},
    { 117, 0x4b, 0x0,  0x80, 239192},   // 12.552x = ananlog
    { 118, 0x0,  0x07, 0xcc, 240672},
    { 119, 0x0,  0x07, 0xd0, 242508},
    { 120, 0x0,  0x07, 0xd4, 244309},
    { 121, 0x0,  0x07, 0xd8, 246076},
    { 122, 0x0,  0x07, 0xdc, 247811},
    { 123, 0x0,  0x07, 0xe0, 249515},
    { 124, 0x0,  0x07, 0xe4, 251188},
    { 125, 0x0,  0x07, 0xe8, 252832},
    { 126, 0x0,  0x07, 0xec, 254449},
    { 127, 0x0,  0x07, 0xf0, 256038},
    { 128, 0x0,  0x07, 0xf4, 257600},
    { 129, 0x0,  0x07, 0xf8, 259138},
    { 130, 0x0,  0x07, 0xfc, 260651},
    { 131, 0x0,  0x0f, 0x80, 262140},  // 16x
};

/*
 * SensorName RAW_30fps
 * width=2048
 * height=1536
 * SlaveID=0x60
 * mclk=24
 * avdd=2.800000
 * dovdd=1.800000
 * dvdd=1.500000
 */
struct regval_list sc301_init_regs_2048_1536_30fps_mipi[] = {

    {0x0103, 0x01},
    {0x0100, 0x00},
    {0x36e9, 0x80},
    {0x37f9, 0x80},
    {0x301c, 0x78},
    {0x301f, 0x11},
    {0x30b8, 0x44},
    {0x3208, 0x08},
    {0x3209, 0x00},
    {0x320a, 0x06},
    {0x320b, 0x00},
    {0x320c, 0x04},
    {0x320d, 0x65},
    {0x320e, 0x06},
    {0x320f, 0x40},
    {0x3214, 0x11},
    {0x3215, 0x11},
    {0x3223, 0xc0},
    {0x3253, 0x0c},
    {0x3274, 0x09},
    {0x3301, 0x08},
    {0x3306, 0x58},
    {0x3308, 0x08},
    {0x330a, 0x00},
    {0x330b, 0xe0},
    {0x330e, 0x10},
    {0x3314, 0x14},
    {0x331e, 0x55},
    {0x331f, 0x7d},
    {0x3333, 0x10},
    {0x3334, 0x40},
    {0x335e, 0x06},
    {0x335f, 0x08},
    {0x3364, 0x5e},
    {0x337c, 0x02},
    {0x337d, 0x0a},
    {0x3390, 0x01},
    {0x3391, 0x03},
    {0x3392, 0x07},
    {0x3393, 0x08},
    {0x3394, 0x08},
    {0x3395, 0x08},
    {0x3396, 0x08},
    {0x3397, 0x09},
    {0x3398, 0x1f},
    {0x3399, 0x08},
    {0x339a, 0x0a},
    {0x339b, 0x40},
    {0x339c, 0x88},
    {0x33a2, 0x04},
    {0x33ad, 0x0c},
    {0x33b1, 0x80},
    {0x33b3, 0x30},
    {0x33f9, 0x68},
    {0x33fb, 0x80},
    {0x33fc, 0x48},
    {0x33fd, 0x5f},
    {0x349f, 0x03},
    {0x34a6, 0x48},
    {0x34a7, 0x5f},
    {0x34a8, 0x30},
    {0x34a9, 0x30},
    {0x34aa, 0x00},
    {0x34ab, 0xf0},
    {0x34ac, 0x01},
    {0x34ad, 0x08},
    {0x34f8, 0x5f},
    {0x34f9, 0x10},
    {0x3630, 0xf0},
    {0x3631, 0x85},
    {0x3632, 0x74},
    {0x3633, 0x22},
    {0x3637, 0x4d},
    {0x3638, 0xcb},
    {0x363a, 0x8b},
    {0x363c, 0x08},
    {0x3640, 0x00},
    {0x3641, 0x38},
    {0x3670, 0x4e},
    {0x3674, 0xc0},
    {0x3675, 0xb0},
    {0x3676, 0xa0},
    {0x3677, 0x83},
    {0x3678, 0x87},
    {0x3679, 0x8a},
    {0x367c, 0x49},
    {0x367d, 0x4f},
    {0x367e, 0x48},
    {0x367f, 0x4b},
    {0x3690, 0x33},
    {0x3691, 0x33},
    {0x3692, 0x44},
    {0x3699, 0x8a},
    {0x369a, 0xa1},
    {0x369b, 0xc2},
    {0x369c, 0x48},
    {0x369d, 0x4f},
    {0x36a2, 0x4b},
    {0x36a3, 0x4f},
    {0x36ea, 0x09},
    {0x36eb, 0x0d},
    {0x36ec, 0x1c},
    {0x36ed, 0x25},
    {0x370f, 0x01},
    {0x3714, 0x80},
    {0x3722, 0x09},
    {0x3724, 0x41},
    {0x3725, 0xc1},
    {0x3728, 0x00},
    {0x3771, 0x09},
    {0x3772, 0x05},
    {0x3773, 0x05},
    {0x377a, 0x48},
    {0x377b, 0x49},
    {0x37fa, 0x09},
    {0x37fb, 0x33},
    {0x37fc, 0x11},
    {0x37fd, 0x18},
    {0x3905, 0x8d},
    {0x391d, 0x08},
    {0x3922, 0x1a},
    {0x3926, 0x21},
    {0x3933, 0x80},
    {0x3934, 0x0d},
    {0x3937, 0x6a},
    {0x3939, 0x00},
    {0x393a, 0x0e},
    {0x39dc, 0x02},
    {0x3e00, 0x00},
    {0x3e01, 0x63},
    {0x3e02, 0x80},
    {0x3e03, 0x0b},
    {0x3e1b, 0x2a},
    {0x4407, 0x34},
    {0x440e, 0x02},
    {0x5001, 0x40},
    {0x5007, 0x80},
    {0x36e9, 0x24},
    {0x37f9, 0x24},
    // {0x4501, 0xcc},
    // {0x3902, 0x80},
    // {0x3e07, 0x40},
    {0x0100, 0x01},
    {SC301_REG_DELAY,10},
    {SC301_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list sc301_regs_stream_on[] = {

    {0x0100, 0x01},            /* RESET_REGISTER */
    {SC301_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list sc301_regs_stream_off[] = {

    {0x0100, 0x00},            /* RESET_REGISTER */
    {SC301_REG_END, 0x00},     /* END MARKER */
};

static int sc301_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
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
        printk(KERN_ERR "sc301: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int sc301_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
        printk(KERN_ERR "sc301(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int sc301_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != SC301_REG_END) {
        if (vals->reg_num == SC301_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = sc301_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int sc301_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != SC301_REG_END) {
        if (vals->reg_num == SC301_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = sc301_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        printk("reg = 0x%02x, val = 0x%02x\n",vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int sc301_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;
    ret = sc301_read(i2c, 0x3107, &h);
    if (ret < 0)
        return ret;
    if (h != SC301_CHIP_ID_H) {
        printk(KERN_ERR "sc301 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = sc301_read(i2c, 0x3108, &l);

    if (ret < 0)
        return ret;

    if (l != SC301_CHIP_ID_L) {
        printk(KERN_ERR "sc301 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "sc301 get chip id = %02x%02x\n", h, l);
    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "sc301_reset");
        if (ret) {
            printk(KERN_ERR "sc301: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "sc301_pwdn");
        if (ret) {
            printk(KERN_ERR "sc301: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "sc301_power");
        if (ret) {
            printk(KERN_ERR "sc301: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        sc301_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(sc301_regulator)) {
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

    if (sc301_regulator)
        regulator_put(sc301_regulator);
}

static void sc301_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (sc301_regulator)
        regulator_disable(sc301_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int sc301_power_on(void)
{
    int ret;
    camera_enable_sensor_mclk(cam_bus_num, 25 * 1000 * 1000);

    if (sc301_regulator)
        regulator_enable(sc301_regulator);

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

    ret = sc301_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "sc301: failed to detect\n");
       sc301_power_off();
        return ret;
    }

    ret = sc301_write_array(i2c_dev, sc301_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "sc301: failed to init regs\n");
        sc301_power_off();
        return ret;
    }
    return 0;
}

static int sc301_stream_on(void)
{
    int ret;
    ret = sc301_write_array(i2c_dev, sc301_regs_stream_on);
    if (ret)
        printk(KERN_ERR "sc301: failed to stream on\n");

    return ret;
}

static void sc301_stream_off(void)
{
    int ret = sc301_write_array(i2c_dev, sc301_regs_stream_off);
    if (ret)
        printk(KERN_ERR "sc301: failed to stream on\n");
}

static int sc301_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = sc301_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int sc301_s_register(struct sensor_dbg_register *reg)
{
    int ret;
    ret = sc301_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);

    return ret;
}

/*
 *  0x3e00[3:0]
 *  0x3e01[7:0]
 *  0x3e02[7:4]
 */
static int sc301_set_integration_time(int value)
{
    int ret = 0;
    ret = sc301_write(i2c_dev, 0x3e00, (unsigned char)(value >> 12));
    ret += sc301_write(i2c_dev, 0x3e01, (unsigned char)((value >> 4) & 0xff));
    ret += sc301_write(i2c_dev, 0x3e02, (unsigned char)((value & 0x0f) << 4));

    if (ret < 0)
        return ret;

    return 0;
}

static unsigned int sc301_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

    struct again_lut *lut = sc301_again_lut;
    while (lut->gain <=sc301_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = lut[0].index;
            return lut[0].gain;
        }
        else if (isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->index;
            return (lut - 1)->gain;
        } else {
            if ((lut->gain ==sc301_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

/*
 *  0x3e09[7:0] analog gain
 *  0x3e06 coarse digital gain
 *  0x3e07 fine digital gain
 */
static int sc301_set_analog_gain(int value)
{
    int ret = 0;
    struct again_lut *val_lut =sc301_again_lut;

    ret = sc301_write(i2c_dev, 0x3e09, val_lut[value].analog_value);
    ret += sc301_write(i2c_dev, 0x3e06, val_lut[value].digital_coarse_value);
    ret += sc301_write(i2c_dev, 0x3e07, val_lut[value].digital_fine_coarse);

    if (ret < 0) {
        printk(KERN_ERR "sc301: failed to set analog gain\n");
        return ret;
    }

    return 0;
}

unsigned int sc301_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int sc301_set_digital_gain(int value)
{
    return 0;
}

static int sc301_set_fps(int fps)
{
    struct sensor_info *sensor_info = &sc301_sensor_attr.sensor_info;
    unsigned int sclk = 0;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp = 0;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "sc301 fps(%d) no in range\n", fps);
        return -1;
    }
    sclk = SC301_SUPPORT_SCLK;

    ret = sc301_read(i2c_dev, 0x320c, &tmp);
    hts = tmp;
    ret += sc301_read(i2c_dev, 0x320d, &tmp);
    if(ret < 0)
        return -1;
    hts = (hts << 8) + tmp;

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = sc301_write(i2c_dev, 0x320f, (unsigned char)(vts & 0xff));
    ret += sc301_write(i2c_dev, 0x320e, (unsigned char)(vts >> 8));
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}

static struct sensor_attr sc301_sensor_attr = {
    .device_name                = SC301_DEVICE_NAME,
    .cbus_addr                  = SC301_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = sc301_init_regs_2048_1536_30fps_mipi,
        .width                  = SC301_WIDTH,
        .height                 = SC301_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 30 << 16 | 1,
        .total_width            = 0x465 * 2,
        .total_height           = 0x640,

        .min_integration_time   = 6,
        .max_integration_time   = 0x640 - 4,
        .one_line_expr_in_us    = 15,
        .max_again              = 262140,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = sc301_power_on,
        .power_off              = sc301_power_off,
        .stream_on              = sc301_stream_on,
        .stream_off             = sc301_stream_off,
        .get_register           = sc301_g_register,
        .set_register           = sc301_s_register,

        .set_integration_time   = sc301_set_integration_time,
        .alloc_again            = sc301_alloc_again,
        .set_analog_gain        = sc301_set_analog_gain,
        .alloc_dgain            = sc301_alloc_dgain,
        .set_digital_gain       = sc301_set_digital_gain,
        .set_fps                = sc301_set_fps,
    },
};


static int sc301_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int ret;

    ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &sc301_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sc301_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &sc301_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id sc301_id[] = {
    { SC301_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sc301_id);

static struct i2c_driver sc301_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = SC301_DEVICE_NAME,
    },
    .probe              = sc301_probe,
    .remove             = sc301_remove,
    .id_table           = sc301_id,
};

static struct i2c_board_info sensor_sc301_info = {
    .type               = SC301_DEVICE_NAME,
    .addr               = SC301_DEVICE_I2C_ADDR,
};

static __init int init_sc301(void)
{
    if (i2c_bus_num < 0)
    {
        printk(KERN_ERR "sc301: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    sc301_driver.driver.name = sensor_name;
    strcpy(sc301_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_sc301_info.addr = i2c_addr;
    strcpy(sensor_sc301_info.type, sensor_name);
    sc301_sensor_attr.device_name = sensor_name;

    int ret = i2c_add_driver(&sc301_driver);
    if (ret) {
        printk(KERN_ERR "sc301: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_sc301_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "sc301: failed to register i2c device\n");
        i2c_del_driver(&sc301_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_sc301(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sc301_driver);
}

module_init(init_sc301);
module_exit(exit_sc301);

MODULE_DESCRIPTION("x2000 sc301 driver");
MODULE_LICENSE("GPL");