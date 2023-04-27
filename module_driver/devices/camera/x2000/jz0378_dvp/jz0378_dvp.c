/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * JZ0378
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>
#include <camera/hal/dvp_gpio_func.h>

#define JZ0378_DEVICE_NAME              "jz0378"
#define JZ0378_DEVICE_I2C_ADDR          0x6e
#define JZ0378_CHIP_ID_H                0x30
#define JZ0378_CHIP_ID_L                0x63

#define JZ0378_REG_END                  0xffff
#define JZ0378_REG_DELAY                0xfffe
#define JZ0378_PAGE_SELECT              0xfe

#define JZ0378_SUPPORT_SCLK             (24000000)

static int power_gpio   = -1;       //PA10
static int reset_gpio   = -1;       //PA09
static int pwdn_gpio    = -1;       //PA11
static int i2c_bus_num  = -1;       //3
static int dvp_gpio_func = -1;      //low 8
static int camera_index = 0;

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param(i2c_bus_num, int, 0644);
module_param(camera_index, int, 0644);

static char *dvp_gpio_func_str = "DVP_PA_LOW_8BIT";
module_param(dvp_gpio_func_str, charp, 0644);
MODULE_PARM_DESC(dvp_gpio_func_str, "Sensor GPIO function");

static char *resolution = "JZ0378_640_480_100FPS";
module_param(resolution, charp, 0644);
MODULE_PARM_DESC(resolution, "jz0378_resolution");

static struct i2c_client *i2c_dev;


static char *resolution_array[] = {
    "JZ0378_640_480_100FPS",
    "JZ0378_400_336_100FPS",
};

static struct sensor_attr jz0378_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

static struct regval_list jz0378_init_regs_640_480_dvp_100fps[] = {
#if 0
    // YUYV mclk 45M 100fps
    {0xf2, 0x01},
    {JZ0378_REG_DELAY,100},
    {0xf0, 0x00},
    {0xf1, 0x01},
    {0xfe, 0x01},
    {0xe0, 0x62},
    {0xe1, 0x5B},
    {0xe2, 0x24},
    {0xe3, 0x1A},
    {0xe4, 0x66},
    {0xe5, 0x7f},
    {0xe6, 0x1A},
    {0xe7, 0x50},
    {0xfe, 0x01},
    {0xb2, 0x01},
    {0xb3, 0x33},
    {0xb0, 0x19},
    {0xb1, 0x14},
    {0x66, 0x38},
    {0x6a, 0x14},    //agian
    {0x6b, 0x00},    //shtter time h
    {0x6c, 0x36},    //shutter timr l
    {0xfe, 0x00},
    {0x00, 0x78},
    {0xc8, 0x80},
    {0x02, 0xd2},
    {0x03, 0x00},
    {0x04, 0x00},
    {0x05, 0x01},
    {0x07, 0x09},
    {0x0c, 0x33},
    {0x0d, 0x3b},
    {0x0e, 0x66},
    {0x0f, 0x6D},
    {0x1b, 0x26},
    {0x1e, 0x46},
    {0x20, 0x46},
    {0x22, 0x66},
    {0x24, 0x47},
    {0x25, 0xD8},
    {0x27, 0x46},
    {0x28, 0x48},
    {0x29, 0xCB},
    {0x2A, 0xDB},
    {0x2B, 0xCC},
    {0xfe, 0x00},
    {0x59, 0x00},
    {0x5a, 0x00},
    {0x5b, 0x00},
    {0x5d, 0x63},
    {0xfe, 0x01},
    {0x04, 0x46},
    {0x05, 0x46},
    {0x06, 0x46},
    {0x07, 0x46},
    {0xfe, 0x00},
    {0x7b, 0x3f},
    {0x7C, 0x00},
    {0x8f, 0x06},
    {0x80, 0x80},
    {0x81, 0x70},
    {0x82, 0x64},
    {0x83, 0x54},
    {0x84, 0x4a},
    {0x85, 0x42},
    {0x86, 0x3c},
    {0x87, 0x36},
    {0x88, 0x32},
    {0x89, 0x2e},
    {0x8a, 0x2a},
    {0x8b, 0x28},
    {0x8c, 0x24},
    {0x8d, 0x22},
    {0x8e, 0x20},
    {0xfe, 0x00},
    {0x98, 0x0f},
    {0x99, 0xcd},
    {0x9a, 0x4f},
    {0x9b, 0x2a},
    {0x9c, 0x46},
    {0x9d, 0x28},
    {0x9e, 0x0e},
    {0x9f, 0x0a},
    {0xa0, 0x08},
    {0xa3, 0x12},
    {0xa6, 0x0e},
    {0xa7, 0x90},
    {0xa8, 0x10},
    {0xfe, 0x01},
    {0x62, 0x70},
    {0x65, 0x2f},
    {0x66, 0x35},
    {0x67, 0x8a},
    {0x6e, 0xa1},
    {0x6f, 0x4f},
    {0x70, 0x25},
    {0x72, 0xda},
    {0x74, 0x01},   //0x0a  自动ae
    {0x76, 0x81},
    {0x77, 0xff},   //0x5d  步长
    {0x78, 0x04},
    {0x7b, 0x03},
    {0x7c, 0xc8},
    {0x7e, 0x06},
    {0x81, 0x20},
    {0x82, 0x40},
    {0x83, 0x42},
    {0x84, 0x50},
    {0x85, 0x70},
    {0xfe, 0x01},
    {0xb2, 0x09},
    {0xb3, 0x33},
    {0xa0, 0x28},
    {0xa1, 0x03},
    {0xa2, 0x08},
    {0xa3, 0x28},
    {0xa4, 0x08},
    {0xa5, 0x20},
    {0xa7, 0x19},
    {0xa8, 0x11},
    {0xa9, 0x18},
    {0xaa, 0x18},
    {0xab, 0x33},
    {0xad, 0x66},
    {0xae, 0x57},
    {0xb4, 0x00},
    {0xb5, 0x00},
    {0xb6, 0xd8},
    {0xb8, 0x0e},
    {0xb9, 0x13},
    {0xba, 0x0e},
    {0xbb, 0x13},
    {0xbc, 0x44},
    {0xbd, 0x22},
    {0xfe, 0x02},
    {0x2c, 0x80},
    {0x2d, 0x80},
    {0x2e, 0x86},
    {0x2f, 0x84},
    {0x30, 0x80},
    {0x31, 0x8d},
    {0x34, 0x85},
    {0x35, 0x03},
    {0x36, 0x90},
    {0x37, 0x90},
    {0x38, 0x88},
    {0x39, 0x92},
    {0xfe, 0x02},
    {0x32, 0x71},
    {0x50, 0xa0},
    {0x51, 0x88},
    {0x52, 0x14},
    {0x53, 0x98},
    {0x54, 0x98},
    {0x55, 0x50},
    {0x57, 0x0f},
    {0xf2,0x02},
#else
    {0xf2, 0x01},
    {JZ0378_REG_DELAY,100},
    {0xf0, 0x00},
    {0xf1, 0x28},
    {0xfe, 0x01},
    {0xe0, 0x62},
    {0xe1, 0x5B},
    {0xe2, 0x6c},
    {0xe3, 0x1A},
    {0xe4, 0x66},
    {0xe5, 0x70},   // 0x7f
    {0xe6, 0x1A},
    {0xe7, 0x50},
    {0xfe, 0x01},
    {0xb2, 0x00},
    {0xb3, 0x00},
    {0xb0, 0x00},
    {0xb1, 0x00},
    {0x66, 0x38},
    {0x6a, 0x14},    //agian
    {0x6b, 0x00},    //shtter time h
    {0x6c, 0x36},    //shutter timr l
    {0xfe, 0x00},
    {0x00, 0x78},
    {0xc8, 0x83},
    {0x02, 0xd2},
    {0x03, 0x00},
    {0x04, 0x00},
    {0x05, 0x01},
    {0x07, 0x09},
    {0x0c, 0x33},
    {0x0d, 0x3b},
    {0x0e, 0x66},
    {0x0f, 0x6D},
    {0x1b, 0x26},
    {0x1e, 0x46},
    {0x20, 0x46},
    {0x22, 0x66},
    {0x24, 0x47},
    {0x25, 0xD8},
    {0x27, 0x46},
    {0x28, 0x48},
    {0x29, 0xCB},
    {0x2A, 0xDB},
    {0x2B, 0xCC},
    {0xfe, 0x00},
    {0x59, 0x00},
    {0x5a, 0x00},
    {0x5b, 0x00},
    {0x5d, 0x63},
    {0xfe, 0x01},
    {0x04, 0x46},
    {0x05, 0x46},
    {0x06, 0x46},
    {0x07, 0x46},
    {0xfe, 0x00},
    {0x7b, 0x3f},
    {0x7c, 0x00},
    {0x8f, 0x06},
    {0x80, 0x60},
    {0x81, 0x58},
    {0x82, 0x4c},
    {0x83, 0x40},
    {0x84, 0x3b},
    {0x85, 0x36},
    {0x86, 0x32},
    {0x87, 0x2f},
    {0x88, 0x2d},
    {0x89, 0x2b},
    {0x8a, 0x29},
    {0x8b, 0x27},
    {0x8c, 0x24},
    {0x8d, 0x22},
    {0x8e, 0x20},
    {0xfe, 0x00},
    {0x98, 0x07},   //0x0f
    {0x99, 0xcd},
    {0x9a, 0x4f},
    {0x9b, 0x4c},
    {0x9c, 0x06},
    {0x9d, 0xa8},
    {0x9e, 0x0e},
    {0x9f, 0x0a},
    {0xa0, 0x08},
    {0xa2, 0xf4},
    {0xa3, 0x12},
    {0xa6, 0x1e},
    {0xa7, 0x80},
    {0xa8, 0x10},
    {0xfe, 0x01},
    {0x62, 0x70},
    {0x65, 0x2f},
    {0x66, 0x05},
    {0x67, 0xbf},
    {0x6e, 0xa1},
    {0x6f, 0x48},
    {0x70, 0x25},
    {0x72, 0xda},
    {0x74, 0x01},   //0x0a  自动ae
    {0x76, 0x81},
    {0x77, 0xff},   //0x5d  步长
    {0x78, 0x74},
    {0x7b, 0x03},
    {0x7c, 0xc8},
    {0x7e, 0x06},
    {0x81, 0x14},
    {0x82, 0x28},
    {0x83, 0x28},
    {0x84, 0x38},
    {0x85, 0x58},
    {0xfe, 0x02},
    {0x32, 0x71},
    {0x50, 0xa0},
    {0x55, 0x60},   //0x50
    {0x57, 0x0f},
    {0xf2,0x02},
#endif
    {JZ0378_REG_END, 0x00},    /* END MARKER */
};


static struct regval_list jz0378_init_regs_336_400_dvp_100fps[] = {

    {0xf2, 0x01},
    {JZ0378_REG_DELAY,100},
    {0xf0, 0x00},
    {0xf1, 0x28},
    {0xfe, 0x01},
    {0xe0, 0x62},
    {0xe1, 0x5B},
    {0xe2, 0x6c},
    {0xe3, 0x1A},
    {0xe4, 0x66},
    {0xe5, 0x70},   //0x7f
    {0xe6, 0x1A},
    {0xe7, 0x50},
    {0xfe, 0x01},
    {0xb2, 0x00},
    {0xb3, 0x00},
    {0xb0, 0x00},
    {0xb1, 0x00},
    {0x66, 0x38},
    {0x6a, 0x14},    //agian
    {0x6b, 0x00},    //shtter time h
    {0x6c, 0x36},    //shutter timr l
    {0xfe, 0x00},
    {0x00, 0x78},
    {0xc8, 0x83},
    {0x02, 0xd2},
    {0x03, 0x00},
    {0x04, 0x00},
    {0x05, 0x01},
    {0x07, 0x09},
    {0x0c, 0x33},
    {0x0d, 0x3b},
    {0x0e, 0x66},
    {0x0f, 0x6D},
    {0x1b, 0x26},
    {0x1e, 0x46},
    {0x20, 0x46},
    {0x22, 0x66},
    {0x24, 0x47},
    {0x25, 0xD8},
    {0x27, 0x46},
    {0x28, 0x48},
    {0x29, 0xCB},
    {0x2A, 0xDB},
    {0x2B, 0xCC},
    {0xfe, 0x00},
    {0x59, 0x00},
    {0x5a, 0x00},
    {0x5b, 0x00},
    {0x5d, 0x63},
    {0xfe, 0x01},
    {0x04, 0x46},
    {0x05, 0x46},
    {0x06, 0x46},
    {0x07, 0x46},
    {0xfe, 0x00},
    {0x7b, 0x3f},
    {0x7c, 0x00},
    {0x8f, 0x06},
    {0x80, 0x60},
    {0x81, 0x58},
    {0x82, 0x4c},
    {0x83, 0x40},
    {0x84, 0x3b},
    {0x85, 0x36},
    {0x86, 0x32},
    {0x87, 0x2f},
    {0x88, 0x2d},
    {0x89, 0x2b},
    {0x8a, 0x29},
    {0x8b, 0x27},
    {0x8c, 0x24},
    {0x8d, 0x22},
    {0x8e, 0x20},
    {0xfe, 0x00},
    {0x98, 0x07},   //0x0f
    {0x99, 0xcd},
    {0x9a, 0x4f},
    {0x9b, 0xcf},   //0x4c
    {0x9c, 0x06},
    {0x9d, 0xb8},   //0xa8
    {0x9e, 0x0e},
    {0x9f, 0x0a},
    {0xa0, 0xcf},   //0x08
    {0xa2, 0xf4},
    {0xa3, 0x12},
    {0xa6, 0x1e},
    {0xa7, 0x80},
    {0xa8, 0xf0},   //0x10
    {0xfe, 0x01},
    {0x62, 0x70},
    {0x65, 0x2f},
    {0x66, 0x05},
    {0x67, 0xbf},
    {0x6e, 0xa1},
    {0x6f, 0x48},
    {0x70, 0x25},
    {0x72, 0xda},
    {0x74, 0x01},   //0x0a  自动ae
    {0x76, 0x81},
    {0x77, 0xff},   //0x5d  步长
    {0x78, 0x74},
    {0x7b, 0x03},
    {0x7c, 0xc8},
    {0x7e, 0x06},
    {0x81, 0x14},
    {0x82, 0x28},
    {0x83, 0x28},
    {0x84, 0x38},
    {0x85, 0x58},
    {0xfe, 0x02},
    {0x32, 0x71},
    {0x50, 0xa0},
    {0x55, 0x78},   //0x60
    {0x57, 0x0f},
    //window  value * 4
    // width 400
    //height 336
    {0xfe,0x00},
    {0xca,0x13},    //x start    76
    {0xcb,0x77},    //x stop     476
    {0xcc,0x10},    //y start    64
    {0xcd,0x64},    //y stop     400
    {0xf2,0x02},

    {JZ0378_REG_END, 0x00},    /* END MARKER */
};


static struct regval_list jz0378_regs_stream_on[] = {

    {0xf2 , 0x00},
    {JZ0378_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list jz0378_regs_stream_off[] = {

    {0xf2 , 0x02},
    {JZ0378_REG_END, 0x00},    /* END MARKER */
};


static int jz0378_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
{
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = 2,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0) {
        printk(KERN_ERR "jz0378: failed to write reg: %x\n", (int)reg);
    }

    return ret;
}

static int jz0378_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
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
    if (ret < 0) {
        printk(KERN_ERR "jz0378(%x): failed to read reg: %x\n", i2c->addr, (int)reg);
    }

    return ret;
}

static int jz0378_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != JZ0378_REG_END) {
        if (vals->reg_num == JZ0378_REG_DELAY) {
            m_msleep(vals->value);
        }else {
            ret = jz0378_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        // printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n", vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static inline int jz0378_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != JZ0378_REG_END) {
        if (vals->reg_num == JZ0378_REG_DELAY) {
            m_msleep(vals->value);
        }else if(vals->reg_num == JZ0378_PAGE_SELECT){
            ret = jz0378_write(i2c, vals->reg_num, vals->value);
        } else {
            ret = jz0378_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "vals->reg_num:0x%x, vals->value:0x%x\n", vals->reg_num, val);
        vals++;
    }

    return 0;
}

static int jz0378_detect(struct i2c_client *i2c)
{
    unsigned char val = 0;
    int ret;

    ret = jz0378_read(i2c, 0xfc, &val);
    if (ret < 0)
        return ret;
    if (val != JZ0378_CHIP_ID_H)
        return -ENODEV;

    ret = jz0378_read(i2c, 0xfd, &val);
    if (ret < 0)
        return ret;
    if (val != JZ0378_CHIP_ID_L)
        return -ENODEV;

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "jz0378_reset");
        if (ret) {
            printk(KERN_ERR "jz0378: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "jz0378_pwdn");
        if (ret) {
            printk(KERN_ERR "jz0378: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "jz0378_power");
        if (ret) {
            printk(KERN_ERR "jz0378: failed to request pwdn pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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
    dvp_deinit_gpio();

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

    dvp_deinit_gpio();
}

static void jz0378_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk(camera_index);
}

extern uint32_t tisp_log2_fixed_to_fixed(const uint32_t val, const int in_fix_point, const uint8_t out_fix_point);
static int jz0378_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(camera_index, 45 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 1);
        m_msleep(5);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(30);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(50);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
    }

    ret = jz0378_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "jz0378: failed to detect\n");
        jz0378_power_off();
        return ret;
    }
    ret = jz0378_write_array(i2c_dev, jz0378_sensor_attr.sensor_info.private_init_setting);
    // ret += jz0378_read_array(i2c_dev,jz0378_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "jz0378: failed to init regs\n");
        jz0378_power_off();
        return ret;
    }

    return 0;
}

static int jz0378_stream_on(void)
{
    int ret = jz0378_write_array(i2c_dev, jz0378_regs_stream_on);
    if (ret)
        printk(KERN_ERR "jz0378: failed to stream on\n");

    return ret;
}

static void jz0378_stream_off(void)
{
    int ret = jz0378_write_array(i2c_dev, jz0378_regs_stream_off);
    if (ret)
        printk(KERN_ERR "jz0378: failed to stream on\n");

}

static int jz0378_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = jz0378_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int jz0378_s_register(struct sensor_dbg_register *reg)
{
    return jz0378_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static int jz0378_set_fps(int fps)
{
    return 0;
}

int jz0378_get_hflip(camera_ops_mode *enable)
{
    unsigned char value;
    int ret;

    ret = jz0378_write(i2c_dev, 0xfe, 0x0);
    ret += jz0378_read(i2c_dev, 0x0, &value);
    if (ret < 0) {
        printk(KERN_ERR "failed to %s\n", __func__);
        return -1;
    }
    *enable = (value >> 5) & 0x1;

    return 0;
}

int jz0378_set_hflip(camera_ops_mode enable)
{
    unsigned char value, tmp;
    int ret, try = 20;

    ret = jz0378_write(i2c_dev, 0xfe, 0x0);
    ret += jz0378_read(i2c_dev, 0x0, &value);

    if(enable)
        value |= 0x20;
    else
        value &= 0xdf;
    ret += jz0378_write(i2c_dev, 0x0, value);
    if (ret < 0) {
        printk(KERN_ERR "failed to %s\n", __func__);
        return -1;
    }

    //等待生效，为了防止其他写0x0寄存器的api被调用导致该api不生效
    while (try--) {
        jz0378_read(i2c_dev, 0x0, &tmp);
        if (tmp != value)
            m_msleep(1);
        else
            break;
    }

    return 0;
}

int jz0378_get_vflip(camera_ops_mode *enable)
{
    unsigned char value;
    int ret;

    ret = jz0378_write(i2c_dev, 0xfe, 0x0);
    ret += jz0378_read(i2c_dev, 0x0, &value);
    if (ret < 0) {
        printk(KERN_ERR "failed to %s\n", __func__);
        return -1;
    }
    *enable = (value >> 4) & 0x1;

    return 0;
}

int jz0378_set_vflip(camera_ops_mode enable)
{
    unsigned char value, tmp;
    int ret, try = 20;

    ret = jz0378_write(i2c_dev, 0xfe, 0x0);
    ret += jz0378_read(i2c_dev, 0x0, &value);

    if(enable)
        value |= 0x10;
    else
        value &= 0xef;
    ret += jz0378_write(i2c_dev, 0x0, value);
    if (ret < 0) {
        printk(KERN_ERR "failed to %s\n", __func__);
        return -1;
    }

    //等待生效，为了防止其他写0x0寄存器的api被调用导致该api不生效
    while (try--) {
        jz0378_read(i2c_dev, 0x0, &tmp);
        if (tmp != value)
            m_msleep(1);
        else
            break;
    }

    return 0;
}

static struct sensor_info jz0378_sensor_info[] = {
    {
        .private_init_setting   = jz0378_init_regs_640_480_dvp_100fps,
        .width                  = 640,
        .height                 = 480,
        .fmt                    = SENSOR_PIXEL_FMT_Y8_1X8,

        .fps                    = 100 << 16 | 1,
        .total_width            = 860,
        .total_height           = 510,

        .min_integration_time   = 4,
        .max_integration_time   = 510,
        .max_again              = 262144,
        .max_dgain              = 0,
    },

    {
        .private_init_setting   = jz0378_init_regs_336_400_dvp_100fps,
        .width                  = 400,
        .height                 = 336,
        .fmt                    = SENSOR_PIXEL_FMT_Y8_1X8,

        .fps                    = 100 << 16 | 1,
        .total_width            = 860,
        .total_height           = 510,

        .min_integration_time   = 4,
        .max_integration_time   = 510-2,
        .max_again              = 262144,
        .max_dgain              = 0,
    },
};

static struct sensor_attr jz0378_sensor_attr = {
    .device_name        = JZ0378_DEVICE_NAME,
    .cbus_addr          = JZ0378_DEVICE_I2C_ADDR,

    .dma_mode           = SENSOR_DATA_DMA_MODE_GREY,    // olny dvp.data_fmt is DVP_YUV422
    .dbus_type          = SENSOR_DATA_BUS_DVP,
    .dvp = {
        .data_fmt       = DVP_RAW8,
        .gpio_mode      = DVP_PA_LOW_8BIT,
        .timing_mode    = DVP_HREF_MODE,
        .yuv_data_order = order_1_2_3_4,
        .pclk_polarity  = POLARITY_SAMPLE_FALLING,
        .hsync_polarity = POLARITY_HIGH_ACTIVE,
        .vsync_polarity = POLARITY_HIGH_ACTIVE,
        .img_scan_mode  = DVP_IMG_SCAN_PROGRESS,
    },

    .isp_clk_rate       = 150 * 1000 * 1000,

    .ops = {
        .power_on               = jz0378_power_on,
        .power_off              = jz0378_power_off,
        .stream_on              = jz0378_stream_on,
        .stream_off             = jz0378_stream_off,
        .get_register           = jz0378_g_register,
        .set_register           = jz0378_s_register,

        .set_fps                = jz0378_set_fps,
        .get_hflip              = jz0378_get_hflip,
        .set_hflip              = jz0378_set_hflip,
        .get_vflip              = jz0378_get_vflip,
        .set_vflip              = jz0378_set_vflip,
    },
};

static int jz0378_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(dvp_gpio_func_array); i++) {
        if (!strcmp(dvp_gpio_func_str, dvp_gpio_func_array[i])) {
            dvp_gpio_func = i;
            break;
        }
    }

    if (i == ARRAY_SIZE(dvp_gpio_func_array)){
        printk(KERN_ERR "sensor dvp_gpio_func set error!\n");
        return -EINVAL;
    }

    int ret = init_gpio();
    if (ret)
        goto err_init_gpio;

    ret = dvp_init_select_gpio(&jz0378_sensor_attr.dvp,dvp_gpio_func);
    if (ret)
        goto err_dvp_select_gpio;

    /* 确定根据传入参数sensor info */
    for (i = 0; i < ARRAY_SIZE(resolution_array); i++) {
        if (!strcmp(resolution, resolution_array[i])) {
            memcpy(&jz0378_sensor_attr.sensor_info, &jz0378_sensor_info[i], sizeof(struct sensor_info));
            break;
        }
    }

    if (i == ARRAY_SIZE(resolution_array)) {
        /* use default sensor info */
        memcpy(&jz0378_sensor_attr.sensor_info, &jz0378_sensor_info[0], sizeof(struct sensor_info));
        printk(KERN_ERR "Match %s Falied! Used Resolution: %d x %d.\n", \
                        resolution,                                     \
                        jz0378_sensor_attr.sensor_info.width,           \
                        jz0378_sensor_attr.sensor_info.height);
    }

    ret = camera_register_sensor(camera_index, &jz0378_sensor_attr);
    if (ret)
        goto err_camera_register;

    return 0;

err_camera_register:
    dvp_deinit_gpio();
err_dvp_select_gpio:
    deinit_gpio();

err_init_gpio:

    return ret;
}

static int jz0378_remove(struct i2c_client *client)
{
    camera_unregister_sensor(camera_index, &jz0378_sensor_attr);
    dvp_deinit_gpio();
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id jz0378_id[] = {
    { JZ0378_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, jz0378_id);

static struct i2c_driver jz0378_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = JZ0378_DEVICE_NAME,
    },
    .probe              = jz0378_probe,
    .remove             = jz0378_remove,
    .id_table           = jz0378_id,
};

static struct i2c_board_info sensor_jz0378_info = {
    .type               = JZ0378_DEVICE_NAME,
    .addr               = JZ0378_DEVICE_I2C_ADDR,
};

static __init int init_jz0378(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "jz0378: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&jz0378_driver);
    if (ret) {
        printk(KERN_ERR "jz0378: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_jz0378_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "jz0378: failed to register i2c device\n");
        i2c_del_driver(&jz0378_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_jz0378(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&jz0378_driver);
}

module_init(init_jz0378);
module_exit(exit_jz0378);

MODULE_DESCRIPTION("x2000 jz0378 driver");
MODULE_LICENSE("GPL");
