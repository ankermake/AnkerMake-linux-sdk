#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <linux/delay.h>
#include <common.h>
#include <vic_sensor.h>

#define GC0328_FLAG_END         0x00
#define GC0328_FLAG_DELAY       0xff
#define GC0328_PAGE_REG         0xfe
#define GC0328_CHIP_ID          0x9d

struct regval_list {
    unsigned char reg_num;
    unsigned char value;
};

#define I2C_ADDR 0x21

static int i2c_sel_gpio = -1;
static int reset_gpio = -1;
static int pwdn_gpio = -1;
static int power_gpio = -1;
static int i2c_bus_num = -1;

module_param_gpio(i2c_sel_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param_gpio(power_gpio, 0644);

module_param(i2c_bus_num, int, 0644);

struct i2c_client *i2c_dev;

#define ENDMARKER { 0xff, 0xff }
// VGA init code ok
static struct regval_list gc0328_init_regs[] = {
        {0xfe, 0x80},
        {0xfe, 0x80},
        {0xfc, 0x16},
        {0xfc, 0x16},
        {0xfc, 0x16},
        {0xfc, 0x16},
        {0xf1, 0x00},
        {0xf2, 0x00},
        {0xfe, 0x00},
        {0x4f, 0x00},
        {0x03, 0x00},
        {0x04, 0xc0},
        {0x42, 0x00},
        {0x77, 0x5a},
        {0x78, 0x40},
        {0x79, 0x56},

        {0xfe, 0x00},
        {0x0d, 0x01},
        {0x0e, 0xe8},
        {0x0f, 0x02},
        {0x10, 0x88},
        {0x09, 0x00},
        {0x0a, 0x00},
        {0x0b, 0x00},
        {0x0c, 0x00},
        {0x16, 0x00},
        {0x17, 0x16}, //mirror
        {0x18, 0x0e},
        {0x19, 0x06},

        {0x1b, 0x48},
        {0x1f, 0xC8},
        {0x20, 0x01},
        {0x21, 0x78},
        {0x22, 0xb0},
        {0x23, 0x06},
        {0x24, 0x12},
        {0x26, 0x00},

#if 1
        {0x50, 0x01},
#else
        {0xfe, 0x00},
        {0x50, 0x01},
        {0x51, 0x00}, //win_y
        {0x52, 0x00},
        {0x53, 0x00}, //win_x
        {0x54, 0x00},
        {0x55, 0x01}, //win_height
        {0x56, 0xe0},
        {0x57, 0x02}, //win_width
        {0x58, 0x80},

        {0x5a, 0x0e},
        {0x59, 0x11},
        {0x5b, 0x02},
        {0x5c, 0x04},
        {0x5d, 0x00},
        {0x5e, 0x00},
        {0x5f, 0x02},
        {0x60, 0x04},
        {0x61, 0x00},
        {0x62, 0x00},
#endif

        //global gain for range
        {0x70, 0x45},

        /////////////banding/////////////
        {0x05, 0x00},//hb
        {0x06, 0x6a},//
        {0x07, 0x00},//vb
        {0x08, 0x70},//
        {0xfe, 0x01},//

        {0x29, 0x00},//anti-flicker step [11:8]
        {0x2a, 0x96},//anti-flicker step [7:0]

        {0x2b, 0x02},//exp level 0
        {0x2c, 0x58},//

        {0x2d, 0x02},//exp level 1
        {0x2e, 0x58},//

        {0x2f, 0x02},//exp level 2
        {0x30, 0x58},//

        {0x31, 0x02},//exp level 3
        {0x32, 0x58},//

        {0xfe, 0x00},//

        ///////////////AWB//////////////
        {0xfe, 0x01},
        {0x50, 0x00},
        {0x4f, 0x00},
        {0x4c, 0x01},
        {0x4f, 0x00},
        {0x4f, 0x00},
        {0x4f, 0x00},
        {0x4f, 0x00},
        {0x4f, 0x00},
        {0x4d, 0x30},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4d, 0x40},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4d, 0x50},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4d, 0x60},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4d, 0x70},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4f, 0x01},
        {0x50, 0x88},
        {0xfe, 0x00},

        //////////// BLK//////////////////////
        {0xfe, 0x00},
        {0x27, 0xb7},
        {0x28, 0x7F},
        {0x29, 0x20},
        {0x33, 0x20},
        {0x34, 0x20},
        {0x35, 0x20},
        {0x36, 0x20},
        {0x32, 0x08},
        {0x3b, 0x00},
        {0x3c, 0x00},
        {0x3d, 0x00},
        {0x3e, 0x00},
        {0x47, 0x00},
        {0x48, 0x00},

        //////////// block enable/////////////
        {0x40, 0x6f}, // Edge enhancement disable
        {0x41, 0x26},
        {0x42, 0xfb},
        {0x44, 0x02}, //yuyv
        {0x45, 0x00},
        {0x46, 0x02},
        {0x4f, 0x01},
        {0x4b, 0x01},
        {0x50, 0x01},

        /////DN & EEINTP/////
        {0x7e, 0x0a},
        {0x7f, 0x03},
        {0x81, 0x15},
        {0x82, 0x90},
        {0x83, 0x02},
        {0x84, 0xe5},
        {0x90, 0x2c},
        {0x92, 0x02},
        {0x94, 0x02},
        {0x95, 0x35},

        ////////////YCP///////////
        {0xd1, 0x24},// 0x30 for front
        {0xd2, 0x24},// 0x30 for front
        {0xd3, 0x40},
        {0xdd, 0xd3},
        {0xde, 0x38},
        {0xe4, 0x88},
        {0xe5, 0x40},
        {0xd7, 0x0e},

        ///////////rgb gamma ////////////
        {0xfe, 0x00},
        {0xbf, 0x0e},
        {0xc0, 0x1c},
        {0xc1, 0x34},
        {0xc2, 0x48},
        {0xc3, 0x5a},
        {0xc4, 0x6e},
        {0xc5, 0x80},
        {0xc6, 0x9c},
        {0xc7, 0xb4},
        {0xc8, 0xc7},
        {0xc9, 0xd7},
        {0xca, 0xe3},
        {0xcb, 0xed},
        {0xcc, 0xf2},
        {0xcd, 0xf8},
        {0xce, 0xfd},
        {0xcf, 0xff},

        /////////////Y gamma//////////
        {0xfe, 0x00},
        {0x63, 0x00},
        {0x64, 0x05},
        {0x65, 0x0b},
        {0x66, 0x19},
        {0x67, 0x2e},
        {0x68, 0x40},
        {0x69, 0x54},
        {0x6a, 0x66},
        {0x6b, 0x86},
        {0x6c, 0xa7},
        {0x6d, 0xc6},
        {0x6e, 0xe4},
        {0x6f, 0xff},

        //////////////ASDE/////////////
        {0xfe, 0x01},
        {0x18, 0x02},
        {0xfe, 0x00},
        {0x97, 0x30},
        {0x98, 0x00},
        {0x9b, 0x60},
        {0x9c, 0x60},
        {0xa4, 0x50},
        {0xa8, 0x80},
        {0xaa, 0x40},
        {0xa2, 0x23},
        {0xad, 0x28},

        //////////////abs///////////
        {0xfe, 0x01},
        {0x9c, 0x00},
        {0x9e, 0xc0},
        {0x9f, 0x40},

        ////////////// AEC////////////
        {0xfe, 0x01},
        {0x08, 0xa0},
        {0x09, 0xe8},
        {0x10, 0x08},
        {0x11, 0x21},
        {0x12, 0x11},
        {0x13, 0x45},
        {0x15, 0xfc},
        {0x18, 0x02},
        {0x21, 0xf0},
        {0x22, 0x60},
        {0x23, 0x30},
        {0x25, 0x00},
        {0x24, 0x14},
        {0x3d, 0x80},
        {0x3e, 0x40},

        ////////////////AWB///////////
        {0xfe, 0x01},
        {0x51, 0x88},
        {0x52, 0x12},
        {0x53, 0x80},
        {0x54, 0x60},
        {0x55, 0x01},
        {0x56, 0x02},
        {0x58, 0x00},
        {0x5b, 0x02},
        {0x5e, 0xa4},
        {0x5f, 0x8a},
        {0x61, 0xdc},
        {0x62, 0xdc},
        {0x70, 0xfc},
        {0x71, 0x10},
        {0x72, 0x30},
        {0x73, 0x0b},
        {0x74, 0x0b},
        {0x75, 0x01},
        {0x76, 0x00},
        {0x77, 0x40},
        {0x78, 0x70},
        {0x79, 0x00},
        {0x7b, 0x00},
        {0x7c, 0x71},
        {0x7d, 0x00},
        {0x80, 0x70},
        {0x81, 0x58},
        {0x82, 0x98},
        {0x83, 0x60},
        {0x84, 0x58},
        {0x85, 0x50},
        {0xfe, 0x00},

        ////////////////LSC////////////////
        {0xfe, 0x01},
        {0xc0, 0x10},
        {0xc1, 0x0c},
        {0xc2, 0x0a},
        {0xc6, 0x0e},
        {0xc7, 0x0b},
        {0xc8, 0x0a},
        {0xba, 0x26},
        {0xbb, 0x1c},
        {0xbc, 0x1d},
        {0xb4, 0x23},
        {0xb5, 0x1c},
        {0xb6, 0x1a},
        {0xc3, 0x00},
        {0xc4, 0x00},
        {0xc5, 0x00},
        {0xc9, 0x00},
        {0xca, 0x00},
        {0xcb, 0x00},
        {0xbd, 0x00},
        {0xbe, 0x00},
        {0xbf, 0x00},
        {0xb7, 0x07},
        {0xb8, 0x05},
        {0xb9, 0x05},
        {0xa8, 0x07},
        {0xa9, 0x06},
        {0xaa, 0x00},
        {0xab, 0x04},
        {0xac, 0x00},
        {0xad, 0x02},
        {0xae, 0x0d},
        {0xaf, 0x05},
        {0xb0, 0x00},
        {0xb1, 0x07},
        {0xb2, 0x03},
        {0xb3, 0x00},
        {0xa4, 0x00},
        {0xa5, 0x00},
        {0xa6, 0x00},
        {0xa7, 0x00},
        {0xa1, 0x3c},
        {0xa2, 0x50},
        {0xfe, 0x00},

        ///////////////CCT ///////////
        {0xb1, 0x12},
        {0xb2, 0xf5},
        {0xb3, 0xfe},
        {0xb4, 0xe0},
        {0xb5, 0x15},
        {0xb6, 0xc8},

        /*/////skin CC for front //////
        {0xb1, 0x00},
        {0xb2, 0x00},
        {0xb3, 0x00},
        {0xb4, 0xf0},
        {0xb5, 0x00},
        {0xb6, 0x00},
        */

        ///////////////AWB////////////////
        {0xfe, 0x01},
        {0x50, 0x00},
        {0xfe, 0x01},
        {0x4f, 0x00},
        {0x4c, 0x01},
        {0x4f, 0x00},
        {0x4f, 0x00},
        {0x4f, 0x00},
        {0x4d, 0x34},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x02},
        {0x4e, 0x02},
        {0x4d, 0x44},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4d, 0x53},
        {0x4e, 0x00},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4e, 0x04},
        {0x4d, 0x65},
        {0x4e, 0x04},
        {0x4d, 0x73},
        {0x4e, 0x20},
        {0x4d, 0x83},
        {0x4e, 0x20},
        {0x4f, 0x01},
        {0x50, 0x88},

        /////////output////////
        {0xfe, 0x00},
        {0xf1, 0x07},
        {0xf2, 0x01},

        {GC0328_FLAG_END, 0x00},    /* END MARKER */
};

static struct regval_list gc0328_vga_regs[] = {
    {0x55, 0x01}, //win_height
    {0x56, 0xe0},
    {0x57, 0x02}, //win_width
    {0x58, 0x80},
    {GC0328_FLAG_END, 0x00},
};

static struct regval_list gc0328_regs_stream_on[] = {
    {0xfe, 0x00},
    {0xf1, 0x07},
    {0xf2, 0x01},
    {GC0328_FLAG_END, 0x00},
};

static struct regval_list gc0328_regs_stream_off[] = {
    {0xfe, 0x00},
    {0xf1, 0x00},
    {0xf2, 0x00},
    {GC0328_FLAG_END, 0x00},
};

static int gc0328_read(struct i2c_client *i2c, unsigned char reg,
        unsigned char *value)
{
    struct i2c_msg msg[2] = {
        [0] = {
            .addr = i2c->addr,
            .flags    = 0,
            .len    = 1,
            .buf    = &reg,
        },
        [1] = {
            .addr = i2c->addr,
            .flags    = I2C_M_RD,
            .len    = 1,
            .buf    = value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

    return ret;
}

void m_msleep(int ms)
{
    usleep_range(ms*1000, ms*1000);
}

static int gc0328_write(struct i2c_client *i2c, unsigned char reg,
        unsigned char value)
{
    unsigned char buf[2] = {reg, value};
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

static inline int gc0328_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != GC0328_FLAG_END) {
        if(vals->reg_num == GC0328_FLAG_DELAY) {
                msleep(vals->value);
        } else {
            ret = gc0328_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
            if (vals->reg_num == GC0328_PAGE_REG){
                val &= 0xf8;
                val |= (vals->value & 0x07);
                ret = gc0328_write(i2c, vals->reg_num, val);
                ret = gc0328_read(i2c, vals->reg_num, &val);
            }
            printk(KERN_ERR "%x[%x]\n", vals->reg_num, val);
        }
        vals++;
    }

    return 0;
}

static int gc0328_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != GC0328_FLAG_END) {
        if (vals->reg_num == GC0328_FLAG_DELAY) {
                msleep(vals->value);
        } else {
            ret = gc0328_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
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
        ret = gpio_request(reset_gpio,"gc0328_reset");
        if (ret) {
            printk(KERN_ERR "gc0328: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gc0328_power");
        if (ret) {
            printk(KERN_ERR "gc0328: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio,"gc0328_pwdn");
        if (ret) {
            printk(KERN_ERR "gc0328: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (i2c_sel_gpio != -1) {
        ret = gpio_request(i2c_sel_gpio,"i2c_sel_gpio");
        if (ret) {
            printk(KERN_ERR "gc0328: failed to request i2c_sel_gpio pin: %s\n", gpio_to_str(i2c_sel_gpio, gpio_str));
            goto err_i2c_sel_gpio;
        }
    }

    return 0;

err_i2c_sel_gpio:
    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);
err_pwdn_gpio:
    if (power_gpio != -1)
        gpio_free(power_gpio);
err_power_gpio:
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

    if (reset_gpio != -1)
        gpio_free(reset_gpio);

    if (power_gpio != -1)
        gpio_free(power_gpio);

    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);

    dvp_deinit_gpio();
}

static void gc0328_power_off(void)
{
    if (i2c_sel_gpio != -1)
       gpio_direction_output(i2c_sel_gpio, 0);

    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 1);

    vic_disable_sensor_mclk();
}

static int gc0328_power_on(void)
{
    int ret;

    vic_enable_sensor_mclk(24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1) {
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(10);
    }

    if (reset_gpio != -1) {
        gpio_direction_output(reset_gpio, 1);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(1);
    }

    if (i2c_sel_gpio != -1)
        gpio_direction_output(i2c_sel_gpio, 1);

    /* check sensor product ID
     */
    unsigned char id = 0;
    ret = gc0328_read(i2c_dev, 0xf0, &id);
    if (ret < 0) {
        printk(KERN_ERR "gc0328: Failed to read chip id\n");
        gc0328_power_off();
        return -ENODEV;
    }

    if (id != GC0328_CHIP_ID) {
        printk(KERN_ERR "gc0328: id1 not match: 0x%x", (int)id);
        gc0328_power_off();
        return -ENODEV;
    }


    /* init sensor register
     */
    ret = gc0328_write_array(i2c_dev, gc0328_init_regs);
    if (ret) {
        printk(KERN_ERR "gc0328: failed to init setting\n");
        gc0328_power_off();
        return ret;
    }

    /* Set resolution
     */
    ret = gc0328_write_array(i2c_dev, gc0328_vga_regs);
    if (ret < 0) {
        printk(KERN_ERR "gc0328: failed to write window size %d", ret);
        gc0328_power_off();
        return ret;
    }

    return 0;
}

static int gc0328_stream_on(void)
{
    int ret = gc0328_write_array(i2c_dev, gc0328_regs_stream_on);
    if (ret)
        printk(KERN_ERR "gc0328: failed to stream on\n");

    return ret;
}

static void gc0328_stream_off(void)
{
    int ret = gc0328_write_array(i2c_dev, gc0328_regs_stream_off);
    if (ret)
        printk(KERN_ERR "gc0328: failed to stream off\n");
}


static struct vic_sensor_config gc0328_sensor_config = {
    .device_name            = "gc0328",

    .vic_interface          = VIC_dvp,
    .dvp_cfg_info = {
        .dvp_data_fmt       = DVP_YUV422,
        .dvp_gpio_mode      = DVP_PA_LOW_8BIT,
        .dvp_timing_mode    = DVP_href_mode,
        .dvp_yuv_data_order = order_1_2_3_4,
        .dvp_hsync_polarity = POLARITY_HIGH_ACTIVE,
        .dvp_vsync_polarity = POLARITY_LOW_ACTIVE,
        .dvp_img_scan_mode  = DVP_img_scan_progress,
    },

    .isp_clk_rate           = 90 * 1000 * 1000,

    .sensor_info = {
        .width              = 640,
        .height             = 480,
        .fmt                = SENSOR_PIXEL_FMT_YUYV8_2X8,
        .fps                = 25 << 16 | 1,
    },

    .ops = {
        .power_on           = gc0328_power_on,
        .power_off          = gc0328_power_off,
        .stream_on          = gc0328_stream_on,
        .stream_off         = gc0328_stream_off,
    },
};

static int sensor_gc0328_probe(struct i2c_client *client,
                 const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = vic_register_sensor(&gc0328_sensor_config);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sensor_gc0328_remove(struct i2c_client *client)
{
    vic_unregister_sensor(&gc0328_sensor_config);
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id gc0328_id_table[] = {
    { "md_sensor_gc0328", 0 },
    {},
};

static struct i2c_driver sensor_gc0328_driver = {
    .driver = {
        .name = "md_sensor_gc0328",
    },
    .probe = sensor_gc0328_probe,
    .remove = sensor_gc0328_remove,
    .id_table = gc0328_id_table,
};

static struct i2c_board_info sensor_gc0328_info = {
    .type = "md_sensor_gc0328",
    .addr = I2C_ADDR,
};

static int gc0328_sensor_init(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "gc0328: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&sensor_gc0328_driver);
    if (ret) {
        printk(KERN_ERR "gc0328: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_gc0328_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "gc0328: failed to register i2c device\n");
        i2c_del_driver(&sensor_gc0328_driver);
        return -EINVAL;
    }

    return ret;
}

module_init(gc0328_sensor_init);

static void gc0328_sensor_exit(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sensor_gc0328_driver);
}

module_exit(gc0328_sensor_exit);

MODULE_DESCRIPTION("x1021 gc0328 driver");
MODULE_LICENSE("GPL");
