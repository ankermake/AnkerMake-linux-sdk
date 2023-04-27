#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <common.h>
#include <vic_sensor.h>

static int reset_gpio = -1;
static int pwdn_gpio = -1;
static int power_gpio = -1;
static int i2c_bus_num = -1;
static int use_y8 = 0;

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(use_y8, int, 0644);
module_param(i2c_bus_num, int, 0644);

#define I2C_ADDR                        0x21

#define DEFAULT_WIDTH                   640
#define DEFAULT_HEIGHT                  480

#define REG_PAGE                        0xfe

#define CHIP_ID_GC0308                  0x9b

#define REG_HFLIP_IMG                   0x01 /* Horizontal mirror image reg bit */
#define REG_VFLIP_IMG                   0x02 /* Vertical flip image reg bit */

/*
 * Struct
 */
struct regval_list {
    unsigned char reg_num;
    unsigned char value;
};

/* Supported resolutions */
enum gc0308_width {
    W_QCIF  = 176,
    W_QVGA  = 320,
    W_CIF   = 352,
    W_VGA   = 640,
};

enum gc0308_height {
    H_QCIF  = 144,
    H_QVGA  = 240,
    H_CIF   = 288,
    H_VGA   = 480,
};

struct gc0308_win_size {
    char *name;
    enum gc0308_width width;
    enum gc0308_height height;
    const struct regval_list *regs;
};


struct gc0308_priv {
    const struct gc0308_win_size *win;

    unsigned char power_en;
    unsigned int  exposure;
    unsigned char exposure_mode;
    unsigned char flag_vflip:1;
    unsigned char flag_hflip:1;
};

static struct i2c_client *i2c_dev;

static inline int gc0308_write_reg(struct i2c_client *i2c, unsigned char addr, unsigned char value)
{
    unsigned char buf[2] = {addr, value};
    struct i2c_msg msg = {
        .addr = i2c->addr,
        .flags    = 0,
        .len    = 2,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;
}

static inline char gc0308_read_reg(struct i2c_client *i2c, unsigned char addr)
{
    char value = 0xff;

    struct i2c_msg msg[2] = {
        [0] = {
            .addr = i2c->addr,
            .flags    = 0,
            .len    = 1,
            .buf    = &addr,
        },
        [1] = {
            .addr = i2c->addr,
            .flags    = I2C_M_RD,
            .len    = 1,
            .buf    = &value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "gc0308: failed to read reg: %x\n", (int )addr);

    return value;
}

/*
 * Registers settings
 */

#define ENDMARKER { 0xff, 0xff }

static struct regval_list gc0308_init_regs[] = {
    {0xfe, 0x80},
    {0xfe, 0x00},   // set page0
    {0xd2, 0x10},   // close AEC
    {0x22, 0x55},   // close AWB

    {0x03, 0x01},
    {0x04, 0x2c},
    {0x5a, 0x56},
    {0x5b, 0x40},
    {0x5c, 0x4a},

    {0x22, 0x57},   // Open AWB

    {0xfe, 0x00},
    {0x01, 0x6a},
    {0x02, 0x0c},//70
    {0x0f, 0x00},

    // {0xe2,0x00},
    {0xe3, 0x96},

    {0xe4, 0x01},
    {0xe5, 0x2c},
    {0xe6, 0x01},
    {0xe7, 0x2c},
    {0xe8, 0x01},
    {0xe9, 0x2c},
    {0xea, 0x01},
    {0xeb, 0x2c},

    {0x05, 0x00},
    {0x06, 0x00},
    {0x07, 0x00},
    {0x08, 0x00},
    {0x09, 0x01},
    {0x0a, 0xe8},
    {0x0b, 0x02},
    {0x0c, 0x88},
    {0x0d, 0x02},
    {0x0e, 0x02},
    {0x10, 0x22},
    {0x11, 0xfd},
    {0x12, 0x2a},
    {0x13, 0x00},

    {0x15, 0x0a},
    {0x16, 0x05},
    {0x17, 0x01},
    {0x18, 0x44},
    {0x19, 0x44},
    {0x1a, 0x1e},
    {0x1b, 0x00},
    {0x1c, 0xc1},
    {0x1d, 0x08},
    {0x1e, 0x60},
    {0x1f, 0x03},   //16


    {0x20, 0xff},
    {0x21, 0xf8},
    {0x22, 0x57},
    {0x24, 0xa2}, // default yuv422
    {0x28, 0x00},   //add

    {0x26, 0x03},// 03
    {0x2f, 0x01},
    {0x30, 0xf7},
    {0x31, 0x50},
    {0x32, 0x00},
    {0x39, 0x04},
    {0x3a, 0x18},
    {0x3b, 0x20},
    {0x3c, 0x00},
    {0x3d, 0x00},
    {0x3e, 0x00},
    {0x3f, 0x00},
    {0x50, 0x10},
    {0x53, 0x82},
    {0x54, 0x80},
    {0x55, 0x80},
    {0x56, 0x82},
    {0x8b, 0x40},
    {0x8c, 0x40},
    {0x8d, 0x40},
    {0x8e, 0x2e},
    {0x8f, 0x2e},
    {0x90, 0x2e},
    {0x91, 0x3c},
    {0x92, 0x50},
    {0x5d, 0x12},
    {0x5e, 0x1a},
    {0x5f, 0x24},
    {0x60, 0x07},
    {0x61, 0x15},
    {0x62, 0x08},
    {0x64, 0x03},
    {0x66, 0xe8},
    {0x67, 0x86},
    {0x68, 0xa2},
    {0x69, 0x18},
    {0x6a, 0x0f},
    {0x6b, 0x00},
    {0x6c, 0x5f},
    {0x6d, 0x8f},
    {0x6e, 0x55},
    {0x6f, 0x38},
    {0x70, 0x15},
    {0x71, 0x33},
    {0x72, 0xdc},
    {0x73, 0x80},
    {0x74, 0x02},
    {0x75, 0x3f},
    {0x76, 0x02},
    {0x77, 0x20},
    {0x78, 0x88},
    {0x79, 0x81},
    {0x7a, 0x81},
    {0x7b, 0x22},
    {0x7c, 0xff},
    {0x93, 0x48},
    {0x94, 0x00},
    {0x95, 0x05},
    {0x96, 0xe8},
    {0x97, 0x40},
    {0x98, 0xf0},
    {0xb1, 0x38},
    {0xb2, 0x38},
    {0xbd, 0x38},
    {0xbe, 0x36},
    {0xd0, 0xc9},
    {0xd1, 0x10},

    {0xd3, 0x80},
    {0xd5, 0xf2},
    {0xd6, 0x16},
    {0xdb, 0x92},
    {0xdc, 0xa5},
    {0xdf, 0x23},
    {0xd9, 0x00},
    {0xda, 0x00},
    {0xe0, 0x09},

    {0xed, 0x04},
    {0xee, 0xa0},
    {0xef, 0x40},
    {0x80, 0x03},
    {0x80, 0x03},
    {0x9F, 0x10},
    {0xA0, 0x20},
    {0xA1, 0x38},
    {0xA2, 0x4E},
    {0xA3, 0x63},
    {0xA4, 0x76},
    {0xA5, 0x87},
    {0xA6, 0xA2},
    {0xA7, 0xB8},
    {0xA8, 0xCA},
    {0xA9, 0xD8},
    {0xAA, 0xE3},
    {0xAB, 0xEB},
    {0xAC, 0xF0},
    {0xAD, 0xF8},
    {0xAE, 0xFD},
    {0xAF, 0xFF},
    {0xc0, 0x00},
    {0xc1, 0x10},
    {0xc2, 0x1C},
    {0xc3, 0x30},
    {0xc4, 0x43},
    {0xc5, 0x54},
    {0xc6, 0x65},
    {0xc7, 0x75},
    {0xc8, 0x93},
    {0xc9, 0xB0},
    {0xca, 0xCB},
    {0xcb, 0xE6},
    {0xcc, 0xFF},
    {0xf0, 0x02},
    {0xf1, 0x01},
    {0xf2, 0x01},
    {0xf3, 0x30},
    {0xf9, 0x9f},
    {0xfa, 0x78},

    //---------------------------------------------------------------
    {0xfe, 0x01},// set page1

    {0x00, 0xf5},
    {0x02, 0x1a},
    {0x0a, 0xa0},
    {0x0b, 0x60},
    {0x0c, 0x08},
    {0x0e, 0x4c},
    {0x0f, 0x39},
    {0x11, 0x3f},
    {0x12, 0x72},
    {0x13, 0x13},
    {0x14, 0x42},
    {0x15, 0x43},
    {0x16, 0xc2},
    {0x17, 0xa8},
    {0x18, 0x18},
    {0x19, 0x40},
    {0x1a, 0xd0},
    {0x1b, 0xf5},
    {0x70, 0x40},
    {0x71, 0x58},
    {0x72, 0x30},
    {0x73, 0x48},
    {0x74, 0x20},
    {0x75, 0x60},
    {0x77, 0x20},
    {0x78, 0x32},
    {0x30, 0x03},
    {0x31, 0x40},
    {0x32, 0xe0},
    {0x33, 0xe0},
    {0x34, 0xe0},
    {0x35, 0xb0},
    {0x36, 0xc0},
    {0x37, 0xc0},
    {0x38, 0x04},
    {0x39, 0x09},
    {0x3a, 0x12},
    {0x3b, 0x1C},
    {0x3c, 0x28},
    {0x3d, 0x31},
    {0x3e, 0x44},
    {0x3f, 0x57},
    {0x40, 0x6C},
    {0x41, 0x81},
    {0x42, 0x94},
    {0x43, 0xA7},
    {0x44, 0xB8},
    {0x45, 0xD6},
    {0x46, 0xEE},
    {0x47, 0x0d},

    //Registers of Page0
    {0xfe, 0x00}, // set page0
    {0x10, 0x26},
    {0x11, 0x0d},  // fd,modified by mormo 2010/07/06
    {0x1a, 0x2a},  // 1e,modified by mormo 2010/07/06

    {0x1c, 0x49}, // c1,modified by mormo 2010/07/06
    {0x1d, 0x9a}, // 08,modified by mormo 2010/07/06
    {0x1e, 0x61}, // 60,modified by mormo 2010/07/06

    {0x3a, 0x20},

    {0x50, 0x14},  // 10,modified by mormo 2010/07/06
    {0x53, 0x80},
    {0x56, 0x80},

    {0x8b, 0x20}, //LSC
    {0x8c, 0x20},
    {0x8d, 0x20},
    {0x8e, 0x14},
    {0x8f, 0x10},
    {0x90, 0x14},

    {0x94, 0x02},
    {0x95, 0x07},
    {0x96, 0xe0},

    {0xb1, 0x40}, // YCPT
    {0xb2, 0x40},
    {0xb3, 0x48},
    {0xb6, 0xe0},

    {0xd0, 0xc9}, // AECT  c9,modifed by mormo 2010/07/06
    {0xd3, 0x68}, // 80,modified by mormor 2010/07/06
    {0xf2, 0x02},
    {0xf7, 0x12},
    {0xf8, 0x0a},

    //Registers of Page1
    {0xfe, 0x01},// set page1
    {0x02, 0x20},
    {0x04, 0x10},
    {0x05, 0x08},
    {0x06, 0x20},
    {0x08, 0x0a},

    {0x0e, 0x44},
    {0x0f, 0x32},
    {0x10, 0x41},
    {0x11, 0x37},
    {0x12, 0x22},
    {0x13, 0x19},
    {0x14, 0x44},
    {0x15, 0x44},

    {0x19, 0x50},
    {0x1a, 0xd8},

    {0x32, 0x10},

    {0x35, 0x00},
    {0x36, 0x80},
    {0x37, 0x00},
    //-----------Update the registers end---------//

    {0xfe, 0x00}, // set page0
    {0xd2, 0x90},

    //-----------GAMMA Select(3)---------------//
    {0x9F, 0x10},
    {0xA0, 0x20},
    {0xA1, 0x38},
    {0xA2, 0x4E},
    {0xA3, 0x63},
    {0xA4, 0x76},
    {0xA5, 0x87},
    {0xA6, 0xA2},
    {0xA7, 0xB8},
    {0xA8, 0xCA},
    {0xA9, 0xD8},
    {0xAA, 0xE3},
    {0xAB, 0xEB},
    {0xAC, 0xF0},
    {0xAD, 0xF8},
    {0xAE, 0xFD},
    {0xAF, 0xFF},

    {0x14, 0x10},
    //{0x25, 0x0f}, // enable output
    ENDMARKER,
};

static const struct regval_list gc0308_vga_regs[] = {
    {0xfe, 0x00},
    {0x46, 0x80}, {0x47, 0x00},
    {0x48, 0x00}, {0x49, 0x01},
    {0x4a, 0xE0}, {0x4b, 0x02},
    {0x4c, 0x80},

    {0xfe, 0x01},
    {0x54, 0x11}, {0x55, 0x00},
    {0x56, 0x00}, {0x57, 0x00},
    {0x58, 0x00}, {0x59, 0x00},
    ENDMARKER,
};

static const struct regval_list gc0308_enable_output_regs[] = {
    {0xfe, 0x00},
    {0x25, 0x0f},
    ENDMARKER,
};

static const struct regval_list gc0308_disable_output_regs[] = {
    {0xfe, 0x00},
    {0x25, 0x00},
    ENDMARKER,
};

static struct regval_list gc0308_chip_id_regs[] = {
    {0xfe, 0x00},
    {0x00, 0x00},
    ENDMARKER,
};

struct regval_list gc0308_fmt_regs[] = {
    {0xfe, 0x00},
    {0x24, 0xa2}, // defaut yuv422
    ENDMARKER,
};

static int gc0308_write_array(struct i2c_client *i2c,
                  const struct regval_list *vals)
{
    int ret;

    while ((vals->reg_num != 0xff) || (vals->value != 0xff)) {
        ret = gc0308_write_reg(i2c, vals->reg_num, vals->value);
        if (ret < 0) {
            return ret;
        }
        vals++;
    }
    return 0;
}

static int gc0308_read_array(struct i2c_client *i2c,
                               struct regval_list *vals)
{
    int ret;

    while ((vals->reg_num != 0xff) || (vals->value != 0xff)) {
        if (vals->reg_num == REG_PAGE) {
            ret = gc0308_write_reg(i2c, vals->reg_num, vals->value);
            if (ret < 0) {
                return ret;
            }
        } else {
            vals->value = gc0308_read_reg(i2c, vals->reg_num);
        }
        vals++;
    }

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    ret = dvp_init_low8bit_gpio();
    if (ret) {
        printk(KERN_ERR "gc0328: failed to init dvp pins\n");
        return ret;
    }

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio,"gc0308_reset");
        if (ret) {
            printk(KERN_ERR "gc0308: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio,"gc0308_power");
        if (ret) {
            printk(KERN_ERR "gc0308: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio,"gc0308_pwdn");
        if (ret) {
            printk(KERN_ERR "gc0308: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    return 0;
err_pwdn_gpio:
    gpio_free(power_gpio);
err_power_gpio:
    gpio_free(reset_gpio);
err_reset_gpio:

    return ret;
}

static void deinit_gpio(void)
{
    if (reset_gpio != -1)
        gpio_free(reset_gpio);

    if (power_gpio != -1)
        gpio_free(power_gpio);

    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);
}

static int gc0308_sensor_stream_on(void)
{
    return gc0308_write_array(i2c_dev, gc0308_enable_output_regs);
}

static void gc0308_sensor_stream_off(void)
{
    gc0308_write_array(i2c_dev, gc0308_disable_output_regs);
}

void gc0308_sensor_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 1);

    vic_disable_sensor_mclk();
}

int gc0308_sensor_power_on(void)
{
    int ret;

    vic_enable_sensor_mclk(24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 0);
        msleep(50);
    }

    if(reset_gpio != -1){
        gpio_direction_output(reset_gpio, 1);
        msleep(20);
        gpio_direction_output(reset_gpio, 0);
        msleep(20);
        gpio_direction_output(reset_gpio, 1);
        msleep(10);
    }

    if(pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 1);
        msleep(10);
        gpio_direction_output(pwdn_gpio, 0);
        msleep(10);
    }

    /*
     * check and show product ID and manufacturer ID
     */
    ret = gc0308_read_array(i2c_dev, gc0308_chip_id_regs);
    if (ret < 0) {
        printk(KERN_ERR "gc0308: Failed to read chip id\n");
        goto err;
    }

    if (gc0308_chip_id_regs[1].value != CHIP_ID_GC0308) {
        printk(KERN_ERR "gc0308: read sensor chip_id is error: %x\n",
                (int)gc0308_chip_id_regs[1].value);
        ret = -ENODEV;
        goto err;
    }

    ret = gc0308_write_array(i2c_dev, gc0308_init_regs);
    if (ret < 0) {
        printk(KERN_ERR "gc0308: failed to write init array %d", ret);
        goto err;
    }

    ret = gc0308_write_array(i2c_dev, gc0308_fmt_regs);
    if (ret < 0) {
        printk(KERN_ERR "gc0308: failed to write fmt array %d", ret);
        goto err;
    }

    /* 设置分辨率  */
    ret = gc0308_write_array(i2c_dev, gc0308_vga_regs);
    if (ret < 0) {
        printk(KERN_ERR "gc0308: failed to write window size %d", ret);
        goto err;
    }

    return 0;
err:
    gc0308_sensor_power_off();
    return ret;
}

static struct vic_sensor_config gc0308_sensor_config = {
    .device_name            = "gc0308",

    .vic_interface          = VIC_dvp,
    .dvp_cfg_info = {
        .dvp_data_fmt       = DVP_YUV422,
        .dvp_gpio_mode      = DVP_PA_LOW_8BIT,
        .dvp_timing_mode    = DVP_href_mode,
        .dvp_yuv_data_order = order_1_2_3_4,
        .dvp_hsync_polarity = POLARITY_HIGH_ACTIVE,
        .dvp_vsync_polarity = POLARITY_HIGH_ACTIVE,
        .dvp_img_scan_mode  = DVP_img_scan_progress,
    },

    .sensor_info = {
        .width              = DEFAULT_WIDTH,
        .height             = DEFAULT_HEIGHT,
        .fmt                = SENSOR_PIXEL_FMT_YUYV8_2X8,
        .fps                = 25 << 16 | 1,
    },


    .isp_clk_rate           = 90 * 1000 * 1000,


    .ops = {
        .power_on           = gc0308_sensor_power_on,
        .power_off          = gc0308_sensor_power_off,
        .stream_on          = gc0308_sensor_stream_on,
        .stream_off         = gc0308_sensor_stream_off,
    },
};
static int sensor_gc0308_probe(struct i2c_client *client,
                 const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = vic_register_sensor(&gc0308_sensor_config);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sensor_gc0308_remove(struct i2c_client *client)
{
    vic_unregister_sensor(&gc0308_sensor_config);
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id gc0308_id_table[] = {
    { "md_sensor_gc0308", 0 },
    {},
};

static struct i2c_driver sensor_gc0308_driver = {
    .driver = {
        .name = "md_sensor_gc0308",
    },
    .probe = sensor_gc0308_probe,
    .remove = sensor_gc0308_remove,
    .id_table = gc0308_id_table,
};

static struct i2c_board_info sensor_gc0308_info = {
    .type = "md_sensor_gc0308",
    .addr = I2C_ADDR,
};

static int gc0308_sensor_init(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "gc0308: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    if (use_y8) {
        gc0308_fmt_regs[1].value = 0xb1;
        gc0308_sensor_config.dvp_cfg_info.dvp_data_fmt = DVP_RAW8;
        gc0308_sensor_config.sensor_info.fmt = SENSOR_PIXEL_FMT_Y8_1X8;
    }

    int ret = i2c_add_driver(&sensor_gc0308_driver);
    if (ret) {
        printk(KERN_ERR "gc0308: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_gc0308_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "gc0308: failed to register i2c device\n");
        i2c_del_driver(&sensor_gc0308_driver);
        return -EINVAL;
    }

    return ret;
}

static void gc0308_sensor_exit(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sensor_gc0308_driver);
}

module_init(gc0308_sensor_init);
module_exit(gc0308_sensor_exit);

MODULE_DESCRIPTION("x1830 gc0308 driver");
MODULE_LICENSE("GPL");
