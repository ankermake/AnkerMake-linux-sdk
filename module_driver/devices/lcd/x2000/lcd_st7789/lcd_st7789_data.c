#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <fb/lcdc_data.h>
#include <linux/regulator/consumer.h>

// #define USE_TE

static struct smart_lcd_data_table st7789_data_table[] = {
    {SMART_CONFIG_CMD, 0x11},

    {SMART_CONFIG_UDELAY, 120000},               //Delay 120ms

    {SMART_CONFIG_CMD, 0x36},
    {SMART_CONFIG_PRM , 0x00},

    {SMART_CONFIG_CMD, 0x3A},
    {SMART_CONFIG_PRM , 0x05},   //06

    {SMART_CONFIG_CMD, 0xB2},
    {SMART_CONFIG_PRM , 0x0C},
    {SMART_CONFIG_PRM , 0x0C},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x33},
    {SMART_CONFIG_PRM , 0x33},

    {SMART_CONFIG_CMD, 0xB7},
    {SMART_CONFIG_PRM , 0x35},

    {SMART_CONFIG_CMD, 0xBB},
    {SMART_CONFIG_PRM , 0x1A},

    {SMART_CONFIG_CMD, 0xC0},
    {SMART_CONFIG_PRM , 0x2C},

    {SMART_CONFIG_CMD, 0xC2},
    {SMART_CONFIG_PRM , 0x01},

    {SMART_CONFIG_CMD, 0xC3},
    {SMART_CONFIG_PRM , 0x0B},

    {SMART_CONFIG_CMD, 0xC4},
    {SMART_CONFIG_PRM , 0x20},

    {SMART_CONFIG_CMD, 0xC6},
    {SMART_CONFIG_PRM , 0x0F},

    {SMART_CONFIG_CMD, 0xD0},
    {SMART_CONFIG_PRM , 0xA4},
    {SMART_CONFIG_PRM , 0xA1},

    {SMART_CONFIG_CMD, 0x21},

    {SMART_CONFIG_CMD, 0xE0},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x03},
    {SMART_CONFIG_PRM , 0x07},
    {SMART_CONFIG_PRM , 0x08},
    {SMART_CONFIG_PRM , 0x07},
    {SMART_CONFIG_PRM , 0x15},
    {SMART_CONFIG_PRM , 0x2A},
    {SMART_CONFIG_PRM , 0x44},
    {SMART_CONFIG_PRM , 0x42},
    {SMART_CONFIG_PRM , 0x0A},
    {SMART_CONFIG_PRM , 0x17},
    {SMART_CONFIG_PRM , 0x18},
    {SMART_CONFIG_PRM , 0x25},
    {SMART_CONFIG_PRM , 0x27},

    {SMART_CONFIG_CMD, 0xE1},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x03},
    {SMART_CONFIG_PRM , 0x08},
    {SMART_CONFIG_PRM , 0x07},
    {SMART_CONFIG_PRM , 0x07},
    {SMART_CONFIG_PRM , 0x23},
    {SMART_CONFIG_PRM , 0x2A},
    {SMART_CONFIG_PRM , 0x43},
    {SMART_CONFIG_PRM , 0x42},
    {SMART_CONFIG_PRM , 0x09},
    {SMART_CONFIG_PRM , 0x18},
    {SMART_CONFIG_PRM , 0x17},
    {SMART_CONFIG_PRM , 0x25},
    {SMART_CONFIG_PRM , 0x27},

    {SMART_CONFIG_CMD, 0x29},
    // {SMART_CONFIG_CMD, 0x35},


    {SMART_CONFIG_CMD, 0x2A},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x23},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0xCC},

    {SMART_CONFIG_CMD, 0x2B},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x01},
    {SMART_CONFIG_PRM , 0x3F},

    // {SMART_CONFIG_CMD, 0x2C},
};

static struct smart_lcd_data_table st7789_data_table_rotator[] = {
    {SMART_CONFIG_CMD, 0x11},

    {SMART_CONFIG_UDELAY, 120000},               //Delay 120ms

    {SMART_CONFIG_CMD, 0x36},
    {SMART_CONFIG_PRM , 0xA0},

    {SMART_CONFIG_CMD, 0x3A},
    {SMART_CONFIG_PRM , 0x05},   //06

    {SMART_CONFIG_CMD, 0xB2},
    {SMART_CONFIG_PRM , 0x0C},
    {SMART_CONFIG_PRM , 0x0C},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x33},
    {SMART_CONFIG_PRM , 0x33},

    {SMART_CONFIG_CMD, 0xB7},
    {SMART_CONFIG_PRM , 0x35},

    {SMART_CONFIG_CMD, 0xBB},
    {SMART_CONFIG_PRM , 0x1A},

    {SMART_CONFIG_CMD, 0xC0},
    {SMART_CONFIG_PRM , 0x2C},

    {SMART_CONFIG_CMD, 0xC2},
    {SMART_CONFIG_PRM , 0x01},

    {SMART_CONFIG_CMD, 0xC3},
    {SMART_CONFIG_PRM , 0x0B},

    {SMART_CONFIG_CMD, 0xC4},
    {SMART_CONFIG_PRM , 0x20},

    {SMART_CONFIG_CMD, 0xC6},
    {SMART_CONFIG_PRM , 0x0F},

    {SMART_CONFIG_CMD, 0xD0},
    {SMART_CONFIG_PRM , 0xA4},
    {SMART_CONFIG_PRM , 0xA1},

    {SMART_CONFIG_CMD, 0x21},

    {SMART_CONFIG_CMD, 0xE0},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x03},
    {SMART_CONFIG_PRM , 0x07},
    {SMART_CONFIG_PRM , 0x08},
    {SMART_CONFIG_PRM , 0x07},
    {SMART_CONFIG_PRM , 0x15},
    {SMART_CONFIG_PRM , 0x2A},
    {SMART_CONFIG_PRM , 0x44},
    {SMART_CONFIG_PRM , 0x42},
    {SMART_CONFIG_PRM , 0x0A},
    {SMART_CONFIG_PRM , 0x17},
    {SMART_CONFIG_PRM , 0x18},
    {SMART_CONFIG_PRM , 0x25},
    {SMART_CONFIG_PRM , 0x27},

    {SMART_CONFIG_CMD, 0xE1},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x03},
    {SMART_CONFIG_PRM , 0x08},
    {SMART_CONFIG_PRM , 0x07},
    {SMART_CONFIG_PRM , 0x07},
    {SMART_CONFIG_PRM , 0x23},
    {SMART_CONFIG_PRM , 0x2A},
    {SMART_CONFIG_PRM , 0x43},
    {SMART_CONFIG_PRM , 0x42},
    {SMART_CONFIG_PRM , 0x09},
    {SMART_CONFIG_PRM , 0x18},
    {SMART_CONFIG_PRM , 0x17},
    {SMART_CONFIG_PRM , 0x25},
    {SMART_CONFIG_PRM , 0x27},

    {SMART_CONFIG_CMD, 0x29},
    // {SMART_CONFIG_CMD, 0x35},


    {SMART_CONFIG_CMD, 0x2B},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x23},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0xCC},

    {SMART_CONFIG_CMD, 0x2A},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x01},
    {SMART_CONFIG_PRM , 0x3F},


    // {SMART_CONFIG_CMD, 0x2C},
};

static int gpio_lcd_power_en = -1;         // GPIO_PB(19)
static int gpio_lcd_rst = -1;              // GPIO_PB(8)
static int gpio_lcd_cs = -1;               // GPIO_PB(24)
static int gpio_lcd_rd = -1;               // GPIO_PB(27)
// static int gpio_lcd_pwm = GPIO_PC(0);
static char *lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_cs, 0644);
module_param_gpio(gpio_lcd_rd, 0644);
module_param(lcd_regulator_name, charp, 0644);

static int lcd_rotator = -1;
module_param(lcd_rotator, int, 0644);

static struct regulator *st7789_regulator = NULL;

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "st7789: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
        return ret;
    }

    return 0;
}

static inline void m_gpio_free(int gpio)
{
    if (gpio >= 0)
        gpio_free(gpio);
}


static inline void m_gpio_direction_output(int gpio, int value)
{
    if (gpio >= 0)
        gpio_direction_output(gpio, value);
}

static int st7789_power_on(struct lcdc *lcdc)
{
    if (st7789_regulator)
        regulator_enable(st7789_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 0);
    usleep_range(20*1000, 20*1000);

    // m_gpio_direction_output(gpio_lcd_pwm, 1);
    // usleep_range(20*1000, 20*1000);
    // m_gpio_direction_output(gpio_lcd_pwm, 0);
    // usleep_range(20*1000, 20*1000);
    // m_gpio_direction_output(gpio_lcd_pwm, 1);

    m_gpio_direction_output(gpio_lcd_cs, 1);
    m_gpio_direction_output(gpio_lcd_rd, 1);

    m_gpio_direction_output(gpio_lcd_rst, 1);
    usleep_range(120*1000, 120*1000);

    m_gpio_direction_output(gpio_lcd_cs, 0);

    return 0;
}

static int st7789_power_off(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_power_en, 1);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    m_gpio_direction_output(gpio_lcd_cs, 1);

    if (st7789_regulator)
        regulator_disable(st7789_regulator);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name       = "st7789",
    .refresh    = 45,
    .xres       = 170,
    .yres       = 320,

    .pixclock   = 0, // 自动计算
    .fb_fmt     = fb_fmt_ARGB8888,
    .lcd_mode   = SLCD_8080,
    .out_format = OUT_FORMAT_RGB565,
    .slcd = {
        .pixclock_when_init      = 170*320*3,
        .mcu_data_width          = MCU_WIDTH_8BITS,
        .mcu_cmd_width           = MCU_WIDTH_8BITS,
        .wr_data_sample_edge     = AT_RISING_EDGE,
        .dc_pin                  = CMD_LOW_DATA_HIGH,
        .te_data_transfered_edge = AT_RISING_EDGE,
#ifdef USE_TE
        .te_pin_mode        = TE_LCDC_TRIGGER,
#else
        .te_pin_mode        = TE_NOT_EANBLE,
#endif
        .enable_rdy_pin     = 0,
        .cmd_of_start_frame = 0x2c,
    },
    .slcd_data_table        = st7789_data_table,
    .slcd_data_table_length = ARRAY_SIZE(st7789_data_table),
    .power_on               = st7789_power_on,
    .power_off              = st7789_power_off,
};

static int lcd_st7789_init(void)
{
    int ret;
    int temp;

    if (lcd_rotator) {
        temp = lcdc_data.xres;
        lcdc_data.xres = lcdc_data.yres;
        lcdc_data.yres = temp;
        lcdc_data.slcd_data_table = st7789_data_table_rotator;
        lcdc_data.slcd_data_table_length = ARRAY_SIZE(st7789_data_table_rotator);
    }

    if (gpio_lcd_cs < 0) {
        printk("st7789: must set gpio_cs\n");
        return -EINVAL;
    }

    if (strcmp("-1", lcd_regulator_name) && strlen(lcd_regulator_name)) {
        st7789_regulator = regulator_get(NULL, lcd_regulator_name);
        if(!st7789_regulator) {
            printk(KERN_ERR "lcd_regulator get err!\n");
            return -EINVAL;
        }
    }

    ret = m_gpio_request(gpio_lcd_rd, "lcd rd");
    if (ret)
        goto err_rd;

    // ret = m_gpio_request(gpio_lcd_pwm, "lcd pwm");
    // if (ret)
    //     return ret;

    ret = m_gpio_request(gpio_lcd_power_en, "lcd power-en");
    if (ret)
        goto err_power_en;

    ret = m_gpio_request(gpio_lcd_rst, "lcd reset");
    if (ret)
        goto err_rst;

    ret = m_gpio_request(gpio_lcd_cs, "lcd cs");
    if (ret)
        goto err_cs;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret) {
        printk(KERN_ERR "st7789: failed to register lcd data\n");
        goto err_lcdc_data;
    }



    return 0;
err_lcdc_data:
    m_gpio_free(gpio_lcd_cs);
err_cs:
    m_gpio_free(gpio_lcd_rst);
err_rst:
    m_gpio_free(gpio_lcd_power_en);
err_power_en:
    m_gpio_free(gpio_lcd_rd);
err_rd:
    if (st7789_regulator)
        regulator_put(st7789_regulator);
    return ret;
}

static void lcd_st7789_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    if(st7789_regulator)
        regulator_put(st7789_regulator);

    m_gpio_free(gpio_lcd_cs);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
    m_gpio_free(gpio_lcd_rd);
}

module_init(lcd_st7789_init);

module_exit(lcd_st7789_exit);

MODULE_DESCRIPTION("Ingenic ST7789 driver");
MODULE_LICENSE("GPL");
