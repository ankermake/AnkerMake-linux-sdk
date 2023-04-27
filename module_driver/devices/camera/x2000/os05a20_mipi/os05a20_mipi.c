/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * OS05A20 mipi driver
 *
 */
#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>


#define OS05A20_DEVICE_NAME             "os05a20"
#define OS05A20_DEVICE_I2C_ADDR         0x10

#define OS05A20_SUPPORT_SCLK_FPS_30     (72000000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5

#define OS05A20_WIDTH                   2048
#define OS05A20_HEIGHT                  1536

#define OS05A20_CHIP_ID_H               (0x53)
#define OS05A20_CHIP_ID_M               (0x05)
#define OS05A20_CHIP_ID_L               (0x41)
#define OS05A20_REG_CHIP_ID_HIGH        0x300A
#define OS05A20_REG_CHIP_ID_MEDIUM      0x300B
#define OS05A20_REG_CHIP_ID_LOW         0x300C

#define OS05A20_REG_DELAY               0xfffe
#define OS05A20_REG_END                 0Xffff

static int power_gpio           = -1;
static int reset_gpio           = -1;
static int pwdn_gpio            = -1;
static int i2c_bus_num          = -1;
static short i2c_addr           = -1;   // 0x10
static int cam_bus_num          = -1;
static char *sensor_name        = NULL;
static char *regulator_name     = "";   // 0

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, short, 0644);
module_param(cam_bus_num, int, 0644);
module_param(sensor_name, charp, 0644);
module_param(regulator_name, charp, 0644);


static struct i2c_client *i2c_dev;
static struct sensor_attr os05a20_sensor_attr;
static struct regulator *os05a20_regulator = NULL;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int index;
    unsigned char reg3508;
    unsigned char reg3509;
    unsigned int gain;
};

static struct again_lut os05a20_again_lut[] = {
    {0, 0x00, 0x80, 0},
    {1, 0x00, 0x88, 5731},
    {2, 0x00, 0x90, 11136},
    {3, 0x00, 0x98, 16247},
    {4, 0x00, 0xa0, 21097},
    {5, 0x00, 0xa8, 25710},
    {6, 0x00, 0xb0, 30108},
    {7, 0x00, 0xb8, 34311},
    {8, 0x00, 0xc0, 38335},
    {9, 0x00, 0xc8, 42195},
    {10, 0x00, 0xd0, 45903},
    {11, 0x00, 0xd8, 49471},
    {12, 0x00, 0xe0, 52910},
    {13, 0x00, 0xe8, 56227},
    {14, 0x00, 0xf0, 59433},
    {15, 0x00, 0xf8, 62533},
    {16, 0x01, 0x00, 65535},
    {17, 0x01, 0x10, 71266},
    {18, 0x01, 0x20, 76671},
    {19, 0x01, 0x30, 81782},
    {20, 0x01, 0x40, 86632},
    {21, 0x01, 0x50, 91245},
    {22, 0x01, 0x60, 95643},
    {23, 0x01, 0x70, 99846},
    {24, 0x01, 0x80, 103870},
    {25, 0x01, 0x90, 107730},
    {26, 0x01, 0xa0, 111438},
    {27, 0x01, 0xb0, 115006},
    {28, 0x01, 0xc0, 118445},
    {29, 0x01, 0xd0, 121762},
    {30, 0x01, 0xe0, 124968},
    {31, 0x01, 0xf0, 128068},
    {32, 0x02, 0x00, 131070},
    {33, 0x02, 0x20, 136801},
    {34, 0x02, 0x40, 142206},
    {35, 0x02, 0x60, 147317},
    {36, 0x02, 0x80, 152167},
    {37, 0x02, 0xa0, 156780},
    {38, 0x02, 0xc0, 161178},
    {39, 0x02, 0xe0, 165381},
    {40, 0x03, 0x00, 169405},
    {41, 0x03, 0x20, 173265},
    {42, 0x03, 0x40, 176973},
    {43, 0x03, 0x60, 180541},
    {44, 0x03, 0x80, 183980},
    {45, 0x03, 0xa0, 187297},
    {46, 0x03, 0xc0, 190503},
    {47, 0x03, 0xe0, 193603},
    {48, 0x04, 0x00, 196605},
    {49, 0x04, 0x40, 202336},
    {50, 0x04, 0x80, 207741},
    {51, 0x04, 0xc0, 212852},
    {52, 0x05, 0x00, 217702},
    {53, 0x05, 0x40, 222315},
    {54, 0x05, 0x80, 226713},
    {55, 0x05, 0xc0, 230916},
    {56, 0x06, 0x00, 234940},
    {57, 0x06, 0x40, 238800},
    {58, 0x06, 0x80, 242508},
    {59, 0x06, 0xc0, 246076},
    {60, 0x07, 0x00, 249515},
    {61, 0x07, 0x40, 252832},
    {62, 0x07, 0x80, 256038},
    {63, 0x07, 0xc0, 259138},
};


/*
 * Interface    : MIPI - 2lane
 * MCLK         : 24Mhz
 * resolution   : 2048*1536
 * FrameRate    : 30fps
 * HTS          : 0x3f0
 * VTS          : 0x94c
 */
#if 1
static struct regval_list os05a20_init_regs_2048_1536_30fps_mipi[] = {
    /* PLL Control */
    {0x0103, 0x01},  //h7: Debug mode [0]:software_reset
    {0x0303, 0x01},  //l3: pll1_prediv
    {0x0305, 0x40},  //pll_divp
    {0x0306, 0x00},
    {0x0307, 0x00},
    {0x0308, 0x03},
    {0x0309, 0x04},
    {0x032a, 0x00},
    {0x031e, 0x09},  //l3: MIPl bit sel: 000:8-bit; 001:10-bit; 010:12-bit.
    {0x0325, 0x48},
    {0x0328, 0x07},
    /* system control */
    {0x300d, 0x11},
    {0x300e, 0x11},
    {0x300f, 0x11},  //[4]: mipi enable
    {0x3010, 0x01},  //mipi pk
    {0x3012, 0x21},  //mipi lanes num; 0x21:2 lanes; 0x41:4 lanes
    {0x3016, 0xf0},
    {0x3018, 0xf0},
    {0x3028, 0xf0},
    {0x301e, 0x98},  //CLOCK SEL [3]: pclk2x_sel
    {0x3010, 0x04},
    {0x3011, 0x06},
    {0x3031, 0xa9},  //MIPI PK [3]: mipi mode 1, 0: CPHY; 1: DPHY
    /* SCCB */
    {0x3103, 0x48},
    {0x3104, 0x01},  //[0]: pwup_dis0
    {0x3106, 0x10},
    {0x3400, 0x04},  //[2]: power saving mode enable
    {0x3025, 0x03},
    /* power saving mode */
    {0x3425, 0x51},
    {0x3428, 0x01},
    {0x3406, 0x08},
    {0x3408, 0x03},
    /* MEC/MGC control */
    {0x3501, 0x09},  //long_exposure_h8
    {0x3502, 0x2c},  //long_exposure_l3
    {0x3505, 0x83},  //Gain Conversation Option.
    {0x3508, 0x00},  //gain_h6
    {0x3509, 0x80},  //gain_l8 1x gain
    {0x350a, 0x04},  //long digigain_h6
    {0x350b, 0x00},
    {0x350c, 0x00},  //short gain_h6
    {0x350d, 0x80},
    {0x350e, 0x04},  //short digigain_h6
    {0x350f, 0x00},
    /* analog control 0x3600~0x3637 */
    {0x3600, 0x00},  //analog control register
    {0x3626, 0xff},
    {0x3605, 0x50},
    {0x3609, 0xb5},
    {0x3610, 0x69},
    {0x360c, 0x01},
    {0x3628, 0xa4},
    {0x3629, 0x6a},
    {0x362d, 0x10},
    {0x3660, 0x43},  //[5]: mipi_pclk_sel: 0:mipi; 1:LVDS
    {0x3661, 0x06},  //[5]: test mode  0:long expo; 1: short expo
    {0x3662, 0x00},
    {0x3663, 0x28},
    {0x3664, 0x0d},
    {0x366a, 0x38},  //hts_pclk_h8  fix timing mode
    {0x366b, 0xa0},  //hts_pclk_l8
    {0x366d, 0x00},
    {0x366e, 0x00},
    {0x3680, 0x00},  //analog control 0x3680~0x36a5
    {0x36c0, 0x00},
    {0x3621, 0x81},
    {0x3634, 0x31},
    {0x3620, 0x00},
    {0x3622, 0x00},
    {0x362a, 0xd0},
    {0x362e, 0x8c},
    {0x362f, 0x98},
    {0x3630, 0xb0},
    {0x3631, 0xd7},
    /* sensor timing control 0x3700~ox37ff */
    {0x3701, 0x0f},
    {0x3737, 0x02},
    {0x3741, 0x04},
    {0x373c, 0x0f},
    {0x373b, 0x02},
    {0x3705, 0x00},
    {0x3706, 0x50},
    {0x370a, 0x00},
    {0x370b, 0xe4},
    {0x3709, 0x4a},
    {0x3714, 0x21},
    {0x371c, 0x00},
    {0x371d, 0x08},
    {0x375e, 0x0e},
    {0x3760, 0x13},
    {0x3776, 0x10},
    {0x3781, 0x02},
    {0x3782, 0x04},
    {0x3783, 0x02},
    {0x3784, 0x08},
    {0x3785, 0x08},
    {0x3788, 0x01},
    {0x3789, 0x01},
    {0x3797, 0x84},
    {0x3798, 0x01},
    {0x3799, 0x00},
    {0x3761, 0x02},
    {0x3762, 0x0d},
    /* timing control */
    {0x3800, 0x01},  //x_addr_start_h3
    {0x3801, 0x40},  //x_addr start l8
    {0x3802, 0x00},  //y_addr_start_h3
    {0x3803, 0xd8},  //y_addr_start_l8
    {0x3804, 0x09},  //x_addr_end_h3
    {0x3805, 0x40},  //x_addr_end_l8
    {0x3806, 0x06},  //y_addr_end_h3
    {0x3807, 0xd8},  //y_addr_end_l8
    {0x3808, 0x08},
    {0x3809, 0x00},  //x_output_size
    {0x380a, 0x06},
    {0x380b, 0x00},  //y_output_size
    {0x380c, 0x03},
    {0x380d, 0xf0},  //HTS 0x3f0    1008
    {0x380e, 0x09},
    {0x380f, 0x4c},  //VTS 0x94c    2380
    {0x3813, 0x04},  //isp_y_win_l8 ISP vertical windowing offset
    {0x3814, 0x01},  //l4:x_odd_inc
    {0x3815, 0x01},  //l4:x_even_inc
    {0x3816, 0x01},  //l4:y_odd_inc
    {0x3817, 0x01},  //l4:y_even_inc
    {0x381c, 0x00},
    {0x3820, 0x00},  /* [2]flip; [1]vertical mono binning; [0]vertical binning */
    {0x3821, 0x04},  /* [2]mirror; [1]horizontal mono binning; [0]horizontal binning */
    {0x3823, 0x18},
    {0x3826, 0x00},
    {0x3827, 0x01},
    {0x3832, 0x02},
    {0x383c, 0x48},
    {0x383d, 0xff},
    {0x3843, 0x20},
    {0x382d, 0x08},
    /* OTP control */
    {0x3d85, 0x0b},
    {0x3d84, 0x40},
    {0x3d8c, 0x63},
    {0x3d8d, 0x00},
    /* BLC */
    {0x4000, 0x78},
    {0x4001, 0x2b},
    {0x4005, 0x40},
    {0x4028, 0x2f},
    {0x400a, 0x01},
    {0x4010, 0x12},
    {0x4008, 0x02},
    {0x4009, 0x0d},  //blc control
    {0x401a, 0x58},
    {0x4050, 0x00},
    {0x4051, 0x01},
    {0x4052, 0x00},
    {0x4053, 0x80},
    {0x4054, 0x00},
    {0x4055, 0x80},
    {0x4056, 0x00},
    {0x4057, 0x80},
    {0x4058, 0x00},
    {0x4059, 0x80},
    {0x430b, 0xff},
    {0x430c, 0xff},
    {0x430d, 0x00},
    {0x430e, 0x00},
    /* CADC sync */
    {0x4501, 0x18},  //CADC sync
    {0x4502, 0x00},
    {0x4643, 0x00},
    {0x4640, 0x01},
    {0x4641, 0x04},
    /* MIPI control */
    {0x480e, 0x00},
    {0x4813, 0x00},
    {0x4815, 0x2b},
    {0x486e, 0x36},
    {0x486f, 0x84},
    {0x4860, 0x00},
    {0x4861, 0xa0},
    {0x484b, 0x05},
    {0x4850, 0x00},
    {0x4851, 0xaa},
    {0x4852, 0xff},
    {0x4853, 0x8a},
    {0x4854, 0x08},
    {0x4855, 0x30},
    {0x4800, 0x00},
    {0x4837, 0x0f},  //pclk_period
    {0x484a, 0x3f},
    /* ISP control */
    {0x5000, 0xc9},
    {0x5001, 0x43},
    {0x5002, 0x00},
    /* DPC */
    {0x5211, 0x03},
    {0x5291, 0x03},
    {0x520d, 0x0f},
    {0x520e, 0xfd},
    {0x520f, 0xa5},
    {0x5210, 0xa5},
    {0x528d, 0x0f},
    {0x528e, 0xfd},
    {0x528f, 0xa5},
    {0x5290, 0xa5},
    {0x5004, 0x40},  /* ISP CTRL4*/
    {0x5005, 0x00},  /* ISP CTRL5*/
    /* OTP */
    {0x5180, 0x00},
    {0x5181, 0x10},
    {0x5182, 0x0f},
    {0x5183, 0xff},
    {0x580b, 0x03},  /* REG0B */
    /* temperature sensor */
    {0x4d00, 0x03},
    {0x4d01, 0xe9},
    {0x4d02, 0xba},
    {0x4d03, 0x66},
    {0x4d04, 0x46},
    {0x4d05, 0xa5},
    {0x3603, 0x3c},
    /* sensor timing control */
    {0x3703, 0x26},
    {0x3709, 0x49},
    {0x3708, 0x2d},
    {0x3719, 0x1c},
    {0x371a, 0x06},
    {0x4000, 0x79},
    {0x380c, 0x05},
    {0x380d, 0xa0},
    {0x380e, 0x09},
    {0x380f, 0xc4},
    {0x3501, 0x09},
    {0x3502, 0xbc},
    {0x0100, 0x01},
    {0x0100, 0x01},
    {0x0100, 0x01},
    {0x0100, 0x01},
    {OS05A20_REG_END, 0x00},
};
#endif

static struct regval_list os05a20_regs_stream_on[] = {
    {0x0100,0x01},
    {OS05A20_REG_END, 0x0},
};

static struct regval_list os05a20_regs_stream_off[] = {
    {0x0100,0x00},
    {OS05A20_REG_END, 0x0},
};

static int os05a20_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
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
        printk(KERN_ERR "os05a20: failed to write reg: %x\n", (int)reg);
        printk(KERN_ERR "ret =  %d\n", ret);
    }

    return ret;
}

static int os05a20_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
        printk(KERN_ERR "os05a20(%x): failed to read reg: %x\n", i2c->addr, (int)reg);
        printk(KERN_ERR "ret =  %d\n\n", ret);
    }

    return ret;
}

static int os05a20_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != OS05A20_REG_END) {
        if (vals->reg_num == OS05A20_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = os05a20_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int os05a20_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char m = 1;
    unsigned char l = 1;
    int ret;

    ret = os05a20_read(i2c, OS05A20_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != OS05A20_CHIP_ID_H) {
        printk(KERN_ERR "os05a20 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = os05a20_read(i2c, OS05A20_REG_CHIP_ID_MEDIUM, &m);
    if (ret < 0)
        return ret;
    if (m != OS05A20_CHIP_ID_M) {
        printk(KERN_ERR "os05a20 read chip id medium failed:0x%x\n", m);
        return -ENODEV;
    }

    ret = os05a20_read(i2c, OS05A20_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;
    if (l != OS05A20_CHIP_ID_L) {
        printk(KERN_ERR "os05a20 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }

    printk(KERN_DEBUG "os05a20 get chip id = %02x%02x%02x\n", h, m, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "os05a20_reset");
        if (ret) {
            printk(KERN_ERR "os05a20: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "os05a20_pwdn");
        if (ret) {
            printk(KERN_ERR "os05a20: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "os05a20_power");
        if (ret) {
            printk(KERN_ERR "os05a20: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        os05a20_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(os05a20_regulator)) {
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
    if (pwdn_gpio != 1)
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

    if (os05a20_regulator)
        regulator_put(os05a20_regulator);
}

static void os05a20_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 0);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (os05a20_regulator)
        regulator_disable(os05a20_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int os05a20_power_on(void)
{
    int ret;
    int retry;

    camera_enable_sensor_mclk(cam_bus_num, 24 * 1000 * 1000);

    if (os05a20_regulator) {
        regulator_enable(os05a20_regulator);
        regulator_disable(os05a20_regulator);
        m_msleep(5);
        regulator_enable(os05a20_regulator);
        m_msleep(20);
    }

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 0);
        m_msleep(5);
        gpio_direction_output(power_gpio, 1);
        m_msleep(20);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(5);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(30);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 0);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(30);
    }

    for (retry = 0; retry < 3; retry++) {
        ret = os05a20_detect(i2c_dev);
        m_msleep(50);
        if (!ret)
            break;
    }
    if (retry >= 3) {
        printk(KERN_ERR "os05a20: failed to detect\n");
        os05a20_power_off();
        return ret;
    }

    ret = os05a20_write_array(i2c_dev, os05a20_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "os05a20: failed to init regs\n");
        os05a20_power_off();
        return ret;
    }

    return 0;
}

static int os05a20_stream_on(void)
{
    int ret = os05a20_write_array(i2c_dev, os05a20_regs_stream_on);
    if (ret)
        printk(KERN_ERR "os05a20: failed to stream on\n");

    return ret;
}

static void os05a20_stream_off(void)
{
    int ret = os05a20_write_array(i2c_dev, os05a20_regs_stream_off);
    if (ret)
        printk(KERN_ERR "os05a20: failed to stream off\n");
}

static int os05a20_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = os05a20_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int os05a20_s_register(struct sensor_dbg_register *reg)
{
    return os05a20_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}


static int os05a20_set_integration_time(int value)
{
    int ret = 0;
    //0x3661 choose long expo, so here choose long expo too.
    ret  = os05a20_write(i2c_dev, 0x3502, (unsigned char)(value & 0xff));
    ret += os05a20_write(i2c_dev, 0x3501, (unsigned char)((value & 0xff00) >> 8));

    if (ret < 0) {
        printk(KERN_ERR "os05a20: set integration time error. line=%d\n" ,__LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int os05a20_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = os05a20_again_lut;

    while (lut->gain <= os05a20_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = lut[0].index;
            return lut[0].gain;
        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut-1)->index;
            return (lut-1)->gain;
        } else {
            if ((lut->gain == os05a20_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

static int os05a20_set_analog_gain(int value)
{
    int ret = 0;
    struct again_lut *val_lut = os05a20_again_lut;

    ret  = os05a20_write(i2c_dev, 0x3508, val_lut[value].reg3508);
    ret += os05a20_write(i2c_dev, 0x3509, val_lut[value].reg3509);
    if (ret < 0) {
        printk(KERN_ERR "os05a20: set analog gain error.  line=%d\n", __LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int os05a20_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int os05a20_set_digital_gain(int value)
{
    return 0;
}

static int os05a20_set_fps(int fps)
{
    struct sensor_info *sensor_info = &os05a20_sensor_attr.sensor_info;
    unsigned int sclk = OS05A20_SUPPORT_SCLK_FPS_30;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "warn: os05a20 fps(0x%x) no in range\n", fps);
        return -1;
    }

    ret = os05a20_read(i2c_dev, 0x380c, &tmp);
    hts = tmp;
    ret += os05a20_read(i2c_dev, 0x380d, &tmp);
    if (ret < 0) {
        printk(KERN_ERR "err: os05a20_read err\n");
        return ret;
    }

    hts = ((hts << 8) + tmp) * 2;

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = os05a20_write(i2c_dev, 0x380e, (unsigned char)(vts & 0xff));
    ret += os05a20_write(i2c_dev, 0x380f, (unsigned char)(vts >> 8));
    if (ret < 0) {
        printk(KERN_ERR "err: os05a20_write err\n");
        return ret;
    }
    printk(KERN_ERR "hts: %x, vts: %x\n", hts, vts);

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}


static struct sensor_attr os05a20_sensor_attr = {
    .device_name                = OS05A20_DEVICE_NAME,
    .cbus_addr                  = OS05A20_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 300 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = os05a20_init_regs_2048_1536_30fps_mipi,
        .width                  = OS05A20_WIDTH,
        .height                 = OS05A20_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,
        .fps                    = 30 << 16 | 1,
        .total_width            = 0x3f0,
        .total_height           = 0x94c,

        .min_integration_time   = 2,
        .max_integration_time   = 0x94c-4,
        .max_again              = 259138,
        .max_dgain              = 0,
    },

    .ops  = {
        .power_on               = os05a20_power_on,
        .power_off              = os05a20_power_off,
        .stream_on              = os05a20_stream_on,
        .stream_off             = os05a20_stream_off,
        .get_register           = os05a20_g_register,
        .set_register           = os05a20_s_register,

        .set_integration_time   = os05a20_set_integration_time,
        .alloc_again            = os05a20_alloc_again,
        .set_analog_gain        = os05a20_set_analog_gain,
        .alloc_dgain            = os05a20_alloc_dgain,
        .set_digital_gain       = os05a20_set_digital_gain,
        .set_fps                = os05a20_set_fps,
    },
};

static int sensor_os05a20_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &os05a20_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sensor_os05a20_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &os05a20_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id os05a20_id_table[] = {
    { OS05A20_DEVICE_NAME, 0 },
    {},
};

static struct i2c_driver os05a20_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = OS05A20_DEVICE_NAME,
    },
    .probe          = sensor_os05a20_probe,
    .remove         = sensor_os05a20_remove,
    .id_table       = os05a20_id_table,
};

static struct i2c_board_info sensor_os05a20_info = {
    .type = OS05A20_DEVICE_NAME,
    .addr = OS05A20_DEVICE_I2C_ADDR,
};

static __init int os05a20_sensor_init(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "os05a20: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    os05a20_driver.driver.name = sensor_name;
    strcpy(os05a20_id_table[0].name, sensor_name);
    strcpy(sensor_os05a20_info.type, sensor_name);
    os05a20_sensor_attr.device_name = sensor_name;
    if (i2c_addr != -1)
        sensor_os05a20_info.addr = i2c_addr;

    int ret = i2c_add_driver(&os05a20_driver);
    if (ret) {
        printk(KERN_ERR "os05a20: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_os05a20_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "os05a20: failed to register i2c device\n");
        i2c_del_driver(&os05a20_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void os05a20_sensor_exit(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&os05a20_driver);
}

module_init(os05a20_sensor_init);
module_exit(os05a20_sensor_exit);

MODULE_DESCRIPTION("x2000 os05a20 driver");
MODULE_LICENSE("GPL");
