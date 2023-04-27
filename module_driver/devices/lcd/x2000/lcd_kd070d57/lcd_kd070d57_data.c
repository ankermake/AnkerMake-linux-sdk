#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>

#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>


int gpio_lcd_power_en = -1; // GPIO_PB(8)
int gpio_lcd_rst = -1; // GPIO_PB(21)
int gpio_lcd_backlight_en = -1; // GPIO_PC(8)

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_backlight_en, 0644);

static struct dsi_cmd_packet kd070d57_cmd_list[] =
{
    {0x15, 0xB0, 0x5A},
    {0x15, 0xB1, 0x03},
    {0x15, 0x14, 0xE1},
};

static void kd070d57_init(void)
{
    int i;
    struct dsi_cmd_packet kd070d57_sleep_out = {0x05, 0x11, 0x00};
    struct dsi_cmd_packet kd070d57_display_on = {0x05, 0x29, 0x00};

    for (i = 0; i < ARRAY_SIZE(kd070d57_cmd_list); i++) {
        dsi_write_cmd(&kd070d57_cmd_list[i]);
    }

    dsi_write_cmd(&kd070d57_sleep_out);
    dsi_write_cmd(&kd070d57_display_on);
}

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "ma0060: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int kd070d57_power_on(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_power_en, 0);
    m_gpio_direction_output(gpio_lcd_backlight_en, 1);
    msleep(20);

    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(30);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    msleep(30);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(30);

    return 0;
}

static int kd070d57_power_off(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_power_en, 0);

    m_gpio_direction_output(gpio_lcd_backlight_en, 0);

    return 0;
}


static struct lcdc_data lcdc_data = {
    .name = "kd070d57",
    .refresh = 60,
    .xres = 600,
    .yres = 1024,
    .pixclock = 0, // 自动计算
    .left_margin = 60,
    .right_margin = 80,
    .upper_margin = 25,
    .lower_margin = 35,
    .hsync_len = 1,
    .vsync_len = 1,

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
        .video_mode = VIDEO_BURST_WITH_SYNC_PULSES,

        .hsync_active_level = AT_HIGH_LEVEL,
        .vsync_active_level = AT_HIGH_LEVEL,
    },
    .height = 153,
    .width = 90,

    .power_on = kd070d57_power_on,
    .power_off = kd070d57_power_off,
    .lcd_init = kd070d57_init,
};

static int lcd_kd070d57_init(void)
{
    int ret;

    ret = m_gpio_request(gpio_lcd_power_en, "gpio_lcd_power_en");
    if(ret)
        return -1;

    ret = m_gpio_request(gpio_lcd_rst, "gpio_lcd_rst");
    if (ret)
        goto rst_err;

    ret = m_gpio_request(gpio_lcd_backlight_en, "gpio_lcd_backlight_en");
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


static void lcd_kd070d57_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
}


module_init(lcd_kd070d57_init);
module_exit(lcd_kd070d57_exit);

MODULE_DESCRIPTION("Ingenic Soc kd070d57_lcd driver");
MODULE_LICENSE("GPL");