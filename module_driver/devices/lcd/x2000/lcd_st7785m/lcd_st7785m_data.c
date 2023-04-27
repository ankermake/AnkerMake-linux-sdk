#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <linux/regulator/consumer.h>
#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>

static int gpio_lcd_power_en = -1;          // -1
static int gpio_lcd_rst = -1;               // GPIO_PC(14)
static char *lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param(lcd_regulator_name, charp, 0644);

static struct regulator *st7785m_regulator = NULL;

static struct dsi_cmd_packet st7785m_cmd_list[] =
{
    //hw_reset
    // {SMART_CONFIG_UDELAY, 120000},  //Delay 120ms
    // {0x05, 0x11, 0x00}, //start
    // {SMART_CONFIG_UDELAY, 120000},  //Delay 120ms

    {0x39, 0x03, 0x00, (uint8_t[]){0x36,0x00,0x00}},
    {0x39, 0x03, 0x00, (uint8_t[]){0x3A,0x00,0x66}},
    /* MIPI Video Mode */
    {0x39, 0x03, 0x00, (uint8_t[]){0xB0,0x10,0x10}},
    {0x39, 0x0B, 0x00, (uint8_t[]){0xB2,0x00,0x0c,0x00,0x0c,0x00,0x00,0x00,0x33,0x00,0x33}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xB7,0x00,0x70}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xBB,0x00,0x27}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC0,0x00,0x2C}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC2,0x00,0x01}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC3,0x00,0x11}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC6,0x00,0x0F}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xD0,0x00,0xA7}},
    {0x39, 0x05, 0x00, (uint8_t[]){0xD0,0x00,0xA4,0x00,0xA1}},
    {0x39, 0x1D, 0x00, (uint8_t[]){0xE0,0x00,0xD0,0x00,0x02,0x00,0x0A,0x00,0x11,0x00,0x13,0x00,0x2D,0x00,0x33,0x00,0x43,0x00,0x45,0x00,0x36,0x00,0x0B,0x00,0x0A,0x00,0x14,0x00,0x18}},
    {0x39, 0x1D, 0x00, (uint8_t[]){0xE1,0x00,0xD0,0x00,0x01,0x00,0x05,0x00,0x07,0x00,0x09,0x00,0x25,0x00,0x34,0x00,0x44,0x00,0x46,0x00,0x0F,0x00,0x1F,0x00,0x1F,0x00,0x1F,0x00,0x23}},

    // {0x05, 0x29, 0x00}, //display on
};

static void st7785m_init(void)
{
    int i;

    struct dsi_cmd_packet st7785m_sleep_out = {0x05, 0x11, 0x00};
    struct dsi_cmd_packet st7785m_display_on = {0x05, 0x29, 0x00};

    msleep(120);
    dsi_write_cmd(&st7785m_sleep_out);
    msleep(120);

    for (i = 0; i < ARRAY_SIZE(st7785m_cmd_list); i++)
        dsi_write_cmd(&st7785m_cmd_list[i]);

    dsi_write_cmd(&st7785m_display_on);
    msleep(5);
}

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "st7785m: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static inline void m_gpio_direction_input(int gpio)
{
    if (gpio >= 0) {
        gpio_direction_input(gpio);
    }
}

static int st7785m_power_on(struct lcdc *lcdc)
{
    if (st7785m_regulator)
        regulator_enable(st7785m_regulator);

    if (gpio_lcd_power_en != -1) {
        m_gpio_direction_output(gpio_lcd_power_en, 0);
        usleep_range(20*1000, 20*1000);
    }

    m_gpio_direction_output(gpio_lcd_rst, 1);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    msleep(120);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(120);

    return 0;
}

static int st7785m_power_off(struct lcdc *lcdc)
{
    if (st7785m_regulator)
        regulator_disable(st7785m_regulator);

    if (gpio_lcd_power_en != -1)
        m_gpio_direction_output(gpio_lcd_power_en, 1);

    m_gpio_direction_output(gpio_lcd_rst, 0);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name       = "st7785m",
    .refresh    = 60,
    .xres       = 240,
    .yres       = 320,
    .pixclock   = 0, // 自动计算

    .left_margin = 40,
    .right_margin = 40,
    .upper_margin = 20,
    .lower_margin = 20,
    .hsync_len = 40,
    .vsync_len = 8,

    .fb_fmt     = fb_fmt_ARGB8888,
    .lcd_mode   = TFT_MIPI,
    .out_format = OUT_FORMAT_RGB666,
    .mipi = {
        .num_of_lanes = 1,
        .virtual_channel = 0,
        .color_coding = COLOR_CODE_18BIT_CONFIG1,
        .data_en_polarity = AT_RISING_EDGE,
        .byte_clock = 0,
        .max_hs_to_lp_cycles = 100,
        .max_lp_to_hs_cycles = 40,
        .max_bta_cycles = 4095,
        .color_mode_polarity = AT_RISING_EDGE,
        .shut_down_polarity = AT_RISING_EDGE,
        .video_mode = VIDEO_BURST_WITH_SYNC_PULSES,

        .hsync_active_level = AT_HIGH_LEVEL,
        .vsync_active_level = AT_HIGH_LEVEL,
    },
    .height = 64,
    .width = 48,

    .power_on               = st7785m_power_on,
    .power_off              = st7785m_power_off,
    .lcd_init               = st7785m_init,
};

static int lcd_st7785m_init(void)
{
    int ret;

    if (strcmp("-1", lcd_regulator_name) && strlen(lcd_regulator_name)) {
        st7785m_regulator = regulator_get(NULL, lcd_regulator_name);
        if(!st7785m_regulator) {
            printk(KERN_ERR "lcd_regulator get err!\n");
            return -EINVAL;
        }
    }

    if (gpio_lcd_power_en != -1) {
        ret = m_gpio_request(gpio_lcd_power_en, "lcd power_en");
        if (ret)
            goto err_power_en;
    }

    ret = m_gpio_request(gpio_lcd_rst, "lcd reset");
    if (ret)
        goto err_rst;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret) {
        printk(KERN_ERR "st7785m: failed to register lcd data\n");
        goto err_lcdc_data;
    }

    return 0;
err_lcdc_data:
    m_gpio_free(gpio_lcd_rst);
err_rst:
    if (gpio_lcd_power_en != -1)
        m_gpio_free(gpio_lcd_power_en);
err_power_en:
    if (st7785m_regulator)
        regulator_put(st7785m_regulator);
    return ret;
}

static void lcd_st7785m_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    if(st7785m_regulator)
        regulator_put(st7785m_regulator);

    if (gpio_lcd_power_en != -1)
        m_gpio_free(gpio_lcd_power_en);

    m_gpio_free(gpio_lcd_rst);
}

module_init(lcd_st7785m_init);

module_exit(lcd_st7785m_exit);

MODULE_DESCRIPTION("Ingenic st7785m driver");
MODULE_LICENSE("GPL");