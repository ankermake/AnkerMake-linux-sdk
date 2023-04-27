#include <utils/gpio.h>
#include <utils/spi.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/module.h>

#include "drivers/spi_fb/lcdc_data.h"

static int gpio_lcd_rst = -1;       // GPIO_PB(0) 低电平有效
static int gpio_lcd_power_en = -1;  // GPIO_PB(4) 低电平有效
static int spi_bus_num = -1;        // 0
static int gpio_spi_cs = -1;        // GPIO_PB(28) 低电平有效
static int gpio_spi_rs = -1;        // D/CX 测试点连GPIO_PA(17)

module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_spi_cs, 0644);
module_param_gpio(gpio_spi_rs, 0644);
module_param(spi_bus_num, int, 0644);

static struct spi_device *spi;

static inline void m_msleep(int ms)
{
    usleep_range(ms * 1000, ms * 1000);
}

static void spi_writecomm(struct spi_device *pdev, unsigned char cmd)
{
    gpio_set_value(gpio_spi_rs, 0);
    spi_write(pdev, &cmd, 1);
}

static void spi_writedata(struct spi_device *pdev, unsigned char value)
{
    gpio_set_value(gpio_spi_rs, 1);
    spi_write(pdev, &value, 1);
}

static void gc9106_spiinit(struct spi_device *pdev)
{
    spi_writecomm (pdev, 0xFE);// set Inter_command high
    spi_writecomm (pdev, 0xFE);// set Inter_command high
    spi_writecomm (pdev, 0xEF);// set Inter_command high

    spi_writecomm (pdev, 0xB3);// 被依赖于0xF0 0xF1
    spi_writedata (pdev, 0x03);

    spi_writecomm (pdev, 0xB6);// 被依赖于0xA3
    spi_writedata (pdev, 0x01);

    spi_writecomm (pdev, 0xA3);// Frame Rate Set
    spi_writedata (pdev, 0x11);

    spi_writecomm (pdev, 0x21);// Display Inversion ON

    spi_writecomm (pdev, 0x36);// Memory Access Ctrl
    spi_writedata (pdev, 0xD0); // 1101 0000

    spi_writecomm (pdev, 0x3A);// COLMOD: Pixel Format Set
    spi_writedata (pdev, 0x05); // 0000 0101  16 bits/pixel RGB565  // 0000 0110  24 bits/pixel RGB666

    spi_writecomm (pdev, 0xB4);// Display Inversion Control
    spi_writedata (pdev, 0x21); // 0010 0001

    spi_writecomm (pdev, 0xF0);// SET_GAMMA1
    spi_writedata (pdev, 0x31);
    spi_writedata (pdev, 0x4C);
    spi_writedata (pdev, 0x24);
    spi_writedata (pdev, 0x58);
    spi_writedata (pdev, 0xA8);
    spi_writedata (pdev, 0x26);
    spi_writedata (pdev, 0x28);
    spi_writedata (pdev, 0x00);
    spi_writedata (pdev, 0x2C);
    spi_writedata (pdev, 0x0C);
    spi_writedata (pdev, 0x0C);
    spi_writedata (pdev, 0x15);
    spi_writedata (pdev, 0x15);
    spi_writedata (pdev, 0x0F);

    spi_writecomm (pdev, 0xF1);// SET_GAMMA2
    spi_writedata (pdev, 0x0E);
    spi_writedata (pdev, 0x2D);
    spi_writedata (pdev, 0x24);
    spi_writedata (pdev, 0x3E);
    spi_writedata (pdev, 0x99);
    spi_writedata (pdev, 0x12);
    spi_writedata (pdev, 0x13);
    spi_writedata (pdev, 0x00);
    spi_writedata (pdev, 0x0A);
    spi_writedata (pdev, 0x0D);
    spi_writedata (pdev, 0x0D);
    spi_writedata (pdev, 0x14);
    spi_writedata (pdev, 0x13);
    spi_writedata (pdev, 0x0F);

    spi_writecomm (pdev, 0xFE);// set Inter_command high
    spi_writecomm (pdev, 0xFF);

    spi_writecomm (pdev, 0x35);// Tearing Effect Line ON
    spi_writedata (pdev, 0x00);
    spi_writecomm (pdev, 0x44);// Scan line set
    spi_writedata (pdev, 0x00);
    spi_writecomm (pdev, 0x11);// Sleep Out
    m_msleep(120);
    spi_writecomm (pdev, 0x38);// Idle Mode OFF
    spi_writecomm (pdev, 0x29);// Display ON
    // spi_writecomm (pdev, 0x2C);// Memory Write
}

static void Lcd_exit_sleep(struct spi_device *pdev)
{
    spi_writecomm (pdev, 0xFE);
    spi_writecomm (pdev, 0xEF);
    spi_writecomm (pdev, 0x11);
    m_msleep(120);
    spi_writecomm (pdev, 0x29);
}

static int gc9106_power_on(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0) {
        gpio_direction_output(gpio_lcd_power_en, 0);
        m_msleep(180);
    }

    gpio_direction_output(gpio_spi_cs, 1);

    gpio_direction_output(gpio_spi_rs, 0);

    gpio_direction_output(gpio_lcd_rst, 1);
    m_msleep(50);
    gpio_direction_output(gpio_lcd_rst, 0);
    m_msleep(50);
    gpio_direction_output(gpio_lcd_rst, 1);
    m_msleep(120);

    gpio_set_value(gpio_spi_cs, 0);

    Lcd_exit_sleep(spi);

    gc9106_spiinit(spi);

    return 0;
}

static int gc9106_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0)
        gpio_direction_output(gpio_lcd_power_en, 1);

    gpio_set_value(gpio_lcd_rst, 0);

    return 0;
}

void gc9106_spi_write_image_data(unsigned char *addr);

struct lcdc_data lcdc_data = {
    .name = "gc9106",
    .refresh = 28,
    .xres = 128,
    .yres = 160,
    .pixclock = 0,
    .left_margin = 0,
    .right_margin = 0,
    .upper_margin = 0,
    .lower_margin = 0,
    .hsync_len = 0,
    .vsync_len = 0,
    .fb_format = fb_fmt_RGB565, /* fb_fmt_RGB888 fb_fmt_RGB565 */
    .power_on = gc9106_power_on,
    .power_off = gc9106_power_off,
    .write_fb_data = gc9106_spi_write_image_data,
};

static inline int bytes_per_pixel(enum fb_fmt fmt)
{
    if (fmt == fb_fmt_RGB888 || fmt == fb_fmt_ARGB8888)
        return 4;
    return 2;
}

static inline int bits_per_pixel(enum fb_fmt fmt)
{
    if (fmt == fb_fmt_RGB888)
        return 24;
    if (fmt == fb_fmt_ARGB8888)
        return 32;
    return 16;
}

static void spi_writedatas(struct spi_device *pdev, unsigned char *value, unsigned int len)
{
    gpio_set_value(gpio_spi_rs, 1);

    struct spi_transfer t = {
            .tx_buf    = value,
            .len       = len,
            .bits_per_word = bits_per_pixel(lcdc_data.fb_format),
        };
    struct spi_message m;

    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    spi_sync(pdev, &m);
}

void gc9106_spi_write_image_data(unsigned char *addr)
{
    struct spi_device *pdev = spi;

    int size = lcdc_data.xres * bytes_per_pixel(lcdc_data.fb_format) * lcdc_data.yres;

    spi_writecomm (pdev, 0x2C); //Memory Write

    spi_writedatas(pdev, addr, size);
}

static int gc9106_probe(struct spi_device *pdev)
{
    spi = pdev;

    spi_fb_register_lcd(&lcdc_data);

    return 0;
}

static int gc9106_remove(struct spi_device *pdev)
{
    spi_fb_unregister_lcd(&lcdc_data);

    return 0;
}

static struct spi_device_id id_table[] = {
    {
        .name = "gc9106_tft",
        .driver_data = 1,
    },
};

static struct spi_driver gc9106_driver = {
    .driver = {
        .name   = "gc9106_tft",
        .bus    = &spi_bus_type,
        .owner  = THIS_MODULE,
    },
    .id_table = id_table,
    .probe    = gc9106_probe,
    .remove   = gc9106_remove,
};

static struct spi_board_info gc9106_device = {
    .modalias = "gc9106_tft",
    .max_speed_hz = 10 * 1000 * 1000,
    .mode = SPI_MODE_0,
};

static int __init gc9106_init(void)
{
    int ret;

    if (spi_bus_num < 0) {
        printk(KERN_ERR "gc9106: spi_bus_num must set\n");
        return -EINVAL;
    }

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

    ret = gpio_request(gpio_spi_rs, "lcd_scl");
    if (ret) {
        char buf[10];
        printk(KERN_ERR "gc9106: failed to request gpio_spi_rs: %s\n",
                gpio_to_str(gpio_spi_rs, buf));
        goto err_lcd_rs;
    }

    ret = gpio_request(gpio_spi_cs, "lcd_rst");
    if (ret) {
        char buf[10];
        printk(KERN_ERR "gc9106: failed to request gpio_spi_cs: %s\n",
                gpio_to_str(gpio_spi_cs, buf));
        goto err_lcd_cs;
    }

    ret = spi_register_driver(&gc9106_driver);
    if (ret) {
        printk(KERN_ERR "gc9106: failed to register spi driver\n");
        goto err_register_driver;
    }

    gc9106_device.controller_data = (void *)-1; /* gpio_spi_cs */
    struct spi_device *dev = spi_register_device(&gc9106_device, spi_bus_num);
    if (dev == NULL) {
        printk(KERN_ERR "gc9106: failed to register spi device\n");
        ret = -1;
        goto err_register_device;
    }

    return 0;

err_register_device:
    spi_unregister_driver(&gc9106_driver);
err_register_driver:
    gpio_free(gpio_spi_cs);
err_lcd_cs:
    gpio_free(gpio_spi_rs);
err_lcd_rs:
    gpio_free(gpio_lcd_rst);
err_lcd_rst:
    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);
    return ret;
}

static void __exit gc9106_exit(void)
{
    if (spi)
        spi_unregister_device(spi);
    spi_unregister_driver(&gc9106_driver);

    gpio_free(gpio_spi_cs);

    gpio_free(gpio_spi_rs);

    gpio_free(gpio_lcd_rst);

    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);
}

module_init(gc9106_init);
module_exit(gc9106_exit);

MODULE_DESCRIPTION("gc9106 lcd panel driver");
MODULE_LICENSE("GPL");