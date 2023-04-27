/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * SC1345 mipi driver
 *
 */
#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>


#define SC1345_DEVICE_NAME             "sc1345"
#define SC1345_DEVICE_I2C_ADDR         0x30

#define SC1345_SUPPORT_SCLK_FPS_60     (72072000)
#define SENSOR_OUTPUT_MAX_FPS           60
#define SENSOR_OUTPUT_MIN_FPS           5

#define SC1345_WIDTH                   1280
#define SC1345_HEIGHT                  720

#define SC1345_CHIP_ID_H               (0xda)
#define SC1345_CHIP_ID_L               (0x23)
#define SC1345_REG_CHIP_ID_HIGH        0x3107
#define SC1345_REG_CHIP_ID_LOW         0x3108

#define SC1345_REG_DELAY               0xfffe
#define SC1345_REG_END                 0Xffff

static int avdd_gpio            = -1;
static int dvdd_gpio            = -1;
static int dovdd_gpio           = -1;
static int reset_gpio           = -1;
static int pwdn_gpio            = -1;
static int i2c_bus_num          = -1;
static short i2c_addr           = -1;
static int cam_bus_num          = -1;
static char *sensor_name        = NULL;

module_param_gpio(avdd_gpio, 0644);
module_param_gpio(dvdd_gpio, 0644);
module_param_gpio(dovdd_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, short, 0644);
module_param(cam_bus_num, int, 0644);
module_param(sensor_name, charp, 0644);


static struct i2c_client *i2c_dev;
static struct sensor_attr sc1345_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int index;
    unsigned char reg3e08;
    unsigned char reg3e09;
    unsigned int gain;
};

struct again_lut sc1345_again_lut[] = {
    {0, 0x03, 0x20, 0},
    {1, 0x03, 0x21, 2913},
    {2, 0x03, 0x22, 5731},
    {3, 0x03, 0x23, 8477},
    {4, 0x03, 0x24, 11136},
    {5, 0x03, 0x25, 13730},
    {6, 0x03, 0x26, 16248},
    {7, 0x03, 0x27, 18707},
    {8, 0x03, 0x28, 21097},
    {9, 0x03, 0x29, 23436},
    {10, 0x03, 0x2a, 25710},
    {11, 0x03, 0x2b, 27939},
    {12, 0x03, 0x2c, 30109},
    {13, 0x03, 0x2d, 32237},
    {14, 0x03, 0x2e, 34312},
    {15, 0x03, 0x2f, 36348},
    {16, 0x03, 0x30, 38336},
    {17, 0x03, 0x31, 40288},
    {18, 0x03, 0x32, 42195},
    {19, 0x03, 0x33, 44071},
    {20, 0x03, 0x34, 45904},
    {21, 0x03, 0x35, 47707},
    {22, 0x03, 0x36, 49472},
    {23, 0x03, 0x37, 51209},
    {24, 0x03, 0x38, 52910},
    {25, 0x03, 0x39, 54586},
    {26, 0x03, 0x3a, 56228},
    {27, 0x03, 0x3b, 57847},
    {28, 0x03, 0x3c, 59433},
    {29, 0x03, 0x3d, 60999},
    {30, 0x03, 0x3e, 62534},
    {31, 0x03, 0x3f, 64049},
    {32, 0x07, 0x20, 65536},
    {33, 0x07, 0x21, 68445},
    {34, 0x07, 0x22, 71267},
    {35, 0x07, 0x23, 74008},
    {36, 0x07, 0x24, 76672},
    {37, 0x07, 0x25, 79262},
    {38, 0x07, 0x26, 81784},
    {39, 0x07, 0x27, 84240},
    {40, 0x07, 0x28, 86633},
    {41, 0x07, 0x29, 88968},
    {42, 0x07, 0x2a, 91246},
    {43, 0x07, 0x2b, 93471},
    {44, 0x07, 0x2c, 95645},
    {45, 0x07, 0x2d, 97770},
    {46, 0x07, 0x2e, 99848},
    {47, 0x07, 0x2f, 101881},
    {48, 0x07, 0x30, 103872},
    {49, 0x07, 0x31, 105821},
    {50, 0x07, 0x32, 107731},
    {51, 0x07, 0x33, 109604},
    {52, 0x07, 0x34, 111440},
    {53, 0x07, 0x35, 113240},
    {54, 0x07, 0x36, 115008},
    {55, 0x07, 0x37, 116743},
    {56, 0x07, 0x38, 118446},
    {57, 0x07, 0x39, 120120},
    {58, 0x07, 0x3a, 121764},
    {59, 0x07, 0x3b, 123380},
    {60, 0x07, 0x3c, 124969},
    {61, 0x07, 0x3d, 126532},
    {62, 0x07, 0x3e, 128070},
    {63, 0x07, 0x3f, 129583},
    {64, 0x0f, 0x20, 131072},
    {65, 0x0f, 0x21, 133981},
    {66, 0x0f, 0x22, 136803},
    {67, 0x0f, 0x23, 139544},
    {68, 0x0f, 0x24, 142208},
    {69, 0x0f, 0x25, 144798},
    {70, 0x0f, 0x26, 147320},
    {71, 0x0f, 0x27, 149776},
    {72, 0x0f, 0x28, 152169},
    {73, 0x0f, 0x29, 154504},
    {74, 0x0f, 0x2a, 156782},
    {75, 0x0f, 0x2b, 159007},
    {76, 0x0f, 0x2c, 161181},
    {77, 0x0f, 0x2d, 163306},
    {78, 0x0f, 0x2e, 165384},
    {79, 0x0f, 0x2f, 167417},
    {80, 0x0f, 0x30, 169408},
    {81, 0x0f, 0x31, 171357},
    {82, 0x0f, 0x32, 173267},
    {83, 0x0f, 0x33, 175140},
    {84, 0x0f, 0x34, 176976},
    {85, 0x0f, 0x35, 178776},
    {86, 0x0f, 0x36, 180544},
    {87, 0x0f, 0x37, 182279},
    {88, 0x0f, 0x38, 183982},
    {89, 0x0f, 0x39, 185656},
    {90, 0x0f, 0x3a, 187300},
    {91, 0x0f, 0x3b, 188916},
    {92, 0x0f, 0x3c, 190505},
    {93, 0x0f, 0x3d, 192068},
    {94, 0x0f, 0x3e, 193606},
    {95, 0x0f, 0x3f, 195119},
    {96, 0x1f, 0x20, 196608},
    {97, 0x1f, 0x21, 199517},
    {98, 0x1f, 0x22, 202339},
    {99, 0x1f, 0x23, 205080},
    {100, 0x1f, 0x24, 207744},
    {101, 0x1f, 0x25, 210334},
    {102, 0x1f, 0x26, 212856},
    {103, 0x1f, 0x27, 215312},
    {104, 0x1f, 0x28, 217705},
    {105, 0x1f, 0x29, 220040},
    {106, 0x1f, 0x2a, 222318},
    {107, 0x1f, 0x2b, 224543},
    {108, 0x1f, 0x2c, 226717},
    {109, 0x1f, 0x2d, 228842},
    {110, 0x1f, 0x2e, 230920},
    {111, 0x1f, 0x2f, 232953},
    {112, 0x1f, 0x30, 234944},
    {113, 0x1f, 0x31, 236893},
    {114, 0x1f, 0x32, 238803},
    {115, 0x1f, 0x33, 240676},
    {116, 0x1f, 0x34, 242512},
    {117, 0x1f, 0x35, 244312},
    {118, 0x1f, 0x36, 246080},
    {119, 0x1f, 0x37, 247815},
    {120, 0x1f, 0x38, 249518},
    {121, 0x1f, 0x39, 251192},
    {122, 0x1f, 0x3a, 252836},
    {123, 0x1f, 0x3b, 254452},
    {124, 0x1f, 0x3c, 256041},
    {125, 0x1f, 0x3d, 257604},
    {126, 0x1f, 0x3e, 259142},
    {127, 0x1f, 0x3f, 260655},
};


/*
 * Interface    : MIPI - 1lane
 * MCLK         : 27Mhz
 * resolution   : 1280*720
 * FrameRate    : 60fps
 * HTS          : 0x3f0
 * VTS          : 0x94c
 */
static struct regval_list sc1345_init_regs_1280_720_60fps_mipi[] = {
    {0X0103, 0X01},
    {0X0100, 0X00},
    {0X36E9, 0X80},
    {0X301F, 0X1C},
    {0X3031, 0X08}, //mipi raw8
    {0X3037, 0X00},
    {0X320C, 0X06},
    {0X320D, 0X04}, //Hts_l8
    {0X320E, 0X03},
    {0X320F, 0X0C}, //Vts_l8
    {0X3253, 0X0A},
    {0X3301, 0X06},
    {0X3306, 0X38},
    {0X330B, 0XBA},
    {0X330E, 0X18},
    {0X3320, 0X05},
    {0X3333, 0X10},
    {0X3364, 0X17},
    {0X3390, 0X08},
    {0X3391, 0X18},
    {0X3392, 0X38},
    {0X3393, 0X09},
    {0X3394, 0X0E},
    {0X3395, 0X26},
    {0X3620, 0X08},
    {0X3622, 0XC6},
    {0X3630, 0X90},
    {0X3631, 0X83},
    {0X3633, 0X33},
    {0X3637, 0X14},
    {0X3638, 0X0E},
    {0X363A, 0X0C},
    {0X363C, 0X05},
    {0X3670, 0X1E},
    {0X3674, 0X90},
    {0X3675, 0X90},
    {0X3676, 0X90},
    {0X3677, 0X83},
    {0X3678, 0X86},
    {0X3679, 0X8B},
    {0X367C, 0X18},
    {0X367D, 0X38},
    {0X367E, 0X08},
    {0X367F, 0X38},
    {0X3690, 0X33},
    {0X3691, 0X33},
    {0X3692, 0X32},
    {0X369C, 0X08},
    {0X369D, 0X38},
    {0X36A4, 0X08},
    {0X36A5, 0X18},
    {0X36A8, 0X00},
    {0X36A9, 0X04},
    {0X36AA, 0X0E},
    {0X36EA, 0X08},
    {0X36EB, 0X80},
    {0X36EC, 0X0A},
    {0X36ED, 0X14},
    {0X391D, 0X8C},
    {0X3E00, 0X00},
    {0X3E01, 0X61},
    {0X3E02, 0X00}, //manual expo
    {0X3E08, 0X03},
    {0X3E09, 0X20}, //analog gain
    {0X4509, 0X20},
    {0X36E9, 0X44},
    // {0X0100, 0X01},
    {SC1345_REG_DELAY, 3},
    {SC1345_REG_END, 0x0},
};


static struct regval_list sc1345_regs_stream_on[] = {
    {0x0100, 0x01},
    {SC1345_REG_END, 0x0},
};

static struct regval_list sc1345_regs_stream_off[] = {
    {0x0100, 0x00},
    {SC1345_REG_END, 0x0},
};

static int sc1345_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
{
    unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = 3,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0) {
        printk(KERN_ERR "sc1345: failed to write reg: %x\n", (int)reg);
    }

    return ret;
}

static int sc1345_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
    if (ret < 0) {
        printk(KERN_ERR "sc1345(%x): failed to read reg: %x\n", i2c->addr, (int)reg);
    }

    return ret;
}

static int sc1345_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != SC1345_REG_END) {
        if (vals->reg_num == SC1345_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = sc1345_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int sc1345_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != SC1345_REG_END) {
        if (vals->reg_num == SC1345_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = sc1345_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        printk("reg = 0x%04x, val = 0x%02x\n",vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int sc1345_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char l = 1;
    int ret;

    ret = sc1345_read(i2c, SC1345_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != SC1345_CHIP_ID_H) {
        printk(KERN_ERR "sc1345 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = sc1345_read(i2c, SC1345_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;
    if (l != SC1345_CHIP_ID_L) {
        printk(KERN_ERR "sc1345 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }

    printk(KERN_DEBUG "sc1345 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "sc1345_reset");
        if (ret) {
            printk(KERN_ERR "sc1345: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "sc1345_pwdn");
        if (ret) {
            printk(KERN_ERR "sc1345: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (avdd_gpio != -1) {
        ret = gpio_request(avdd_gpio, "sc1345_avdd");
        if (ret) {
            printk(KERN_ERR "sc1345: failed to request avdd pin: %s\n", gpio_to_str(avdd_gpio, gpio_str));
            goto err_avdd_gpio;
        }
    }
    if (dvdd_gpio != -1) {
        ret = gpio_request(dvdd_gpio, "sc1345_dvdd");
        if (ret) {
            printk(KERN_ERR "sc1345: failed to request dvdd pin: %s\n", gpio_to_str(dvdd_gpio, gpio_str));
            goto err_dvdd_gpio;
        }
    }
    if (dovdd_gpio != -1) {
        ret = gpio_request(dovdd_gpio, "sc1345_dovdd");
        if (ret) {
            printk(KERN_ERR "sc1345: failed to request dovdd pin: %s\n", gpio_to_str(dovdd_gpio, gpio_str));
            goto err_dovdd_gpio;
        }
    }

err_dovdd_gpio:
    if (dvdd_gpio != -1)
        gpio_free(dvdd_gpio);
err_dvdd_gpio:
    if (avdd_gpio != -1)
        gpio_free(avdd_gpio);
err_avdd_gpio:
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
    if (avdd_gpio != -1)
        gpio_free(avdd_gpio);

    if (dvdd_gpio != -1)
        gpio_free(dvdd_gpio);

    if (dovdd_gpio != -1)
        gpio_free(dovdd_gpio);

    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);

    if (reset_gpio != -1)
        gpio_free(reset_gpio);
}

static void sc1345_power_off(void)
{
    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 0);

    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    camera_disable_sensor_mclk(cam_bus_num);

    if (avdd_gpio != -1)
        gpio_direction_output(avdd_gpio, 0);

    if (dvdd_gpio != -1)
        gpio_direction_output(dvdd_gpio, 0);

    if (dovdd_gpio != -1)
        gpio_direction_output(dovdd_gpio, 0);

}

static int sc1345_power_on(void)
{
    int ret;
    int retry;

    camera_enable_sensor_mclk(cam_bus_num, 27 * 1000 * 1000);

    if ((avdd_gpio != -1) && (dvdd_gpio != -1) && (dovdd_gpio != -1)) {
        gpio_direction_output(dovdd_gpio, 1);
        gpio_direction_output(dvdd_gpio, 1);
        gpio_direction_output(avdd_gpio, 1);
        m_msleep(10);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 0);
        m_msleep(5);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(5);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
    }

    for (retry = 0; retry < 3; retry++) {
        ret = sc1345_detect(i2c_dev);
        m_msleep(50);
        if (!ret)
            break;
    }
    if (retry >= 3) {
        printk(KERN_ERR "sc1345: failed to detect\n");
        sc1345_power_off();
        return ret;
    }

    ret = sc1345_write_array(i2c_dev, sc1345_sensor_attr.sensor_info.private_init_setting);
    sc1345_read_array(i2c_dev, sc1345_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "sc1345: failed to init regs\n");
        sc1345_power_off();
        return ret;
    }

    return 0;
}

static int sc1345_stream_on(void)
{
    int ret = sc1345_write_array(i2c_dev, sc1345_regs_stream_on);
    if (ret)
        printk(KERN_ERR "sc1345: failed to stream on\n");

    return ret;
}

static void sc1345_stream_off(void)
{
    int ret = sc1345_write_array(i2c_dev, sc1345_regs_stream_off);
    if (ret)
        printk(KERN_ERR "sc1345: failed to stream off\n");
}

static int sc1345_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = sc1345_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int sc1345_s_register(struct sensor_dbg_register *reg)
{
    return sc1345_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}


static int sc1345_set_integration_time(int value)
{
    int ret = 0;

    value *= 2;
    ret  = sc1345_write(i2c_dev, 0x3e00, (unsigned char)((value & 0xf000) >> 12));
    ret += sc1345_write(i2c_dev, 0x3e01, (unsigned char)((value & 0xff0) >> 4));
    ret += sc1345_write(i2c_dev, 0x3e02, (unsigned char)((value & 0xf) << 4));

    if (ret < 0) {
        printk(KERN_ERR "sc1345: set integration time error. line=%d\n" ,__LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int sc1345_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = sc1345_again_lut;

    while (lut->gain <= sc1345_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = lut[0].index;
            return lut[0].gain;
        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut-1)->index;
            return (lut-1)->gain;
        } else {
            if ((lut->gain == sc1345_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

static int sc1345_set_analog_gain(int value)
{
    int ret = 0;
    struct again_lut *val_lut = sc1345_again_lut;

    ret  = sc1345_write(i2c_dev, 0x3e08, val_lut[value].reg3e08);
    ret += sc1345_write(i2c_dev, 0x3e09, val_lut[value].reg3e09);
    if (ret < 0) {
        printk(KERN_ERR "sc1345: set analog gain error.  line=%d\n", __LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int sc1345_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int sc1345_set_digital_gain(int value)
{
    return 0;
}

static int sc1345_set_fps(int fps)
{
    struct sensor_info *sensor_info = &sc1345_sensor_attr.sensor_info;
    unsigned int sclk = SC1345_SUPPORT_SCLK_FPS_60;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "warn: sc1345 fps(0x%x) no in range\n", fps);
        return -1;
    }

    ret = sc1345_read(i2c_dev, 0x320c, &tmp);
    hts = tmp;
    ret += sc1345_read(i2c_dev, 0x320d, &tmp);
    if (ret < 0) {
        printk(KERN_ERR "err: sc1345_read err\n");
        return ret;
    }

    hts = (hts << 8) + tmp;

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = sc1345_write(i2c_dev, 0x320e, (unsigned char)(vts >> 8) & 0x7f);
    ret += sc1345_write(i2c_dev, 0x320f, (unsigned char)(vts & 0xff));
    if (ret < 0) {
        printk(KERN_ERR "err: sc1345_write err\n");
        return ret;
    }

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}


static struct sensor_attr sc1345_sensor_attr = {
    .device_name                = SC1345_DEVICE_NAME,
    .cbus_addr                  = SC1345_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW8,
        .lanes                  = 1,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 300 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = sc1345_init_regs_1280_720_60fps_mipi,
        .width                  = SC1345_WIDTH,
        .height                 = SC1345_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR8_1X8,
        .fps                    = 60 << 16 | 1,
        .total_width            = 0x604,
        .total_height           = 0x30c,

        .min_integration_time   = 4,
        .max_integration_time   = 0x30c - 4,
        .max_again              = 259142,
        .max_dgain              = 0,
    },

    .ops  = {
        .power_on               = sc1345_power_on,
        .power_off              = sc1345_power_off,
        .stream_on              = sc1345_stream_on,
        .stream_off             = sc1345_stream_off,
        .get_register           = sc1345_g_register,
        .set_register           = sc1345_s_register,

        .set_integration_time   = sc1345_set_integration_time,
        .alloc_again            = sc1345_alloc_again,
        .set_analog_gain        = sc1345_set_analog_gain,
        .alloc_dgain            = sc1345_alloc_dgain,
        .set_digital_gain       = sc1345_set_digital_gain,
        .set_fps                = sc1345_set_fps,
    },
};

static int sensor_sc1345_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &sc1345_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sensor_sc1345_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &sc1345_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id sc1345_id_table[] = {
    { SC1345_DEVICE_NAME, 0 },
    {},
};

static struct i2c_driver sc1345_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = SC1345_DEVICE_NAME,
    },
    .probe          = sensor_sc1345_probe,
    .remove         = sensor_sc1345_remove,
    .id_table       = sc1345_id_table,
};

static struct i2c_board_info sensor_sc1345_info = {
    .type = SC1345_DEVICE_NAME,
    .addr = SC1345_DEVICE_I2C_ADDR,
};

static __init int sc1345_sensor_init(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "sc1345: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    sc1345_driver.driver.name = sensor_name;
    strcpy(sc1345_id_table[0].name, sensor_name);
    strcpy(sensor_sc1345_info.type, sensor_name);
    sc1345_sensor_attr.device_name = sensor_name;
    if (i2c_addr != -1)
        sensor_sc1345_info.addr = i2c_addr;

    int ret = i2c_add_driver(&sc1345_driver);
    if (ret) {
        printk(KERN_ERR "sc1345: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_sc1345_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "sc1345: failed to register i2c device\n");
        i2c_del_driver(&sc1345_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void sc1345_sensor_exit(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sc1345_driver);
}

module_init(sc1345_sensor_init);
module_exit(sc1345_sensor_exit);

MODULE_DESCRIPTION("x2000 sc1345 driver");
MODULE_LICENSE("GPL");