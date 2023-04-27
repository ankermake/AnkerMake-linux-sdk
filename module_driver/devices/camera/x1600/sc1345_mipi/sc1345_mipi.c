/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * SC1345
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera/camera_sensor.h>

#define SC1345_DEVICE_NAME              "sc1345-mipi"
#define SC1345_DEVICE_I2C_ADDR          0x30

#define SC1345_CHIP_ID_H                0xda
#define SC1345_CHIP_ID_L                0x23
#define SC1345_REG_CHIP_ID_HIGH         0x3107
#define SC1345_REG_CHIP_ID_LOW          0x3108

#define SC1345_REG_END                  0xffff
#define SC1345_REG_DELAY                0xfffe

static int power_gpio       = -1;
static int reset_gpio       = -1;
static int pwdn_gpio        = -1;
static int i2c_bus_num      = -1;

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr sc1345_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

/*
 * SensorName RAW_60fps
 * width=1280
 * height=720
 * SlaveID=0x60/0x66
 * mclk=24
 * avdd=2.800000
 * dovdd=1.800000
 * dvdd=1.500000
 */
static struct regval_list sc1345_init_regs_1280_780_60fps_mipi[] = {
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
    {SC1345_REG_DELAY, 0x17},
    {SC1345_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list sc1345_regs_stream_off[] = {
    {0x0100, 0x00},
    {SC1345_REG_END, 0x00},  /* END MARKER */
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
    if (ret < 0)
        printk(KERN_ERR "sc1345: failed to write reg: %x\n", (int)reg);

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
    if (ret < 0)
        printk(KERN_ERR "sc1345(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

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

        if (val == vals->value)
            printk(KERN_ERR "reg = 0x%02x, val = 0x%02x  == 0x%02x\n",vals->reg_num, val, vals->value);
        else
            printk(KERN_ERR "reg = 0x%02x, val = 0x%02x  != 0x%02x\n",vals->reg_num, val, vals->value);

        vals++;
    }
    return 0;
}

static int sc1345_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = sc1345_read(i2c, 0x3107, &h);
    if (ret < 0)
        return ret;
    if (h != SC1345_CHIP_ID_H) {
        printk(KERN_ERR "sc1345 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = sc1345_read(i2c, 0x3108, &l);
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

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "sc1345_power");
        if (ret) {
            printk(KERN_ERR "sc1345: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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

static void sc1345_power_off(void)
{
    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 0);

    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk();
}

static int sc1345_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
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

    ret = sc1345_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "sc1345: failed to detect\n");
        sc1345_power_off();
        return ret;
    }

    ret = sc1345_write_array(i2c_dev, sc1345_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "sc1345: failed to init regs\n");
        sc1345_power_off();
        return ret;
    }

    return 0;
}

static int sc1345_stream_on(void)
{
    int ret;

    ret = sc1345_write_array(i2c_dev, sc1345_regs_stream_on);
    if (ret)
        printk(KERN_ERR "sc1345: failed to stream on\n");

    return ret;
}

static void sc1345_stream_off(void)
{
    int ret = sc1345_write_array(i2c_dev, sc1345_regs_stream_off);
    if (ret)
        printk(KERN_ERR "sc1345: failed to stream on\n");
}

static int sc1345_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = sc1345_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 1;
    return ret;
}

static int sc1345_s_register(struct sensor_dbg_register *reg)
{
    return sc1345_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}


static struct sensor_attr sc1345_sensor_attr = {
    .device_name                = SC1345_DEVICE_NAME,
    .cbus_addr                  = SC1345_DEVICE_I2C_ADDR,

    .dma_mode                   = SENSOR_DATA_DMA_MODE_GREY, /* 是否单独提取Y数据 */

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .lanes                  = 1,
        .clk                    = 600 * 1000 * 1000,  /* ns */
    },

    .sensor_info = {
        .private_init_setting   = sc1345_init_regs_1280_780_60fps_mipi,
        .width                  = 1280,
        .height                 = 720,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR8_1X8,

        .fps                    = 60 << 16 | 1,
    },

    .ops = {
        .power_on               = sc1345_power_on,
        .power_off              = sc1345_power_off,
        .stream_on              = sc1345_stream_on,
        .stream_off             = sc1345_stream_off,

        .get_register           = sc1345_g_register,
        .set_register           = sc1345_s_register,
    },
};


static int sc1345_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int ret;

    ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(&sc1345_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sc1345_remove(struct i2c_client *client)
{
    camera_unregister_sensor(&sc1345_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id sc1345_id[] = {
    { SC1345_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sc1345_id);

static struct i2c_driver sc1345_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = SC1345_DEVICE_NAME,
    },
    .probe              = sc1345_probe,
    .remove             = sc1345_remove,
    .id_table           = sc1345_id,
};

static struct i2c_board_info sensor_sc1345_info = {
    .type               = SC1345_DEVICE_NAME,
    .addr               = SC1345_DEVICE_I2C_ADDR,
};

static __init int init_sc1345(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "sc1345: i2c_bus_num must be set\n");
        return -EINVAL;
    }

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

static __exit void exit_sc1345(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sc1345_driver);
}

module_init(init_sc1345);
module_exit(exit_sc1345);

MODULE_DESCRIPTION("x1600 sc1345 mipi driver");
MODULE_LICENSE("GPL");