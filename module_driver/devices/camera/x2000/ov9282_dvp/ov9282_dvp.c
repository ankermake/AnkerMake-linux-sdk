/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * OV9282 DVP
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>
#include <camera/hal/dvp_gpio_func.h>


#define OV9282_DEVICE_NAME              "ov9282"
#define OV9282_DEVICE_I2C_ADDR          0x60

#define OV9282_SCLK                     (40000000)
#define SENSOR_OUTPUT_MIN_FPS           5
#define SENSOR_OUTPUT_MAX_FPS           60
#define OV9282_WIDTH                    1280
#define OV9282_HEIGHT                   800

#define OV9282_CHIP_ID_H                0x92
#define OV9282_CHIP_ID_L                0x81
#define OV9282_REG_END                  0xffff
#define OV9282_REG_DELAY                0x0000

static int power_gpio       = -1;
static int reset_gpio       = -1;
static int pwdn_gpio        = -1;
static int i2c_bus_num      = -1;
static short i2c_addr       = -1;
static int camera_index      = 0;
static char *sensor_name    = OV9282_DEVICE_NAME;

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, short, 0644);
module_param(camera_index, int, 0644);
module_param(sensor_name, charp, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr ov9282_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

static struct again_lut ov9282_again_lut[] = {
    {0x10, 0},
    {0x11, 5731},
    {0x12, 11136},
    {0x13, 16247},
    {0x14, 21097},
    {0x15, 25710},
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
    {0x20, 65535},
    {0x21, 68444},
    {0x22, 71266},
    {0x23, 74007},
    {0x24, 76671},
    {0x25, 79261},
    {0x26, 81782},
    {0x27, 84238},
    {0x28, 86632},
    {0x29, 88967},
    {0x2a, 91245},
    {0x2b, 93470},
    {0x2c, 95643},
    {0x2d, 97768},
    {0x2e, 99846},
    {0x2f, 101879},
    {0x30, 103870},
    {0x31, 105820},
    {0x32, 107730},
    {0x33, 109602},
    {0x34, 111438},
    {0x35, 113239},
    {0x36, 115006},
    {0x37, 116741},
    {0x38, 118445},
    {0x39, 120118},
    {0x3a, 121762},
    {0x3b, 123379},
    {0x3c, 124968},
    {0x3d, 126530},
    {0x3e, 128068},
    {0x3f, 129581},
    {0x40, 131070},
    {0x41, 132535},
    {0x42, 133979},
    {0x43, 135401},
    {0x44, 136801},
    {0x45, 138182},
    {0x46, 139542},
    {0x47, 140883},
    {0x48, 142206},
    {0x49, 143510},
    {0x4a, 144796},
    {0x4b, 146065},
    {0x4c, 147317},
    {0x4d, 148553},
    {0x4e, 149773},
    {0x4f, 150978},
    {0x50, 152167},
    {0x51, 153342},
    {0x52, 154502},
    {0x53, 155648},
    {0x54, 156780},
    {0x55, 157899},
    {0x56, 159005},
    {0x57, 160098},
    {0x58, 161178},
    {0x59, 162247},
    {0x5a, 163303},
    {0x5b, 164348},
    {0x5c, 165381},
    {0x5d, 166403},
    {0x5e, 167414},
    {0x5f, 168415},
    {0x60, 169405},
    {0x61, 170385},
    {0x62, 171355},
    {0x63, 172314},
    {0x64, 173265},
    {0x65, 174205},
    {0x66, 175137},
    {0x67, 176059},
    {0x68, 176973},
    {0x69, 177878},
    {0x6a, 178774},
    {0x6b, 179662},
    {0x6c, 180541},
    {0x6d, 181412},
    {0x6e, 182276},
    {0x6f, 183132},
    {0x70, 183980},
    {0x71, 184820},
    {0x72, 185653},
    {0x73, 186479},
    {0x74, 187297},
    {0x75, 188109},
    {0x76, 188914},
    {0x77, 189711},
    {0x78, 190503},
    {0x79, 191287},
    {0x7a, 192065},
    {0x7b, 192837},
    {0x7c, 193603},
    {0x7d, 194362},
    {0x7e, 195116},
    {0x7f, 195863},
    {0x80, 196605},
    {0x81, 197340},
    {0x82, 198070},
    {0x83, 198795},
    {0x84, 199514},
    {0x85, 200227},
    {0x86, 200936},
    {0x87, 201639},
    {0x88, 202336},
    {0x89, 203029},
    {0x8a, 203717},
    {0x8b, 204399},
    {0x8c, 205077},
    {0x8d, 205750},
    {0x8e, 206418},
    {0x8f, 207082},
    {0x90, 207741},
    {0x91, 208395},
    {0x92, 209045},
    {0x93, 209690},
    {0x94, 210331},
    {0x95, 210968},
    {0x96, 211600},
    {0x97, 212228},
    {0x98, 212852},
    {0x99, 213472},
    {0x9a, 214088},
    {0x9b, 214700},
    {0x9c, 215308},
    {0x9d, 215912},
    {0x9e, 216513},
    {0x9f, 217109},
    {0xa0, 217702},
    {0xa1, 218291},
    {0xa2, 218877},
    {0xa3, 219458},
    {0xa4, 220037},
    {0xa5, 220611},
    {0xa6, 221183},
    {0xa7, 221751},
    {0xa8, 222315},
    {0xa9, 222876},
    {0xaa, 223434},
    {0xab, 223988},
    {0xac, 224540},
    {0xad, 225088},
    {0xae, 225633},
    {0xaf, 226175},
    {0xb0, 226713},
    {0xb1, 227249},
    {0xb2, 227782},
    {0xb3, 228311},
    {0xb4, 228838},
    {0xb5, 229362},
    {0xb6, 229883},
    {0xb7, 230401},
    {0xb8, 230916},
    {0xb9, 231429},
    {0xba, 231938},
    {0xbb, 232445},
    {0xbc, 232949},
    {0xbd, 233451},
    {0xbe, 233950},
    {0xbf, 234446},
    {0xc0, 234940},
    {0xc1, 235431},
    {0xc2, 235920},
    {0xc3, 236406},
    {0xc4, 236890},
    {0xc5, 237371},
    {0xc6, 237849},
    {0xc7, 238326},
    {0xc8, 238800},
    {0xc9, 239271},
    {0xca, 239740},
    {0xcb, 240207},
    {0xcc, 240672},
    {0xcd, 241134},
    {0xce, 241594},
    {0xcf, 242052},
    {0xd0, 242508},
    {0xd1, 242961},
    {0xd2, 243413},
    {0xd3, 243862},
    {0xd4, 244309},
    {0xd5, 244754},
    {0xd6, 245197},
    {0xd7, 245637},
    {0xd8, 246076},
    {0xd9, 246513},
    {0xda, 246947},
    {0xdb, 247380},
    {0xdc, 247811},
    {0xdd, 248240},
    {0xde, 248667},
    {0xdf, 249091},
    {0xe0, 249515},
    {0xe1, 249936},
    {0xe2, 250355},
    {0xe3, 250772},
    {0xe4, 251188},
    {0xe5, 251602},
    {0xe6, 252014},
    {0xe7, 252424},
    {0xe8, 252832},
    {0xe9, 253239},
    {0xea, 253644},
    {0xeb, 254047},
    {0xec, 254449},
    {0xed, 254848},
    {0xee, 255246},
    {0xef, 255643},
    {0xf0, 256038},
    {0xf1, 256431},
    {0xf2, 256822},
    {0xf3, 257212},
    {0xf4, 257600},
    {0xf5, 257987},
    {0xf6, 258372},
    {0xf7, 258756},
    {0xf8, 259138},
};

/*
 * SensorName RAW_60fps
 * width=1280
 * height=800
 * SlaveID=0x60
 * mclk=24
 * avdd=2.800000
 * dovdd=1.800000
 * dvdd=1.500000
 */
static struct regval_list ov9282_init_regs_1280_800_60fps_dvp[] = {
    {0x0103, 0x01},
    {0x0302, 0x23}, //30
    {0x030d, 0x46}, //60
    {0x030e, 0x06},
    {0x3001, 0x62},
    {0x3004, 0x01},
    {0x3005, 0xff},
    {0x3006, 0xee},
    {0x3011, 0x0a},
    {0x3013, 0x18},
    {0x3022, 0x07},
    {0x3030, 0x10},
    {0x3039, 0x2e},
    {0x303a, 0xf0},
    {0x3500, 0x00},
    {0x3501, 0x2a}, //0x2a
    {0x3502, 0x90},
    {0x3503, 0x08},
    {0x3505, 0x8c},
    {0x3507, 0x03},
    {0x3508, 0x00},
    {0x3509, 0x10},
    {0x3610, 0x80},
    {0x3611, 0xa0},
    {0x3620, 0x6f},
    {0x3632, 0x56},
    {0x3633, 0x78},
    {0x3662, 0x05},
    {0x3666, 0x5a},
    {0x366f, 0x7e},
    {0x3680, 0x84},
    {0x3712, 0x80},
    {0x372d, 0x22},
    {0x3731, 0x80},
    {0x3732, 0x30},
    {0x3778, 0x00},
    {0x377d, 0x22},
    {0x3788, 0x02},
    {0x3789, 0xa4},
    {0x378a, 0x00},
    {0x378b, 0x4a},
    {0x3799, 0x20},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x05},
    {0x3805, 0x0f},
    {0x3806, 0x03},
    {0x3807, 0x2f},
    {0x3808, 0x05}, //0x05
    {0x3809, 0x00}, //0x00
    {0x380a, 0x03}, //0x03
    {0x380b, 0x20}, //0x20
    {0x380c, 0x02},
    {0x380d, 0xd8},
    {0x380e, 0x03},
    {0x380f, 0x8e},
    {0x3810, 0x00},
    {0x3811, 0x08},
    {0x3812, 0x00},
    {0x3813, 0x08},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3820, 0x40},
    {0x3821, 0x00},
    {0x3881, 0x42},
    {0x38b1, 0x00},
    {0x3920, 0xff},
    {0x392f, 0xc0},
    {0x3921, 0x00},
    {0x3922, 0x00},
    {0x3923, 0x00},
    {0x3924, 0x00},
    {0x3926, 0x00},
    {0x3927, 0x00},
    {0x3928, 0x80},
    {0x4003, 0x40},
    {0x4008, 0x04},
    {0x4009, 0x0b},
    {0x400c, 0x00},
    {0x400d, 0x07},
    {0x4010, 0x40},
    {0x4043, 0x40},
    {0x4307, 0x30},
    {0x4317, 0x01},
    {0x4501, 0x00},
    {0x4507, 0x00},
    {0x4509, 0x00},
    {0x450a, 0x08},
    {0x4601, 0x04},
    {0x470f, 0xe0},
    {0x4f07, 0x00},
    {0x4800, 0x00},
    {0x5000, 0x9f},
    {0x5001, 0x00},
    {0x5e00, 0x00},
    {0x5d00, 0x0b},
    {0x5d01, 0x02},
    {0x4708, 0x0b},
    {0x0100, 0x01},
    {OV9282_REG_END, 0x00}, /* END MARKER */
};

static struct regval_list ov9282_regs_stream_on[] = {
    {0x0100, 0x01},
    {OV9282_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list ov9282_regs_stream_off[] = {
    {0x0100, 0x00},
    {OV9282_REG_END, 0x00},     /* END MARKER */
};

static int ov9282_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
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
        printk(KERN_ERR "ov9282: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int ov9282_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
        printk(KERN_ERR "ov9282(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int ov9282_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != OV9282_REG_END) {
        if (vals->reg_num == OV9282_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = ov9282_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int ov9282_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != OV9282_REG_END) {
        if (vals->reg_num == OV9282_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = ov9282_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        printk(KERN_ERR "reg = 0x%02x, val = 0x%02x\n",vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int ov9282_detect(struct i2c_client *i2c)
{
    unsigned char h;
    unsigned char l;
    int ret;

    ret = ov9282_read(i2c, 0x300a, &h);
    if (ret < 0)
        return ret;
    if (h != OV9282_CHIP_ID_H) {
        printk(KERN_ERR "ov9282 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = ov9282_read(i2c, 0x300b, &l);
    if (ret < 0)
        return ret;
    if (l != OV9282_CHIP_ID_L) {
        printk(KERN_ERR "ov9282 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }

    printk(KERN_ERR "ov9282 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "ov9282_reset");
        if (ret) {
            printk(KERN_ERR "ov9282: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "ov9282_pwdn");
        if (ret) {
            printk(KERN_ERR "ov9282: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ov9282_power");
        if (ret) {
            printk(KERN_ERR "ov9282: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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

static void ov9282_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk(camera_index);
}

static int ov9282_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(camera_index, 24 * 1000 * 1000);

    if (power_gpio != -1) {
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
        m_msleep(5);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
    }

    ret = ov9282_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "ov9282: failed to detect\n");
       ov9282_power_off();
        return ret;
    }

    ret = ov9282_write_array(i2c_dev, ov9282_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "ov9282: failed to init regs\n");
        ov9282_power_off();
        return ret;
    }

    return 0;
}

static int ov9282_stream_on(void)
{
    int ret = ov9282_write_array(i2c_dev, ov9282_regs_stream_on);
    if (ret)
        printk(KERN_ERR "ov9282: failed to stream on\n");

    return ret;
}

static void ov9282_stream_off(void)
{
    int ret = ov9282_write_array(i2c_dev, ov9282_regs_stream_off);
    if (ret)
        printk(KERN_ERR "ov9282: failed to stream on\n");
}

static int ov9282_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = ov9282_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int ov9282_s_register(struct sensor_dbg_register *reg)
{
    return ov9282_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static int ov9282_set_integration_time(int value)
{
    int ret = 0;
    unsigned int expo = value << 4;

    ret = ov9282_write(i2c_dev, 0x3500, (unsigned char)((expo >> 16) & 0xff));
    ret += ov9282_write(i2c_dev, 0x3501, (unsigned char)((expo >> 8) & 0xff));
    ret += ov9282_write(i2c_dev, 0x3502, (unsigned char)(expo & 0xff));
    if (ret < 0)
        return ret;

    return 0;
}

static unsigned int ov9282_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = ov9282_again_lut;

    while(lut->gain <= ov9282_sensor_attr.sensor_info.max_again) {
        if(isp_gain <= 0) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == ov9282_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

/**************************************
  ov9282 sensor gain control by [0x3509]
    value from 0x10 to 0xF8 represents gain of 1x to 15.5x
**************************************/
static int ov9282_set_analog_gain(int value)
{
    int ret = 0;

    ret =ov9282_write(i2c_dev, 0x3509, (unsigned char)(value & 0xff));
    if (ret < 0) {
        printk(KERN_ERR "ov9282 set analog gain error\n");
        return ret;
    }

    return 0;
}


static unsigned int ov9282_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int ov9282_set_digital_gain(int value)
{
    return 0;
}

static int ov9282_set_fps(int fps)
{
    struct sensor_info *sensor_info = &ov9282_sensor_attr.sensor_info;
    unsigned int sclk = OV9282_SCLK;
    unsigned short vts = 0;
    unsigned short hts = 0;
    unsigned char tmp = 0;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "ov9282 fps(%x) no in range\n", fps);
        return -1;
    }

    ret = ov9282_read(i2c_dev, 0x380c, &tmp);
    hts = tmp<<8;
    ret += ov9282_read(i2c_dev, 0x380d, &tmp);
    hts |= tmp;
    if(ret < 0)
        return -1;

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = ov9282_write(i2c_dev, 0x380e, (unsigned char)(vts >> 8));
    ret += ov9282_write(i2c_dev, 0x380f, (unsigned char)(vts & 0xff));
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 25;

    return 0;
}


static struct sensor_attr ov9282_sensor_attr = {
    .device_name            = OV9282_DEVICE_NAME,
    .cbus_addr              = OV9282_DEVICE_I2C_ADDR,

    .dbus_type              = SENSOR_DATA_BUS_DVP,
    .dvp = {
        .data_fmt           = DVP_RAW8,
        .gpio_mode          = DVP_PA_LOW_8BIT,
        .timing_mode        = DVP_HREF_MODE,
        .yuv_data_order = 0,
        .hsync_polarity = POLARITY_HIGH_ACTIVE,
        .vsync_polarity = POLARITY_HIGH_ACTIVE,
        .img_scan_mode  = DVP_IMG_SCAN_PROGRESS,
    },

    .isp_clk_rate           = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = ov9282_init_regs_1280_800_60fps_dvp,
        .width                  = OV9282_WIDTH,
        .height                 = OV9282_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SGRBG8_1X8,

        .fps                    = 60 << 16 | 1,
        .total_width            = 0x2d8,
        .total_height           = 0x38e,

        .min_integration_time   = 3,
        .max_integration_time   = 0x38e - 25,
        .max_again              = 259138,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = ov9282_power_on,
        .power_off              = ov9282_power_off,
        .stream_on              = ov9282_stream_on,
        .stream_off             = ov9282_stream_off,
        .get_register           = ov9282_g_register,
        .set_register           = ov9282_s_register,

        .set_integration_time   = ov9282_set_integration_time,
        .alloc_again            = ov9282_alloc_again,
        .set_analog_gain        = ov9282_set_analog_gain,
        .alloc_dgain            = ov9282_alloc_dgain,
        .set_digital_gain       = ov9282_set_digital_gain,
        .set_fps                = ov9282_set_fps,
    },
};

static int ov9282_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = dvp_init_select_gpio(&ov9282_sensor_attr.dvp,ov9282_sensor_attr.dvp.gpio_mode);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    ret = camera_register_sensor(camera_index, &ov9282_sensor_attr);
    if (ret) {
        dvp_deinit_gpio();
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int ov9282_remove(struct i2c_client *client)
{
    camera_unregister_sensor(camera_index, &ov9282_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id ov9282_id[] = {
    { OV9282_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ov9282_id);

static struct i2c_driver ov9282_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = OV9282_DEVICE_NAME,
    },
    .probe              = ov9282_probe,
    .remove             = ov9282_remove,
    .id_table           = ov9282_id,
};

static struct i2c_board_info sensor_ov9282_info = {
    .type               = OV9282_DEVICE_NAME,
    .addr               = OV9282_DEVICE_I2C_ADDR,
};

static __init int init_ov9282(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "ov9282: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    ov9282_driver.driver.name = sensor_name;
    strcpy(ov9282_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_ov9282_info.addr = i2c_addr;
    strcpy(sensor_ov9282_info.type, sensor_name);
    ov9282_sensor_attr.device_name = sensor_name;

    int ret = i2c_add_driver(&ov9282_driver);
    if (ret) {
        printk(KERN_ERR "ov9282: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_ov9282_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "ov9282: failed to register i2c device\n");
        i2c_del_driver(&ov9282_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_ov9282(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&ov9282_driver);
}

module_init(init_ov9282);
module_exit(exit_ov9282);

MODULE_DESCRIPTION("x2000 ov9282 driver");
MODULE_LICENSE("GPL");
