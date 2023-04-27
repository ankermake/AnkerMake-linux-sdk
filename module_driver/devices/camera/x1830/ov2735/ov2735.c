#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <common.h>
#include <vic_sensor.h>

enum camera_reg_ops {
    CAMERA_REG_OP_DATA = 1,
    CAMERA_REG_OP_DELAY,
    CAMERA_REG_OP_END,
};

struct camera_reg_op {
    unsigned int flag;
    unsigned int reg;
    unsigned int val;
};

#define CONFIG_OV2735_1920X1080

#define OV2735_CHIP_ID_H                (0x27)
#define OV2735_CHIP_ID_L                (0x35)
#define OV2735_PAGE_REG                 0xfd

#define OV2735_SUPPORT_SCLK_FPS_30      (84000000)
#define OV2735_SUPPORT_SCLK_FPS_15      (60000000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5
#define OV2735_MAX_WIDTH                1920
#define OV2735_MAX_HEIGHT               1080
#if defined CONFIG_OV2735_640X1072
#define OV2735_WIDTH                    640
#define OV2735_HEIGHT                   1072
#elif defined CONFIG_OV2735_1920X1080
#define OV2735_WIDTH                    1920
#define OV2735_HEIGHT                   1080
#endif
#define SENSOR_VERSION                  "H20170911a"

#define I2C_ADDR                        0x3c

static int power_gpio = -1; // GPIO_PB(30);
static int reset_gpio = -1; // GPIO_PA(19);
static int pwdn_gpio = -1; // -1;
static int i2c_sel_gpio = -1; // GPIO_PA(20);
static int i2c_bus_num = -1; // 0

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param_gpio(i2c_sel_gpio, 0644);

module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;

static int ov2735_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
{
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr = i2c->addr,
        .flags  = 0,
        .len    = 2,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "ov2735: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int ov2735_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
{
    struct i2c_msg msg[2] = {
        [0] = {
            .addr = i2c->addr,
            .flags  = 0,
            .len    = 1,
            .buf    = &reg,
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
        printk(KERN_ERR "ov2735: failed to read reg: %x\n", (int)reg);

    return ret;
}

void m_msleep(int ms)
{
    usleep_range(ms*1000, ms*1000);
}

static int ov2735_write_array(struct i2c_client *i2c, struct camera_reg_op *vals)
{
    int ret;

    while (vals->flag != CAMERA_REG_OP_END) {
        if (vals->flag == CAMERA_REG_OP_DATA) {
            ret = ov2735_write(i2c, vals->reg, vals->val);
            if (ret < 0)
                return ret;
        } else if (vals->flag == CAMERA_REG_OP_DELAY) {
            m_msleep(vals->val);
        } else {
            printk(KERN_ERR "%s(%d), error flag: %d\n", __func__, __LINE__, vals->flag);
            return -1;
        }
        vals++;
    }

    return 0;
}

static struct camera_reg_op ov2735_init_regs_1920_1080_25fps_dvp[] = {
    {CAMERA_REG_OP_DATA, 0xfd, 0x00},
    /*{CAMERA_REG_OP_DATA, 0x20, 0x00},*///soft reset enable, cause i2c error
    {CAMERA_REG_OP_DELAY, 0x0, 0x05},
    {CAMERA_REG_OP_DATA, 0xfd, 0x00},
    {CAMERA_REG_OP_DATA, 0x2f, 0x10},
    {CAMERA_REG_OP_DATA, 0x34, 0x00},
    {CAMERA_REG_OP_DATA, 0x30, 0x15},
    {CAMERA_REG_OP_DATA, 0x33, 0x01},
    {CAMERA_REG_OP_DATA, 0x35, 0x20},
    {CAMERA_REG_OP_DATA, 0x1b, 0x00},
    {CAMERA_REG_OP_DATA, 0x1d, 0xa5},
    {CAMERA_REG_OP_DATA, 0xfd, 0x01},
    {CAMERA_REG_OP_DATA, 0x0d, 0x00},
    {CAMERA_REG_OP_DATA, 0x30, 0x00},
    {CAMERA_REG_OP_DATA, 0x03, 0x01},
    {CAMERA_REG_OP_DATA, 0x04, 0x8f},
    {CAMERA_REG_OP_DATA, 0x01, 0x01},
    {CAMERA_REG_OP_DATA, 0x09, 0x00},
    {CAMERA_REG_OP_DATA, 0x0a, 0x20},
    {CAMERA_REG_OP_DATA, 0x06, 0x0a},
    {CAMERA_REG_OP_DATA, 0x24, 0x10},
    {CAMERA_REG_OP_DATA, 0x01, 0x01},
    {CAMERA_REG_OP_DATA, 0xfb, 0x73},
    {CAMERA_REG_OP_DATA, 0x01, 0x01},
    {CAMERA_REG_OP_DATA, 0xfd, 0x01},
    {CAMERA_REG_OP_DATA, 0x1a, 0x6b},
    {CAMERA_REG_OP_DATA, 0x1c, 0xea},
    {CAMERA_REG_OP_DATA, 0x16, 0x0c},
    {CAMERA_REG_OP_DATA, 0x21, 0x00},
    {CAMERA_REG_OP_DATA, 0x11, 0x63},
    {CAMERA_REG_OP_DATA, 0x19, 0xc3},
    {CAMERA_REG_OP_DATA, 0x26, 0x5a},
    {CAMERA_REG_OP_DATA, 0x29, 0x01},
    {CAMERA_REG_OP_DATA, 0x33, 0x6f},
    {CAMERA_REG_OP_DATA, 0x2a, 0xd2},
    {CAMERA_REG_OP_DATA, 0x2c, 0x40},
    {CAMERA_REG_OP_DATA, 0xd0, 0x02},
    {CAMERA_REG_OP_DATA, 0xd1, 0x01},
    {CAMERA_REG_OP_DATA, 0xd2, 0x20},
    {CAMERA_REG_OP_DATA, 0xd3, 0x04},
    {CAMERA_REG_OP_DATA, 0xd4, 0x2a},
    {CAMERA_REG_OP_DATA, 0x50, 0x00},
    {CAMERA_REG_OP_DATA, 0x51, 0x2c},
    {CAMERA_REG_OP_DATA, 0x52, 0x29},
    {CAMERA_REG_OP_DATA, 0x53, 0x00},
    {CAMERA_REG_OP_DATA, 0x55, 0x44},
    {CAMERA_REG_OP_DATA, 0x58, 0x29},
    {CAMERA_REG_OP_DATA, 0x5a, 0x00},
    {CAMERA_REG_OP_DATA, 0x5b, 0x00},
    {CAMERA_REG_OP_DATA, 0x5d, 0x00},
    {CAMERA_REG_OP_DATA, 0x64, 0x2f},
    {CAMERA_REG_OP_DATA, 0x66, 0x62},
    {CAMERA_REG_OP_DATA, 0x68, 0x5b},
    {CAMERA_REG_OP_DATA, 0x75, 0x46},
    {CAMERA_REG_OP_DATA, 0x76, 0x36},
    {CAMERA_REG_OP_DATA, 0x77, 0x4f},
    {CAMERA_REG_OP_DATA, 0x78, 0xef},
    {CAMERA_REG_OP_DATA, 0x72, 0xcf},
    {CAMERA_REG_OP_DATA, 0x73, 0x36},
    {CAMERA_REG_OP_DATA, 0x7d, 0x0d},
    {CAMERA_REG_OP_DATA, 0x7e, 0x0d},
    {CAMERA_REG_OP_DATA, 0x8a, 0x77},
    {CAMERA_REG_OP_DATA, 0x8b, 0x77},
    {CAMERA_REG_OP_DATA, 0xfd, 0x01},
    {CAMERA_REG_OP_DATA, 0xb1, 0x82},
    {CAMERA_REG_OP_DATA, 0xb3, 0x0b},
    {CAMERA_REG_OP_DATA, 0xb4, 0x14},
    {CAMERA_REG_OP_DATA, 0x9d, 0x40},
    {CAMERA_REG_OP_DATA, 0xa1, 0x05},
    {CAMERA_REG_OP_DATA, 0x94, 0x44},
    {CAMERA_REG_OP_DATA, 0x95, 0x33},
    {CAMERA_REG_OP_DATA, 0x96, 0x1f},
    {CAMERA_REG_OP_DATA, 0x98, 0x45},
    {CAMERA_REG_OP_DATA, 0x9c, 0x10},
    {CAMERA_REG_OP_DATA, 0xb5, 0x70},
    {CAMERA_REG_OP_DATA, 0xa0, 0x00},
    {CAMERA_REG_OP_DATA, 0x25, 0xe0},
    {CAMERA_REG_OP_DATA, 0x20, 0x7b},
    {CAMERA_REG_OP_DATA, 0x8f, 0x88},
    {CAMERA_REG_OP_DATA, 0x91, 0x40},
    {CAMERA_REG_OP_DATA, 0xfd, 0x01},
    {CAMERA_REG_OP_DATA, 0xfd, 0x02},
    {CAMERA_REG_OP_DATA, 0x60, 0xf0},
    {CAMERA_REG_OP_DATA, 0xa1, 0x04},
    {CAMERA_REG_OP_DATA, 0xa3, 0x40},
    {CAMERA_REG_OP_DATA, 0xa5, 0x02},
    {CAMERA_REG_OP_DATA, 0xa7, 0xc4},
    {CAMERA_REG_OP_DATA, 0xfd, 0x01},
    {CAMERA_REG_OP_DATA, 0x86, 0x77},
    {CAMERA_REG_OP_DATA, 0x89, 0x77},
    {CAMERA_REG_OP_DATA, 0x87, 0x74},
    {CAMERA_REG_OP_DATA, 0x88, 0x74},
    {CAMERA_REG_OP_DATA, 0xfc, 0xe0},
    {CAMERA_REG_OP_DATA, 0xfe, 0xe0},
    {CAMERA_REG_OP_DATA, 0xf0, 0x40},
    {CAMERA_REG_OP_DATA, 0xf1, 0x40},
    {CAMERA_REG_OP_DATA, 0xf2, 0x40},
    {CAMERA_REG_OP_DATA, 0xf3, 0x40},
    {CAMERA_REG_OP_DATA, 0xfd, 0x02},
    {CAMERA_REG_OP_DATA, 0x36, 0x08},
    {CAMERA_REG_OP_DATA, 0xa0, (OV2735_MAX_HEIGHT-OV2735_HEIGHT)/2/256}, //v_start_h
    {CAMERA_REG_OP_DATA, 0xa1, 8+(OV2735_MAX_HEIGHT-OV2735_HEIGHT)/2%256}, //v_start_l
    {CAMERA_REG_OP_DATA, 0xa2, OV2735_HEIGHT/256}, //v_size_h
    {CAMERA_REG_OP_DATA, 0xa3, OV2735_HEIGHT%256}, //v_size_l
    {CAMERA_REG_OP_DATA, 0xa4, (OV2735_MAX_WIDTH-OV2735_WIDTH)/2/2/256}, //h_start_half_h
    {CAMERA_REG_OP_DATA, 0xa5, 8+(OV2735_MAX_WIDTH-OV2735_WIDTH)/2/2%256}, //h_start_half_l
    {CAMERA_REG_OP_DATA, 0xa6, OV2735_WIDTH/2/256}, //h_size_half_h
    {CAMERA_REG_OP_DATA, 0xa7, OV2735_WIDTH/2%256}, //h_size_half_l

    {CAMERA_REG_OP_DATA, 0xfd, 0x01},
    {CAMERA_REG_OP_DATA, 0x06, 0xe0},
    {CAMERA_REG_OP_DATA, 0x01, 0x01},

    {CAMERA_REG_OP_DATA, 0xfd, 0x01},
    {CAMERA_REG_OP_DATA, 0x0d, 0x10},
    {CAMERA_REG_OP_DATA, 0x0e, 0x06},
    {CAMERA_REG_OP_DATA, 0x0f, 0x1b},
    {CAMERA_REG_OP_DATA, 0x01, 0x01},

    {CAMERA_REG_OP_DATA, 0xfd, 0x01},
    {CAMERA_REG_OP_DATA, 0x3f, 0x03},

    {CAMERA_REG_OP_END,0x0,0x0},    /* END MARKER */
};

/*
 * the part of driver was fixed.
 */

static struct camera_reg_op ov2735_regs_stream_on[] = {
    {CAMERA_REG_OP_DATA, 0xfd, 0x00},
    {CAMERA_REG_OP_DATA, 0x36, 0x00},
    {CAMERA_REG_OP_DATA, 0x37, 0x00},//fake stream on

    {CAMERA_REG_OP_END,0x0,0x0},    /* END MARKER */
};

static struct camera_reg_op ov2735_regs_stream_off[] = {
    {CAMERA_REG_OP_DATA, 0xfd, 0x00},
    {CAMERA_REG_OP_DATA, 0x36, 0x01},
    {CAMERA_REG_OP_DATA, 0x37, 0x01},//fake stream off

    {CAMERA_REG_OP_END,0x0,0x0},    /* END MARKER */
};

static int ov2735_detect(struct i2c_client *i2c)
{
    unsigned char v = 1;
    int ret;

    ret = ov2735_read(i2c, 0x02, &v);
    if (ret < 0)
        return ret;
    if (v != OV2735_CHIP_ID_H)
        return -ENODEV;

    ret = ov2735_read(i2c, 0x03, &v);
    if (ret < 0)
        return ret;
    if (v != OV2735_CHIP_ID_L)
        return -ENODEV;

    ret = ov2735_read(i2c, 0x04, &v);
    if (ret < 0)
        return ret;
    if (v != 0x06 && v != 0x07)
        return -ENODEV;

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    ret = dvp_init_low10bit_gpio();
    if (ret) {
        printk(KERN_ERR "ov2735: failed to init dvp pins\n");
        return ret;
    }

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "ov2735_reset");
        if (ret) {
            printk(KERN_ERR "ov2735: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "ov2735_pwdn");
        if (ret) {
            printk(KERN_ERR "ov2735: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ov2735_power");
        if (ret) {
            printk(KERN_ERR "ov2735: failed to request pwdn pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (i2c_sel_gpio != -1) {
        ret = gpio_request(i2c_sel_gpio, "ov2735_i2c_sel");
        if (ret) {
            printk(KERN_ERR "ov2735: failed to request pwdn pin: %s\n", gpio_to_str(i2c_sel_gpio, gpio_str));
            goto err_i2c_sel_gpio;
        }
    }

    return 0;
err_i2c_sel_gpio:
    if (power_gpio != -1)
        gpio_free(power_gpio);
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
    if (i2c_sel_gpio != -1)
        gpio_free(i2c_sel_gpio);

    if (power_gpio != -1)
        gpio_free(power_gpio);

    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);

    if (reset_gpio != -1)
        gpio_free(reset_gpio);

    dvp_deinit_gpio();
}

static void ov2735_power_off(void)
{
    if (i2c_sel_gpio != -1)
       gpio_direction_output(i2c_sel_gpio, 0);

    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    vic_disable_sensor_mclk();
}

static int ov2735_power_on(void)
{
    int ret;

    vic_enable_sensor_mclk(24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(50);
        gpio_direction_output(pwdn_gpio, 0);
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

    if (i2c_sel_gpio != -1)
        gpio_direction_output(i2c_sel_gpio, 1);

    ret = ov2735_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "ov2735: failed to detect\n");
        ov2735_power_off();
        return ret;
    }

    ret = ov2735_write_array(i2c_dev, ov2735_init_regs_1920_1080_25fps_dvp);
    if (ret) {
        printk(KERN_ERR "ov2735: failed to init regs\n");
        ov2735_power_off();
        return ret;
    }

    return 0;
}

static int ov2735_stream_on(void)
{
    int ret = ov2735_write_array(i2c_dev, ov2735_regs_stream_on);
    if (ret)
        printk(KERN_ERR "ov2735: failed to stream on\n");

    return ret;
}

static void ov2735_stream_off(void)
{
    int ret = ov2735_write_array(i2c_dev, ov2735_regs_stream_off);
    if (ret)
        printk(KERN_ERR "ov2735: failed to stream on\n");
}


static struct vic_sensor_config ov2735_sensor_config = {
    .device_name            = "ov2735",
    .cbus_addr              = I2C_ADDR,

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
        .width              = OV2735_WIDTH,
        .height             = OV2735_HEIGHT,
        .fmt                = SENSOR_PIXEL_FMT_SGRBG10_1X10,
        .fps                = 25 << 16 | 1,
    },

    .ops  = {
        .power_on           = ov2735_power_on,
        .power_off          = ov2735_power_off,
        .stream_on          = ov2735_stream_on,
        .stream_off         = ov2735_stream_off,
    },
};

static int sensor_ov2735_probe(struct i2c_client *client,
                 const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = vic_register_sensor(&ov2735_sensor_config);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sensor_ov2735_remove(struct i2c_client *client)
{
    vic_unregister_sensor(&ov2735_sensor_config);
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id ov2735_id_table[] = {
    { "md_sensor_ov2735", 0 },
    {},
};

static struct i2c_driver sensor_ov2735_driver = {
    .driver = {
        .name = "md_sensor_ov2735",
    },
    .probe = sensor_ov2735_probe,
    .remove = sensor_ov2735_remove,
    .id_table = ov2735_id_table,
};

static struct i2c_board_info sensor_ov2735_info = {
    .type = "md_sensor_ov2735",
    .addr = I2C_ADDR,
};

static int ov2735_sensor_init(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "ov2735: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&sensor_ov2735_driver);
    if (ret) {
        printk(KERN_ERR "ov2735: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_ov2735_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "ov2735: failed to register i2c device\n");
        i2c_del_driver(&sensor_ov2735_driver);
        return -EINVAL;
    }

    return ret;
}

static void ov2735_sensor_exit(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sensor_ov2735_driver);
}

module_init(ov2735_sensor_init);
module_exit(ov2735_sensor_exit);

MODULE_DESCRIPTION("x1830 ov2735 driver");
MODULE_LICENSE("GPL");
