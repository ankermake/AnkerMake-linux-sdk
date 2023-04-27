#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include "soc/x1000/fb/lcdc_data.h"

static struct smart_lcd_data_table truly_tft240240_data_table[] = {
    /* LCD init code */
    {SMART_CONFIG_CMD, 0x01}, //soft reset, 120 ms = 120 000 us
    {SMART_CONFIG_UDELAY, 120000},
    {SMART_CONFIG_CMD, 0x11},
    {SMART_CONFIG_UDELAY, 5000}, /* sleep out 5 ms  */

    {SMART_CONFIG_CMD, 0x36},

    {SMART_CONFIG_DATA, 0x00}, //40

    {SMART_CONFIG_CMD, 0x2a},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0xef},

    {SMART_CONFIG_CMD, 0x2b},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0xef},


    {SMART_CONFIG_CMD, 0x3a},
    {SMART_CONFIG_DATA, 0x05}, //5-6-5

    //{SMART_CONFIG_DATA, 0x55},

    {SMART_CONFIG_CMD, 0xb2},
    {SMART_CONFIG_DATA, 0x7f},
    {SMART_CONFIG_DATA, 0x7f},
    {SMART_CONFIG_DATA, 0x01},
    {SMART_CONFIG_DATA, 0xde},
    {SMART_CONFIG_DATA, 0x33},

    {SMART_CONFIG_CMD, 0xb3},
    {SMART_CONFIG_DATA, 0x10},
    {SMART_CONFIG_DATA, 0x05},
    {SMART_CONFIG_DATA, 0x0f},

    {SMART_CONFIG_CMD, 0xb4},
    {SMART_CONFIG_DATA, 0x0b},

    {SMART_CONFIG_CMD, 0xb7},
    {SMART_CONFIG_DATA, 0x35},

    {SMART_CONFIG_CMD, 0xbb},
    {SMART_CONFIG_DATA, 0x28}, //23

    {SMART_CONFIG_CMD, 0xbc},
    {SMART_CONFIG_DATA, 0xec},

    {SMART_CONFIG_CMD, 0xc0},
    {SMART_CONFIG_DATA, 0x2c},

    {SMART_CONFIG_CMD, 0xc2},
    {SMART_CONFIG_DATA, 0x01},

    {SMART_CONFIG_CMD, 0xc3},
    {SMART_CONFIG_DATA, 0x1e}, //14

    {SMART_CONFIG_CMD, 0xc4},
    {SMART_CONFIG_DATA, 0x20},

    {SMART_CONFIG_CMD, 0xc6},
    {SMART_CONFIG_DATA, 0x14},

    {SMART_CONFIG_CMD, 0xd0},
    {SMART_CONFIG_DATA, 0xa4},
    {SMART_CONFIG_DATA, 0xa1},

    {SMART_CONFIG_CMD, 0xe0},
    {SMART_CONFIG_DATA, 0xd0},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x08},
    {SMART_CONFIG_DATA, 0x07},
    {SMART_CONFIG_DATA, 0x05},
    {SMART_CONFIG_DATA, 0x29},
    {SMART_CONFIG_DATA, 0x54},
    {SMART_CONFIG_DATA, 0x41},
    {SMART_CONFIG_DATA, 0x3c},
    {SMART_CONFIG_DATA, 0x17},
    {SMART_CONFIG_DATA, 0x15},
    {SMART_CONFIG_DATA, 0x1a},
    {SMART_CONFIG_DATA, 0x20},

    {SMART_CONFIG_CMD, 0xe1},
    {SMART_CONFIG_DATA, 0xd0},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x08},
    {SMART_CONFIG_DATA, 0x07},
    {SMART_CONFIG_DATA, 0x04},
    {SMART_CONFIG_DATA, 0x29},
    {SMART_CONFIG_DATA, 0x44},
    {SMART_CONFIG_DATA, 0x42},
    {SMART_CONFIG_DATA, 0x3b},
    {SMART_CONFIG_DATA, 0x16},
    {SMART_CONFIG_DATA, 0x15},
    {SMART_CONFIG_DATA, 0x1b},
    {SMART_CONFIG_DATA, 0x1f},

    {SMART_CONFIG_CMD, 0x35}, // TE on
    {SMART_CONFIG_DATA, 0x00}, // TE mode: 0, mode1; 1, mode2
    //{SMART_CONFIG_CMD, 0x34}, // TE off

    {SMART_CONFIG_CMD, 0x29}, //Display ON

    /* set window size*/
    //{SMART_CONFIG_CMD, 0xcd},
    {SMART_CONFIG_CMD, 0x2a},
    {SMART_CONFIG_DATA, 0},
    {SMART_CONFIG_DATA, 0},
    {SMART_CONFIG_DATA, (239>> 8) & 0xff},
    {SMART_CONFIG_DATA, 239 & 0xff},

    {SMART_CONFIG_CMD, 0x2b},
    {SMART_CONFIG_DATA, 0},
    {SMART_CONFIG_DATA, 0},
    {SMART_CONFIG_DATA, (239>> 8) & 0xff},
    {SMART_CONFIG_DATA, 239 & 0xff},

    {SMART_CONFIG_CMD, 0x2C},
    {SMART_CONFIG_CMD, 0x2C},
    {SMART_CONFIG_CMD, 0x2C},
    {SMART_CONFIG_CMD, 0x2C},
};


static int gpio_lcd_rd = -1;
static int gpio_lcd_cs = -1;
static int gpio_lcd_rst = -1;
static int gpio_lcd_te = -1;


module_param_gpio(gpio_lcd_rd, 0644); //PB(16)
module_param_gpio(gpio_lcd_cs, 0644); //PB(18)
module_param_gpio(gpio_lcd_rst, 0644); //PD(0)
module_param_gpio(gpio_lcd_te, 0644);

static inline int m_gpio_request(int gpio , const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "truly_240240: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int truly_tft240240_power_on(void)
{
    m_gpio_direction_output(gpio_lcd_cs, 1);
    m_gpio_direction_output(gpio_lcd_rd, 1);

    m_gpio_direction_output(gpio_lcd_rst, 0);
    usleep_range(20000, 20000);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    usleep_range(20000, 20000);

    m_gpio_direction_output(gpio_lcd_cs, 0);

    return 0;
}

static int truly_tft240240_power_off(void)
{
    m_gpio_direction_output(gpio_lcd_cs, 1);
    return 0;
}

struct lcdc_data lcdc_data = {
    .name = "240x240",
    .refresh = 0,
    .xres = 240,
    .yres = 240,
    .pixclock = 240 * 240 * 2 * 60,

    .pixclock_when_init = 240 * 240,

    .fb_format = fb_fmt_RGB565,
    .out_format = OUT_FORMAT_565,
    .lcd_mode = SLCD_8080,

    .slcd = {
        .mcu_data_width = MCU_WIDTH_8BITS,
        .mcu_cmd_width = MCU_WIDTH_8BITS,
        .wr_data_sample_edge = AT_RISING_EDGE,
        .dc_pin = CMD_LOW_DATA_HIGH,
        .te_data_transfered_edge = AT_RISING_EDGE,
        .te_pin_mode = TE_GPIO_IRQ_TRIGGER,
    },
    .slcd_data_table = truly_tft240240_data_table,
    .slcd_data_table_length = ARRAY_SIZE(truly_tft240240_data_table),
    .cmd_of_start_frame = 0x2c,
    .width = 240,
    .height = 240,
    .power_on = truly_tft240240_power_on,
    .power_off = truly_tft240240_power_off,
};

static int truly_240240_init(void)
{
    int ret = 0;

    if (gpio_lcd_cs < 0) {
        printk("truly240240: must set gpio_cs\n");
        return -EINVAL;
    }

    lcdc_data.slcd.te_gpio = gpio_lcd_te;

    ret = m_gpio_request(gpio_lcd_rd, "lcd rd");
    if (ret)
        return ret;

    ret = m_gpio_request(gpio_lcd_cs, "lcd cs");
    if (ret)
        goto err_cs;

    ret = m_gpio_request(gpio_lcd_rst, "lcd rst");
    if (ret)
        goto err_rst;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret)
        goto err_vdd_en;

    return 0;

err_vdd_en:
    m_gpio_free(gpio_lcd_rst);
err_rst:
    m_gpio_free(gpio_lcd_cs);
err_cs:
    m_gpio_free(gpio_lcd_rd);

    return -EIO;


}
module_init(truly_240240_init);

static void truly_240240_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_rd);
    m_gpio_free(gpio_lcd_cs);
    m_gpio_free(gpio_lcd_rst);

}
module_exit(truly_240240_exit);

MODULE_DESCRIPTION("x1000 lcd truly240240 device");
MODULE_LICENSE("GPL");
