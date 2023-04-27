#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>

#define OV7251_DEVICE_NAME              "ov7251"
#define OV7251_DEVICE_I2C_ADDR          0x70
#define OV7251_SCLK                     (80000000)
#define SENSOR_OUTPUT_MIN_FPS           5
#define SENSOR_OUTPUT_MAX_FPS           120
#define OV7251_HTS                      0x304
#define OV7251_VTS                      0x130

#define OV7251_DEVICE_WIDTH             160
#define OV7251_DEVICE_HEIGHT            120
#define OV7251_CHIP_ID_H                (0x77)
#define OV7251_CHIP_ID_L                (0x50)
#define OV7251_REG_CHIP_ID_HIGH         0x300a
#define OV7251_REG_CHIP_ID_LOW          0x300b

#define OV7251_REG_END                  0xffff
#define OV7251_REG_DELAY                0xfffe

static int reset_gpio   = -1; // PA 10
static int i2c_bus_num  = -1; // 3
static int camera_index =  0;

module_param_gpio(reset_gpio, 0644);
module_param(i2c_bus_num, int, 0644);
module_param(camera_index, int, 0644);


static char *resolution = "OV7251_FULL_640_480_120FPS";
module_param(resolution, charp, 0644);
MODULE_PARM_DESC(resolution, "ov7251_resolution");

static char *resolution_array[] = {

    "OV7251_FULL_640_480_120FPS",
    "OV7251_BINNING_320_240_200FPS",
};

static struct i2c_client *i2c_dev;

static struct sensor_attr ov7251_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

struct again_lut ov7251_again_lut[] = {

    {0x80, 0},
    {0x88, 5731},
    {0x90, 11136},
    {0x98, 16248},
    {0xa0, 21097},
    {0xa8, 25710},
    {0xb0, 30109},
    {0xb8, 34312},
    {0xc0, 38336},
    {0xc8, 42195},
    {0xd0, 45904},
    {0xd8, 49472},
    {0xe0, 52910},
    {0xe8, 56228},
    {0xf0, 59433},
    {0xf8, 62534},
    {0x178, 65536},
    {0x17c, 68445},
    {0x180, 71267},
    {0x184, 74008},
    {0x188, 76672},
    {0x18c, 79262},
    {0x190, 81784},
    {0x194, 84240},
    {0x198, 86633},
    {0x19c, 88968},
    {0x1a0, 91246},
    {0x1a4, 93471},
    {0x1a8, 95645},
    {0x1ac, 97770},
    {0x1b0, 99848},
    {0x1b4, 101881},
    {0x1b8, 103872},
    {0x1bc, 105821},
    {0x1c0, 107731},
    {0x1c4, 109604},
    {0x1c8, 111440},
    {0x1cc, 113241},
    {0x1d0, 115008},
    {0x1d4, 116743},
    {0x1d8, 118446},
    {0x1dc, 120120},
    {0x1e0, 121764},
    {0x1e4, 123380},
    {0x1e8, 124969},
    {0x1ec, 126532},
    {0x1f0, 128070},
    {0x1f4, 129583},
    {0x374, 131072},
    {0x376, 132537},
    {0x378, 133981},
    {0x37a, 135403},
    {0x37c, 136803},
    {0x37e, 138184},
    {0x380, 139544},
    {0x382, 140885},
    {0x384, 142208},
    {0x386, 143512},
    {0x388, 144798},
    {0x38a, 146067},
    {0x38c, 147320},
    {0x38e, 148556},
    {0x390, 149776},
    {0x392, 150980},
    {0x394, 152169},
    {0x396, 153344},
    {0x398, 154504},
    {0x39a, 155650},
    {0x39c, 156782},
    {0x39e, 157901},
    {0x3a0, 159007},
    {0x3a2, 160100},
    {0x3a4, 161181},
    {0x3a6, 162249},
    {0x3a8, 163306},
    {0x3aa, 164350},
    {0x3ac, 165384},
    {0x3ae, 166406},
    {0x3b0, 167417},
    {0x3b2, 168418},
    {0x3b4, 169408},
    {0x3b6, 170387},
    {0x3b8, 171357},
    {0x3ba, 172317},
    {0x3bc, 173267},
    {0x3be, 174208},
    {0x3c0, 175140},
    {0x3c2, 176062},
    {0x3c4, 176976},
    {0x3c6, 177880},
    {0x3c8, 178777},
    {0x3ca, 179664},
    {0x3cc, 180544},
    {0x3ce, 181415},
    {0x3d0, 182279},
    {0x3d2, 183134},
    {0x3d4, 183982},
    {0x3d6, 184823},
    {0x3d8, 185656},
    {0x3da, 186482},
    {0x3dc, 187300},
    {0x3de, 188112},
    {0x3e0, 188916},
    {0x3e2, 189714},
    {0x3e4, 190505},
    {0x3e6, 191290},
    {0x3e8, 192068},
    {0x3ea, 192840},
    {0x3ec, 193606},
    {0x3ee, 194365},
    {0x3f0, 195119},
    {0x3f2, 195866},
    {0x778, 196608},
    {0x779, 197343},
    {0x77a, 198073},
    {0x77b, 198798},
    {0x77c, 199517},
    {0x77d, 200230},
    {0x77e, 200939},
    {0x77f, 201642},
    {0x780, 202339},
    {0x781, 203032},
    {0x782, 203720},
    {0x783, 204402},
    {0x784, 205080},
    {0x785, 205753},
    {0x786, 206421},
    {0x787, 207085},
    {0x788, 207744},
    {0x789, 208398},
    {0x78a, 209048},
    {0x78b, 209693},
    {0x78c, 210334},
    {0x78d, 210971},
    {0x78e, 211603},
    {0x78f, 212232},
    {0x790, 212856},
    {0x791, 213476},
    {0x792, 214092},
    {0x793, 214704},
    {0x794, 215312},
    {0x795, 215916},
    {0x796, 216516},
    {0x797, 217113},
    {0x798, 217705},
    {0x799, 218294},
    {0x79a, 218880},
    {0x79b, 219462},
    {0x79c, 220040},
    {0x79d, 220615},
    {0x79e, 221186},
    {0x79f, 221754},
    {0x7a0, 222318},
    {0x7a1, 222880},
    {0x7a2, 223437},
    {0x7a3, 223992},
    {0x7a4, 224543},
    {0x7a5, 225091},
    {0x7a6, 225636},
    {0x7a7, 226178},
    {0x7a8, 226717},
    {0x7a9, 227253},
    {0x7aa, 227785},
    {0x7ab, 228315},
    {0x7ac, 228842},
    {0x7ad, 229365},
    {0x7ae, 229886},
    {0x7af, 230404},
    {0x7b0, 230920},
    {0x7b1, 231432},
    {0x7b2, 231942},
    {0x7b3, 232449},
    {0x7b4, 232953},
    {0x7b5, 233455},
    {0x7b6, 233954},
    {0x7b7, 234450},
    {0x7b8, 234944},
    {0x7b9, 235435},
    {0x7ba, 235923},
    {0x7bb, 236410},
    {0x7bc, 236893},
    {0x7bd, 237374},
    {0x7be, 237853},
    {0x7bf, 238329},
    {0x7c0, 238803},
    {0x7c1, 239275},
    {0x7c2, 239744},
    {0x7c3, 240211},
    {0x7c4, 240676},
    {0x7c5, 241138},
    {0x7c6, 241598},
    {0x7c7, 242056},
    {0x7c8, 242512},
    {0x7c9, 242965},
    {0x7ca, 243416},
    {0x7cb, 243865},
    {0x7cc, 244313},
    {0x7cd, 244757},
    {0x7ce, 245200},
    {0x7cf, 245641},
    {0x7d0, 246080},
    {0x7d1, 246517},
    {0x7d2, 246951},
    {0x7d3, 247384},
    {0x7d4, 247815},
    {0x7d5, 248243},
    {0x7d6, 248670},
    {0x7d7, 249095},
    {0x7d8, 249518},
    {0x7d9, 249939},
    {0x7da, 250359},
    {0x7db, 250776},
    {0x7dc, 251192},
    {0x7dd, 251606},
    {0x7de, 252018},
    {0x7df, 252428},
    {0x7e0, 252836},
    {0x7e1, 253243},
    {0x7e2, 253648},
    {0x7e3, 254051},
    {0x7e4, 254452},
    {0x7e5, 254852},
    {0x7e6, 255250},
    {0x7e7, 255647},
    {0x7e8, 256041},
    {0x7e9, 256435},
    {0x7ea, 256826},
    {0x7eb, 257216},
    {0x7ec, 257604},
    {0x7ed, 257991},
    {0x7ee, 258376},
    {0x7ef, 258760},
    {0x7f0, 259142},
    {0x7f1, 259522},
    {0x7f2, 259901},
    {0x7f3, 260279},
    {0x7f4, 260655},
    {0x7f5, 261029},
    {0x7f6, 261402},
    {0x7f7, 261773},
};

static struct regval_list ov7251_init_regs_full_640_480_mipi[] = {

    {0x0103, 0x01},
    {0x0100, 0x00},
    {0x3005, 0x00},
    {0x3012, 0xc0},
    {0x3013, 0xd2},
    {0x3014, 0x04},
    {0x3016, 0x10},
    {0x3017, 0x00},
    {0x3018, 0x00},
    {0x301a, 0x00},
    {0x301b, 0x00},
    {0x301c, 0x00},
    {0x3023, 0x05},
    {0x3037, 0xf0},
    {0x3098, 0x04},
    {0x3099, 0x28},
    {0x309a, 0x04},
    {0x309b, 0x04},
    {0x30b0, 0x0a},
    {0x30b1, 0x01},
    {0x30b3, 0x64},
    {0x30b4, 0x03},
    {0x30b5, 0x05},
    {0x3106, 0xda},
    {0x3500, 0x00},
    {0x3501, 0x1f},
    {0x3502, 0x80},
    {0x3503, 0x07},
    {0x3509, 0x10},
    {0x350b, 0x10},
    {0x3600, 0x1c},
    {0x3602, 0x62},
    {0x3620, 0xb7},
    {0x3622, 0x04},
    {0x3626, 0x21},
    {0x3627, 0x30},
    {0x3630, 0x44},
    {0x3631, 0x35},
    {0x3634, 0x60},
    {0x3636, 0x00},
    {0x3662, 0x01},
    {0x3663, 0x70},
    {0x3664, 0xf0},
    {0x3666, 0x0a},
    {0x3669, 0x1a},
    {0x366a, 0x00},
    {0x366b, 0x50},
    {0x3673, 0x01},
    {0x3674, 0xef},
    {0x3675, 0x03},
    {0x3705, 0xc1},
    {0x3709, 0x40},
    {0x373c, 0x08},
    {0x3742, 0x00},
    {0x3757, 0xb3},
    {0x3788, 0x00},
    {0x37a8, 0x01},
    {0x37a9, 0xc0},
    {0x3800, 0x00},
    {0x3801, 0x04},
    {0x3802, 0x00},
    {0x3803, 0x04},
    {0x3804, 0x02},
    {0x3805, 0x8b},
    {0x3806, 0x01},
    {0x3807, 0xeb},
    {0x3808, 0x02},
    {0x3809, 0x80},
    {0x380a, 0x01},
    {0x380b, 0xe0},
    {0x380c, 0x03},
    {0x380d, 0xa0},
    {0x380e, 0x02},
    {0x380f, 0x0a},
    {0x3810, 0x00},
    {0x3811, 0x04},
    {0x3812, 0x00},
    {0x3813, 0x05},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3820, 0x40},
    {0x3821, 0x00},
    {0x382f, 0x0e},
    {0x3832, 0x00},
    {0x3833, 0x05},
    {0x3834, 0x00},
    {0x3835, 0x0c},
    {0x3837, 0x00},
    {0x3b80, 0x00},
    {0x3b81, 0xa5},
    {0x3b82, 0x10},
    {0x3b83, 0x00},
    {0x3b84, 0x08},
    {0x3b85, 0x00},
    {0x3b86, 0x01},
    {0x3b87, 0x00},
    {0x3b88, 0x00},
    {0x3b89, 0x00},
    {0x3b8a, 0x00},
    {0x3b8b, 0x05},
    {0x3b8c, 0x00},
    {0x3b8d, 0x00},
    {0x3b8e, 0x00},
    {0x3b8f, 0x1a},
    {0x3b94, 0x05},
    {0x3b95, 0xf2},
    {0x3b96, 0x40},
    {0x3c00, 0x89},
    {0x3c01, 0x63},
    {0x3c02, 0x01},
    {0x3c03, 0x00},
    {0x3c04, 0x00},
    {0x3c05, 0x03},
    {0x3c06, 0x00},
    {0x3c07, 0x06},
    {0x3c0c, 0x01},
    {0x3c0d, 0xd0},
    {0x3c0e, 0x02},
    {0x3c0f, 0x0a},
    {0x4001, 0x42},
    {0x4004, 0x04},
    {0x4005, 0x00},
    {0x404e, 0x01},
    {0x4300, 0xff},
    {0x4301, 0x00},
    {0x4501, 0x48},
    {0x4600, 0x00},
    {0x4601, 0x4e},
    {0x4801, 0x0f},
    {0x4806, 0x0f},
    {0x4819, 0xaa},
    {0x4823, 0x3e},
    {0x4837, 0x19},
    {0x4a0d, 0x00},
    {0x4a47, 0x7f},
    {0x4a49, 0xf0},
    {0x4a4b, 0x30},
    {0x5000, 0x85},
    {0x5001, 0x80},
    {0x0100, 0x00},

    {OV7251_REG_DELAY,20},
    {OV7251_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list ov7251_init_regs_binning_320_240_mipi[] = {

    {0x0103, 0x01},
    {0x0100, 0x00},
    {0x3005, 0x00},
    {0x3012, 0xc0},
    {0x3013, 0xd2},
    {0x3014, 0x04},
    {0x3016, 0x10},
    {0x3017, 0x00},
    {0x3018, 0x00},
    {0x301a, 0x00},
    {0x301b, 0x00},
    {0x301c, 0x00},
    {0x3023, 0x05},
    {0x3037, 0xf0},
    {0x3098, 0x04},
    {0x3099, 0x28},
    {0x309a, 0x05},
    {0x309b, 0x04},
    {0x30b0, 0x0a},
    {0x30b1, 0x01},
    {0x30b3, 0x64},
    {0x30b4, 0x03},
    {0x30b5, 0x05},
    {0x3106, 0xda},
    {0x3500, 0x00},
    {0x3501, 0x00},
    {0x3502, 0xa0},
    {0x3503, 0x07},
    {0x3509, 0x10},
    {0x350b, 0x10},
    {0x3600, 0x1c},
    {0x3602, 0x62},
    {0x3620, 0xb7},
    {0x3622, 0x04},
    {0x3626, 0x21},
    {0x3627, 0x30},
    {0x3630, 0x44},
    {0x3631, 0x35},
    {0x3634, 0x60},
    {0x3636, 0x00},
    {0x3662, 0x01},
    {0x3663, 0x70},
    {0x3664, 0xf0},
    {0x3666, 0x0a},
    {0x3669, 0x1a},
    {0x366a, 0x00},
    {0x366b, 0x50},
    {0x3673, 0x01},
    {0x3674, 0xef},
    {0x3675, 0x03},
    {0x3705, 0x41},
    {0x3709, 0x40},
    {0x373c, 0xe8},
    {0x3742, 0x00},
    {0x3757, 0xb3},
    {0x3788, 0x00},
    {0x37a8, 0x02},
    {0x37a9, 0x14},
    {0x3800, 0x00},
    {0x3801, 0x04},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x02},
    {0x3805, 0x8b},
    {0x3806, 0x01},
    {0x3807, 0xef},
    {0x3808, 0x01},
    {0x3809, 0x40},
    {0x380a, 0x00},
    {0x380b, 0xf0},
    {0x380c, 0x03},
    {0x380d, 0x04},
    {0x380e, 0x01},
    {0x380f, 0x30},
    {0x3810, 0x00},
    {0x3811, 0x04},
    {0x3812, 0x00},
    {0x3813, 0x05},
    {0x3814, 0x31},
    {0x3815, 0x31},
    {0x3820, 0x42},
    {0x3821, 0x01},
    {0x382f, 0x0e},
    {0x3832, 0x00},
    {0x3833, 0x05},
    {0x3834, 0x00},
    {0x3835, 0x0c},
    {0x3837, 0x00},
    {0x3b80, 0x00},
    {0x3b81, 0xa5},
    {0x3b82, 0x10},
    {0x3b83, 0x00},
    {0x3b84, 0x08},
    {0x3b85, 0x00},
    {0x3b86, 0x01},
    {0x3b87, 0x00},
    {0x3b88, 0x00},
    {0x3b89, 0x00},
    {0x3b8a, 0x00},
    {0x3b8b, 0x05},
    {0x3b8c, 0x00},
    {0x3b8d, 0x00},
    {0x3b8e, 0x00},
    {0x3b8f, 0x1a},
    {0x3b94, 0x05},
    {0x3b95, 0xf2},
    {0x3b96, 0x40},
    {0x3c00, 0x89},
    {0x3c01, 0x63},
    {0x3c02, 0x01},
    {0x3c03, 0x00},
    {0x3c04, 0x00},
    {0x3c05, 0x03},
    {0x3c06, 0x00},
    {0x3c07, 0x06},
    {0x3c0c, 0x01},
    {0x3c0d, 0x82},
    {0x3c0e, 0x01},
    {0x3c0f, 0x30},
    {0x4001, 0x40},
    {0x4004, 0x02},
    {0x4005, 0x00},
    {0x404e, 0x01},
    {0x4300, 0xff},
    {0x4301, 0x00},
    {0x4501, 0x48},
    {0x4600, 0x00},
    {0x4601, 0x4e},
    {0x4801, 0x0f},
    {0x4806, 0x0f},
    {0x4819, 0xaa},
    {0x4823, 0x3e},
    {0x4837, 0x19},
    {0x4a0d, 0x00},
    {0x4a47, 0x7f},
    {0x4a49, 0xf0},
    {0x4a4b, 0x30},
    {0x5000, 0x85},
    {0x5001, 0x80},
    {0x0100, 0x00},

    {OV7251_REG_DELAY,20},
    {OV7251_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list ov7251_stream_on_mipi[] = {

    {0x0100, 0x01},
    {OV7251_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list ov7251_stream_off_mipi[] = {

    {0x0100, 0x00},
    {OV7251_REG_END, 0x00},    /* END MARKER */
};

static int ov7251_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
{
    unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
    struct i2c_msg msg = {
        .addr = i2c->addr,
        .flags  = 0,
        .len    = 3,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "ov7251: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int ov7251_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
{
    unsigned char buf[2] = {reg >> 8, reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr = i2c->addr,
            .flags  = 0,
            .len    = 2,
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
        printk(KERN_ERR "ov7251(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int ov7251_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != OV7251_REG_END) {
        if (vals->reg_num == OV7251_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = ov7251_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int ov7251_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != OV7251_REG_END) {
        if (vals->reg_num == OV7251_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = ov7251_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        printk("reg = 0x%02x, val = 0x%02x\n",vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int ov7251_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char l = 1;
    int ret;

    ret = ov7251_read(i2c, OV7251_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != OV7251_CHIP_ID_H) {
        printk(KERN_ERR "ov7251 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = ov7251_read(i2c, OV7251_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;

    if (l != OV7251_CHIP_ID_L) {
        printk(KERN_ERR"ov7251 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }

    return 0;
}

static void ov7251_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    camera_disable_sensor_mclk(camera_index);
}

static int ov7251_power_on(void)
{
    int ret;
    camera_enable_sensor_mclk(camera_index, 12 * 1000 * 1000);

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 0);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
    }

    ret = ov7251_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "ov7251: failed to detect\n");
        ov7251_power_off();
        return ret;
    }


    ret = ov7251_write_array(i2c_dev, ov7251_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "ov7251: failed to init regs\n");
        ov7251_power_off();
        return ret;
    }

    return 0;
}

static int ov7251_stream_on(void)
{
    printk("----------ov7251_stream_on--------\n");
    int ret = ov7251_write_array(i2c_dev, ov7251_stream_on_mipi);

    if (ret)
        printk(KERN_ERR "ov7251: failed to stream on\n");

    return ret;
}

static void ov7251_stream_off(void)
{
    int ret = ov7251_write_array(i2c_dev, ov7251_stream_off_mipi);

    if (ret)
        printk(KERN_ERR "ov7251: failed to stream on\n");

}

static int ov7251_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = ov7251_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 1;
    return ret;
}

static int ov7251_s_register(struct sensor_dbg_register *reg)
{
    return ov7251_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xff);
}

static int ov7251_set_integration_time(int value)
{
//     int ret = 0;
//     unsigned int expo = value;
//     unsigned char tmp;
//     unsigned short vts;
//     unsigned int max_expo;

//     ret = ov7251_read(i2c_dev, 0x380e, &tmp);
//     vts = tmp << 8;
//     ret += ov7251_read(i2c_dev, 0x380f, &tmp);
//     if (ret < 0)
//         return ret;
//     vts |= tmp;
//     max_expo = vts -20;
//     if (expo > max_expo)
//         expo = max_expo;

//     ret  = ov7251_write(i2c_dev, 0x3500, (unsigned char)((expo & 0xffff) >> 12));
//     ret += ov7251_write(i2c_dev, 0x3501, (unsigned char)((expo & 0x0fff) >> 4));
//     ret += ov7251_write(i2c_dev, 0x3502, (unsigned char)((expo & 0x000f) << 4));
//     if (ret < 0) {
//         printk(KERN_ERR "ov7251 set integration time error\n");
//         return ret;
//     }

    return 0;
}

static unsigned int ov7251_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
//     struct again_lut *lut = ov7251_again_lut;

//     while (lut->gain <= ov7251_sensor_attr.sensor_info.max_again) {
//         if (isp_gain <= 0) {
//             *sensor_again = lut[0].value;
//             return lut[0].gain;
//         } else if (isp_gain < lut->gain) {
//             *sensor_again = (lut -1)->value;
//             return (lut-1)->gain;
//         } else {
//             if ((lut->gain == ov7251_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
//                 *sensor_again = lut->value;
//                 return lut->gain;
//             }
//         }
//         lut++;
//     }

//     return isp_gain;
    return 0;
}

static int ov7251_set_analog_gain(int value)
{
//     int ret = 0;

//     ret = ov7251_write(i2c_dev, 0x3504, (unsigned char)((value & 0x3ff) >> 8));
//     if (ret < 0) {
//         printk(KERN_ERR "ov7251 set analog gain error\n");
//         return ret;
//     }

//     ret = ov7251_write(i2c_dev, 0x3505, (unsigned char)(value & 0xff));
//     if (ret < 0) {
//         printk(KERN_ERR "ov7251 set analog gain error\n");
//         return ret;
//     }

    return 0;
}

static unsigned int ov7251_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{

    return isp_gain;
}

static int ov7251_set_digital_gain(int value)
{
    return 0;
}

static int ov7251_set_fps(int fps)
{
    // struct sensor_info *sensor_info = &ov7251_sensor_attr.sensor_info;
    // unsigned int sclk = 0;
    // unsigned short vts = 0;
    // unsigned short hts = 0;
    // unsigned int max_fps = 0;
    // unsigned char tmp;
    // unsigned int newformat = 0;
    // int ret = 0;

    // sclk = OV7251_SCLK;
    // max_fps = SENSOR_OUTPUT_MAX_FPS;

    //  /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    // newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    // if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
    //     printk(KERN_ERR "warn: fps(%x) no in range\n", fps);
    //     return -1;
    // }

    // /* hts */
    // ret = ov7251_read(i2c_dev, 0x380c, &tmp);
    // hts = tmp << 8;
    // ret += ov7251_read(i2c_dev, 0x380d, &tmp);
    // if (ret < 0)
    //     return -1;

    // hts |= tmp;

    // /* vts */
    // vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    // ret = ov7251_write(i2c_dev, 0x380e, (unsigned char)(vts >> 8));
    // ret += ov7251_write(i2c_dev, 0x380f, (unsigned char)(vts & 0xff));
    // if (ret < 0)
    //     return -1;

    // sensor_info->fps = fps;
    // sensor_info->total_height = vts;
    // sensor_info->max_integration_time = vts - 25;

    return 0;
}

static struct sensor_info ov7251_sensor_info[] = {
    {
        .private_init_setting   = ov7251_init_regs_full_640_480_mipi,
        .width                  = 640,
        .height                 = 480,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 120 << 16 | 1,  /* 60/1 */
        .total_width            = OV7251_HTS,
        .total_height           = OV7251_VTS,

        .min_integration_time   = 2,
        .max_integration_time   = OV7251_VTS - 20,
        .max_again              = 261773,
        .max_dgain              = 0,
    },

    {
        .private_init_setting   = ov7251_init_regs_binning_320_240_mipi,
        .width                  = 320,
        .height                 = 240,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 200 << 16 | 1,  /* 60/1 */
        .total_width            = 0x304,
        .total_height           = 0x130,

        .min_integration_time   = 2,
        .max_integration_time   = OV7251_VTS - 20,
        .max_again              = 261773,
        .max_dgain              = 0,
    },

};

static struct sensor_attr ov7251_sensor_attr = {
    .device_name                = OV7251_DEVICE_NAME,
    .cbus_addr                  = OV7251_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 1,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .ops = {
        .power_on               = ov7251_power_on,
        .power_off              = ov7251_power_off,
        .stream_on              = ov7251_stream_on,
        .stream_off             = ov7251_stream_off,
        .get_register           = ov7251_g_register,
        .set_register           = ov7251_s_register,

        .set_integration_time   = ov7251_set_integration_time,
        .alloc_again            = ov7251_alloc_again,
        .set_analog_gain        = ov7251_set_analog_gain,
        .alloc_dgain            = ov7251_alloc_dgain,
        .set_digital_gain       = ov7251_set_digital_gain,
        .set_fps                = ov7251_set_fps,
    },
};

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "ov7251_reset");
        if (ret) {
            printk(KERN_ERR "ov7251: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    return 0;

err_reset_gpio:
    return ret;
}

static void deinit_gpio(void)
{
    if (reset_gpio != -1)
        gpio_free(reset_gpio);
}

static int ov7251_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int i;
    int ret = init_gpio();
    if (ret)
        return ret;

    /* 确定根据传入参数sensor info */
    for (i = 0; i < ARRAY_SIZE(resolution_array); i++) {
        if (!strcmp(resolution, resolution_array[i])) {
            memcpy(&ov7251_sensor_attr.sensor_info, &ov7251_sensor_info[i], sizeof(struct sensor_info));
            break;
        }
    }

    if (i == ARRAY_SIZE(resolution_array)) {
        /* use default sensor info */
        memcpy(&ov7251_sensor_attr.sensor_info, &ov7251_sensor_info[0], sizeof(struct sensor_info));
        printk(KERN_ERR "Match %s Falied! Used Resolution: %d x %d.\n", \
                        resolution,                                     \
                        ov7251_sensor_attr.sensor_info.width,           \
                        ov7251_sensor_attr.sensor_info.height);
    }

    ret = camera_register_sensor(camera_index, &ov7251_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int ov7251_remove(struct i2c_client *client)
{
    camera_unregister_sensor(camera_index, &ov7251_sensor_attr);
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id ov7251_id[] = {
    { OV7251_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ov7251_id);

static struct i2c_driver ov7251_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = OV7251_DEVICE_NAME,
    },
    .probe          = ov7251_probe,
    .remove         = ov7251_remove,
    .id_table       = ov7251_id,
};

static struct i2c_board_info sensor_ov7251_info = {
    .type           = OV7251_DEVICE_NAME,
    .addr           = OV7251_DEVICE_I2C_ADDR,
};

static __init int init_ov7251(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "ov7251: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    i2c_dev = i2c_register_device(&sensor_ov7251_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "ov7251: failed to register i2c device\n");
        i2c_del_driver(&ov7251_driver);
        return -EINVAL;
    }

    int ret = i2c_add_driver(&ov7251_driver);
    if (ret) {
        printk(KERN_ERR "ov7251: failed to register i2c driver\n");
        return ret;
    }

    return 0;
}

static __exit void exit_ov7251(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&ov7251_driver);
}

module_init(init_ov7251);
module_exit(exit_ov7251);

MODULE_DESCRIPTION("X2000 OV7251 driver");
MODULE_LICENSE("GPL");