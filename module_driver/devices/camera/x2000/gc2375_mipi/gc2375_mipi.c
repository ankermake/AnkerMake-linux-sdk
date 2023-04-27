/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * GC2375 mipi driver
 *
 */
#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>

#define GC2375_DEVICE_NAME              "gc2375"
#define GC2375_DEVICE_I2C_ADDR          0x37
#define GC2375A_SUPPORT_30FPS_SCLK      (78000000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5

#define GC2375_DEVICE_WIDTH             1600
#define GC2375_DEVICE_HEIGHT            1200

#define GC2375_CHIP_ID_H                (0x23)
#define GC2375_CHIP_ID_L                (0xA5)
#define GC2375_REG_CHIP_ID_HIGH         0xF0
#define GC2375_REG_CHIP_ID_LOW          0xF1

#define GC2375A_MAX_WIDTH               1600
#define GC2375A_MAX_HEIGHT              1200


#define GC2375_REG_END                  0xffff
#define GC2375_REG_DELAY                0xfffe

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

static int power_gpio   = -1; // GPIO_PB(20);
static int reset_gpio   = -1; // -1
static int pwdn_gpio    = -1; // GPIO_PB(18);
static int i2c_sel_gpio = -1; // -1
static int i2c_bus_num  = 0;  // 5

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param_gpio(i2c_sel_gpio, 0644);

module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static int vic_index = 0;

static struct sensor_attr gc2375_sensor_attr;



/*
 * 适配不同分辨率
 */
#define SENSOR_CROP_HEIGTH_H            ((GC2375A_MAX_HEIGHT - GC2375_DEVICE_HEIGHT) / 2 / 256)
#define SENSOR_CROP_HEIGTH_L            ((GC2375A_MAX_HEIGHT - GC2375_DEVICE_HEIGHT) / 2 % 256)
#define SENSOR_CROP_WIDTH_H             ((GC2375A_MAX_WIDTH - GC2375_DEVICE_WIDTH) / 2 / 256)
#define SENSOR_CROP_WIDTH_L             ((GC2375A_MAX_WIDTH - GC2375_DEVICE_WIDTH) / 2 % 256)

#define SENSOR_OUT_HEIGHT_H             (GC2375_DEVICE_HEIGHT / 256)
#define SENSOR_OUT_HEIGHT_L             (GC2375_DEVICE_HEIGHT % 256)
#define SENSOR_OUT_WIDTH_H              (GC2375_DEVICE_WIDTH / 256)
#define SENSOR_OUT_WIDTH_L              (GC2375_DEVICE_WIDTH % 256)

#define SENSOR_CSI_8BIT_WIDTH_H         (GC2375_DEVICE_WIDTH / 256)
#define SENSOR_CSI_8BIT_WIDTH_L         (GC2375_DEVICE_WIDTH % 256)
#define SENSOR_CSI_10BIT_WIDTH_H        ((GC2375_DEVICE_WIDTH * 5 / 4) / 256)
#define SENSOR_CSI_10BIT_WIDTH_L        ((GC2375_DEVICE_WIDTH * 5 / 4) % 256)

#define SENSOR_CSI_BUF_WIDTH_H          (GC2375_DEVICE_WIDTH / 256)
#define SENSOR_CSI_BUF_WIDTH_L          (GC2375_DEVICE_WIDTH % 256)

/*
 * Interface    : MIPI - 1-lane
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 624MHz
 * resolution   : 1600*1200
 * FrameRate    : 30fps
 */
static struct regval_list gc2375_1600_1200_30fps_mipi_init_regs[] = {
    {0xfe,0xF0},
    {0xfe,0xF0},
    {0xfe,0x00},
    {0xfe,0x00},
    {0xfe,0x00},
    {0xf7,0x01},
    {0xf8,0x0c},
    {0xf9,0x42},
    {0xfa,0x88},
    {0xfc,0x8e},
    {0xfe,0x00},
    {0x88,0x03},

    /*Crop*/
    {0x90,0x01}, //crop mode en
    {0x91,SENSOR_CROP_HEIGTH_H},    //win_y_h
    {0x92,8 + SENSOR_CROP_HEIGTH_L},//win_y_l
    {0x93,SENSOR_CROP_WIDTH_H},     //win_x_h
    {0x94,8 + SENSOR_CROP_WIDTH_L}, //win_x_l

    {0x95,SENSOR_OUT_HEIGHT_H},     //win_height_h
    {0x96,SENSOR_OUT_HEIGHT_L},     //win_height_l
    {0x97,SENSOR_OUT_WIDTH_H},      //win_width_h
    {0x98,SENSOR_OUT_WIDTH_L},      //win_width_l

    /*Analog*/
    {0x03,0x04}, //exp_in_h :1125
    {0x04,0x65}, //exp_in_l
    {0x05,0x02}, //HB_h :613
    {0x06,0x65}, //HB_l
    {0x07,0x00}, //VB_h :16
    {0x08,0x10}, //VB_l
    {0x09,0x00}, //row_start_h :0
    {0x0a,0x00}, //row_start_l
    {0x0b,0x00}, //col_start_h :20
    {0x0c,0x14}, //col_start_l
    {0x0d,0x04}, //win_height_h :1216
    {0x0e,0xc0}, //win_height_l
    {0x0f,0x06}, //win_width_h :1616
    {0x10,0x50}, //win_width_l
    {0x17,0xc0}, //mirror & flip
    {0x19,0x17},
    {0x1c,0x10},
    {0x1d,0x13},

    {0x20,0x0d},
    {0x21,0x6d},
    {0x22,0x0d},
    {0x25,0xc1},
    {0x26,0x0d},
    {0x27,0x22},
    {0x29,0x5f},
    {0x2b,0x88},
    {0x2f,0x12},

    {0x38,0x86},
    {0x3d,0x00},
    {0xcd,0xa3},
    {0xce,0x57},
    {0xd0,0x09},
    {0xd1,0xca},
    {0xd2,0x74},
    {0xd3,0xbb},
    {0xd8,0x60},
    {0xe0,0x08},
    {0xe1,0x1f},
    {0xe4,0xf8},
    {0xe5,0x0c},
    {0xe6,0x10},
    {0xe7,0xcc},
    {0xe8,0x02},
    {0xe9,0x01},
    {0xea,0x02},
    {0xeb,0x01},

    /*BLK*/
    {0x18,0x02},
    {0x1a,0x18},
    {0x28,0x00},
    {0x3f,0x40},
    {0x40,0x26},
    {0x41,0x00},
    {0x43,0x03},
    {0x4a,0x00},
    {0x4e,0x3c},
    {0x4f,0x00},
    {0x60,0x00},
    {0x61,0x80},
    {0x66,0xc0},
    {0x67,0x00},
    {0xfe,0x01},
    {0x41,0x00},
    {0x42,0x00},
    {0x43,0x00},
    {0x44,0x00},

    /*Dark sun*/
    {0xfe,0x00},
    {0x68,0x00},

    /*Gain*/
    {0xb0,0x58}, //global gain :88
    {0xb1,0x01}, //auto_pregain_h[3:0] :64
    {0xb2,0x00}, //auto_pregain_l[7:2]
    {0xb6,0x00}, //gain code[3:0]

    /*MIPI*/
    {0xfe,0x03},
    {0x01,0x03}, //phy lane0 en, phy clk en
    {0x02,0x33},
    {0x03,0x90},
    {0x04,0x04},
    {0x05,0x00},
    {0x06,0x80},
    {0x11,0x2b}, //RAW10 ouput
    {0x12,SENSOR_CSI_10BIT_WIDTH_L}, //LWC_set_l :RAW10:win_width*5/4
    {0x13,SENSOR_CSI_10BIT_WIDTH_H}, //LWC_set_h
    {0x15,0x00},
    {0x42,SENSOR_CSI_BUF_WIDTH_L}, //buf_win_width_l
    {0x43,SENSOR_CSI_BUF_WIDTH_H}, //buf_win_width_h

    {0x21,0x08},
    {0x22,0x05},
    {0x23,0x13},
    {0x24,0x02},
    {0x25,0x13},
    {0x26,0x08},
    {0x29,0x06},
    {0x2a,0x08},
    {0x2b,0x08},
    {0xfe,0x00},
    {GC2375_REG_END, 0x0}, /* END MARKER */
};


static struct regval_list gc2375_regs_stream_on[] = {
    {0xfe,0x00},
    {0xef,0x90},
    {GC2375_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list gc2375_regs_stream_off[] = {
    {0xfe,0x00},
    {0xef,0x00},
    {GC2375_REG_END, 0x0}, /* END MARKER */
};


static int gc2375_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
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
        printk(KERN_ERR "gc2375: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int gc2375_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
{
    unsigned char buf[1] = {reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr = i2c->addr,
            .flags  = 0,
            .len    = 1,
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
        printk(KERN_ERR "gc2375(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int gc2375_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != GC2375_REG_END) {
        if (vals->reg_num == GC2375_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc2375_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        // printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static inline int gc2375_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != GC2375_REG_END) {
        if (vals->reg_num == GC2375_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc2375_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n",vals->reg_num, val);
        vals++;
    }

    return 0;
}

static int gc2375_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char l = 1;
    int ret;

    ret = gc2375_read(i2c, GC2375_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != GC2375_CHIP_ID_H) {
        printk(KERN_ERR "gc2375 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = gc2375_read(i2c, GC2375_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;

    if (l != GC2375_CHIP_ID_L) {
        printk(KERN_ERR"gc2375 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }

    printk(KERN_DEBUG "gc2375 get chip id = %02x%02x\n", h, l);

    return 0;
}


static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "gc2375_reset");
        if (ret) {
            printk(KERN_ERR "gc2375: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "gc2375_pwdn");
        if (ret) {
            printk(KERN_ERR "gc2375: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gc2375_power");
        if (ret) {
            printk(KERN_ERR "gc2375: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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

static void gc2375_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk(vic_index);
}


static int gc2375_power_on(void)
{
    camera_enable_sensor_mclk(vic_index, 24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 0);
        m_msleep(10);
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
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(20);
    }

    if (i2c_sel_gpio != -1)
        gpio_direction_output(i2c_sel_gpio, 1);

    int ret;
    ret = gc2375_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "gc2375: failed to detect\n");
        gc2375_power_off();
        return ret;
    }

    ret = gc2375_write_array(i2c_dev, gc2375_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "gc2375: failed to init regs\n");
        gc2375_power_off();
        return ret;
    }

    return 0;
}

static int gc2375_stream_on(void)
{
    int ret = gc2375_write_array(i2c_dev, gc2375_regs_stream_on);

    if (ret)
        printk(KERN_ERR "gc2375: failed to stream on\n");

    return ret;
}

static void gc2375_stream_off(void)
{
    int ret = gc2375_write_array(i2c_dev, gc2375_regs_stream_off);

    if (ret)
        printk(KERN_ERR "gc2375: failed to stream on\n");

}

static int gc2375_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = gc2375_read(i2c_dev, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;
    return ret;
}

static int gc2375_s_register(struct sensor_dbg_register *reg)
{
    return gc2375_write(i2c_dev, reg->reg & 0xff, reg->val & 0xff);
}

static int gc2375_set_integration_time(int value)
{
    int ret = 0;
    unsigned int expo = value;

    ret = gc2375_write(i2c_dev, 0xfe, 0x00);
    ret += gc2375_write(i2c_dev, 0x04, (unsigned char)(expo & 0xff));
    ret += gc2375_write(i2c_dev, 0x03, (unsigned char)((expo >> 8) & 0x3f));
    if (ret < 0) {
        printk(KERN_ERR "gc2375: set integration time error. line=%d\n" ,__LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int gc2375_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

    return isp_gain;
}

static int gc2375_set_analog_gain(int value)
{
    int ret = 0;

    ret =  gc2375_write(i2c_dev, 0xfe, 0x00);
    ret += gc2375_write(i2c_dev, 0xb6, (value >> 12) & 0xf);
    ret += gc2375_write(i2c_dev, 0xb1, (value >> 8) & 0xf);
    ret += gc2375_write(i2c_dev, 0xb2, (value << 2) & 0xff);
    if (ret < 0) {
        printk(KERN_ERR "gc2375: set analog gain error  line=%d" ,__LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int gc2375_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{

    return isp_gain;
}

static int gc2375_set_digital_gain(int value)
{
    return 0;
}

static int gc2375_set_fps(int fps)
{
    struct sensor_info *sensor_info = &gc2375_sensor_attr.sensor_info;
    unsigned int wpclk = 0;
    unsigned short win_high=0;
    unsigned short vts = 0;
    unsigned short hb=0;
    unsigned short vb = 0;
    unsigned short hts=0;
    unsigned int max_fps = 0;
    unsigned char tmp;
    unsigned int newformat = 0;
    int ret = 0;

    wpclk = GC2375A_SUPPORT_30FPS_SCLK;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk(KERN_ERR "warn: fps(%x) no in range\n", fps);
        return -1;
    }
    /* H Blanking */
    ret = gc2375_read(i2c_dev, 0x05, &tmp);
    hb = tmp;
    ret += gc2375_read(i2c_dev, 0x06, &tmp);
    if(ret < 0)
        return -1;

    hb = (hb << 8) + tmp;
    hts = hb << 2;

    /* P0:0x0d win_height[10:8], P0:0x0e win_height[7:0] */
    ret = gc2375_read(i2c_dev, 0x0d, &tmp);
    win_high = tmp;
    ret += gc2375_read(i2c_dev, 0x0e, &tmp);
    if(ret < 0)
        return -1;

    win_high = (win_high << 8) + tmp;

    vts = wpclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    vb = vts - win_high - 16;

    /* V Blanking */
    ret = gc2375_write(i2c_dev, 0x08, (unsigned char)(vb & 0xff));
    ret += gc2375_write(i2c_dev, 0x07, (unsigned char)(vb >> 8) & 0x1f);
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}


static struct sensor_attr gc2375_sensor_attr = {
    .device_name                = GC2375_DEVICE_NAME,
    .cbus_addr                  = GC2375_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 1,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = gc2375_1600_1200_30fps_mipi_init_regs,

        .width                  = GC2375_DEVICE_WIDTH,
        .height                 = GC2375_DEVICE_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SRGGB10_1X10,

        .fps                    = 30 << 16 | 1,  /* 30/1 */
        .total_width            = 2080,
        .total_height           = 1240,

        .min_integration_time   = 2,
        .max_integration_time   = 1240 - 2,
        .max_again              = 263489,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = gc2375_power_on,
        .power_off              = gc2375_power_off,
        .stream_on              = gc2375_stream_on,
        .stream_off             = gc2375_stream_off,
        .get_register           = gc2375_g_register,
        .set_register           = gc2375_s_register,

        .set_integration_time   = gc2375_set_integration_time,
        .alloc_again            = gc2375_alloc_again,
        .set_analog_gain        = gc2375_set_analog_gain,
        .alloc_dgain            = gc2375_alloc_dgain,
        .set_digital_gain       = gc2375_set_digital_gain,
        .set_fps                = gc2375_set_fps,
    },
};

static int gc2375_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(vic_index, &gc2375_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int gc2375_remove(struct i2c_client *client)
{
    camera_unregister_sensor(vic_index, &gc2375_sensor_attr);
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id gc2375_id[] = {
    { GC2375_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, gc2375_id);

static struct i2c_driver gc2375_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = GC2375_DEVICE_NAME,
    },
    .probe          = gc2375_probe,
    .remove         = gc2375_remove,
    .id_table       = gc2375_id,
};

static struct i2c_board_info sensor_gc2375_info = {
    .type           = GC2375_DEVICE_NAME,
    .addr           = GC2375_DEVICE_I2C_ADDR,
};


static __init int init_gc2375(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "gc2375: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&gc2375_driver);
    if (ret) {
        printk(KERN_ERR "gc2375: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_gc2375_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "gc2375: failed to register i2c device\n");
        i2c_del_driver(&gc2375_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_gc2375(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&gc2375_driver);
}

module_init(init_gc2375);
module_exit(exit_gc2375);

MODULE_DESCRIPTION("X2000 GC2375 driver");
MODULE_LICENSE("GPL");
