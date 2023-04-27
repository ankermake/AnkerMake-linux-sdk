#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include "soc/x1830/fb/lcdc_data.h"

#define USE_TE

static struct smart_lcd_data_table kwh035_data_table[] = {
    {SMART_CONFIG_CMD, 0xE0},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x07},
    {SMART_CONFIG_PRM, 0x10},
    {SMART_CONFIG_PRM, 0x09},
    {SMART_CONFIG_PRM, 0x17},
    {SMART_CONFIG_PRM, 0x0B},
    {SMART_CONFIG_PRM, 0x40},
    {SMART_CONFIG_PRM, 0x8A},
    {SMART_CONFIG_PRM, 0x4B},
    {SMART_CONFIG_PRM, 0x0A},
    {SMART_CONFIG_PRM, 0x0D},
    {SMART_CONFIG_PRM, 0x0F},
    {SMART_CONFIG_PRM, 0x15},
    {SMART_CONFIG_PRM, 0x16},
    {SMART_CONFIG_PRM, 0x0F},

    {SMART_CONFIG_CMD, 0xE1},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x1A},
    {SMART_CONFIG_PRM, 0x1B},
    {SMART_CONFIG_PRM, 0x02},
    {SMART_CONFIG_PRM, 0x0D},
    {SMART_CONFIG_PRM, 0x05},
    {SMART_CONFIG_PRM, 0x30},
    {SMART_CONFIG_PRM, 0x35},
    {SMART_CONFIG_PRM, 0x43},
    {SMART_CONFIG_PRM, 0x02},
    {SMART_CONFIG_PRM, 0x0A},
    {SMART_CONFIG_PRM, 0x09},
    {SMART_CONFIG_PRM, 0x32},
    {SMART_CONFIG_PRM, 0x36},
    {SMART_CONFIG_PRM, 0x0F},

    {SMART_CONFIG_CMD, 0xB1},
    {SMART_CONFIG_PRM, 0xA0},
    {SMART_CONFIG_PRM, 0x11},

    {SMART_CONFIG_CMD, 0xB4},
    {SMART_CONFIG_PRM, 0x02},

    {SMART_CONFIG_CMD, 0xC0},
    {SMART_CONFIG_PRM, 0x17},
    {SMART_CONFIG_PRM, 0x15},

    {SMART_CONFIG_CMD, 0xC1},
    {SMART_CONFIG_PRM, 0x41},

    {SMART_CONFIG_CMD, 0xC5},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x0A},
    {SMART_CONFIG_PRM, 0x80},

    {SMART_CONFIG_CMD, 0xB6},
    {SMART_CONFIG_PRM, 0x02},


/*
    {SMART_CONFIG_CMD, 0x2A},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x01},
    {SMART_CONFIG_PRM, 0xDF},

    {SMART_CONFIG_CMD, 0x2B},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x01},
    {SMART_CONFIG_PRM, 0x3F},

    {SMART_CONFIG_CMD, 0x36},
    {SMART_CONFIG_PRM, 0x28},

*/

    {SMART_CONFIG_CMD, 0x36},
    {SMART_CONFIG_PRM, 0x48},

    {SMART_CONFIG_CMD, 0x3a},
    {SMART_CONFIG_PRM, 0x56},

    {SMART_CONFIG_CMD, 0xE9},
    {SMART_CONFIG_PRM, 0x00},

    {SMART_CONFIG_CMD, 0XF7},
    {SMART_CONFIG_PRM, 0xA9},
    {SMART_CONFIG_PRM, 0x51},
    {SMART_CONFIG_PRM, 0x2C},
    {SMART_CONFIG_PRM, 0x82},
#ifdef USE_TE
    {SMART_CONFIG_CMD, 0x35},
    {SMART_CONFIG_PRM, 0x00},
#endif
    {SMART_CONFIG_CMD, 0x11},
    {SMART_CONFIG_UDELAY, 120000},
    {SMART_CONFIG_CMD, 0x29},
};

int gpio_lcd_power_en = -1; // GPIO_PD(8)
int gpio_lcd_rst = -1; // GPIO_PD(14)
int gpio_lcd_cs = -1; // GPIO_PD(18)
int gpio_lcd_rd = -1; // -1

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_cs, 0644);
module_param_gpio(gpio_lcd_rd, 0644);

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "kwh035: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int kwh035_power_on(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_cs, 1);
    m_gpio_direction_output(gpio_lcd_rd, 1);

    m_gpio_direction_output(gpio_lcd_power_en, 0);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    usleep_range(20*1000, 20*1000);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    usleep_range(120*1000, 120*1000);
    m_gpio_direction_output(gpio_lcd_cs, 0);

    return 0;
}

static int kwh035_power_off(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_power_en, 1);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    m_gpio_direction_output(gpio_lcd_cs, 1);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "st7701",
    .refresh = 45,
    .xres = 320,
    .yres = 480,
    .pixclock = 0, // 自动计算
    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = SLCD_8080,
    .out_format = OUT_FORMAT_RGB888,
    .slcd = {
        .pixclock_when_init = 320*480*3,
        .mcu_data_width = MCU_WIDTH_8BITS,
        .mcu_cmd_width = MCU_WIDTH_8BITS,
        .wr_data_sample_edge = AT_RISING_EDGE,
        .dc_pin = CMD_LOW_DATA_HIGH,
        .te_data_transfered_edge = AT_RISING_EDGE,
        .te_pin_mode = TE_LCDC_TRIGGER,
        .enable_rdy_pin = 0,
        .cmd_of_start_frame = 0x2c,
    },
    .slcd_data_table = kwh035_data_table,
    .slcd_data_table_length = ARRAY_SIZE(kwh035_data_table),
    .power_on = kwh035_power_on,
    .power_off = kwh035_power_off,
};

static int lcd_kwh035_init(void)
{
    int ret;

    if (gpio_lcd_cs < 0) {
        printk("kwh035: must set gpio_cs\n");
        return -EINVAL;
    }

    ret = m_gpio_request(gpio_lcd_rd, "lcd rd");
    if (ret)
        return ret;

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
        printk(KERN_ERR "kwh035: failed to register lcd data\n");
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
    return ret;
}

static void lcd_kwh035_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_cs);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
    m_gpio_free(gpio_lcd_rd);
}

module_init(lcd_kwh035_init);

module_exit(lcd_kwh035_exit);

MODULE_DESCRIPTION("Ingenic Soc FB driver");
MODULE_LICENSE("GPL");
