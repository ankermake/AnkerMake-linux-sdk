#include <linux/kernel.h>
#include <linux/gpio.h>
#include <soc/gpio.h>
#include <jz_fb_x1021/jzfb_pdata.h>
#include <linux/platform_device.h>
#include "ingenic_common.h"
#include "board.h"

// see drivers/video/jz_fb_x1021/x1021_slcd_gpio_init.c
extern void x1021_init_slcd_gpio_pb(struct jzfb *jzfb);

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
#if LCD_ENABLE_TE_PIN
    {SMART_CONFIG_CMD, 0x35},
    {SMART_CONFIG_PRM, 0x00},
#endif
    {SMART_CONFIG_CMD, 0x11},
    {SMART_CONFIG_UDELAY, 120000},
    {SMART_CONFIG_CMD, 0x29},
};

static unsigned int lcd_power_inited;

#define m_gpio_direction_output(gpio, value)    \
    do {                                        \
        if (gpio >= 0) {                        \
            gpio_direction_output(gpio, value); \
        }                                       \
    } while (0)

#define m_gpio_request(gpio, lable)                 \
    (gpio) >= 0 ? gpio_request(gpio, lable) : -1

static int kwh035_gpio_init(void)
{
    int ret;

    if (lcd_power_inited)
        return 0;

    ret = m_gpio_request(GPIO_LCD_RST, "lcd rst");
    if (ret) {
        printk(KERN_ERR "can's request lcd rst gpio\n");
        // return ret;
    }

    ret = m_gpio_request(GPIO_LCD_CS, "lcd cs");
    if (ret) {
        printk(KERN_ERR "can's request lcd cs gpio\n");
        // return ret;
    }

    ret = m_gpio_request(GPIO_LCD_RD, "lcd rd");
    if (ret) {
        printk(KERN_ERR "can's request lcd rd gpio\n");
        // return ret;
    }

    ret = m_gpio_request(GPIO_LCD_POWER_EN, "lcd power en");
    if (ret) {
        printk(KERN_ERR "can's request lcd power gpio\n");
        // return ret;
    }

    lcd_power_inited = 1;

    return 0;
}

static int kwh035_power_on(struct jzfb *jzfb)
{
    if (kwh035_gpio_init())
        return -EFAULT;

    m_gpio_direction_output(GPIO_LCD_CS, 1);
    m_gpio_direction_output(GPIO_LCD_RD, 1);
    // m_gpio_direction_output(GPIO_LCD_POWER_EN, 1);

    m_gpio_direction_output(GPIO_LCD_POWER_EN, 0);
    mdelay(20);
    m_gpio_direction_output(GPIO_LCD_POWER_EN, 1);
    mdelay(120);
    m_gpio_direction_output(GPIO_LCD_CS, 0);

    // m_gpio_direction_output(GPIO_LCD_PWM, 1);

    return 0;
}

static int kwh035_power_off(struct jzfb *jzfb)
{
    if (kwh035_gpio_init())
        return -EFAULT;

    m_gpio_direction_output(GPIO_LCD_CS, 1);
    m_gpio_direction_output(GPIO_LCD_RD, 1);
    m_gpio_direction_output(GPIO_LCD_RST, 0);
    m_gpio_direction_output(GPIO_LCD_POWER_EN, 0);

    // m_gpio_direction_output(GPIO_LCD_PWM, 0);

    return 0;
}

struct jzfb_lcd_pdata jzfb_pdata = {
    .config = {
        .name = "kwh035[480x320]",
        .refresh = 60,
        .xres = 320,
        .yres = 480,
        .pixclock = 480 * 320 * 3 * 60,
        .left_margin = 0,
        .right_margin = 0,
        .upper_margin = 0,
        .lower_margin = 0,
        .hsync_len = 0,
        .vsync_len = 0,

        .pixclock_when_init = 480 * 320 * 3,

        .fb_format = SRC_FORMAT_888,
        .out_format = OUT_FORMAT_888,
        .out_order = ORDER_RGB,
        .mcu_data_width = MCU_WIDTH_8BITS,
        .mcu_cmd_width = MCU_WIDTH_8BITS,
        .wr_data_sample_edge = AT_RISING_EDGE,
        .dc_pin = CMD_LOW_DATA_HIGH,
        .te_data_transfered_level = AT_HIGH_LEVEL,
        .rdy_cmd_send_level =  AT_LOW_LEVEL,
        .refresh_mode = LCD_REFRESH_MODE,
        .te_pin = TE_GPIO_IRQ_TRIGGER,
        .enable_rdy_pin = LCD_ENABLE_RDY_PIN,
    },
    .slcd_data_table = kwh035_data_table,
    .slcd_data_table_length = ARRAY_SIZE(kwh035_data_table),
    .cmd_of_start_frame = 0x2c,
    .init_lcd_when_start = LCD_INIT_LCD_WHEN_BOOTUP,
    .refresh_lcd_when_resume = LCD_REFRESH_LCD_WHEN_RESUME,
    .wait_frame_end_when_pan_display = LCD_WAIT_FRAME_END_WHEN_PAN_DISPLAY,
    .init_slcd_gpio = x1021_init_slcd_gpio_pb,
    .fb_copy_type = FB_COPY_TYPE_ROTATE_0,
    .width = 320,
    .height = 480,
    .power_on = kwh035_power_on,
    .power_off = kwh035_power_off,
};
