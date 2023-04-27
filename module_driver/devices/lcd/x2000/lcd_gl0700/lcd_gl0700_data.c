#include <utils/gpio.h>
#include <utils/spi.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>

#include <fb/lcdc_data.h>

static int gpio_lcd_power_en    = -1; // GPIO_PC(09)

module_param_gpio(gpio_lcd_power_en, 0644);

static inline void m_msleep(int ms)
{
    usleep_range(ms * 1000, ms * 1000);
}

static int gl0700_power_on(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0) {
        gpio_direction_output(gpio_lcd_power_en, 1);
        m_msleep(180);
    }

    return 0;
}

static int gl0700_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0)
        gpio_direction_output(gpio_lcd_power_en, 0);

    return 0;
}

struct lcdc_data lcdc_data = {
    .name = "gl0700",
    .refresh = 60,
    .xres = 800,
    .yres = 480,
    .pixclock = 0, // 自动计算
    .left_margin = 40,
    .right_margin = 40,
    .upper_margin = 31,
    .lower_margin = 13,
    .hsync_len = 48,
    .vsync_len = 1,
    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = TFT_24BITS,
    .out_format = OUT_FORMAT_RGB888,
    .tft = {
        .even_line_order = ORDER_RGB,
        .odd_line_order = ORDER_RGB,
        .pix_clk_polarity = AT_FALLING_EDGE,
        .de_active_level = AT_HIGH_LEVEL,
        .hsync_vsync_active_level = AT_LOW_LEVEL,
    },
    .power_on = gl0700_power_on,
    .power_off = gl0700_power_off,
    .tft.pix_clk_inv = 1,//防止闪屏
};

static int __init gl0700_init(void)
{
    int ret;

    if (gpio_lcd_power_en >= 0) {
        ret = gpio_request(gpio_lcd_power_en, "lcd_power_en");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "gl0700: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_power_en, buf));
            return ret;
        }
    }

    jzfb_register_lcd(&lcdc_data);

    return 0;
}


static void __exit gl0700_exit(void)
{
    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);
}

module_init(gl0700_init);
module_exit(gl0700_exit);

MODULE_DESCRIPTION("gl0700 lcd panel driver");
MODULE_LICENSE("GPL");