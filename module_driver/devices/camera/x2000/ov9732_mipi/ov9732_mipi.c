/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * OV9732
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <linux/regulator/consumer.h>
#include <camera/hal/camera_sensor.h>


#define CONFIG_OV9732_1280X720

#define OV9732_DEVICE_NAME              "ov9732"
#define OV9732_DEVICE_I2C_ADDR          0x36

#define OV9732_CHIP_ID_H                0x97
#define OV9732_CHIP_ID_L                0x32

#define OV9732_REG_END                  0xff
#define OV9732_REG_DELAY                0xfe
#define OV9732_SUPPORT_SCLK             (36000000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5
#define OV9732_MAX_WIDTH                1280
#define OV9732_MAX_HEIGHT               720
#if defined CONFIG_OV9732_1280X720
#define OV9732_WIDTH                    1280
#define OV9732_HEIGHT                   720
#endif
//#define OV9732_BINNING_MODE

static int power_gpio       = -1;    // GPIO_PC(21);
static int reset_gpio       = -1;    // GPIO_PB(12);    GPIO_PB(20);
static int pwdn_gpio        = -1;    // GPIO_PB(18);    GPIO_PB(19);
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
static struct sensor_attr ov9732_sensor_attr;
static struct regulator *ov9732_regulator = NULL;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

struct again_lut ov9732_again_lut[] = {
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
 *  OV9732_1280x720_MIPI_Full_30fps_raw
 */
static struct regval_list ov9732_init_regs_1280_720_30fps_mipi[] = {

    {0x0103 ,0x01},
    {0x0100 ,0x00},
    {0x3001 ,0x00},
    {0x3002 ,0x00},
    {0x3007 ,0x1f},
    {0x3008 ,0xff},
    {0x3009 ,0x02},
    {0x3010 ,0x00},
    {0x3011 ,0x08},
    {0x3014 ,0x22},
    {0x301e ,0x15},
    {0x3030 ,0x19},
    {0x3080 ,0x02},
    {0x3081 ,0x3c},
    {0x3082 ,0x04},
    {0x3083 ,0x00},
    {0x3084 ,0x02},
    {0x3085 ,0x01},
    {0x3086 ,0x01},
    {0x3089 ,0x01},
    {0x308a ,0x00},
    {0x3103 ,0x01},
    {0x3600 ,0xf6},
    {0x3601 ,0x72},
    {0x3605 ,0x66},
    {0x3610 ,0x0c},
    {0x3611 ,0x60},
    {0x3612 ,0x35},
    {0x3654 ,0x10},
    {0x3655 ,0x77},
    {0x3656 ,0x77},
    {0x3657 ,0x07},
    {0x3658 ,0x22},
    {0x3659 ,0x22},
    {0x365a ,0x02},
    {0x3700 ,0x1f},
    {0x3701 ,0x10},
    {0x3702 ,0x0c},
    {0x3703 ,0x0b},
    {0x3704 ,0x3c},
    {0x3705 ,0x51},
    {0x370d ,0x20},
    {0x3710 ,0x0d},
    {0x3782 ,0x58},
    {0x3783 ,0x60},
    {0x3784 ,0x05},
    {0x3785 ,0x55},
    {0x37c0 ,0x07},
    {0x3800 ,0x00},
    {0x3801 ,0x04}, // X ADDR START
    {0x3802 ,0x00},
    {0x3803 ,0x04}, // Y ADDR START
    {0x3804 ,0x05},
    {0x3805 ,0x0b}, // X ADDR END
    {0x3806 ,0x02},
    {0x3807 ,0xdb}, // X ADDR END
    {0x3808 ,0x05},
    {0x3809 ,0x00}, // X OUTPUT SIZE
    {0x380a ,0x02},
    {0x380b ,0xd0}, // Y OUTPUT SIZE
    {0x380c ,0x05},
    {0x380d ,0xc6}, // HTS
    {0x380e ,0x03},
    {0x380f ,0x22}, // VTS
    {0x3810 ,0x00},
    {0x3811 ,0x04},
    {0x3812 ,0x00},
    {0x3813 ,0x04},
    {0x3816 ,0x00},
    {0x3817 ,0x00},
    {0x3818 ,0x00},
    {0x3819 ,0x04},
    {0x3820 ,0x10},
    {0x3821 ,0x00},
    {0x382c ,0x06},
    {0x3500 ,0x00},
    {0x3501 ,0x31},
    {0x3502 ,0x00},
    {0x3503 ,0x03},
    {0x3504 ,0x00},
    {0x3505 ,0x00},
    {0x3509 ,0x10},
    {0x350a ,0x00},
    {0x350b ,0x40},
    {0x3d00 ,0x00},
    {0x3d01 ,0x00},
    {0x3d02 ,0x00},
    {0x3d03 ,0x00},
    {0x3d04 ,0x00},
    {0x3d05 ,0x00},
    {0x3d06 ,0x00},
    {0x3d07 ,0x00},
    {0x3d08 ,0x00},
    {0x3d09 ,0x00},
    {0x3d0a ,0x00},
    {0x3d0b ,0x00},
    {0x3d0c ,0x00},
    {0x3d0d ,0x00},
    {0x3d0e ,0x00},
    {0x3d0f ,0x00},
    {0x3d80 ,0x00},
    {0x3d81 ,0x00},
    {0x3d82 ,0x38},
    {0x3d83 ,0xa4},
    {0x3d84 ,0x00},
    {0x3d85 ,0x00},
    {0x3d86 ,0x1f},
    {0x3d87 ,0x03},
    {0x3d8b ,0x00},
    {0x3d8f ,0x00},
    {0x4001 ,0xe0},
    {0x4004 ,0x00},
    {0x4005 ,0x02},
    {0x4006 ,0x01},
    {0x4007 ,0x40},
    {0x4009 ,0x0b},
    {0x4300 ,0x03},
    {0x4301 ,0xff},
    {0x4304 ,0x00},
    {0x4305 ,0x00},
    {0x4309 ,0x00},
    {0x4600 ,0x00},
    {0x4601 ,0x04},
    {0x4800 ,0x00},
    {0x4805 ,0x00},
    {0x4821 ,0x50},
    {0x4823 ,0x50},
    {0x4837 ,0x2d},
    {0x4a00 ,0x00},
    {0x4f00 ,0x80},
    {0x4f01 ,0x10},
    {0x4f02 ,0x00},
    {0x4f03 ,0x00},
    {0x4f04 ,0x00},
    {0x4f05 ,0x00},
    {0x4f06 ,0x00},
    {0x4f07 ,0x00},
    {0x4f08 ,0x00},
    {0x4f09 ,0x00},
    {0x5000 ,0x07},
    {0x500c ,0x00},
    {0x500d ,0x00},
    {0x500e ,0x00},
    {0x500f ,0x00},
    {0x5010 ,0x00},
    {0x5011 ,0x00},
    {0x5012 ,0x00},
    {0x5013 ,0x00},
    {0x5014 ,0x00},
    {0x5015 ,0x00},
    {0x5016 ,0x00},
    {0x5017 ,0x00},
    {0x5080 ,0x00},
    {0x5180 ,0x01},
    {0x5181 ,0x00},
    {0x5182 ,0x01},
    {0x5183 ,0x00},
    {0x5184 ,0x01},
    {0x5185 ,0x00},
    {0x5708 ,0x06},
    {0x5781 ,0x0e},
    {0x5783 ,0x0f},
    {0x3603 ,0x70},
    {0x3620 ,0x1e},
    {0x400a ,0x01},
    {0x400b ,0xc0},
    // {0x0100 ,0x01},
    {OV9732_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list ov9732_regs_stream_on[] = {

    {0x0100, 0x01},             /* RESET_REGISTER */
    {OV9732_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list ov9732_regs_stream_off[] = {

    {0x0100, 0x00},             /* RESET_REGISTER */
    {OV9732_REG_END, 0x00},     /* END MARKER */
};

static int ov9732_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
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
        printk(KERN_ERR "ov9732: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int ov9732_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
        printk(KERN_ERR "ov9732(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int ov9732_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != OV9732_REG_END) {
        if (vals->reg_num == OV9732_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = ov9732_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int ov9732_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != OV9732_REG_END) {
        if (vals->reg_num == OV9732_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = ov9732_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        printk("reg = 0x%02x, val = 0x%02x\n",vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int ov9732_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = ov9732_read(i2c, 0x300A, &h);
    if (ret < 0)
        return ret;
    if (h != OV9732_CHIP_ID_H) {
        printk(KERN_ERR "ov9732 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = ov9732_read(i2c, 0x300B, &l);
    if (ret < 0)
        return ret;

    if (l != OV9732_CHIP_ID_L) {
        printk(KERN_ERR "ov9732 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "ov9732 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "ov9732_reset");
        if (ret) {
            printk(KERN_ERR "ov9732: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "ov9732_pwdn");
        if (ret) {
            printk(KERN_ERR "ov9732: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ov9732_power");
        if (ret) {
            printk(KERN_ERR "ov9732: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        ov9732_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(ov9732_regulator)) {
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

    if (ov9732_regulator)
        regulator_put(ov9732_regulator);
}

static void ov9732_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (ov9732_regulator)
        regulator_disable(ov9732_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int ov9732_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 24 * 1000 * 1000);

    if (ov9732_regulator)
        regulator_enable(ov9732_regulator);

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

    ret = ov9732_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "ov9732: failed to detect\n");
       ov9732_power_off();
        return ret;
    }

    ret = ov9732_write_array(i2c_dev, ov9732_sensor_attr.sensor_info.private_init_setting);
    // ret += ov9732_read_array(i2c_dev, ov9732_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "ov9732: failed to init regs\n");
        ov9732_power_off();
        return ret;
    }

    return 0;
}

static int ov9732_stream_on(void)
{
    int ret = ov9732_write_array(i2c_dev, ov9732_regs_stream_on);
    if (ret)
        printk(KERN_ERR "ov9732: failed to stream on\n");

    return ret;
}

static void ov9732_stream_off(void)
{
    int ret = ov9732_write_array(i2c_dev, ov9732_regs_stream_off);
    if (ret)
        printk(KERN_ERR "ov9732: failed to stream on\n");
}

static int ov9732_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = ov9732_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int ov9732_s_register(struct sensor_dbg_register *reg)
{
    return ov9732_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static int ov9732_set_integration_time(int value)
{
    int ret = 0;

    ret  = ov9732_write(i2c_dev, 0x3500, (unsigned char)((value & 0xffff) >> 12));
    ret += ov9732_write(i2c_dev, 0x3501, (unsigned char)((value & 0x0fff) >> 4));
    ret += ov9732_write(i2c_dev, 0x3502, (unsigned char)((value & 0x000f) << 4));

    if (ret < 0)
        return ret;

    return 0;

}

unsigned int ov9732_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

    struct again_lut *lut = ov9732_again_lut;
    while(lut->gain <= ov9732_sensor_attr.sensor_info.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == ov9732_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static int ov9732_set_analog_gain(int value)
{

    int ret = 0;

    ret = ov9732_write(i2c_dev, 0x350B, (unsigned char)(value & 0xff));
    if (ret < 0)
        return ret;
    return 0;

}

unsigned int ov9732_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int ov9732_set_digital_gain(int value)
{
    return 0;
}

static int ov9732_set_fps(int fps)
{
    struct sensor_info *sensor_info = &ov9732_sensor_attr.sensor_info;
    unsigned int sclk = 0;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp = 0;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "ov9732 fps(%d) no in range\n", fps);
        return -1;
    }
    sclk = OV9732_SUPPORT_SCLK;

    ret = ov9732_read(i2c_dev, 0x380c, &tmp);
    hts = tmp;
    ret += ov9732_read(i2c_dev, 0x380d, &tmp);
    if(ret < 0)
        return -1;
    hts = ((hts << 8) + tmp)*2;

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = ov9732_write(i2c_dev, 0x380e, (unsigned char)(vts & 0xff));
    ret += ov9732_write(i2c_dev, 0x380f, (unsigned char)(vts >> 8));
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}


static struct sensor_attr ov9732_sensor_attr = {
    .device_name                = OV9732_DEVICE_NAME,
    .cbus_addr                  = OV9732_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 1,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = ov9732_init_regs_1280_720_30fps_mipi,
#ifndef OV9732_BINNING_MODE
        .width                  = OV9732_WIDTH,
        .height                 = OV9732_HEIGHT,
#else
        .width                  = OV9732_WIDTH/2,
        .height                 = OV9732_HEIGHT/2,
#endif
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 60 << 16 | 1,
        .total_width            = 0x05c6,
        .total_height           = 0x0322,

        .min_integration_time   = 3,
        .max_integration_time   = 0x0322 - 3,
        .one_line_expr_in_us    = 15,
        .max_again              = 259138,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = ov9732_power_on,
        .power_off              = ov9732_power_off,
        .stream_on              = ov9732_stream_on,
        .stream_off             = ov9732_stream_off,
        .get_register           = ov9732_g_register,
        .set_register           = ov9732_s_register,

        .set_integration_time   = ov9732_set_integration_time,
        .alloc_again            = ov9732_alloc_again,
        .set_analog_gain        = ov9732_set_analog_gain,
        .alloc_dgain            = ov9732_alloc_dgain,
        .set_digital_gain       = ov9732_set_digital_gain,
        .set_fps                = ov9732_set_fps,
    },
};

static int ov9732_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &ov9732_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int ov9732_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &ov9732_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id ov9732_id[] = {
    { OV9732_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ov9732_id);

static struct i2c_driver ov9732_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = OV9732_DEVICE_NAME,
    },
    .probe              = ov9732_probe,
    .remove             = ov9732_remove,
    .id_table           = ov9732_id,
};

static struct i2c_board_info sensor_ov9732_info = {
    .type               = OV9732_DEVICE_NAME,
    .addr               = OV9732_DEVICE_I2C_ADDR,
};

static __init int init_ov9732(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "ov9732: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    ov9732_driver.driver.name = sensor_name;
    strcpy(ov9732_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_ov9732_info.addr = i2c_addr;
    strcpy(sensor_ov9732_info.type, sensor_name);
    ov9732_sensor_attr.device_name = sensor_name;

    int ret = i2c_add_driver(&ov9732_driver);
    if (ret) {
        printk(KERN_ERR "ov9732: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_ov9732_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "ov9732: failed to register i2c device\n");
        i2c_del_driver(&ov9732_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_ov9732(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&ov9732_driver);
}

module_init(init_ov9732);
module_exit(exit_ov9732);

MODULE_DESCRIPTION("x2000 ov9732 driver");
MODULE_LICENSE("GPL");
