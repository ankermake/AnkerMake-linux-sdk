/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * SC4238
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <linux/regulator/consumer.h>
#include <camera/hal/camera_sensor.h>


#define CONFIG_SC4238_1920X1080

#define SC4238_DEVICE_NAME              "sc4238"
#define SC4238_DEVICE_I2C_ADDR          0x32

#define SC4238_CHIP_ID_H                0x42
#define SC4238_CHIP_ID_L                0x35

#define SC4238_REG_END                  0xff
#define SC4238_REG_DELAY                0xfe
#define SC4238_SUPPORT_SCLK             (68000000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5
#define SC4238_MAX_WIDTH                2048
#define SC4238_MAX_HEIGHT               1448
#if defined CONFIG_SC4238_1920X1080
#define SC4238_WIDTH                    2048
#define SC4238_HEIGHT                   1448
#endif
//#define SC4238_BINNING_MODE

static int power_gpio       = -1;
static int reset_gpio       = -1;    // GPIO_PB(20);
static int pwdn_gpio        = -1;    // GPIO_PB(19);
static int i2c_bus_num      = -1;
static short i2c_addr       = -1;
static int cam_bus_num      = -1;
static char *sensor_name    = NULL;
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
static struct sensor_attr sc4238_sensor_attr;
static struct regulator *sc4238_regulator = NULL;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

struct again_lut sc4238_again_lut[] = {
    {0x0340, 0},
    {0x0341, 1500},
    {0x0342, 2886},
    {0x0343, 4342},
    {0x0344, 5776},
    {0x0345, 7101},
    {0x0346, 8494},
    {0x0347, 9781},
    {0x0348, 11136},
    {0x0349, 12471},
    {0x034a, 13706},
    {0x034b, 15005},
    {0x034c, 16287},
    {0x034d, 17474},
    {0x034e, 18723},
    {0x034f, 19879},
    {0x0350, 21097},
    {0x0351, 22300},
    {0x0352, 23413},
    {0x0353, 24587},
    {0x0354, 25746},
    {0x0355, 26820},
    {0x0356, 27952},
    {0x0357, 29002},
    {0x0358, 30108},
    {0x0359, 31202},
    {0x035a, 32216},
    {0x035b, 33286},
    {0x035c, 34344},
    {0x035d, 35325},
    {0x035e, 36361},
    {0x035f, 37321},
    {0x0360, 38335},
    {0x0361, 39338},
    {0x0362, 40269},
    {0x0363, 41252},
    {0x0364, 42225},
    {0x0365, 43128},
    {0x0366, 44082},
    {0x0367, 44967},
    {0x0368, 45903},
    {0x0369, 46829},
    {0x036a, 47689},
    {0x036b, 48599},
    {0x036c, 49499},
    {0x036d, 50336},
    {0x036e, 51220},
    {0x036f, 52041},
    {0x0370, 52910},
    {0x0371, 53770},
    {0x0372, 54570},
    {0x0373, 55415},
    {0x0374, 56253},
    {0x0375, 57032},
    {0x0376, 57856},
    {0x0377, 58622},
    {0x0378, 59433},
    {0x0379, 60236},
    {0x037a, 60983},
    {0x037b, 61773},
    {0x037c, 62557},
    {0x037d, 63286},
    {0x037e, 64058},
    {0x037f, 64775},
    {0x0740, 65535},
    {0x0741, 66989},
    {0x0742, 68467},
    {0x0743, 69877},
    {0x0744, 71266},
    {0x0745, 72636},
    {0x0746, 74029},
    {0x0747, 75359},
    {0x0748, 76671},
    {0x0749, 77964},
    {0x074a, 79281},
    {0x074b, 80540},
    {0x074c, 81782},
    {0x074d, 83009},
    {0x074e, 84258},
    {0x074f, 85452},
    {0x0750, 86632},
    {0x0751, 87797},
    {0x0752, 88985},
    {0x0753, 90122},
    {0x0754, 91245},
    {0x0755, 92355},
    {0x0756, 93487},
    {0x0757, 94571},
    {0x0758, 95643},
    {0x0759, 96703},
    {0x075a, 97785},
    {0x075b, 98821},
    {0x075c, 99846},
    {0x075d, 100860},
    {0x075e, 101896},
    {0x075f, 102888},
    {0x0760, 103870},
    {0x0761, 104842},
    {0x0762, 105835},
    {0x0763, 106787},
    {0x0764, 107730},
    {0x0765, 108663},
    {0x0766, 109617},
    {0x0767, 110532},
    {0x0768, 111438},
    {0x0769, 112335},
    {0x076a, 113253},
    {0x076b, 114134},
    {0x076c, 115006},
    {0x076d, 115871},
    {0x076e, 116755},
    {0x076f, 117385},
    {0x0770, 118445},
    {0x0771, 119278},
    {0x0772, 120131},
    {0x0773, 120950},
    {0x0774, 121762},
    {0x0775, 122567},
    {0x0776, 123391},
    {0x0777, 124183},
    {0x0778, 124968},
    {0x0779, 125746},
    {0x077a, 126543},
    {0x077b, 127308},
    {0x077c, 128068},
    {0x077d, 128821},
    {0x077e, 129593},
    {0x077f, 130334},
    {0x0f40, 131070},
    {0x0f41, 132547},
    {0x0f42, 133979},
    {0x0f43, 135412},
    {0x0f44, 136801},
    {0x0f45, 138193},
    {0x0f46, 139542},
    {0x0f47, 140894},
    {0x0f48, 142206},
    {0x0f49, 143520},
    {0x0f4a, 144796},
    {0x0f4b, 146075},
    {0x0f4c, 147317},
    {0x0f4d, 148563},
    {0x0f4e, 149773},
    {0x0f4f, 150987},
    {0x0f50, 152167},
    {0x0f51, 153351},
    {0x0f52, 154502},
    {0x0f53, 155657},
    {0x0f54, 156780},
    {0x0f55, 157908},
    {0x0f56, 159005},
    {0x0f57, 160106},
    {0x0f58, 161178},
    {0x0f59, 162255},
    {0x0f5a, 163303},
    {0x0f5b, 164356},
    {0x0f5c, 165381},
    {0x0f5d, 166411},
    {0x0f5e, 167414},
    {0x0f5f, 168423},
    {0x0f60, 169405},
    {0x0f61, 170393},
    {0x0f62, 171355},
    {0x0f63, 172322},
    {0x0f64, 173265},
    {0x0f65, 174213},
    {0x0f66, 175137},
    {0x0f67, 176067},
    {0x0f68, 176973},
    {0x0f69, 177885},
    {0x0f6a, 178774},
    {0x0f6b, 179669},
    {0x0f6c, 180541},
    {0x0f6d, 181419},
    {0x0f6e, 182276},
    {0x0f6f, 183138},
    {0x0f70, 183980},
    {0x0f71, 184827},
    {0x0f72, 185653},
    {0x0f73, 186485},
    {0x0f74, 187297},
    {0x0f75, 188115},
    {0x0f76, 188914},
    {0x0f77, 189718},
    {0x0f78, 190503},
    {0x0f79, 191293},
    {0x0f7a, 192065},
    {0x0f7b, 192843},
    {0x0f7c, 193603},
    {0x0f7d, 194368},
    {0x0f7e, 195116},
    {0x0f7f, 195869},
    {0x1f40, 196605},
    {0x1f41, 198070},
    {0x1f42, 199514},
    {0x1f43, 200936},
    {0x1f44, 202336},
    {0x1f45, 203717},
    {0x1f46, 205077},
    {0x1f47, 206418},
    {0x1f48, 207741},
    {0x1f49, 209045},
    {0x1f4a, 210331},
    {0x1f4b, 211600},
    {0x1f4c, 212852},
    {0x1f4d, 214088},
    {0x1f4e, 215308},
    {0x1f4f, 216513},
    {0x1f50, 217702},
    {0x1f51, 218877},
    {0x1f52, 220037},
    {0x1f53, 221183},
    {0x1f54, 222315},
    {0x1f55, 223434},
    {0x1f56, 224540},
    {0x1f57, 225633},
    {0x1f58, 226713},
    {0x1f59, 227782},
    {0x1f5a, 228838},
    {0x1f5b, 229883},
    {0x1f5c, 230916},
    {0x1f5d, 231938},
    {0x1f5e, 232949},
    {0x1f5f, 233950},
    {0x1f60, 234940},
    {0x1f61, 235920},
    {0x1f62, 236890},
    {0x1f63, 237849},
    {0x1f64, 238800},
    {0x1f65, 239740},
    {0x1f66, 240672},
    {0x1f67, 241594},
    {0x1f68, 242508},
    {0x1f69, 243413},
    {0x1f6a, 244309},
    {0x1f6b, 245197},
    {0x1f6c, 246076},
    {0x1f6d, 246947},
    {0x1f6e, 247811},
    {0x1f6f, 248667},
    {0x1f70, 249515},
    {0x1f71, 250355},
    {0x1f72, 251188},
    {0x1f73, 252014},
    {0x1f74, 252832},
    {0x1f75, 253644},
    {0x1f76, 254449},
    {0x1f77, 255246},
    {0x1f78, 256038},
    {0x1f79, 256822},
    {0x1f7a, 257600},
    {0x1f7b, 258372},
    {0x1f7c, 259138},
    {0x1f7d, 259897},
    {0x1f7e, 260651},
    {0x1f7f, 261398},

};

/*
 * SensorName RAW_60fps
 * width=2048
 * height=1448
 * SlaveID=0x32
 * mclk=24
 * avdd=2.800000
 * dovdd=1.800000
 * dvdd=1.500000
 */
static struct regval_list sc4238_init_regs_2048_1448_60fps_mipi[] = {
    {0x0103,0x01},
    {0x0100,0x00},
    {0x36e9,0x80},
    {0x36f9,0x80},
    {0x3018,0x32},
    {0x301f,0xa5},
    {0x3031,0x0c},
    {0x3037,0x40},
    {0x3038,0x22},
    {0x3106,0x81},
    {0x3200,0x01},
    {0x3201,0x40},
    {0x3202,0x00},
    {0x3203,0x24},
    {0x3204,0x09},
    {0x3205,0x47},
    {0x3206,0x05},
    {0x3207,0xd3},
    {0x3208,0x08},
    {0x3209,0x00},
    {0x320a,0x05},
    {0x320b,0xa8},
    {0x320c,0x05},
    {0x320d,0xa0},
    {0x320e,0x06},
    {0x320f,0x1a},
    {0x3210,0x00},
    {0x3211,0x04},
    {0x3212,0x00},
    {0x3213,0x04},
    {0x3251,0x88},
    {0x3253,0x0a},
    {0x325f,0x0c},
    {0x3273,0x01},
    {0x3301,0x30},
    {0x3304,0x30},
    {0x3306,0x70},
    {0x3308,0x10},
    {0x3309,0x50},
    {0x330b,0xf0},
    {0x330e,0x14},
    {0x3314,0x94},
    {0x331e,0x29},
    {0x331f,0x49},
    {0x3320,0x09},
    {0x334c,0x10},
    {0x3352,0x02},
    {0x3356,0x1f},
    {0x335e,0x02},
    {0x335f,0x04},
    {0x3363,0x00},
    {0x3364,0x1e},
    {0x3366,0x92},
    {0x336d,0x03},
    {0x337a,0x08},
    {0x337b,0x10},
    {0x337c,0x06},
    {0x337d,0x0a},
    {0x337f,0x2d},
    {0x3390,0x08},
    {0x3391,0x18},
    {0x3392,0x38},
    {0x3393,0x30},
    {0x3394,0x30},
    {0x3395,0x30},
    {0x3399,0xff},
    {0x33a2,0x08},
    {0x33a3,0x0c},
    {0x33e0,0xa0},
    {0x33e1,0x08},
    {0x33e2,0x00},
    {0x33e3,0x10},
    {0x33e4,0x10},
    {0x33e5,0x00},
    {0x33e6,0x10},
    {0x33e7,0x10},
    {0x33e8,0x00},
    {0x33e9,0x10},
    {0x33ea,0x16},
    {0x33eb,0x00},
    {0x33ec,0x10},
    {0x33ed,0x18},
    {0x33ee,0xa0},
    {0x33ef,0x08},
    {0x33f4,0x00},
    {0x33f5,0x10},
    {0x33f6,0x10},
    {0x33f7,0x00},
    {0x33f8,0x10},
    {0x33f9,0x10},
    {0x33fa,0x00},
    {0x33fb,0x10},
    {0x33fc,0x16},
    {0x33fd,0x00},
    {0x33fe,0x10},
    {0x33ff,0x18},
    {0x360f,0x05},
    {0x3622,0xee},
    {0x3625,0x0a},
    {0x3630,0xa8},
    {0x3631,0x80},
    {0x3633,0x44},
    {0x3634,0x34},
    {0x3635,0x60},
    {0x3636,0x20},
    {0x3637,0x11},
    {0x3638,0x2a},
    {0x363a,0x1f},
    {0x363b,0x03},
    {0x366e,0x04},
    {0x3670,0x4a},
    {0x3671,0xee},
    {0x3672,0x0e},
    {0x3673,0x0e},
    {0x3674,0x70},
    {0x3675,0x40},
    {0x3676,0x45},
    {0x367a,0x08},
    {0x367b,0x38},
    {0x367c,0x08},
    {0x367d,0x38},
    {0x3690,0x43},
    {0x3691,0x63},
    {0x3692,0x63},
    {0x3699,0x80},
    {0x369a,0x9f},
    {0x369b,0x9f},
    {0x369c,0x08},
    {0x369d,0x38},
    {0x36a2,0x08},
    {0x36a3,0x38},
    {0x36ea,0x3b},
    {0x36eb,0x07},
    {0x36ec,0x0e},
    {0x36ed,0x14},
    {0x36fa,0x36},
    {0x36fb,0x09},
    {0x36fc,0x00},
    {0x36fd,0x24},
    {0x3902,0xc5},
    {0x3905,0xd8},
    {0x3908,0x11},
    {0x391b,0x80},
    {0x391c,0x0f},
    {0x3933,0x28},
    {0x3934,0x20},
    {0x3940,0x6c},
    {0x3942,0x08},
    {0x3943,0x28},
    {0x3980,0x00},
    {0x3981,0x00},
    {0x3982,0x00},
    {0x3983,0x00},
    {0x3984,0x00},
    {0x3985,0x00},
    {0x3986,0x00},
    {0x3987,0x00},
    {0x3988,0x00},
    {0x3989,0x00},
    {0x398a,0x00},
    {0x398b,0x04},
    {0x398c,0x00},
    {0x398d,0x04},
    {0x398e,0x00},
    {0x398f,0x08},
    {0x3990,0x00},
    {0x3991,0x10},
    {0x3992,0x03},
    {0x3993,0xd8},
    {0x3994,0x03},
    {0x3995,0xe0},
    {0x3996,0x03},
    {0x3997,0xf0},
    {0x3998,0x03},
    {0x3999,0xf8},
    {0x399a,0x00},
    {0x399b,0x00},
    {0x399c,0x00},
    {0x399d,0x08},
    {0x399e,0x00},
    {0x399f,0x10},
    {0x39a0,0x00},
    {0x39a1,0x18},
    {0x39a2,0x00},
    {0x39a3,0x28},
    {0x39af,0x58},
    {0x39b5,0x30},
    {0x39b6,0x00},
    {0x39b7,0x34},
    {0x39b8,0x00},
    {0x39b9,0x00},
    {0x39ba,0x34},
    {0x39bb,0x00},
    {0x39bc,0x00},
    {0x39bd,0x00},
    {0x39be,0x00},
    {0x39bf,0x00},
    {0x39c0,0x00},
    {0x39c1,0x00},
    {0x39c5,0x21},
    {0x39c8,0x00},
    {0x39db,0x20},
    {0x39dc,0x00},
    {0x39de,0x20},
    {0x39df,0x00},
    {0x39e0,0x00},
    {0x39e1,0x00},
    {0x39e2,0x00},
    {0x39e3,0x00},
    {0x3e00,0x00},
    {0x3e01,0xc2},
    {0x3e02,0xa0},
    {0x3e03,0x0b},
    {0x3e06,0x00},
    {0x3e07,0x80},
    {0x3e08,0x03},
    {0x3e09,0x40},
    {0x3e14,0xb1},
    {0x3e25,0x03},
    {0x3e26,0x40},
    {0x4501,0xb4},
    {0x4509,0x20},
    {0x4800,0x64},
    {0x4837,0x1e},
    {0x5784,0x10},
    {0x5785,0x08},
    {0x5787,0x06},
    {0x5788,0x06},
    {0x5789,0x00},
    {0x578a,0x06},
    {0x578b,0x06},
    {0x578c,0x00},
    {0x5790,0x10},
    {0x5791,0x10},
    {0x5792,0x00},
    {0x5793,0x10},
    {0x5794,0x10},
    {0x5795,0x00},
    {0x57c4,0x10},
    {0x57c5,0x08},
    {0x57c7,0x06},
    {0x57c8,0x06},
    {0x57c9,0x00},
    {0x57ca,0x06},
    {0x57cb,0x06},
    {0x57cc,0x00},
    {0x57d0,0x10},
    {0x57d1,0x10},
    {0x57d2,0x00},
    {0x57d3,0x10},
    {0x57d4,0x10},
    {0x57d5,0x00},
    {0x5988,0x86},
    {0x598e,0x05},
    {0x598f,0x74},
    {0x36e9,0x25},
    {0x36f9,0x54},
    {0x0100,0x01},

    {SC4238_REG_DELAY,10},
    {SC4238_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list sc4238_regs_stream_on[] = {

    {0x0100, 0x01},             /* RESET_REGISTER */
    {SC4238_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list sc4238_regs_stream_off[] = {

    {0x0100, 0x00},             /* RESET_REGISTER */
    {SC4238_REG_END, 0x00},     /* END MARKER */
};

static int sc4238_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
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
        printk(KERN_ERR "sc4238: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int sc4238_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
        printk(KERN_ERR "sc4238(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int sc4238_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != SC4238_REG_END) {
        if (vals->reg_num == SC4238_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = sc4238_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int sc4238_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != SC4238_REG_END) {
        if (vals->reg_num == SC4238_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = sc4238_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        printk("reg = 0x%02x, val = 0x%02x\n",vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int sc4238_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = sc4238_read(i2c, 0x3107, &h);
    if (ret < 0)
        return ret;
    if (h != SC4238_CHIP_ID_H) {
        printk(KERN_ERR "sc4238 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = sc4238_read(i2c, 0x3108, &l);
    if (ret < 0)
        return ret;

    if (l != SC4238_CHIP_ID_L) {
        printk(KERN_ERR "sc4238 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_ERR "sc4238 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "sc4238_reset");
        if (ret) {
            printk(KERN_ERR "sc4238: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "sc4238_pwdn");
        if (ret) {
            printk(KERN_ERR "sc4238: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "sc4238_power");
        if (ret) {
            printk(KERN_ERR "sc4238: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        sc4238_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(sc4238_regulator)) {
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

    if (sc4238_regulator)
        regulator_put(sc4238_regulator);
}

static void sc4238_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (sc4238_regulator)
        regulator_disable(sc4238_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int sc4238_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 27 * 1000 * 1000);

    if (sc4238_regulator)
        regulator_enable(sc4238_regulator);

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

    ret = sc4238_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "sc4238: failed to detect\n");
       sc4238_power_off();
        return ret;
    }

    ret = sc4238_write_array(i2c_dev, sc4238_sensor_attr.sensor_info.private_init_setting);
    // ret += sc4238_read_array(i2c_dev, sc4238_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "sc4238: failed to init regs\n");
        sc4238_power_off();
        return ret;
    }

    return 0;
}

static int sc4238_stream_on(void)
{
    int ret;

    ret = sc4238_write_array(i2c_dev, sc4238_regs_stream_on);
    if (ret)
        printk(KERN_ERR "sc4238: failed to stream on\n");

    return ret;
}

static void sc4238_stream_off(void)
{
    int ret = sc4238_write_array(i2c_dev, sc4238_regs_stream_off);
    if (ret)
        printk(KERN_ERR "sc4238: failed to stream on\n");
}

static int sc4238_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = sc4238_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int sc4238_s_register(struct sensor_dbg_register *reg)
{
    return sc4238_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static int sc4238_set_integration_time(int value)
{
    int ret = 0;
    unsigned int expo = 0;

    expo = value*2;
    ret += sc4238_write(i2c_dev, 0x3e00, (unsigned char)((expo >> 12) & 0x0f));
    ret += sc4238_write(i2c_dev, 0x3e01, (unsigned char)((expo >> 4) & 0xff));
    ret += sc4238_write(i2c_dev, 0x3e02, (unsigned char)((expo & 0x0f) << 4));
    if (value < 0x50) {
        ret += sc4238_write(i2c_dev, 0x3812, 0x00);
        ret += sc4238_write(i2c_dev, 0x3314, 0x14);
        ret += sc4238_write(i2c_dev, 0x3812, 0x30);
    }
    else if(value > 0xa0){
        ret += sc4238_write(i2c_dev, 0x3812, 0x00);
        ret += sc4238_write(i2c_dev, 0x3314, 0x04);
        ret += sc4238_write(i2c_dev, 0x3812, 0x30);
    }
    if (ret < 0)
        return ret;

    return 0;

}

unsigned int sc4238_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

    struct again_lut *lut = sc4238_again_lut;
    while(lut->gain <= sc4238_sensor_attr.sensor_info.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == sc4238_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static int sc4238_set_analog_gain(int value)
{

    int ret = 0;
    ret = sc4238_write(i2c_dev, 0x3e09, (unsigned char)(value & 0xff));
    ret += sc4238_write(i2c_dev, 0x3e08, (unsigned char)(value >> 8 & 0xff));
    if (ret < 0)
        return ret;
    return 0;

}

unsigned int sc4238_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int sc4238_set_digital_gain(int value)
{
    return 0;
}

static int sc4238_set_fps(int fps)
{
    struct sensor_info *sensor_info = &sc4238_sensor_attr.sensor_info;
    unsigned int sclk = 0;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp = 0;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "sc4238 fps(%d) no in range\n", fps);
        return -1;
    }
    sclk = SC4238_SUPPORT_SCLK;

    ret = sc4238_read(i2c_dev, 0x320c, &tmp);
    hts = tmp;
    ret += sc4238_read(i2c_dev, 0x320d, &tmp);
    if(ret < 0)
        return -1;
    hts = ((hts << 8) + tmp) * 2;

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = sc4238_write(i2c_dev, 0x320f, (unsigned char)(vts & 0xff));
    ret += sc4238_write(i2c_dev, 0x320e, (unsigned char)(vts >> 8));
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}

static struct sensor_attr sc4238_sensor_attr = {
    .device_name                = SC4238_DEVICE_NAME,
    .cbus_addr                  = SC4238_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW12,
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = sc4238_init_regs_2048_1448_60fps_mipi,
        .width                  = SC4238_WIDTH,
        .height                 = SC4238_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR12_1X12,
        .fps                    = 30 << 16 | 1,
        .total_width            = 0x05a0 * 2,
        .total_height           = 0x061a,

        .min_integration_time   = 3,
        .max_integration_time   = 0x061a - 10,
        .one_line_expr_in_us    = 15,
        .max_again              = 261398,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = sc4238_power_on,
        .power_off              = sc4238_power_off,
        .stream_on              = sc4238_stream_on,
        .stream_off             = sc4238_stream_off,
        .get_register           = sc4238_g_register,
        .set_register           = sc4238_s_register,

        .set_integration_time   = sc4238_set_integration_time,
        .alloc_again            = sc4238_alloc_again,
        .set_analog_gain        = sc4238_set_analog_gain,
        .alloc_dgain            = sc4238_alloc_dgain,
        .set_digital_gain       = sc4238_set_digital_gain,
        .set_fps                = sc4238_set_fps,
    },
};

static int sc4238_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &sc4238_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sc4238_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &sc4238_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id sc4238_id[] = {
    { SC4238_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sc4238_id);

static struct i2c_driver sc4238_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = SC4238_DEVICE_NAME,
    },
    .probe              = sc4238_probe,
    .remove             = sc4238_remove,
    .id_table           = sc4238_id,
};

static struct i2c_board_info sensor_sc4238_info = {
    .type               = SC4238_DEVICE_NAME,
    .addr               = SC4238_DEVICE_I2C_ADDR,
};

static __init int init_sc4238(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "sc4238: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    sc4238_driver.driver.name = sensor_name;
    strcpy(sc4238_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_sc4238_info.addr = i2c_addr;
    strcpy(sensor_sc4238_info.type, sensor_name);
    sc4238_sensor_attr.device_name = sensor_name;

    int ret = i2c_add_driver(&sc4238_driver);
    if (ret) {
        printk(KERN_ERR "sc4238: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_sc4238_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "sc4238: failed to register i2c device\n");
        i2c_del_driver(&sc4238_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_sc4238(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sc4238_driver);
}

module_init(init_sc4238);
module_exit(exit_sc4238);

MODULE_DESCRIPTION("x2000 sc4238 driver");
MODULE_LICENSE("GPL");
