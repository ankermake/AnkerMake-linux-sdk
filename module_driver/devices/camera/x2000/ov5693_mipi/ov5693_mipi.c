/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * OV5693
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <linux/regulator/consumer.h>
#include <camera/hal/camera_sensor.h>


#define OV5693_DEVICE_NAME              "ov5693"
#define OV5693_DEVICE_I2C_ADDR          0x36

#define OV5693_CHIP_ID_H                0x56
#define OV5693_CHIP_ID_L                0x90

#define OV5693_REG_END                  0xffff
#define OV5693_REG_DELAY                0xfffe
#define OV5693_SUPPORT_SCLK             (160000000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5
#define OV5693_WIDTH                    1280
#define OV5693_HEIGHT                   960


static int power_gpio       = -1;
static int reset_gpio       = -1;   // PA11
static int pwdn_gpio        = -1;   // PA10
static int i2c_bus_num      = -1;   // 3
static short i2c_addr       = -1;   // 0x36
static int cam_bus_num      = -1;   // 0
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
static struct sensor_attr ov5693_sensor_attr;
static struct regulator *ov5693_regulator = NULL;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

struct again_lut ov5693_again_lut[] = {
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
    {0x22, 71266},
    {0x24, 76671},
    {0x26, 81782},
    {0x28, 86632},
    {0x2a, 91245},
    {0x2c, 95643},
    {0x2e, 99846},
    {0x30, 103870},
    {0x32, 107730},
    {0x34, 111438},
    {0x36, 115006},
    {0x38, 118445},
    {0x3a, 121762},
    {0x3c, 124968},
    {0x3e, 128068},
    {0x40, 131070},
    {0x44, 136801},
    {0x48, 142206},
    {0x4c, 147317},
    {0x50, 152167},
    {0x54, 156780},
    {0x58, 161178},
    {0x5c, 165381},
    {0x60, 169405},
    {0x64, 173265},
    {0x68, 176973},
    {0x6c, 180541},
    {0x70, 183980},
    {0x74, 187297},
    {0x78, 190503},
    {0x7c, 193603},
    {0x80, 196605},
    {0x88, 202336},
    {0x90, 207741},
    {0x98, 212852},
    {0xa0, 217702},
    {0xa8, 222315},
    {0xb0, 226713},
    {0xb8, 230916},
    {0xc0, 234940},
    {0xc8, 238800},
    {0xd0, 242508},
    {0xd8, 246076},
    {0xe0, 249515},
    {0xe8, 252832},
    {0xf0, 256038},
    {0xf8, 259138},
};

/*
 *  OV5693_1280x720_MIPI_Full_30fps_raw
 */
static struct regval_list ov5693_init_regs_1280_960_30fps_mipi[] = {
//FQuarter_1280x960_30FPS     24M MCLK

    {0x0103,0x01},    //SOFTWARE_RST
    {0x3001,0x0a},    //system control [0x3001 ~ 0x303F] start
    {0x0100,0x00},
    {0x3002,0x80},
    {0x3006,0x00},
    {0x3011,0x21},
    {0x3012,0x09},
    {0x3013,0x10},
    {0x3014,0x00},
    {0x3015,0x08},
    {0x3016,0xf0},
    {0x3017,0xf0},
    {0x3018,0xf0},
    {0x301b,0xb4},
    {0x301d,0x02},
    {0x3021,0x00},
    {0x3022,0x01},
    {0x3028,0x44},    //system control end
    {0x3098,0x03},    //PLL control [0x3080 ~ 0x30B6] start
    {0x3099,0x1e},
    {0x309a,0x02},
    {0x309b,0x01},
    {0x309c,0x00},
    {0x30a0,0xd2},
    {0x30a2,0x01},
    {0x30b2,0x00},
    {0x30b3,0x50},    //64
    {0x30b4,0x03},
    {0x30b5,0x04},
    {0x30b6,0x01},    //PLL control end
    {0x3104,0x21},    //SCCB control start
    {0x3106,0x00},    //SCCB control end
    {0x3400,0x04},    //manual white balance start
    {0x3401,0x00},
    {0x3402,0x04},
    {0x3403,0x00},
    {0x3404,0x04},
    {0x3405,0x00},
    {0x3406,0x01},    //manual white balance end
    {0x3500,0x00},    //manual exposure control start
    {0x3501,0x3d},
    {0x3502,0x00},
    {0x3503,0x37},
    {0x3504,0x00},
    {0x3505,0x00},
    {0x3506,0x00},
    {0x3507,0x02},
    {0x3508,0x00},
    {0x3509,0x10},    //01 sensor gain;10 real gain
    {0x350a,0x00},
    {0x350b,0x40},    //manual exposure control end
    {0x3601,0x0a},    //ADC and analog start
    {0x3602,0x38},
    {0x3612,0x80},
    {0x3620,0x54},
    {0x3621,0xc7},
    {0x3622,0x0f},
    {0x3625,0x10},
    {0x3630,0x55},
    {0x3631,0xf4},
    {0x3632,0x00},
    {0x3633,0x34},
    {0x3634,0x02},
    {0x364d,0x0d},
    {0x364f,0xdd},
    {0x3660,0x04},
    {0x3662,0x10},
    {0x3663,0xf1},
    {0x3665,0x00},
    {0x3666,0x20},
    {0x3667,0x00},
    {0x366a,0x80},
    {0x3680,0xe0},
    {0x3681,0x00},    //ADC and analog end
    {0x3700,0x42},    //sensor control [0x3700 ~ 0x377F] start
    {0x3701,0x14},
    {0x3702,0xa0},
    {0x3703,0xd8},
    {0x3704,0x78},
    {0x3705,0x02},
    {0x3708,0xe6},
    {0x3709,0xc7},
    {0x370a,0x00},
    {0x370b,0x20},
    {0x370c,0x0c},
    {0x370d,0x11},
    {0x370e,0x00},
    {0x370f,0x40},
    {0x3710,0x00},
    {0x371a,0x1c},
    {0x371b,0x05},
    {0x371c,0x01},
    {0x371e,0xa1},
    {0x371f,0x0c},
    {0x3721,0x00},
    {0x3724,0x10},
    {0x3726,0x00},
    {0x372a,0x01},
    {0x3730,0x10},
    {0x3738,0x22},
    {0x3739,0xe5},
    {0x373a,0x50},
    {0x373b,0x02},
    {0x373c,0x41},
    {0x373f,0x02},
    {0x3740,0x42},
    {0x3741,0x02},
    {0x3742,0x18},
    {0x3743,0x01},
    {0x3744,0x02},
    {0x3747,0x10},
    {0x374c,0x04},
    {0x3751,0xf0},
    {0x3752,0x00},
    {0x3753,0x00},
    {0x3754,0xc0},
    {0x3755,0x00},
    {0x3756,0x1a},
    {0x3758,0x00},
    {0x3759,0x0f},
    {0x376b,0x44},
    {0x375c,0x04},
    {0x3774,0x10},
    {0x3776,0x00},
    {0x377f,0x08},    //sensor control end
    {0x3780,0x22},    //PSRAM control start
    {0x3781,0x0c},
    {0x3784,0x2c},
    {0x3785,0x1e},
    {0x378f,0xf5},
    {0x3791,0xb0},
    {0x3795,0x00},
    {0x3796,0x64},
    {0x3797,0x11},
    {0x3798,0x30},
    {0x3799,0x41},
    {0x379a,0x07},
    {0x379b,0xb0},
    {0x379c,0x0c},    //PSRAM control end
    {0x37c5,0x00},    //FREX control start
    {0x37c6,0x00},
    {0x37c7,0x00},
    {0x37c9,0x00},
    {0x37ca,0x00},
    {0x37cb,0x00},
    {0x37de,0x00},
    {0x37df,0x00},    //FREX control end

    {0x3800,0x00},    //timing control [0x3800 ~ 0x382F] start
    {0x3801,0x00},    //X_ADDR_START
    {0x3802,0x00},
    {0x3803,0x00},    //Y_ADDR_START
    {0x3804,0x0a},
    {0x3805,0x3f},    //X_ADDR_END    0x0a3f 2623
    {0x3806,0x07},
    {0x3807,0xa3},    //Y_ADDR_END    0x07a3 1955
    {0x3808,0x05},
    {0x3809,0x00},    //X_OUTPUT_SIZE 0x0500 1280
    {0x380a,0x03},
    {0x380b,0xC0},    //Y_OUTPUT_SIZE    0x03c0 960
    {0x380c,0x0a},
    {0x380d,0x80},    //HTS 0x0A80    2688
    {0x380e,0x07},
    {0x380f,0xc0},    //VTS 0x07C0    1984
    {0x3810,0x00},
    {0x3811,0x08},
    {0x3812,0x00},
    {0x3813,0x02},
    {0x3814,0x31},
    {0x3815,0x31},
    {0x3820,0x01},    //0x04
    {0x3821,0x1f},    //HDR surport disable
    {0x3823,0x00},
    {0x3824,0x00},
    {0x3825,0x00},
    {0x3826,0x00},
    {0x3827,0x00},
    {0x382a,0x04},    //timing control end
    {0x3a04,0x06},
    {0x3a05,0x14},
    {0x3a06,0x00},
    {0x3a07,0xfe},
    {0x3b00,0x00},    //strobe control start
    {0x3b02,0x00},
    {0x3b03,0x00},
    {0x3b04,0x00},
    {0x3b05,0x00},    //strobe control end
    {0x3e07,0x20},
    {0x4000,0x08},    //BLC control start
    {0x4001,0x04},
    {0x4002,0x45},
    {0x4004,0x08},
    {0x4005,0x18},
    {0x4006,0x20},
    {0x4008,0x24},
    {0x4009,0x10},
    {0x400c,0x00},
    {0x400d,0x00},    //BLC control end
    {0x4058,0x00},    //---
    {0x404e,0x37},
    {0x404f,0x8f},
    {0x4058,0x00},
    {0x4101,0xb2},    //---
    {0x4303,0x00},    //format control start
    {0x4304,0x08},
    {0x4307,0x30},
    {0x4311,0x04},
    {0x4315,0x01},    //format control end
    {0x4511,0x05},
    {0x4512,0x01},    //INPUT_SWAP_MAN_EN
    {0x4806,0x00},    //MIPI control start
    {0x4816,0x52},
    {0x481f,0x30},
    {0x4826,0x2c},
    {0x4831,0x64},    //MIPI control end

    {0x4d00,0x04},    //temperature monitor start
    {0x4d01,0x71},
    {0x4d02,0xfd},
    {0x4d03,0xf5},
    {0x4d04,0x0c},
    {0x4d05,0xcc},    //temperature monitor end
    {0x4837,0x0a},    //MIPI control add
    {0x5000,0x06},    //ISP top start
    {0x5001,0x01},
    {0x5002,0x00},
    {0x5003,0x20},
    {0x5046,0x0a},
    {0x5013,0x00},
    {0x5046,0x0a},    //ISP top end
    {0x5780,0x1c},    //DPC control start
    {0x5786,0x20},
    {0x5787,0x10},
    {0x5788,0x18},
    {0x578a,0x04},
    {0x578b,0x02},
    {0x578c,0x02},
    {0x578e,0x06},
    {0x578f,0x02},
    {0x5790,0x02},
    {0x5791,0xff},    //DPC control end
    {0x5842,0x01},    //LENC start
    {0x5843,0x2b},
    {0x5844,0x01},
    {0x5845,0x92},
    {0x5846,0x01},
    {0x5847,0x8f},
    {0x5848,0x01},
    {0x5849,0x0c},    //LENC end
    {0x5e00,0x00},    //color bar/scalar control start
    {0x5e10,0x0c},    //color bar/scalar control    end
    {0x0100,0x00},    //MODE_SELECT 0:software_standby 1:Streaming

    {OV5693_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list ov5693_regs_stream_on[] = {

    {0x0100, 0x01},             /* RESET_REGISTER */
    {OV5693_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list ov5693_regs_stream_off[] = {

    {0x0100, 0x00},             /* RESET_REGISTER */
    {OV5693_REG_END, 0x00},     /* END MARKER */
};

static int ov5693_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
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
        printk(KERN_ERR "ov5693: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int ov5693_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
        printk(KERN_ERR "ov5693(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int ov5693_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != OV5693_REG_END) {
        if (vals->reg_num == OV5693_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = ov5693_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int ov5693_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != OV5693_REG_END) {
        if (vals->reg_num == OV5693_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = ov5693_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        printk("reg = 0x%02x, val = 0x%02x\n",vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int ov5693_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = ov5693_read(i2c, 0x300A, &h);
    if (ret < 0)
        return ret;
    if (h != OV5693_CHIP_ID_H) {
        printk(KERN_ERR "ov5693 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = ov5693_read(i2c, 0x300B, &l);
    if (ret < 0)
        return ret;

    if (l != OV5693_CHIP_ID_L) {
        printk(KERN_ERR "ov5693 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk("ov5693 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "ov5693_reset");
        if (ret) {
            printk(KERN_ERR "ov5693: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "ov5693_pwdn");
        if (ret) {
            printk(KERN_ERR "ov5693: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ov5693_power");
        if (ret) {
            printk(KERN_ERR "ov5693: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        ov5693_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(ov5693_regulator)) {
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

    if (ov5693_regulator)
        regulator_put(ov5693_regulator);
}

static void ov5693_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (ov5693_regulator)
        regulator_disable(ov5693_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int ov5693_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 24 * 1000 * 1000);

    if (ov5693_regulator)
        regulator_enable(ov5693_regulator);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }
    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 0);
        m_msleep(5);
        gpio_direction_output(reset_gpio, 1);
    }
    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(10);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
    }

    ret = ov5693_detect(i2c_dev);

    if (ret) {
        printk(KERN_ERR "ov5693: failed to detect\n");
        ov5693_power_off();
        return ret;
    }

    ret = ov5693_write_array(i2c_dev, ov5693_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "ov5693: failed to init regs\n");
        ov5693_power_off();
        return ret;
    }

    return 0;
}

static int ov5693_stream_on(void)
{
    int ret = ov5693_write_array(i2c_dev, ov5693_regs_stream_on);
    if (ret)
        printk(KERN_ERR "ov5693: failed to stream on\n");

    return ret;
}

static void ov5693_stream_off(void)
{
    int ret = ov5693_write_array(i2c_dev, ov5693_regs_stream_off);
    if (ret)
        printk(KERN_ERR "ov5693: failed to stream on\n");
}

static int ov5693_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = ov5693_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;

    return ret;
}

static int ov5693_s_register(struct sensor_dbg_register *reg)
{
    return ov5693_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static int ov5693_set_integration_time(int value)
{
    int ret = 0;

    ret  = ov5693_write(i2c_dev, 0x3500, (unsigned char)((value & 0xffff) >> 12));
    ret += ov5693_write(i2c_dev, 0x3501, (unsigned char)((value & 0x0fff) >> 4));
    ret += ov5693_write(i2c_dev, 0x3502, (unsigned char)((value & 0x000f) << 4));

    if (ret < 0)
        return ret;

    return 0;

}

static unsigned int ov5693_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

    struct again_lut *lut = ov5693_again_lut;
    while(lut->gain <= ov5693_sensor_attr.sensor_info.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == ov5693_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static int ov5693_set_analog_gain(int value)
{

    int ret = 0;

    ret = ov5693_write(i2c_dev, 0x350B, (unsigned char)(value & 0xff));
    if (ret < 0)
        return ret;
    return 0;

}

static unsigned int ov5693_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int ov5693_set_digital_gain(int value)
{
    return 0;
}

static int ov5693_set_fps(int fps)
{
    struct sensor_info *sensor_info = &ov5693_sensor_attr.sensor_info;
    unsigned int sclk = 0;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp = 0;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "ov5693 fps(%d) no in range\n", fps);
        return -1;
    }
    sclk = OV5693_SUPPORT_SCLK;

    ret = ov5693_read(i2c_dev, 0x380c, &tmp);
    hts = tmp << 8;
    tmp = 0;
    ret += ov5693_read(i2c_dev, 0x380d, &tmp);
    hts |= tmp;
    if(ret < 0) {
        printk("err: ov5693_read err\n");
        return -1;
    }

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = ov5693_write(i2c_dev, 0x3208, 0x02);
    ret += ov5693_write(i2c_dev, 0x380f, (unsigned char)(vts & 0xff));
    ret += ov5693_write(i2c_dev, 0x380e, (unsigned char)((vts >> 8) & 0xff));
    ret += ov5693_write(i2c_dev, 0x3208, 0x12);
    ret += ov5693_write(i2c_dev, 0x3208, 0xa2);
    if (ret < 0) {
        printk("err: ov5693_write err\n");
        return -1;
    }

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}


static struct sensor_attr ov5693_sensor_attr = {
    .device_name                = OV5693_DEVICE_NAME,
    .cbus_addr                  = OV5693_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = ov5693_init_regs_1280_960_30fps_mipi,
        .width                  = OV5693_WIDTH,
        .height                 = OV5693_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 30 << 16 | 1,
        .total_width            = 0x0A80,
        .total_height           = 0x07C0,

        .min_integration_time   = 3,
        .max_integration_time   = 0x07C0 - 2,
        .one_line_expr_in_us    = 15,
        .max_again              = 259138,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = ov5693_power_on,
        .power_off              = ov5693_power_off,
        .stream_on              = ov5693_stream_on,
        .stream_off             = ov5693_stream_off,
        .get_register           = ov5693_g_register,
        .set_register           = ov5693_s_register,

        .set_integration_time   = ov5693_set_integration_time,
        .alloc_again            = ov5693_alloc_again,
        .set_analog_gain        = ov5693_set_analog_gain,
        .alloc_dgain            = ov5693_alloc_dgain,
        .set_digital_gain       = ov5693_set_digital_gain,
        .set_fps                = ov5693_set_fps,
    },
};

static int ov5693_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &ov5693_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int ov5693_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &ov5693_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id ov5693_id[] = {
    { OV5693_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ov5693_id);

static struct i2c_driver ov5693_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = OV5693_DEVICE_NAME,
    },
    .probe              = ov5693_probe,
    .remove             = ov5693_remove,
    .id_table           = ov5693_id,
};

static struct i2c_board_info sensor_ov5693_info = {
    .type               = OV5693_DEVICE_NAME,
    .addr               = OV5693_DEVICE_I2C_ADDR,
};

static __init int init_ov5693(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "ov5693: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    ov5693_driver.driver.name = sensor_name;
    strcpy(ov5693_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_ov5693_info.addr = i2c_addr;
    strcpy(sensor_ov5693_info.type, sensor_name);
    ov5693_sensor_attr.device_name = sensor_name;

    int ret = i2c_add_driver(&ov5693_driver);
    if (ret) {
        printk(KERN_ERR "ov5693: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_ov5693_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "ov5693: failed to register i2c device\n");
        i2c_del_driver(&ov5693_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_ov5693(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&ov5693_driver);
}

module_init(init_ov5693);
module_exit(exit_ov5693);

MODULE_DESCRIPTION("x2000 ov5693 driver");
MODULE_LICENSE("GPL");
