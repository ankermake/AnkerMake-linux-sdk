#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <linux/regulator/consumer.h>

#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>

int gpio_lcd_power_en = -1;     // GPIO_PB(24) 0
int gpio_lcd_rst = -1;          // GPIO_PD(19) 0
int gpio_lcd_backlight_en = -1; // GPIO_PC(0) 0
int gpio_lcd_te = -1;           //GPIO_PB(27) 0
static char *lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_backlight_en, 0644);
module_param_gpio(gpio_lcd_te, 0644);
module_param(lcd_regulator_name, charp, 0644);

static struct regulator *st7701s_regulator = NULL;

static struct dsi_cmd_packet fitipower_st7701s_480_800_cmd_list1[] =
{

/**st7701s***/
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x13}},
    {0x15, 0xEF, 0x08},
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x10}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC0, 0x63, 0x00}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC1, 0x11, 0x0C}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC2, 0x07, 0x08}},
    {0x15, 0xCC, 0x10},
    {0x39, 0x11, 0x00, (uint8_t[]){0xB0, 0x00, 0x08, 0x0E, 0x0C, 0x10, 0x06, 0x01,0x07,0x07, 0x1E, 0x05, 0x13, 0x10, 0x2C, 0x34, 0x1E}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xB1, 0x00, 0x0F, 0x16, 0x0D, 0x10, 0x05, 0x01,0x08,0x07, 0x1E, 0x04, 0x12, 0x11, 0x28, 0x30, 0x1E}},
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x11}},
    {0x15, 0xB0, 0x72},
    {0x15, 0xB1, 0x8E},
    {0x15, 0xB2, 0x87},
    {0x15, 0xB3, 0x80},
    {0x15, 0xB5, 0x4C},
    {0x15, 0xB7, 0x85},
    {0x15, 0xB8, 0x20},
    {0x15, 0xC1, 0x78},
    {0x15, 0xC2, 0x78},
    {0x15, 0xD0, 0x88},
    {0x39, 0x07, 0x00, (uint8_t[]){0xE0, 0x00, 0x00, 0x02, 0x00, 0x00, 0x0C}},
    {0x39, 0x0C, 0x00, (uint8_t[]){0xE1, 0x06, 0x8C, 0x08, 0x8C, 0x07, 0x8C, 0x09, 0x8C, 0x00, 0x44, 0x44}},
    {0x39, 0x0D, 0x00, (uint8_t[]){0xE2, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x03, 0xA0, 0x00, 0x00, 0x03, 0xA0}},
    {0x39, 0x05, 0x00, (uint8_t[]){0xE3, 0x00, 0x00, 0x33, 0x33}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xE4, 0x44, 0x44}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xE5, 0x0D, 0x31, 0x0C, 0xA0, 0x0F, 0x33, 0x0C, 0xA0, 0x09, 0x2D, 0x0C, 0xA0, 0x0B, 0x2F, 0x0C, 0xA0}},
    {0x39, 0x05, 0x00, (uint8_t[]){0xE6, 0x00, 0x00, 0x33, 0x33}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xE7, 0x44, 0x44}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xE8, 0x0E, 0x32, 0x0C, 0xA0, 0x10, 0x34, 0x0C, 0xA0, 0x0A, 0x2E, 0x0C, 0xA0, 0x0C, 0x30, 0x0C, 0xA0}},
    {0x39, 0x07, 0x00, (uint8_t[]){0xEB, 0x00, 0x01, 0xE4, 0xE4, 0x44, 0x00}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xED, 0xF3, 0xC1, 0xAB, 0x0F, 0x67, 0x45, 0xFF, 0xFF, 0xFF, 0xFF, 0x54, 0x76, 0xF0, 0xBA, 0x1C, 0x3F}},
    {0x39, 0x07, 0x00, (uint8_t[]){0xEF, 0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F}},
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x13}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xE8, 0x00, 0x0E}},
    //{0x39, 0x03, 0x00, {0x36, 0x08, 0x00}}, /*madctl:d3:0(rgb);1:bgr*/
    /**/
    {0x05, 0x11, 0x00},//delay 120ms
     /***/
//    {0x39, 0x03, 0x00, {0xE8, 0x00, 0x0C}},//delay 20ms
//    {0x39, 0x03, 0x00, {0xE8, 0x00, 0x00}},
//    {0x39, 0x06, 0x00, {0xFF, 0x77, 0x01, 0x00, 0x00,0x00}},
//    {0x39, 0x03, 0x00, {0x29, 0x36, 0x00}},
/**st7701s***/

};

static struct dsi_cmd_packet fitipower_st7701s_480_800_cmd_list2[] =
{
    {0x39, 0x03, 0x00, (uint8_t[]){0xE8, 0x00, 0x0C}},//delay 20ms
};

static struct dsi_cmd_packet fitipower_st7701s_480_800_cmd_list3[] =
{
    {0x39, 0x03, 0x00, (uint8_t[]){0xE8, 0x00, 0x00}},
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00,0x00}},
    {0x39, 0x03, 0x00, (uint8_t[]){0x29, 0x36, 0x00}},
};

static void st7701s_init(void)
{
    int i;
    struct dsi_cmd_packet st7701s_sleep_out = {0x05, 0x11, 0x00};
    struct dsi_cmd_packet st7701s_display_on = {0x05, 0x29, 0x00};

    for (i = 0; i < ARRAY_SIZE(fitipower_st7701s_480_800_cmd_list1); i++) {
        dsi_write_cmd(&fitipower_st7701s_480_800_cmd_list1[i]);
    }
    msleep(120);
    for (i = 0; i < ARRAY_SIZE(fitipower_st7701s_480_800_cmd_list2); i++) {
        dsi_write_cmd(&fitipower_st7701s_480_800_cmd_list2[i]);
    }
    msleep(20);
    for (i = 0; i < ARRAY_SIZE(fitipower_st7701s_480_800_cmd_list3); i++) {
        dsi_write_cmd(&fitipower_st7701s_480_800_cmd_list3[i]);
    }

    dsi_write_cmd(&st7701s_sleep_out);
    msleep(120);
    dsi_write_cmd(&st7701s_display_on);
    msleep(5);
}

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "st7701s: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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
    if (gpio >= 0) {
        gpio_direction_output(gpio, value);
    }
}

static inline void m_gpio_direction_input(int gpio)
{
    if (gpio >= 0) {
        gpio_direction_input(gpio);
    }
}

static int st7701s_power_on(struct lcdc *lcdc)
{
    if (st7701s_regulator)
        regulator_enable(st7701s_regulator);

    if (gpio_lcd_power_en != -1)
        m_gpio_direction_output(gpio_lcd_power_en, 0);

    m_gpio_direction_input(gpio_lcd_te);
    msleep(50);

    m_gpio_direction_output(gpio_lcd_rst, 0);
    msleep(50);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(120);

    return 0;
}

static int st7701s_power_off(struct lcdc *lcdc)
{
    if (st7701s_regulator)
        regulator_disable(st7701s_regulator);

    if (gpio_lcd_power_en != -1)
        m_gpio_direction_output(gpio_lcd_power_en, 1);

    m_gpio_direction_output(gpio_lcd_backlight_en, 0);
    return 0;
}


static struct lcdc_data lcdc_data = {
    .name = "st7701s",
    .refresh = 60,
    .xres = 480,
    .yres = 800,
    // .xres = 268,
    // .yres = 800,
    .pixclock = 0, // 自动计算
    .left_margin = 20,
    .right_margin = 20,
    .upper_margin = 18,
    .lower_margin = 16,
    .hsync_len = 20,
    .vsync_len = 4,

    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = TFT_MIPI,
    .out_format = OUT_FORMAT_RGB888,

    .mipi = {
        .num_of_lanes = 2,
        .virtual_channel = 0,
        .color_coding = COLOR_CODE_24BIT,
        .data_en_polarity = AT_RISING_EDGE,
        .byte_clock = 0,
        .max_hs_to_lp_cycles = 100,
        .max_lp_to_hs_cycles = 40,
        .max_bta_cycles = 4095,
        .color_mode_polarity = AT_RISING_EDGE,
        .shut_down_polarity = AT_RISING_EDGE,
        .video_mode = VIDEO_BURST_WITH_SYNC_PULSES, //?

        .hsync_active_level = AT_HIGH_LEVEL,
        .vsync_active_level = AT_HIGH_LEVEL,
    },
    .height = 121,
    .width = 68,
    // .width = 700,
    // .height = 1230,

    .power_on = st7701s_power_on,
    .power_off = st7701s_power_off,
    .lcd_init = st7701s_init,
};



static int lcd_st7701s_init(void)
{
    int ret;

    if (strcmp("-1", lcd_regulator_name) && strlen(lcd_regulator_name)) {
        st7701s_regulator = regulator_get(NULL, lcd_regulator_name);
        if(!st7701s_regulator) {
            printk(KERN_ERR "lcd_regulator get err!\n");
            return -EINVAL;
        }
    }

    if (gpio_lcd_power_en != -1) {
        ret = m_gpio_request(gpio_lcd_power_en, "gpio_lcd_power_en");
        if(ret)
            return -1;
    }

    ret = m_gpio_request(gpio_lcd_rst, "gpio_lcd_rst");
    if (ret)
        goto rst_err;

    ret = m_gpio_request(gpio_lcd_backlight_en, "gpio_lcd_backlight_en");
    if(ret)
        goto backlight_err;

    ret = m_gpio_request(gpio_lcd_te, "gpio_lcd_te");
    if(ret)
        goto backlight_err;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret < 0) {
        goto register_err;
    }

    return 0;

register_err:
    gpio_free(gpio_lcd_backlight_en);
backlight_err:
    gpio_free(gpio_lcd_rst);
rst_err:
    gpio_free(gpio_lcd_power_en);
    return -1;
}


static void lcd_st7701s_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
}


module_init(lcd_st7701s_init);
module_exit(lcd_st7701s_exit);

MODULE_DESCRIPTION("Ingenic Soc st7701s_lcd driver");
MODULE_LICENSE("GPL");