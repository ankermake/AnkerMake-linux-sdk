#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <common.h>


#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>

struct dsi_cmd_packet visionox_ma0060_720p_cmd_list1[] =
{
    {0X39, 0X02, 0X00, (uint8_t[]){0XFE, 0XD0}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X07, 0X84}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X40, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X4B, 0X4C}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X49, 0X01}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XFE, 0X40}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XC7, 0X85}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XC8, 0X32}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XC9, 0X18}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XCA, 0X09}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XCB, 0X22}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XCC, 0X44}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XCD, 0X11}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X05, 0X0F}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X06, 0X09}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X08, 0X0F}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X09, 0X09}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X0A, 0XE6}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X0B, 0X88}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X0D, 0X90}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X0E, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X20, 0X93}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X21, 0X93}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X24, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X26, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X28, 0X05}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X2A, 0X05}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X2D, 0X23}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X2F, 0X23}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X30, 0X23}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X31, 0X23}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X36, 0X55}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X37, 0X80}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X38, 0X50}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X39, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X46, 0X27}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X6F, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X74, 0X2F}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X75, 0X19}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X79, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XAD, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XFE, 0X60}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X00, 0XC4}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X01, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X02, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X03, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X04, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X05, 0X07}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X06, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X07, 0X83}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X09, 0XC4}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X0A, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X0B, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X0C, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X0D, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X0E, 0X08}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X0F, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X10, 0X83}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X12, 0XCC}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X13, 0X0F}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X14, 0XFF}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X15, 0X01}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X16, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X17, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X18, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X19, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X1B, 0XC4}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X1C, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X1D, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X1E, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X1F, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X20, 0X08}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X21, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X22, 0X89}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X24, 0XC4}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X25, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X26, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X27, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X28, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X29, 0X07}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X2A, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X2B, 0X89}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X2F, 0XC4}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X30, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X31, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X32, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X33, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X34, 0X06}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X35, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X36, 0X89}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X38, 0XC4}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X39, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X3A, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X3B, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X3D, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X3F, 0X07}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X40, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X41, 0X89}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X4C, 0XC4}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X4D, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X4E, 0X04}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X4F, 0X01}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X50, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X51, 0X08}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X52, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X53, 0X61}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X55, 0XC4}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X56, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X58, 0X04}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X59, 0X01}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X5A, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X5B, 0X06}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X5C, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X5D, 0X61}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X5F, 0XCE}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X60, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X61, 0X07}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X62, 0X05}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X63, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X64, 0X04}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X65, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X66, 0X60}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X67, 0X80}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X9B, 0X03}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XA9, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XAA, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XAB, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XAC, 0X04}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XAD, 0X03}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XAE, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XAF, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XB0, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XB1, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XB2, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XB3, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XB4, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XB5, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XB6, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XB7, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XB8, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XB9, 0X08}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XBA, 0X09}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XBB, 0X0A}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XBC, 0X05}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XBD, 0X06}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XBE, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XBF, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XC0, 0X10}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XC1, 0X03}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XC4, 0X80}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XFE, 0X70}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X48, 0X05}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X52, 0X00}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X5A, 0XFF}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X5C, 0XF6}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X5D, 0X07}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X7D, 0X75}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X86, 0X07}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XA7, 0X02}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XA9, 0X2C}},
    {0X39, 0X02, 0X00, (uint8_t[]){0XFE, 0XA0}},
    {0X39, 0X02, 0X00, (uint8_t[]){0X2B, 0X18}},

    {0x39, 0x02, 0x00, (uint8_t[]){0xFE, 0xD0}}, /* 【2 lane 设置】 */
    {0x39, 0x02, 0x00, (uint8_t[]){0x1E, 0x05}},

    {0x39, 0x02, 0x00, (uint8_t[]){0xFE, 0x00}},
    {0x39, 0x05, 0x00, (uint8_t[]){0x2A, 0x00, 0x00, 0x02, 0xCF}},
    {0x39, 0x05, 0x00, (uint8_t[]){0x2B, 0x00, 0x00, 0x04, 0xFF}},
};

struct dsi_cmd_packet visionox_ma0060_720p_cmd_list2[] =
{
    {0x15, 0xFE, 0x00},
    {0x15, 0xC2, 0x08},
    {0x15, 0x35, 0x00},
};

int gpio_lcd_power_en = -1; // GPIO_PB(8)
int power_valid_level = -1; //0,1
int gpio_lcd_rst = -1; // GPIO_PB(21)
int gpio_lcd_backlight_en = -1; // -1

module_param(power_valid_level, int, 0644);
module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_backlight_en, 0644);


static void ma0060_init(void)
{
    struct dsi_cmd_packet ma0060_sleep_out = {0x05, 0x11, 0x00};
    struct dsi_cmd_packet ma0060_display_on = {0x05, 0x29, 0x00};
    int i;

    for (i = 0; i < ARRAY_SIZE(visionox_ma0060_720p_cmd_list1); i++)
        dsi_write_cmd(&visionox_ma0060_720p_cmd_list1[i]);

    msleep(20);

    for (i = 0; i < ARRAY_SIZE(visionox_ma0060_720p_cmd_list2); i++)
        dsi_write_cmd(&visionox_ma0060_720p_cmd_list2[i]);


    dsi_write_cmd(&ma0060_sleep_out);
    usleep_range(120*1000, 120*1000);

    dsi_write_cmd(&ma0060_display_on);
    usleep_range(80*1000, 80*1000);

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

static int ma0060_power_on(struct lcdc *lcdc)
{
    if (power_valid_level)
        m_gpio_direction_output(gpio_lcd_power_en, 1);
    else
        m_gpio_direction_output(gpio_lcd_power_en, 0);

    m_gpio_direction_output(gpio_lcd_backlight_en, 1);

    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(100);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    msleep(150);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(100);

    return 0;
}

static int ma0060_power_off(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_power_en, 0);
    m_gpio_direction_output(gpio_lcd_backlight_en, 0);
    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "ma0060",
    .refresh = 60,
    .xres = 720,
    .yres = 1280,
    .pixclock = 0, // 自动计算

    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = SLCD_MIPI,
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

        .slcd_te_pin_mode = TE_LCDC_TRIGGER,
        .slcd_te_data_transfered_edge = AT_RISING_EDGE,

    },
    .power_on = ma0060_power_on,
    .power_off = ma0060_power_off,
    .lcd_init = ma0060_init,
};

static int lcd_ma0060_init(void)
{
    int ret;

    ret = m_gpio_request(gpio_lcd_power_en, "gpio_lcd_power_en");
    if(ret)
        return -1;

    ret = m_gpio_request(gpio_lcd_rst, "gpio_lcd_rst");
    if (ret)
        goto rst_err;

    ret = m_gpio_request(gpio_lcd_backlight_en, "gpio_lcd_backlight_en");
    if (ret)
        goto backlight_err;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret < 0) {
        printk("fb_register err!\n");
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


static void lcd_ma0060_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
}


module_init(lcd_ma0060_init);
module_exit(lcd_ma0060_exit);

MODULE_DESCRIPTION("Ingenic Soc ma0060_lcd driver");
MODULE_LICENSE("GPL");