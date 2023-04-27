#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <common.h>
#include <vic_sensor.h>

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

#define OV9732_REG_END                  0xffff
#define OV9732_REG_DELAY                0xfffe

static int reset_gpio = -1; // GPIO_PA(10);
static int power_gpio = -1; // GPIO_PA(11);
static int i2c_bus_num = -1; // 0
static int i2c_addr = 0x36; // 0x36

module_param_gpio(reset_gpio, 0644);
module_param_gpio(power_gpio, 0644);

module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, int, 0644);

static struct i2c_client *i2c_dev;

static int ov9732_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
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
        printk(KERN_ERR "ov9732: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int ov9732_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
        printk(KERN_ERR "ov9732: failed to read reg: %x\n", (int)reg);

    return ret;
}

void m_msleep(int ms)
{
    usleep_range(ms*1000, ms*1000);
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
        // printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static struct regval_list ov9732_init_regs_1280_720_25fps[] = {
    /*
      @@ DVP interface 1280*720 25fps
    */
    {0x0103, 0x01},
    {0x0100, 0x00},
    {0x3001, 0x3f},
    {0x3002, 0xff},
    {0x3007, 0x00},
#ifdef DRIVE_CAPABILITY_1
    {0x3009, 0x03},//pad driver
#elif defined(DRIVE_CAPABILITY_2)
    {0x3009,0x23},
#elif defined(DRIVE_CAPABILITY_3)
    {0x3009,0x43},
#elif defined(DRIVE_CAPABILITY_4)
    {0x3009,0x63},
#endif
    {0x3010, 0x00},
    {0x3011, 0x00},
    {0x3014, 0x36},
    {0x301e, 0x15},
    {0x3030, 0x09},
    {0x3080, 0x02},
    {0x3081, 0x3c},
    {0x3082, 0x04},
    {0x3083, 0x00},
    {0x3084, 0x02},
    {0x3085, 0x01},
    {0x3086, 0x01},
    {0x3089, 0x01},
    {0x308a, 0x00},
    {0x3103, 0x01},
    {0x3600, 0xf6},
    {0x3601, 0x72},
    {0x3610, 0x0c},
    {0x3611, 0xf0},
    {0x3612, 0x35},
    {0x3654, 0x10},
    {0x3655, 0x77},
    {0x3656, 0x77},
    {0x3657, 0x07},
    {0x3658, 0x22},
    {0x3659, 0x22},
    {0x365a, 0x02},
    {0x3700, 0x1f},
    {0x3701, 0x10},
    {0x3702, 0x0c},
    {0x3703, 0x07},
    {0x3704, 0x3c},
    {0x3705, 0x41},
    {0x370d, 0x10},
    {0x3710, 0x0c},
    {0x3783, 0x08},
    {0x3784, 0x05},
    {0x3785, 0x55},
    {0x37c0, 0x07},
    {0x3800, 0x00},
    {0x3801, 0x04},
    {0x3802, 0x00},
    {0x3803, 0x04},
    {0x3804, 0x05},
    {0x3805, 0x0b},
    {0x3806, 0x02},
    {0x3807, 0xdb},
    {0x3808, 0x05},  /* 1280 */
    {0x3809, 0x00},
    {0x380a, 0x02},  /* 720 */
    {0x380b, 0xd0},
    {0x380c, 0x05},
    {0x380d, 0xc6},
    {0x380e, 0x03},
    {0x380f, 0xc4}, /* 25fps */
    {0x3810, 0x00},
    {0x3811, 0x04},
    {0x3812, 0x00},
    {0x3813, 0x04},
    {0x3816, 0x00},
    {0x3817, 0x00},
    {0x3818, 0x00},
    {0x3819, 0x04},
    {0x3820, 0x10},
    {0x3821, 0x00},
    {0x382c, 0x06},
    {0x3500, 0x00},
    {0x3501, 0x31},
    {0x3502, 0x00},
    {0x3503, 0x3f},
    {0x3504, 0x00},
    {0x3505, 0x00},
    {0x3509, 0x10},
    {0x350a, 0x00},
    {0x350b, 0x40},
    {0x3d00, 0x00},
    {0x3d01, 0x00},
    {0x3d02, 0x00},
    {0x3d03, 0x00},
    {0x3d04, 0x00},
    {0x3d05, 0x00},
    {0x3d06, 0x00},
    {0x3d07, 0x00},
    {0x3d08, 0x00},
    {0x3d09, 0x00},
    {0x3d0a, 0x00},
    {0x3d0b, 0x00},
    {0x3d0c, 0x00},
    {0x3d0d, 0x00},
    {0x3d0e, 0x00},
    {0x3d0f, 0x00},
    {0x3d80, 0x00},
    {0x3d81, 0x00},
    {0x3d82, 0x38},
    {0x3d83, 0xa4},
    {0x3d84, 0x00},
    {0x3d85, 0x00},
    {0x3d86, 0x1f},
    {0x3d87, 0x03},
    {0x3d8b, 0x00},
    {0x3d8f, 0x00},
    {0x4001, 0xe0},//BLC
    {0x4004, 0x00},
    {0x4005, 0x02},
    {0x4006, 0x01},
    {0x4007, 0x40},
    {0x4009, 0x0b},
    {0x4300, 0x03},
    {0x4301, 0xff},
    {0x4304, 0x00},
    {0x4305, 0x00},
    {0x4309, 0x00},
    {0x4600, 0x00},
    {0x4601, 0x04},
    {0x4800, 0x04},
    {0x4805, 0x00},
    {0x4821, 0x3c},
    {0x4823, 0x3c},
    {0x4837, 0x2d},
    {0x4a00, 0x00},
    {0x4f00, 0x80},
    {0x4f01, 0x10},
    {0x4f02, 0x00},
    {0x4f03, 0x00},
    {0x4f04, 0x00},
    {0x4f05, 0x00},
    {0x4f06, 0x00},
    {0x4f07, 0x00},
    {0x4f08, 0x00},
    {0x4f09, 0x00},
    {0x5000, 0x0f},//eanble dpc
    {0x500c, 0x00},
    {0x500d, 0x00},
    {0x500e, 0x00},
    {0x500f, 0x00},
    {0x5010, 0x00},
    {0x5011, 0x00},
    {0x5012, 0x00},
    {0x5013, 0x00},
    {0x5014, 0x00},
    {0x5015, 0x00},
    {0x5016, 0x00},
    {0x5017, 0x00},
    {0x5080, 0x00}, //color bar
    {0x5180, 0x01},
    {0x5181, 0x00},
    {0x5182, 0x01},
    {0x5183, 0x00},
    {0x5184, 0x01},
    {0x5185, 0x00},
    {0x5708, 0x06},
    {0x5781, 0x00},
    {0x5782, 0x77},//decrease dpc strength
    {0x5783, 0x0f},
    {OV9732_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list ov9732_regs_stream_on[] = {
    {0x0100, 0x01},
    {OV9732_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list ov9732_regs_stream_off[] = {
    {0x0100, 0x00},
    {OV9732_REG_END, 0x00},    /* END MARKER */
};

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    ret = dvp_init_low10bit_gpio();
    if (ret) {
        printk(KERN_ERR "ov9732: failed to init dvp pins\n");
        return ret;
    }

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "ov9732_reset");
        if (ret) {
            printk(KERN_ERR "ov9732: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ov9732_power_gpio");
        if (ret) {
            printk(KERN_ERR "ov9732: failed to request pwdn pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    return 0;
err_power_gpio:
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

    if (reset_gpio != -1)
        gpio_free(reset_gpio);

    dvp_deinit_gpio();
}

static void ov9732_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    vic_disable_sensor_mclk();
}

static int ov9732_power_on(void)
{
    int ret;

    vic_enable_sensor_mclk(24 * 1000 * 1000);

    if (power_gpio != -1){
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 1);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(1);
    }

    unsigned char v1 = 0, v2 = 0;

    ov9732_read(i2c_dev, 0x300a, &v1);
    if (v1 != 0x97) {
        printk(KERN_ERR "ov9732: id1 not match: 0x%x", (int)v1);
        ov9732_power_off();
        return -ENODEV;
    }

    ov9732_read(i2c_dev, 0x300b, &v2);
    if (v2 != 0x32) {
        printk(KERN_ERR "ov9732: id2 not match: 0x%x", (int)v2);
        ov9732_power_off();
        return -ENODEV;
    }

    ret = ov9732_write_array(i2c_dev, ov9732_init_regs_1280_720_25fps);
    if (ret) {
        printk(KERN_ERR "ov9732: failed to init setting\n");
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


static struct vic_sensor_config ov9732_sensor_config = {
    .device_name            = "ov9732",

    .vic_interface          = VIC_dvp,
    .dvp_cfg_info = {
        .dvp_data_fmt       = DVP_RAW10,
        .dvp_gpio_mode      = DVP_PA_LOW_10BIT,
        .dvp_timing_mode    = DVP_href_mode,
        .dvp_yuv_data_order = 0,
        .dvp_hsync_polarity = POLARITY_HIGH_ACTIVE,
        .dvp_vsync_polarity = POLARITY_HIGH_ACTIVE,
        .dvp_img_scan_mode  = DVP_img_scan_progress,
    },

    .isp_clk_rate           = 90 * 1000 * 1000,

    .sensor_info = {
        .width              = 1280,
        .height             = 720,
        .fmt                = SENSOR_PIXEL_FMT_SBGGR10_1X10,
        .fps                = 25 << 16 | 1,
    },

    .ops = {
        .power_on           = ov9732_power_on,
        .power_off          = ov9732_power_off,
        .stream_on          = ov9732_stream_on,
        .stream_off         = ov9732_stream_off,
    },
};

static int sensor_ov9732_probe(struct i2c_client *client,
                 const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = vic_register_sensor(&ov9732_sensor_config);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sensor_ov9732_remove(struct i2c_client *client)
{
    vic_unregister_sensor(&ov9732_sensor_config);
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id ov9732_id_table[] = {
    { "md_sensor_ov9732", 0 },
    {},
};

static struct i2c_driver sensor_ov9732_driver = {
    .driver = {
        .name = "md_sensor_ov9732",
    },
    .probe = sensor_ov9732_probe,
    .remove = sensor_ov9732_remove,
    .id_table = ov9732_id_table,
};

static struct i2c_board_info sensor_ov9732_info = {
    .type = "md_sensor_ov9732",
    // .addr = I2C_ADDR,
};

static int ov9732_sensor_init(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "ov9732: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&sensor_ov9732_driver);
    if (ret) {
        printk(KERN_ERR "ov9732: failed to register i2c driver\n");
        return ret;
    }

    sensor_ov9732_info.addr = i2c_addr;
    i2c_dev = i2c_register_device(&sensor_ov9732_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "ov9732: failed to register i2c device\n");
        i2c_del_driver(&sensor_ov9732_driver);
        return -EINVAL;
    }

    return ret;
}

static void ov9732_sensor_exit(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sensor_ov9732_driver);
}
module_init(ov9732_sensor_init);
module_exit(ov9732_sensor_exit);

MODULE_DESCRIPTION("x1520 ov9732 driver");
MODULE_LICENSE("GPL");
