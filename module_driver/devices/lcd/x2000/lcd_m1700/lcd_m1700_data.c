#include <utils/gpio.h>
#include <utils/spi.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>

#include <fb/lcdc_data.h>

static int gpio_lcd_power_en    = -1; // -1
static int gpio_lcd_rst         = -1; // GPIO_PB(29)
static int gpio_lcd_sda         = -1; // GPIO_PB(30)
static int gpio_lcd_scl         = -1; // GPIO_PB(31)
static int gpio_lcd_cs          = -1; // GPIO_PB(28)

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_sda, 0644);
module_param_gpio(gpio_lcd_scl, 0644);
module_param_gpio(gpio_lcd_cs, 0644);

void m1700_spi_write(unsigned short val)
{
    unsigned char no;
    unsigned short value;

    value = val & 0xFFFF;

    gpio_direction_output(gpio_lcd_cs, 1);
    gpio_direction_output(gpio_lcd_scl, 0);
    gpio_direction_output(gpio_lcd_sda, 0);
    gpio_direction_output(gpio_lcd_cs, 0);
    udelay(50);
    for(no = 0; no < 16;no++) {
        gpio_direction_output(gpio_lcd_scl, 0);
        if((value & 0x8000) == 0x8000)
            gpio_direction_output(gpio_lcd_sda, 1);
        else
            gpio_direction_output(gpio_lcd_sda, 0);
        udelay(50);
        gpio_direction_output(gpio_lcd_scl, 1);
        value <<= 1;
        udelay(50);
     }
    gpio_direction_output(gpio_lcd_cs, 1);
    udelay(50);
}

static inline void m_msleep(int ms)
{
    usleep_range(ms * 1000, ms * 1000);
}

static void m1700_spi_init(void)
{
    m1700_spi_write(0xFD00); //SW reset
    m1700_spi_write(0x0200);
    m_msleep(10);

    m1700_spi_write(0xFD00);
    m1700_spi_write(0x1800);
    m1700_spi_write(0x4420);
    m1700_spi_write(0x1601);

    m1700_spi_write(0x1996);// 0x1996

    m1700_spi_write(0xFDC5);
    m1700_spi_write(0xA2B4);//VGH=15V, VGL=-14V  BA

    m1700_spi_write(0xFDC4);
    m1700_spi_write(0x8245);//DCVCOM precharge


    m1700_spi_write(0xFDC1);
    m1700_spi_write(0x9102);//03


    m1700_spi_write(0xFDC0);
    m1700_spi_write(0xA101);
    m1700_spi_write(0xA21F);
    m1700_spi_write(0xA30B);
    m1700_spi_write(0xA438);
    m1700_spi_write(0xA500);
    m1700_spi_write(0xA60A);
    m1700_spi_write(0xA738);
    m1700_spi_write(0xA800);
    m1700_spi_write(0xA90A);
    m1700_spi_write(0xAA37);


    m1700_spi_write(0xFDCE);
    m1700_spi_write(0x8118);
    m1700_spi_write(0x8243);
    m1700_spi_write(0x8343);
    m1700_spi_write(0x9106);
    m1700_spi_write(0x9338);
    m1700_spi_write(0x9402);
    m1700_spi_write(0x9506);
    m1700_spi_write(0x9738);
    m1700_spi_write(0x9802);
    m1700_spi_write(0x9906);
    m1700_spi_write(0x9B38);
    m1700_spi_write(0x9C02);

    // m1700_spi_write(0xFDC5);
    // m1700_spi_write(0xD194);
    // m1700_spi_write(0xD242);

    // m1700_spi_write(0xFDD8);
    // m1700_spi_write(0x0122);
    // m1700_spi_write(0x0222);

    //Gamma 2.2 balance_140618
    m1700_spi_write(0xFDE1);//R+
    m1700_spi_write(0x017E);
    m1700_spi_write(0x021E);
    m1700_spi_write(0x032B);
    m1700_spi_write(0x0436);
    m1700_spi_write(0x050F);
    m1700_spi_write(0x0611);
    m1700_spi_write(0x071E);
    m1700_spi_write(0x0808);
    m1700_spi_write(0x0907);
    m1700_spi_write(0x0A09);
    m1700_spi_write(0x0B0B);
    m1700_spi_write(0x0C1A);
    m1700_spi_write(0x0D11);
    m1700_spi_write(0x0E0F);
    m1700_spi_write(0x0F16);
    m1700_spi_write(0x100D);
    m1700_spi_write(0x1105);
    m1700_spi_write(0x1204);

    m1700_spi_write(0xFDE2);//R-
    m1700_spi_write(0x017D);
    m1700_spi_write(0x021E);
    m1700_spi_write(0x032B);
    m1700_spi_write(0x0436);
    m1700_spi_write(0x050E);
    m1700_spi_write(0x060F);
    m1700_spi_write(0x071C);
    m1700_spi_write(0x0804);
    m1700_spi_write(0x0904);
    m1700_spi_write(0x0A0C);
    m1700_spi_write(0x0B0E);
    m1700_spi_write(0x0C19);
    m1700_spi_write(0x0D11);
    m1700_spi_write(0x0E0D);
    m1700_spi_write(0x0F16);
    m1700_spi_write(0x100D);
    m1700_spi_write(0x1105);
    m1700_spi_write(0x1204);

    m1700_spi_write(0xFDE3);//G+
    m1700_spi_write(0x017E);
    m1700_spi_write(0x021E);
    m1700_spi_write(0x032B);
    m1700_spi_write(0x0436);
    m1700_spi_write(0x050F);
    m1700_spi_write(0x0611);
    m1700_spi_write(0x071E);
    m1700_spi_write(0x0808);
    m1700_spi_write(0x0907);
    m1700_spi_write(0x0A09);
    m1700_spi_write(0x0B0B);
    m1700_spi_write(0x0C1A);
    m1700_spi_write(0x0D11);
    m1700_spi_write(0x0E0F);
    m1700_spi_write(0x0F16);
    m1700_spi_write(0x100D);
    m1700_spi_write(0x1105);
    m1700_spi_write(0x1204);

    m1700_spi_write(0xFDE4);//G-
    m1700_spi_write(0x017D);
    m1700_spi_write(0x021E);
    m1700_spi_write(0x032B);
    m1700_spi_write(0x0436);
    m1700_spi_write(0x050E);
    m1700_spi_write(0x060F);
    m1700_spi_write(0x071C);
    m1700_spi_write(0x0804);
    m1700_spi_write(0x0904);
    m1700_spi_write(0x0A0C);
    m1700_spi_write(0x0B0E);
    m1700_spi_write(0x0C19);
    m1700_spi_write(0x0D11);
    m1700_spi_write(0x0E0D);
    m1700_spi_write(0x0F16);
    m1700_spi_write(0x100D);
    m1700_spi_write(0x1105);
    m1700_spi_write(0x1204);

    m1700_spi_write(0xFDE5);//B+
    m1700_spi_write(0x017E);
    m1700_spi_write(0x021E);
    m1700_spi_write(0x032B);
    m1700_spi_write(0x0436);
    m1700_spi_write(0x050F);
    m1700_spi_write(0x0611);
    m1700_spi_write(0x071E);
    m1700_spi_write(0x0808);
    m1700_spi_write(0x0907);
    m1700_spi_write(0x0A09);
    m1700_spi_write(0x0B0B);
    m1700_spi_write(0x0C1A);
    m1700_spi_write(0x0D11);
    m1700_spi_write(0x0E0F);
    m1700_spi_write(0x0F16);
    m1700_spi_write(0x100D);
    m1700_spi_write(0x1105);
    m1700_spi_write(0x1204);

    m1700_spi_write(0xFDE6); //B-
    m1700_spi_write(0x017D);
    m1700_spi_write(0x021E);
    m1700_spi_write(0x032B);
    m1700_spi_write(0x0436);
    m1700_spi_write(0x050E);
    m1700_spi_write(0x060F);
    m1700_spi_write(0x071C);
    m1700_spi_write(0x0804);
    m1700_spi_write(0x0904);
    m1700_spi_write(0x0A0C);
    m1700_spi_write(0x0B0E);
    m1700_spi_write(0x0C19);
    m1700_spi_write(0x0D11);
    m1700_spi_write(0x0E0D);
    m1700_spi_write(0x0F16);
    m1700_spi_write(0x100D);
    m1700_spi_write(0x1105);
    m1700_spi_write(0x1204);

    m1700_spi_write(0xFD00);
    m1700_spi_write(0x4400);

    m1700_spi_write(0xFDC4);
    m1700_spi_write(0x8245);
    m_msleep(20);

    m1700_spi_write(0xFD00);
    m1700_spi_write(0x1D05);// Polarity Setting 01

    m1700_spi_write(0xFD00);
    m1700_spi_write(0x0101);
}

static int m1700_power_on(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0) {
        gpio_direction_output(gpio_lcd_power_en, 0);
        m_msleep(180);
    }

    if (gpio_lcd_rst >= 0) {
        gpio_direction_output(gpio_lcd_rst, 1);
        m_msleep(2);
        gpio_direction_output(gpio_lcd_rst, 0);
        m_msleep(2);
        gpio_direction_output(gpio_lcd_rst, 1);
        m_msleep(2);
        gpio_direction_output(gpio_lcd_rst, 1);
        m_msleep(20);
    }

    //black_light
    // gpio_direction_output(GPIO_PC(14), 1);

    gpio_direction_output(gpio_lcd_scl, 1);
    gpio_direction_output(gpio_lcd_cs, 1);
    gpio_direction_output(gpio_lcd_sda, 0);

    m1700_spi_init();

    return 0;
}

static int m1700_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0)
        gpio_direction_output(gpio_lcd_power_en, 1);

    return 0;
}

struct lcdc_data lcdc_data = {
    .name = "m1700",
    .refresh = 60,
    .xres = 640,
    .yres = 480,
    .pixclock = 0, // 自动计算
    .left_margin = 81,
    .right_margin = 121,
    .upper_margin = 22,
    .lower_margin = 4,
    .hsync_len = 50,
    .vsync_len = 1,
    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = TFT_24BITS,
    .out_format = OUT_FORMAT_RGB888,
    .tft = {
        .even_line_order = ORDER_RGB,
        .odd_line_order = ORDER_RGB,
        .pix_clk_polarity = AT_FALLING_EDGE,
        .de_active_level = AT_LOW_LEVEL,
        .hsync_vsync_active_level = AT_LOW_LEVEL,
    },
    .power_on = m1700_power_on,
    .power_off = m1700_power_off,
};

static int __init m1700_init(void)
{
    int ret;

    if (gpio_lcd_power_en >= 0) {
        ret = gpio_request(gpio_lcd_power_en, "lcd_power_en");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "m1700: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_power_en, buf));
            return ret;
        }
    }

    if (gpio_lcd_rst >= 0) {
        ret = gpio_request(gpio_lcd_rst, "lcd_rst");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "m1700: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_rst;
        }
    }

    if (gpio_lcd_scl >= 0) {
        ret = gpio_request(gpio_lcd_scl, "lcd_scl");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "m1700: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_scl;
        }
    }

    if (gpio_lcd_sda >= 0) {
        ret = gpio_request(gpio_lcd_sda, "lcd_sda");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "m1700: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_sda;
        }
    }

    if (gpio_lcd_cs >= 0) {
        ret = gpio_request(gpio_lcd_cs, "lcd_cs");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "m1700: failed to request: %s\n",
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


static void __exit m1700_exit(void)
{
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

module_init(m1700_init);
module_exit(m1700_exit);

MODULE_DESCRIPTION("m1700 lcd panel driver");
MODULE_LICENSE("GPL");