#include <utils/gpio.h>
#include <utils/spi.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/module.h>

#include <fb/lcdc_data.h>

static int gpio_lcd_rst = -1;       // RST      -- GPIO_PB(2) 低电平有效
static int gpio_lcd_power_en = -1;  // POWER_EN -- -1
static int gpio_spi_cs = -1;        // CE       -- GPIO_PB(24) 低电平有效

module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_spi_cs, 0644);

// #define USE_TE

static inline void m_msleep(int ms)
{
    usleep_range(ms * 1000, ms * 1000);
}

static struct smart_lcd_data_table gc9106_data_table[] = {
    /* ----------sleep exit---------- */
    {SMART_CONFIG_CMD, 0xFE},
    {SMART_CONFIG_CMD, 0xEF},
    {SMART_CONFIG_CMD, 0x11},
    {SMART_CONFIG_UDELAY, 120000},
    {SMART_CONFIG_CMD, 0x29},
    /* ----------Init_LCD---------- */
    {SMART_CONFIG_CMD, 0xFE},
    {SMART_CONFIG_CMD, 0xFE},
    {SMART_CONFIG_CMD, 0xEF},

    {SMART_CONFIG_CMD, 0xB3},
    {SMART_CONFIG_PRM , 0x03},

    {SMART_CONFIG_CMD, 0xB6},
    {SMART_CONFIG_PRM , 0x01},

    {SMART_CONFIG_CMD, 0xA3},
    {SMART_CONFIG_PRM , 0x11},

    {SMART_CONFIG_CMD, 0x21},

    {SMART_CONFIG_CMD, 0x36},
    {SMART_CONFIG_PRM , 0xD0},

    {SMART_CONFIG_CMD, 0x3A},
    {SMART_CONFIG_PRM , 0x05}, // 0x05  16 bits/pixel RGB565  // 0x06  24 bits/pixel RGB666

    {SMART_CONFIG_CMD, 0xB4},
    {SMART_CONFIG_PRM , 0x21},

    {SMART_CONFIG_CMD, 0xF0},
    {SMART_CONFIG_PRM , 0x31},
    {SMART_CONFIG_PRM , 0x4C},
    {SMART_CONFIG_PRM , 0x24},
    {SMART_CONFIG_PRM , 0x58},
    {SMART_CONFIG_PRM , 0xA8},
    {SMART_CONFIG_PRM , 0x26},
    {SMART_CONFIG_PRM , 0x28},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x2C},
    {SMART_CONFIG_PRM , 0x0C},
    {SMART_CONFIG_PRM , 0x0C},
    {SMART_CONFIG_PRM , 0x15},
    {SMART_CONFIG_PRM , 0x15},
    {SMART_CONFIG_PRM , 0x0F},

    {SMART_CONFIG_CMD, 0xF1},
    {SMART_CONFIG_PRM , 0x0E},
    {SMART_CONFIG_PRM , 0x2D},
    {SMART_CONFIG_PRM , 0x24},
    {SMART_CONFIG_PRM , 0x3E},
    {SMART_CONFIG_PRM , 0x99},
    {SMART_CONFIG_PRM , 0x12},
    {SMART_CONFIG_PRM , 0x13},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x0A},
    {SMART_CONFIG_PRM , 0x0D},
    {SMART_CONFIG_PRM , 0x0D},
    {SMART_CONFIG_PRM , 0x14},
    {SMART_CONFIG_PRM , 0x13},
    {SMART_CONFIG_PRM , 0x0F},

    {SMART_CONFIG_CMD, 0xFE},
    {SMART_CONFIG_CMD, 0xFF},

    {SMART_CONFIG_CMD, 0x35},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_CMD, 0x44},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_CMD, 0x11},

    {SMART_CONFIG_UDELAY, 120000},
    {SMART_CONFIG_CMD, 0x38},
    {SMART_CONFIG_CMD, 0x29},
};

static int gc9106_power_on(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0) {
        gpio_direction_output(gpio_lcd_power_en, 0);
        m_msleep(180);
    }

    gpio_direction_output(gpio_lcd_rst, 1);
    m_msleep(50);
    gpio_direction_output(gpio_lcd_rst, 0);
    m_msleep(50);
    gpio_direction_output(gpio_lcd_rst, 1);
    m_msleep(120);

    gpio_direction_output(gpio_spi_cs, 0);

    return 0;
}

static int gc9106_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0)
        gpio_direction_output(gpio_lcd_power_en, 1);

    gpio_set_value(gpio_lcd_rst, 0);

    return 0;
}

struct lcdc_data lcdc_data = {
    .name = "lcd_spi_gc9106",
    .refresh = 28,
    .xres = 128,
    .yres = 160,
    .pixclock = 0,
    .fb_fmt = fb_fmt_ARGB8888, /* fb_fmt_RGB565 // fb_fmt_RGB888 */
    .lcd_mode = SLCD_SPI_4LINE,
    .out_format = OUT_FORMAT_RGB565, /* OUT_FORMAT_RGB565 // OUT_FORMAT_RGB888 */
    .slcd = {
        .pixclock_when_init      = 128 * 160 * 3,
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
    .slcd_data_table        = gc9106_data_table,
    .slcd_data_table_length = ARRAY_SIZE(gc9106_data_table),
    .power_on = gc9106_power_on,
    .power_off = gc9106_power_off,
};

static int __init lcd_spi_gc9106_init(void)
{
    int ret;

    if (gpio_lcd_power_en >= 0) {
        ret = gpio_request(gpio_lcd_power_en, "lcd_power_en");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "gc9106: failed to request gpio_lcd_power_en: %s\n",
                 gpio_to_str(gpio_lcd_power_en, buf));
            return ret;
        }
    }

    ret = gpio_request(gpio_lcd_rst, "lcd_rst");
    if (ret) {
        char buf[10];
        printk(KERN_ERR "gc9106: failed to request gpio_lcd_rst: %s\n",
                gpio_to_str(gpio_lcd_rst, buf));
        goto err_lcd_rst;
    }

    ret = gpio_request(gpio_spi_cs, "lcd_cs");
    if (ret) {
        char buf[10];
        printk(KERN_ERR "gc9106: failed to request gpio_spi_cs: %s\n",
                gpio_to_str(gpio_spi_cs, buf));
        goto err_lcd_cs;
    }

    jzfb_register_lcd(&lcdc_data);

    return 0;

err_lcd_cs:
    gpio_free(gpio_lcd_rst);
err_lcd_rst:
    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);
    return ret;
}

static void __exit lcd_spi_gc9106_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    gpio_free(gpio_spi_cs);

    gpio_free(gpio_lcd_rst);

    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);
}

module_init(lcd_spi_gc9106_init);
module_exit(lcd_spi_gc9106_exit);

MODULE_DESCRIPTION("lcd_spi_gc9106 lcd panel driver");
MODULE_LICENSE("GPL");