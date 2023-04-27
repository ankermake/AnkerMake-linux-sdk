#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <fb/lcdc_data.h>
#include <linux/regulator/consumer.h>

// #define USE_TE

static struct smart_lcd_data_table jd9851_data_table[] = {
    {SMART_CONFIG_CMD, 0xDF},
    {SMART_CONFIG_PRM, 0x98},
    {SMART_CONFIG_PRM, 0x51},
    {SMART_CONFIG_PRM, 0xE9},

    {SMART_CONFIG_CMD, 0xDE},
    {SMART_CONFIG_PRM, 0x00},

    {SMART_CONFIG_CMD, 0xB7},
    {SMART_CONFIG_PRM, 0x1E},
    {SMART_CONFIG_PRM, 0x7D},
    {SMART_CONFIG_PRM, 0x1E},
    {SMART_CONFIG_PRM, 0x2B},

    {SMART_CONFIG_CMD, 0xC8},
    {SMART_CONFIG_PRM, 0x3F},
    {SMART_CONFIG_PRM, 0x37},
    {SMART_CONFIG_PRM, 0x30},
    {SMART_CONFIG_PRM, 0x2E},
    {SMART_CONFIG_PRM, 0x31},
    {SMART_CONFIG_PRM, 0x34},
    {SMART_CONFIG_PRM, 0x2F},
    {SMART_CONFIG_PRM, 0x2F},
    {SMART_CONFIG_PRM, 0x2D},
    {SMART_CONFIG_PRM, 0x2C},
    {SMART_CONFIG_PRM, 0x27},
    {SMART_CONFIG_PRM, 0x1A},
    {SMART_CONFIG_PRM, 0x14},
    {SMART_CONFIG_PRM, 0x0A},
    {SMART_CONFIG_PRM, 0x06},
    {SMART_CONFIG_PRM, 0x0E},
    {SMART_CONFIG_PRM, 0x3F},
    {SMART_CONFIG_PRM, 0x37},
    {SMART_CONFIG_PRM, 0x30},
    {SMART_CONFIG_PRM, 0x2E},
    {SMART_CONFIG_PRM, 0x31},
    {SMART_CONFIG_PRM, 0x34},
    {SMART_CONFIG_PRM, 0x2F},
    {SMART_CONFIG_PRM, 0x2F},
    {SMART_CONFIG_PRM, 0x2D},
    {SMART_CONFIG_PRM, 0x2C},
    {SMART_CONFIG_PRM, 0x27},
    {SMART_CONFIG_PRM, 0x1A},
    {SMART_CONFIG_PRM, 0x14},
    {SMART_CONFIG_PRM, 0x0A},
    {SMART_CONFIG_PRM, 0x06},
    {SMART_CONFIG_PRM, 0x0E},

    {SMART_CONFIG_CMD, 0xB9},
    {SMART_CONFIG_PRM, 0x33},
    {SMART_CONFIG_PRM, 0x08},
    {SMART_CONFIG_PRM, 0xCC},

    {SMART_CONFIG_CMD, 0xBB},
    {SMART_CONFIG_PRM, 0x46},  /* VGH 14.75,VGL-9.6 */
    {SMART_CONFIG_PRM, 0x7A},
    {SMART_CONFIG_PRM, 0x30},
    {SMART_CONFIG_PRM, 0x40},
    {SMART_CONFIG_PRM, 0x7C},
    {SMART_CONFIG_PRM, 0x60},
    {SMART_CONFIG_PRM, 0x70},
    {SMART_CONFIG_PRM, 0x70},

    {SMART_CONFIG_CMD, 0xBC},
    {SMART_CONFIG_PRM, 0x38},
    {SMART_CONFIG_PRM, 0x3C},

    {SMART_CONFIG_CMD, 0xC0},
    {SMART_CONFIG_PRM, 0x31},
    {SMART_CONFIG_PRM, 0x20},

    {SMART_CONFIG_CMD, 0xC1},
    {SMART_CONFIG_PRM, 0x12},

    {SMART_CONFIG_CMD, 0xC3},
    {SMART_CONFIG_PRM, 0x08},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x0A},
    {SMART_CONFIG_PRM, 0x10},
    {SMART_CONFIG_PRM, 0x08},
    {SMART_CONFIG_PRM, 0x54},
    {SMART_CONFIG_PRM, 0x45},
    {SMART_CONFIG_PRM, 0x71},
    {SMART_CONFIG_PRM, 0x2C},

    {SMART_CONFIG_CMD, 0xC4},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0xA0},
    {SMART_CONFIG_PRM, 0x79},
    {SMART_CONFIG_PRM, 0x0E},
    {SMART_CONFIG_PRM, 0x0A},
    {SMART_CONFIG_PRM, 0x16},
    {SMART_CONFIG_PRM, 0x79},
    {SMART_CONFIG_PRM, 0x0E},
    {SMART_CONFIG_PRM, 0x0A},
    {SMART_CONFIG_PRM, 0x16},
    {SMART_CONFIG_PRM, 0x79},
    {SMART_CONFIG_PRM, 0x0E},
    {SMART_CONFIG_PRM, 0x0A},
    {SMART_CONFIG_PRM, 0x16},
    {SMART_CONFIG_PRM, 0x82},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x03},

    {SMART_CONFIG_CMD, 0xD0},
    {SMART_CONFIG_PRM, 0x04},
    {SMART_CONFIG_PRM, 0x0C},
    {SMART_CONFIG_PRM, 0x6B},
    {SMART_CONFIG_PRM, 0x0F},
    {SMART_CONFIG_PRM, 0x07},
    {SMART_CONFIG_PRM, 0x03},

    {SMART_CONFIG_CMD, 0xD7},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},

    {SMART_CONFIG_CMD, 0xDE},
    {SMART_CONFIG_PRM, 0x02},
    {SMART_CONFIG_CMD, 0xB8},
    {SMART_CONFIG_PRM, 0x1D},
    {SMART_CONFIG_PRM, 0xA0},
    {SMART_CONFIG_PRM, 0x2F},
    {SMART_CONFIG_PRM, 0x04},
    {SMART_CONFIG_PRM, 0x33},

    {SMART_CONFIG_CMD, 0xC1},
    {SMART_CONFIG_PRM, 0x10},
    {SMART_CONFIG_PRM, 0x66},
    {SMART_CONFIG_PRM, 0x66},
    {SMART_CONFIG_PRM, 0x01},
    {SMART_CONFIG_CMD, 0xDE},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_CMD, 0x11},
    {SMART_CONFIG_UDELAY, 120000},

    {SMART_CONFIG_CMD, 0xDE},
    {SMART_CONFIG_PRM, 0x02},
    {SMART_CONFIG_CMD, 0xC4},
    {SMART_CONFIG_PRM, 0x76},
    {SMART_CONFIG_PRM, 0x0E},
    {SMART_CONFIG_CMD, 0xC5},
    {SMART_CONFIG_PRM, 0x4E},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},

    {SMART_CONFIG_CMD, 0xCA},
    {SMART_CONFIG_PRM, 0x30},
    {SMART_CONFIG_PRM, 0x20},
    {SMART_CONFIG_PRM, 0xF4},
    {SMART_CONFIG_CMD, 0xDE},
    {SMART_CONFIG_PRM, 0x00},

    {SMART_CONFIG_CMD, 0x3A},  /* pixel format set */
    {SMART_CONFIG_PRM, 0x05},

    {SMART_CONFIG_CMD, 0x36},
    {SMART_CONFIG_PRM, 0x00},

    {SMART_CONFIG_CMD, 0x35},
    {SMART_CONFIG_PRM, 0x00},

    {SMART_CONFIG_CMD, 0x2A},  /* lcd width */
    {SMART_CONFIG_PRM, 0x00},  /* start point */
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},  /* end point */
    {SMART_CONFIG_PRM, 0xEF},

    {SMART_CONFIG_CMD, 0x2B},  /* lcd height */
    {SMART_CONFIG_PRM, 0x00},  /* start point */
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x01},  /* end point */
    {SMART_CONFIG_PRM, 0x3F},

    // {SMART_CONFIG_CMD, 0xC2},  /* test model */
    // {SMART_CONFIG_PRM, 0x08},

    {SMART_CONFIG_CMD, 0x29},  /* display on */

    // {SMART_CONFIG_CMD, 0x2C},  /* memory write */
};

static int gpio_lcd_power_en = -1;    // GPIO_PB(08)
static int gpio_lcd_rst = -1;         // GPIO_PB(17)
static int gpio_lcd_cs = -1;          // GPIO_PB(24)
static int gpio_lcd_rd = -1;          // GPIO_PB(16)
// static int gpio_lcd_pwm = -1;         // GPIO_PC(00)
static char *lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_cs, 0644);
module_param_gpio(gpio_lcd_rd, 0644);
module_param(lcd_regulator_name, charp, 0644);

static struct regulator *jd9851_regulator = NULL;

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "jd9851: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int jd9851_power_on(struct lcdc *lcdc)
{
    if (jd9851_regulator)
        regulator_enable(jd9851_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 0);
    usleep_range(20*1000, 20*1000);

    // m_gpio_direction_output(gpio_lcd_pwm, 1);

    m_gpio_direction_output(gpio_lcd_cs, 1);
    m_gpio_direction_output(gpio_lcd_rd, 1);

    m_gpio_direction_output(gpio_lcd_rst, 1);
    usleep_range(120*1000, 120*1000);

    m_gpio_direction_output(gpio_lcd_cs, 0);

    return 0;
}

static int jd9851_power_off(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_power_en, 1);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    m_gpio_direction_output(gpio_lcd_cs, 1);

    if (jd9851_regulator)
        regulator_disable(jd9851_regulator);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name       = "jd9851",
    .refresh    = 45,
    .xres       = 240,
    .yres       = 320,
    .pixclock   = 0, // 自动计算
    .fb_fmt     = fb_fmt_RGB888,
    .lcd_mode   = SLCD_8080,
    .out_format = OUT_FORMAT_RGB565,
    .slcd = {
        .pixclock_when_init      = 240*320*3,
        .mcu_data_width          = MCU_WIDTH_8BITS,
        .mcu_cmd_width           = MCU_WIDTH_8BITS,
        .wr_data_sample_edge     = AT_RISING_EDGE,
        .rd_data_sample_edge     = AT_RISING_EDGE,
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
    .slcd_data_table        = jd9851_data_table,
    .slcd_data_table_length = ARRAY_SIZE(jd9851_data_table),
    .power_on               = jd9851_power_on,
    .power_off              = jd9851_power_off,
};

static int lcd_jd9851_init(void)
{
    int ret;

    if (gpio_lcd_cs < 0) {
        printk("jd9851: must set gpio_cs\n");
        return -EINVAL;
    }

    if (strcmp("-1", lcd_regulator_name) && strlen(lcd_regulator_name)) {
        jd9851_regulator = regulator_get(NULL, lcd_regulator_name);
        if(!jd9851_regulator) {
            printk(KERN_ERR "lcd_regulator get err!\n");
            return -EINVAL;
        }
    }

    lcdc_data.slcd.rd_gpio = gpio_lcd_rd;
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
        printk(KERN_ERR "jd9851: failed to register lcd data\n");
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
    if (jd9851_regulator)
        regulator_put(jd9851_regulator);
    return ret;
}

static void lcd_jd9851_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    if(jd9851_regulator)
        regulator_put(jd9851_regulator);

    m_gpio_free(gpio_lcd_cs);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
    m_gpio_free(gpio_lcd_rd);
}

module_init(lcd_jd9851_init);

module_exit(lcd_jd9851_exit);

MODULE_DESCRIPTION("Ingenic JD9851 driver");
MODULE_LICENSE("GPL");