#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>

#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>
#include <linux/regulator/consumer.h>


int gpio_lcd_power_en = -1; // GPIO_PC(3)
int gpio_lcd_rst = -1; // GPIO_PC(4)
int gpio_lcd_backlight_en = -1; // -1
static char *gpio_lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_backlight_en, 0644);
module_param(gpio_lcd_regulator_name, charp, 0644);

static struct regulator *ili9881d_regulator = NULL;


static struct dsi_cmd_packet ili9881d_cmd_list[] = {
    {0x39, 0x04, 0x00, (uint8_t[]){0xFF,0x98,0x81,0x01}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x01,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x02,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x03,0x56}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x04,0x13}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x05,0x13}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x06,0x0a}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x07,0x05}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x08,0x05}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x09,0x1D}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x0a,0x01}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x0b,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x0c,0x3F}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x0d,0x29}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x0e,0x29}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x0f,0x1D}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x10,0x1D}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x11,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x12,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x13,0x08}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x14,0x08}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x15,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x16,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x17,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x18,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x19,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x1a,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x1b,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x1c,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x1d,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x1e,0x40}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x1f,0x88}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x20,0x08}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x21,0x01}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x22,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x23,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x24,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x25,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x26,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x27,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x28,0x33}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x29,0x03}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x2a,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x2b,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x2c,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x2d,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x2e,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x2f,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x30,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x31,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x32,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x33,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x34,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x35,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x36,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x37,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x38,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x39,0x0f}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x3a,0x2a}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x3b,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x3c,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x3d,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x3e,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x3f,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x40,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x41,0xe0}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x42,0x40}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x43,0x0f}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x44,0x11}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x45,0xa8}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x46,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x47,0x08}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x48,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x49,0x01}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x4a,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x4b,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x4c,0xb2}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x4d,0x22}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x4e,0x07}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x4f,0xf1}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x50,0x29}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x51,0x72}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x52,0x25}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x53,0xb2}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x54,0x22}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x55,0x22}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x56,0x22}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x57,0xa2}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x58,0x22}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x59,0x06}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x5a,0xe1}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x5b,0x28}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x5c,0x62}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x5d,0x24}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x5e,0xa2}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x5f,0x22}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x60,0x22}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x61,0x22}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x62,0xee}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x63,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x64,0x0b}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x65,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x66,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x67,0x0F}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x68,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x69,0x01}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x6a,0x07}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x6b,0x55}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x6c,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x6d,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x6e,0x5b}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x6f,0x59}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x70,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x71,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x72,0x57}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x73,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x74,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x75,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x76,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x77,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x78,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x79,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x7a,0x0a}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x7b,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x7c,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x7d,0x0E}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x7e,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x7f,0x01}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x80,0x06}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x81,0x54}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x82,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x83,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x84,0x5a}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x85,0x58}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x86,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x87,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x88,0x56}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x89,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x8a,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x8b,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x8c,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x8d,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x8e,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x8f,0x44}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x90,0x44}},

    {0x39, 0x04, 0x00, (uint8_t[]){0xFF,0x98,0x81,0x02}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x01,0x50}},    ////SDT=3us
    {0x39, 0x02, 0x00, (uint8_t[]){0x15,0x10}},   //timeout black
    {0x39, 0x02, 0x00, (uint8_t[]){0x42,0x01}},   //Data_in

    {0x39, 0x15, 0x00, (uint8_t[]){0x57,0x00,0x1A,0x29,0x13,0x17,0x2B,0x1F,0x20,0x8E,0x1E,0x2A,0x76,0x1A,0x17,0x4B,0x21,0x27,0x45,0x53,0x24}},
    {0x39, 0x15, 0x00, (uint8_t[]){0x6B,0x00,0x1A,0x29,0x13,0x17,0x2B,0x1F,0x20,0x8E,0x1E,0x2A,0x76,0x1A,0x17,0x4B,0x21,0x27,0x45,0x53,0x24}},

    {0x39, 0x04, 0x00, (uint8_t[]){0xFF,0x98,0x81,0x05}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x03,0x00}},    //VCM1[8]
    {0x39, 0x02, 0x00, (uint8_t[]){0x04,0x1E}},    //VCM1[7:0] (-0.324V @9881V)
    {0x39, 0x02, 0x00, (uint8_t[]){0x1A,0x50}},    //debounce 32us
    {0x39, 0x02, 0x00, (uint8_t[]){0x1B,0x09}},    //11, keep LVD function
    {0x39, 0x02, 0x00, (uint8_t[]){0x1E,0x11}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x26,0x0E}},    //Auto 1/2 VCOM-new (9881V)
    {0x39, 0x02, 0x00, (uint8_t[]){0x38,0xA0}},    //IOVCC LVD (IOVCC 1.4V @9881V)
    {0x39, 0x02, 0x00, (uint8_t[]){0x4C,0x11}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x4D,0x22}},    //bypass VREG to FPC
    {0x39, 0x02, 0x00, (uint8_t[]){0x54,0x28}},    //VGH CLP=12.03
    {0x39, 0x02, 0x00, (uint8_t[]){0x55,0x25}},    //VGL CLP=-12.03
    {0x39, 0x02, 0x00, (uint8_t[]){0x78,0x01}},
    {0x39, 0x02, 0x00, (uint8_t[]){0xA9,0xC0}},
    {0x39, 0x02, 0x00, (uint8_t[]){0xB1,0x70}},
    {0x39, 0x02, 0x00, (uint8_t[]){0xB2,0x70}},    //panda timing clr delay 1 frame

    {0x39, 0x04, 0x00, (uint8_t[]){0xFF,0x98,0x81,0x06}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x01,0x03}},    //LEDPWM/SDO hi-z
    {0x39, 0x02, 0x00, (uint8_t[]){0x2B,0x0A}},    //BGR_PANEL+SS_PANEL BW:09
    {0x39, 0x02, 0x00, (uint8_t[]){0x04,0x73}},    //70£º4lane, 71£º3lane,   73£º2lane,
    {0x39, 0x02, 0x00, (uint8_t[]){0xC0,0x7F}},    //720*1280
    {0x39, 0x02, 0x00, (uint8_t[]){0xC1,0x2A}},    //720*1280

    {0x39, 0x04, 0x00, (uint8_t[]){0xFF,0x98,0x81,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x35,0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0x36,0x00}},
};

static void ili9881d_init(void)
{
    int i;
    struct dsi_cmd_packet ili9881d_sleep_out = {0x05, 0x11, 0x00};
    struct dsi_cmd_packet ili9881d_display_on = {0x05, 0x29, 0x00};

    for (i = 0; i < ARRAY_SIZE(ili9881d_cmd_list); i++) {
        dsi_write_cmd(&ili9881d_cmd_list[i]);
    }

    dsi_write_cmd(&ili9881d_sleep_out);
    msleep(120);
    dsi_write_cmd(&ili9881d_display_on);
    msleep(5);

}

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "ili9881d: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int ili9881d_power_on(struct lcdc *lcdc)
{
    if (ili9881d_regulator)
        regulator_enable(ili9881d_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 0);
    m_gpio_direction_output(gpio_lcd_backlight_en, 1);
    msleep(50);

    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(30);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    msleep(10);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(120);

    return 0;
}

static int ili9881d_power_off(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_power_en, 0);

    m_gpio_direction_output(gpio_lcd_backlight_en, 0);

    if (ili9881d_regulator)
        regulator_disable(ili9881d_regulator);

    return 0;
}


static struct lcdc_data lcdc_data = {
    .name = "ili9881d",
    .refresh = 60,
    .xres = 720,
    .yres = 1280,
    .pixclock = 0, // 自动计算
    .left_margin = 32,
    .right_margin = 48,
    .upper_margin = 12,
    .lower_margin = 16,
    .hsync_len = 10,
    .vsync_len = 8,

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
    .height = 110,
    .width = 62,

    .power_on = ili9881d_power_on,
    .power_off = ili9881d_power_off,
    .lcd_init = ili9881d_init,
};

static int lcd_ili9881d_init(void)
{
    int ret;

    if (strcmp("-1", gpio_lcd_regulator_name) && strlen(gpio_lcd_regulator_name)) {
        ili9881d_regulator = regulator_get(NULL, gpio_lcd_regulator_name);
        if(!ili9881d_regulator) {
            printk(KERN_ERR "lcd_regulator get err!\n");
            return -EINVAL;
        }
    }

    ret = m_gpio_request(gpio_lcd_power_en, "gpio_lcd_power_en");
    if(ret)
        goto power_on_err;

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
    m_gpio_free(gpio_lcd_backlight_en);
backlight_err:
    m_gpio_free(gpio_lcd_rst);
rst_err:
    m_gpio_free(gpio_lcd_power_en);
power_on_err:
    if (ili9881d_regulator)
        regulator_put(ili9881d_regulator);
    return -1;
}


static void lcd_ili9881d_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    if(ili9881d_regulator)
        regulator_put(ili9881d_regulator);

    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
}


module_init(lcd_ili9881d_init);
module_exit(lcd_ili9881d_exit);

MODULE_DESCRIPTION("Ingenic Soc ili9881d_lcd driver");
MODULE_LICENSE("GPL");