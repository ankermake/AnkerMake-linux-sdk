#include <utils/gpio.h>
#include <utils/spi.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>

#include "soc/x1830/fb/lcdc_data.h"

static int gpio_lcd_power_en = -1; // GPIO_PC(14)
static int gpio_lcd_rst      = -1; // GPIO_PB(27)
static int gpio_lcd_sda = -1; // GPIO_PC(28);
static int gpio_lcd_scl = -1; // GPIO_PC(27);
static int gpio_lcd_cs = -1; // GPIO_PC(11);

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_sda, 0644);
module_param_gpio(gpio_lcd_scl, 0644);
module_param_gpio(gpio_lcd_cs, 0644);

static void st7701_9bit_write(unsigned int data)
{
    int i;

    gpio_set_value(gpio_lcd_cs, 0);

    for (i = 8; i >= 0 ; i--) {
        if (data & (1 << i))
            gpio_set_value(gpio_lcd_sda, 1);
        else
            gpio_set_value(gpio_lcd_sda, 0);

        usleep_range(1, 1);

        gpio_set_value(gpio_lcd_scl, 1);

        usleep_range(1, 1);

        gpio_set_value(gpio_lcd_scl, 0);
    }

    gpio_set_value(gpio_lcd_cs, 1);
}

static void spi_writecomm(unsigned int command)
{
    unsigned int cmd = command | (0 << 8);

    st7701_9bit_write(cmd);
}

static void spi_writedata(unsigned int data)
{
    unsigned int cmd = data | (1 << 8);

    st7701_9bit_write(cmd);
}

static void st7701s_spi_gpio_init(void)
{
    if (gpio_lcd_sda >= 0)
        gpio_direction_output(gpio_lcd_sda, 0);

    if (gpio_lcd_scl >= 0)
        gpio_direction_output(gpio_lcd_scl, 0);

    if (gpio_lcd_cs >= 0)
        gpio_direction_output(gpio_lcd_cs, 1);
}

static inline void m_msleep(int ms)
{
    usleep_range(ms * 1000, ms * 1000);
}

static void st7701s_spiinit(void)
{
    st7701s_spi_gpio_init();

    //--------------------------------------ST7701 Reset Sequence---------------------------------------//
    spi_writecomm(0x3A);
    spi_writedata(0x60);
    //-------------------------------------Bank0 Setting-------------------------------------------------//
    //----------------------------------Display Control setting----------------------------------------------//
    spi_writecomm(0xFF);
    spi_writedata(0x77);
    spi_writedata(0x01);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x13);

    spi_writecomm(0xEF);
    spi_writedata(0x08);

    spi_writecomm(0xFF);
    spi_writedata(0x77);
    spi_writedata(0x01);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x10);

    spi_writecomm(0xC0);
    spi_writedata(0x63);
    spi_writedata(0x00);

    spi_writecomm(0xC1);
    spi_writedata(0x0B);
    spi_writedata(0x02);

    spi_writecomm(0xC2);
    spi_writedata(0x01);
    spi_writedata(0x04);

    spi_writecomm(0xCC);
    spi_writedata(0x10);

    spi_writecomm(0xCD);
    spi_writedata(0x08);
    //--------------------------------Gamma Cluster Setting----------------------------------------//
    spi_writecomm(0xB0);
    spi_writedata(0x00);
    spi_writedata(0x02);
    spi_writedata(0x07);
    spi_writedata(0x10);
    spi_writedata(0x14);
    spi_writedata(0x0A);
    spi_writedata(0x07);
    spi_writedata(0x09);
    spi_writedata(0x09);
    spi_writedata(0x19);
    spi_writedata(0x05);
    spi_writedata(0x14);
    spi_writedata(0x12);
    spi_writedata(0x12);
    spi_writedata(0x1A);
    spi_writedata(0xDE);

    spi_writecomm(0xB1);
    spi_writedata(0x00);
    spi_writedata(0x02);
    spi_writedata(0x08);
    spi_writedata(0x0F);
    spi_writedata(0x17);
    spi_writedata(0x0B);
    spi_writedata(0x0F);
    spi_writedata(0x09);
    spi_writedata(0x09);
    spi_writedata(0x28);
    spi_writedata(0x09);
    spi_writedata(0x15);
    spi_writedata(0x12);
    spi_writedata(0x16);
    spi_writedata(0x19);
    spi_writedata(0xDE);
    //----------------------------------End Gamma Setting-------------------------------------------//
    //-------------------------------End Display Control setting-------------------------------------//
    //------------------------------------Bank0 Setting End------------------------------------------//
    //--------------------------------------Bank1 Setting------------------------------------------------//
    //--------------------------- Power Control Registers Initial -----------------------------------//
    spi_writecomm(0xFF);
    spi_writedata(0x77);
    spi_writedata(0x01);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x11);

    spi_writecomm(0xB0);
    spi_writedata(0x42);
    //--------------------------------------Vcom Setting------------------------------------------------//
    spi_writecomm(0xB1);
    spi_writedata(0x66);
    //------------------------------------End Vcom Setting--------------------------------------------//
    spi_writecomm(0xB2);
    spi_writedata(0x87);

    spi_writecomm(0xB3);
    spi_writedata(0x80);

    spi_writecomm(0xB5);
    spi_writedata(0x4D);

    spi_writecomm(0xB7);
    spi_writedata(0x85);

    spi_writecomm(0xB8);
    spi_writedata(0x20);

    spi_writecomm(0xB9);
    spi_writedata(0x10);

    spi_writecomm(0xC1);
    spi_writedata(0x08);

    spi_writecomm(0xC2);
    spi_writedata(0x08);

    spi_writecomm(0xD0);
    spi_writedata(0x88);
    //----------------------------End Power Control Registers Initial ----------------------------//
    m_msleep (100);
    //------------------------------------------GIP Setting-------------------------------------------------//
    spi_writecomm(0xE0);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x02);

    spi_writecomm(0xE1);
    spi_writedata(0x04);
    spi_writedata(0xA0);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x05);
    spi_writedata(0xA0);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x20);
    spi_writedata(0x20);

    spi_writecomm(0xE2);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);

    spi_writecomm(0xE3);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x33);
    spi_writedata(0x00);

    spi_writecomm(0xE4);
    spi_writedata(0x22);
    spi_writedata(0x00);

    spi_writecomm(0xE5);
    spi_writedata(0x03);
    spi_writedata(0x34);
    spi_writedata(0xA0);
    spi_writedata(0xA0);
    spi_writedata(0x05);
    spi_writedata(0x34);
    spi_writedata(0xA0);
    spi_writedata(0xA0);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);

    spi_writecomm(0xE6);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x33);
    spi_writedata(0x00);

    spi_writecomm(0xE7);
    spi_writedata(0x22);
    spi_writedata(0x00);

    spi_writecomm(0xE8);
    spi_writedata(0x04);
    spi_writedata(0x34);
    spi_writedata(0xA0);
    spi_writedata(0xA0);
    spi_writedata(0x06);
    spi_writedata(0x34);
    spi_writedata(0xA0);
    spi_writedata(0xA0);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);

    spi_writecomm(0xEB);
    spi_writedata(0x02);
    spi_writedata(0x00);
    spi_writedata(0x40);
    spi_writedata(0x40);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);

    spi_writecomm(0xEC);
    spi_writedata(0x00);
    spi_writedata(0x00);

    spi_writecomm(0xED);
    spi_writedata(0xFA);
    spi_writedata(0x45);
    spi_writedata(0x0B);
    spi_writedata(0xFF);
    spi_writedata(0xFF);
    spi_writedata(0xFF);
    spi_writedata(0xFF);
    spi_writedata(0xFF);
    spi_writedata(0xFF);
    spi_writedata(0xFF);
    spi_writedata(0xFF);
    spi_writedata(0xFF);
    spi_writedata(0xFF);
    spi_writedata(0xB0);
    spi_writedata(0x54);
    spi_writedata(0xAF);

    spi_writecomm(0xEF);
    spi_writedata(0x08);
    spi_writedata(0x08);
    spi_writedata(0x08);
    spi_writedata(0x40);
    spi_writedata(0x3F);
    spi_writedata(0x54);
    //----------------------------------------End GIP Setting--------------------------------------------//
    //------------------------- Power Control Registers Initial End--------------------------------//
    //-------------------------------------Bank1 Setting------------------------------------------------
    //-------------------------------------Bank1 Setting-------------------------------------------------//
    spi_writecomm(0xFF);
    spi_writedata(0x77);
    spi_writedata(0x01);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);

    spi_writecomm(0x55);
    spi_writedata(0xB0);

    spi_writecomm(0xFF);
    spi_writedata(0x77);
    spi_writedata(0x01);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x13);

    spi_writecomm(0xE8);
    spi_writedata(0x00);
    spi_writedata(0x0E);

    spi_writecomm(0x11);

    m_msleep (120);

    spi_writecomm(0xE8);
    spi_writedata(0x00);
    spi_writedata(0x00);

    spi_writecomm(0xFF);
    spi_writedata(0x77);
    spi_writedata(0x01);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x00);

    spi_writecomm(0x29);
#if 0
    /**************显示st7701内置测试图像********************** */
    spi_writecomm(0xFF);
    spi_writedata(0x77);
    spi_writedata(0x01);
    spi_writedata(0x00);
    spi_writedata(0x00);
    spi_writedata(0x12);

    spi_writecomm(0xD1);
    spi_writedata(0x81);

    spi_writecomm(0xD2);
    spi_writedata(0x06);
    /************************************************** */
#endif
}

static int st7701_power_on(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0) {
        gpio_direction_output(gpio_lcd_power_en, 0);
        m_msleep(180);
    }

    if (gpio_lcd_rst >= 0) {
        gpio_direction_output(gpio_lcd_rst, 1);
        m_msleep(20);
        gpio_direction_output(gpio_lcd_rst, 0);
        m_msleep(20);
        gpio_direction_output(gpio_lcd_rst, 1);
        m_msleep(20);
    }

    st7701s_spiinit();

    return 0;
}

static int st7701_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0)
        gpio_direction_output(gpio_lcd_power_en, 1);

    return 0;
}

struct lcdc_data lcdc_data = {
    .name = "st7701_9bit",
    .refresh = 45,
    .xres = 480,
    .yres = 800,
    .pixclock = 0, // 自动计算
    .left_margin = 88,
    .right_margin = 40,
    .upper_margin = 8,
    .lower_margin = 35,
    .hsync_len = 128,
    .vsync_len = 100,
    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = TFT_24BITS,
    .out_format = OUT_FORMAT_RGB666,
    .tft = {
        .even_line_order = ORDER_RGB,
        .odd_line_order = ORDER_RGB,
        .pix_clk_polarity = AT_RISING_EDGE,
        .de_active_level = AT_HIGH_LEVEL,
        .hsync_vsync_active_level = AT_HIGH_LEVEL,
    },
    .power_on = st7701_power_on,
    .power_off = st7701_power_off,
};

static int __init st7701s_init(void)
{
    int ret;

    if (gpio_lcd_power_en >= 0) {
        ret = gpio_request(gpio_lcd_power_en, "lcd_power_en");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "st7701: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_power_en, buf));
            return ret;
        }
    }

    if (gpio_lcd_rst >= 0) {
        ret = gpio_request(gpio_lcd_rst, "lcd_rst");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "st7701: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_rst;
        }
    }

    if (gpio_lcd_scl >= 0) {
        ret = gpio_request(gpio_lcd_scl, "lcd_scl");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "st7701: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_scl;
        }
    }

    if (gpio_lcd_sda >= 0) {
        ret = gpio_request(gpio_lcd_sda, "lcd_sda");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "st7701: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_sda;
        }
    }

    if (gpio_lcd_cs >= 0) {
        ret = gpio_request(gpio_lcd_cs, "lcd_cs");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "st7701: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_cs;
        }
    }

    jzfb_register_lcd(&lcdc_data);

    return 0;

err_lcd_cs:
    if (gpio_lcd_sda >= 0)
        gpio_free(gpio_lcd_sda);
err_lcd_sda:
    if (gpio_lcd_scl >= 0)
        gpio_free(gpio_lcd_scl);
err_lcd_scl:
    if (gpio_lcd_rst >= 0)
        gpio_free(gpio_lcd_rst);
err_lcd_rst:
    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);
    return ret;
}


static void __exit st7701s_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    if (gpio_lcd_cs >= 0)
        gpio_free(gpio_lcd_cs);

    if (gpio_lcd_sda >= 0)
        gpio_free(gpio_lcd_sda);

    if (gpio_lcd_scl >= 0)
        gpio_free(gpio_lcd_scl);

    if (gpio_lcd_rst >= 0)
        gpio_free(gpio_lcd_rst);

    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);
}

module_init(st7701s_init);
module_exit(st7701s_exit);

MODULE_DESCRIPTION("st7701s lcd panel driver");
MODULE_LICENSE("GPL");
