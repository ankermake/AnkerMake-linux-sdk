#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <fb/lcdc_data.h>
#include <soc/gpio.h>


static struct smart_lcd_data_table s6d04d1_data_table[] = {
    {SMART_CONFIG_CMD, 0xf4},
    {SMART_CONFIG_PRM, 0x59},
    {SMART_CONFIG_PRM, 0x59},
    {SMART_CONFIG_PRM, 0x52},
    {SMART_CONFIG_PRM, 0x52},
    {SMART_CONFIG_PRM, 0x11},

    {SMART_CONFIG_CMD, 0xf5},
    {SMART_CONFIG_PRM, 0x12},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x0b},
    {SMART_CONFIG_PRM, 0xf0},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_UDELAY, 10},

    {SMART_CONFIG_CMD, 0xf3},
    {SMART_CONFIG_PRM, 0xff},
    {SMART_CONFIG_PRM, 0x2a},
    {SMART_CONFIG_PRM, 0x2a},
    {SMART_CONFIG_PRM, 0x0a},
    {SMART_CONFIG_PRM, 0x22},
    {SMART_CONFIG_PRM, 0x72},
    {SMART_CONFIG_PRM, 0x72},
    {SMART_CONFIG_PRM, 0x20},

    {SMART_CONFIG_CMD, 0x3a},
    {SMART_CONFIG_PRM, 0x77},

    {SMART_CONFIG_CMD, 0xf2},
    {SMART_CONFIG_PRM, 0x10},
    {SMART_CONFIG_PRM, 0x10},
    {SMART_CONFIG_PRM, 0x01},
    {SMART_CONFIG_PRM, 0x08},
    {SMART_CONFIG_PRM, 0x08},
    {SMART_CONFIG_PRM, 0x08},
    {SMART_CONFIG_PRM, 0x08},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x1a},
    {SMART_CONFIG_PRM, 0x1a},

    {SMART_CONFIG_CMD, 0xf6},
    {SMART_CONFIG_PRM, 0x48},
    {SMART_CONFIG_PRM, 0x88},
    {SMART_CONFIG_PRM, 0x10},

    {SMART_CONFIG_CMD, 0xf7},
    {SMART_CONFIG_PRM, 0x0d},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x03},
    {SMART_CONFIG_PRM, 0x0e},
    {SMART_CONFIG_PRM, 0x1c},
    {SMART_CONFIG_PRM, 0x29},
    {SMART_CONFIG_PRM, 0x2d},
    {SMART_CONFIG_PRM, 0x34},
    {SMART_CONFIG_PRM, 0x0e},
    {SMART_CONFIG_PRM, 0x12},
    {SMART_CONFIG_PRM, 0x24},
    {SMART_CONFIG_PRM, 0x1e},
    {SMART_CONFIG_PRM, 0x07},
    {SMART_CONFIG_PRM, 0x22},
    {SMART_CONFIG_PRM, 0x22},

    {SMART_CONFIG_CMD, 0xf8},
    {SMART_CONFIG_PRM, 0x0d},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x03},
    {SMART_CONFIG_PRM, 0x0e},
    {SMART_CONFIG_PRM, 0x1c},
    {SMART_CONFIG_PRM, 0x29},
    {SMART_CONFIG_PRM, 0x2d},
    {SMART_CONFIG_PRM, 0x34},
    {SMART_CONFIG_PRM, 0x0e},
    {SMART_CONFIG_PRM, 0x12},
    {SMART_CONFIG_PRM, 0x24},
    {SMART_CONFIG_PRM, 0x1e},
    {SMART_CONFIG_PRM, 0x07},
    {SMART_CONFIG_PRM, 0x22},
    {SMART_CONFIG_PRM, 0x22},

    {SMART_CONFIG_CMD, 0xf9},
    {SMART_CONFIG_PRM, 0x1e},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x0a},
    {SMART_CONFIG_PRM, 0x19},
    {SMART_CONFIG_PRM, 0x23},
    {SMART_CONFIG_PRM, 0x31},
    {SMART_CONFIG_PRM, 0x37},
    {SMART_CONFIG_PRM, 0x3f},
    {SMART_CONFIG_PRM, 0x01},
    {SMART_CONFIG_PRM, 0x03},
    {SMART_CONFIG_PRM, 0x16},
    {SMART_CONFIG_PRM, 0x19},
    {SMART_CONFIG_PRM, 0x07},
    {SMART_CONFIG_PRM, 0x22},
    {SMART_CONFIG_PRM, 0x22},

    {SMART_CONFIG_CMD, 0xfA},
    {SMART_CONFIG_PRM, 0x0D},
    {SMART_CONFIG_PRM, 0x11},
    {SMART_CONFIG_PRM, 0x0A},
    {SMART_CONFIG_PRM, 0x19},
    {SMART_CONFIG_PRM, 0x23},
    {SMART_CONFIG_PRM, 0x31},
    {SMART_CONFIG_PRM, 0x37},
    {SMART_CONFIG_PRM, 0x3f},
    {SMART_CONFIG_PRM, 0x01},
    {SMART_CONFIG_PRM, 0x03},
    {SMART_CONFIG_PRM, 0x16},
    {SMART_CONFIG_PRM, 0x19},
    {SMART_CONFIG_PRM, 0x07},
    {SMART_CONFIG_PRM, 0x22},
    {SMART_CONFIG_PRM, 0x22},

    {SMART_CONFIG_CMD, 0xfB},
    {SMART_CONFIG_PRM, 0x0D},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x03},
    {SMART_CONFIG_PRM, 0x0E},
    {SMART_CONFIG_PRM, 0x1C},
    {SMART_CONFIG_PRM, 0x29},
    {SMART_CONFIG_PRM, 0x2D},
    {SMART_CONFIG_PRM, 0x34},
    {SMART_CONFIG_PRM, 0x0E},
    {SMART_CONFIG_PRM, 0x12},
    {SMART_CONFIG_PRM, 0x24},
    {SMART_CONFIG_PRM, 0x1E},
    {SMART_CONFIG_PRM, 0x07},
    {SMART_CONFIG_PRM, 0x22},
    {SMART_CONFIG_PRM, 0x22},

    {SMART_CONFIG_CMD, 0xfC},
    {SMART_CONFIG_PRM, 0x0D},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x03},
    {SMART_CONFIG_PRM, 0x0E},
    {SMART_CONFIG_PRM, 0x1C},
    {SMART_CONFIG_PRM, 0x29},
    {SMART_CONFIG_PRM, 0x2D},
    {SMART_CONFIG_PRM, 0x34},
    {SMART_CONFIG_PRM, 0x0E},
    {SMART_CONFIG_PRM, 0x12},
    {SMART_CONFIG_PRM, 0x24},
    {SMART_CONFIG_PRM, 0x1E},
    {SMART_CONFIG_PRM, 0x07},
    {SMART_CONFIG_PRM, 0x22},
    {SMART_CONFIG_PRM, 0x22},

    {SMART_CONFIG_CMD, 0xFD},
    {SMART_CONFIG_PRM, 0x11},
    {SMART_CONFIG_PRM, 0x01},

    {SMART_CONFIG_CMD, 0x36},
    {SMART_CONFIG_PRM, 0x00},

    {SMART_CONFIG_CMD, 0x35},
    {SMART_CONFIG_PRM, 0x00},

    {SMART_CONFIG_CMD, 0x2A},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0xEF},

    {SMART_CONFIG_CMD, 0x2B},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x01},
    {SMART_CONFIG_PRM, 0x8F},


    {SMART_CONFIG_CMD, 0xF1},
    {SMART_CONFIG_PRM, 0x5A},

    {SMART_CONFIG_CMD, 0xFF},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x40},

    {SMART_CONFIG_CMD, 0x11},
    {SMART_CONFIG_UDELAY, 120},

    {SMART_CONFIG_CMD, 0xF1},
    {SMART_CONFIG_PRM, 0x00},

    {SMART_CONFIG_CMD, 0x29},
    {SMART_CONFIG_UDELAY, 40},
};

int gpio_lcd_rst = -1; // GPIO_PB(21)
int gpio_lcd_cs = -1; // GPIO_PB(24)
int gpio_lcd_power_en = -1; //BUCK_3V3

module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_cs, 0644);
module_param_gpio(gpio_lcd_power_en, 0644);

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ALERT "s6d04d1: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int s6d04d1_power_on(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_power_en, 1);
    usleep_range(20*1000, 20*1000);

    m_gpio_direction_output(gpio_lcd_cs, 1);

    m_gpio_direction_output(gpio_lcd_rst, 1);
    usleep_range(20*1000, 20*1000);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    usleep_range(20*1000, 20*1000);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    usleep_range(20*1000, 20*1000);

    m_gpio_direction_output(gpio_lcd_cs, 0);

    return 0;
}

static int s6d04d1_power_off(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_rst, 0);
    m_gpio_direction_output(gpio_lcd_cs, 1);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "s6d04d1",
    .refresh = 45,
    .xres = 240,
    .yres = 400,
    .pixclock = 0, // 自动计算
    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = SLCD_8080,
    .out_format = OUT_FORMAT_RGB888,
    .slcd = {
        .pixclock_when_init = 240*400*3,
        .mcu_data_width = MCU_WIDTH_8BITS,
        .mcu_cmd_width = MCU_WIDTH_8BITS,
        .wr_data_sample_edge = AT_RISING_EDGE,
        .dc_pin = CMD_LOW_DATA_HIGH,
        .cmd_of_start_frame = 0x2c,
    },
    .slcd_data_table = s6d04d1_data_table,
    .slcd_data_table_length = ARRAY_SIZE(s6d04d1_data_table),
    .power_on = s6d04d1_power_on,
    .power_off = s6d04d1_power_off,
};

static int lcd_s6d04d1_init(void)
{
    int ret;

    if (gpio_lcd_cs < 0) {
        printk("s6d04d1: must set gpio_cs\n");
        return -EINVAL;
    }

    if (gpio_lcd_rst < 0) {
        printk("s6d04d1: must set gpio_rst\n");
        return -EINVAL;
    }

    ret = m_gpio_request(gpio_lcd_rst, "lcd reset");
    if (ret)
        return ret;

    ret = m_gpio_request(gpio_lcd_cs, "lcd cs");
    if (ret)
        goto err_cs;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret) {
        printk(KERN_ALERT "s6d04d1: failed to register lcd data\n");
        goto err_lcdc_data;
    }

    return 0;
err_lcdc_data:
    m_gpio_free(gpio_lcd_cs);
err_cs:
    m_gpio_free(gpio_lcd_rst);
    return ret;
}

static void lcd_s6d04d1_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_cs);
    m_gpio_free(gpio_lcd_rst);
}

module_init(lcd_s6d04d1_init);
module_exit(lcd_s6d04d1_exit);

MODULE_DESCRIPTION("Ingenic Soc FB driver");
MODULE_LICENSE("GPL");