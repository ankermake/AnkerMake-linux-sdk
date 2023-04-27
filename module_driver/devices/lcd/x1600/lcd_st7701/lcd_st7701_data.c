#include <utils/gpio.h>
#include <utils/spi.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>

#include <fb/lcdc_data.h>


static int gpio_lcd_power_en = -1; // GPIO_PB(2)
static int gpio_lcd_rst      = -1; // GPIO_PB(3)
static int spi_bus_num = -1;
static int gpio_spi_cs = -1;

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_spi_cs, 0644);
module_param(spi_bus_num, int, 0644);

static inline void m_msleep(int ms)
{
    usleep_range(ms * 1000, ms * 1000);
}


void m_spi_write(struct spi_device *pdev, unsigned char *buf, int num)
{
    gpio_direction_output(gpio_spi_cs, 0);
    usleep_range(20, 20);

    spi_write(pdev, buf, num);

    gpio_direction_output(gpio_spi_cs, 1);
    usleep_range(20, 20);
}

static void spi_writecomm(struct spi_device *pdev, unsigned short cmd)
{
    unsigned char buf[4]={0x20, cmd>>8, 0x00, cmd};
    m_spi_write(pdev,buf,4);
}

static void spi_writedata(struct spi_device *pdev, unsigned char value)
{
    unsigned char buf[2]={0x40, value};
    m_spi_write(pdev,buf,2);
}


static void st7701s_spiinit(struct spi_device *pdev)
{
    spi_writecomm(pdev, 0xFF00);spi_writedata(pdev, 0x77);
	spi_writecomm(pdev, 0XFF01);spi_writedata(pdev, 0x01);
	spi_writecomm(pdev, 0xFF02);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xFF03);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xFF04);spi_writedata(pdev, 0x10);

	//spi_writecomm(pdev, 0xef00);spi_writedata(pdev, 0x08);


	//spi_writecomm(pdev, 0xC000);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xC000);spi_writedata(pdev, 0x63);
	spi_writecomm(pdev, 0xC001);spi_writedata(pdev, 0x00);

	//spi_writecomm(pdev, 0xC100);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xC100);spi_writedata(pdev, 0x11);
	spi_writecomm(pdev, 0xC101);spi_writedata(pdev, 0x02);

	//spi_writecomm(pdev, 0xC200);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xC200);spi_writedata(pdev, 0x01);
	spi_writecomm(pdev, 0xC201);spi_writedata(pdev, 0x08);

    // spi_writecomm (pdev,0xC300); spi_writedata (pdev,0x02);
    // spi_writecomm (pdev,0xC301); spi_writecomm (pdev,0xCC);
    // spi_writecomm (pdev,0xC302); spi_writedata (pdev,0x10);
	//spi_writecomm(pdev, 0xCc00);spi_writedata(pdev, 0x00);

	spi_writecomm(pdev, 0xCC00);spi_writedata(pdev, 0x10);

	//spi_writecomm(pdev, 0xB000);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xB000);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xB001);spi_writedata(pdev, 0x0d);
	spi_writecomm(pdev, 0xB002);spi_writedata(pdev, 0x14);
	spi_writecomm(pdev, 0xB003);spi_writedata(pdev, 0x0e);
	spi_writecomm(pdev, 0xB004);spi_writedata(pdev, 0x11);
	spi_writecomm(pdev, 0xB005);spi_writedata(pdev, 0x07);
	spi_writecomm(pdev, 0xB006);spi_writedata(pdev, 0x44);
	spi_writecomm(pdev, 0xB007);spi_writedata(pdev, 0x08);
	spi_writecomm(pdev, 0xB008);spi_writedata(pdev, 0x08);
	spi_writecomm(pdev, 0xB009);spi_writedata(pdev, 0x60);
	spi_writecomm(pdev, 0xB00A);spi_writedata(pdev, 0x03);
	spi_writecomm(pdev, 0xB00B);spi_writedata(pdev, 0x11);
	spi_writecomm(pdev, 0xB00C);spi_writedata(pdev, 0x10);
	spi_writecomm(pdev, 0xB00D);spi_writedata(pdev, 0x2d);
	spi_writecomm(pdev, 0xB00E);spi_writedata(pdev, 0x34);
	spi_writecomm(pdev, 0xB00F);spi_writedata(pdev, 0x1F);

	//spi_writecomm(pdev, 0xB100);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xB100);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xB101);spi_writedata(pdev, 0x0c);
	spi_writecomm(pdev, 0xB102);spi_writedata(pdev, 0x13);
	spi_writecomm(pdev, 0xB103);spi_writedata(pdev, 0x0d);
	spi_writecomm(pdev, 0xB104);spi_writedata(pdev, 0x10);
	spi_writecomm(pdev, 0xB105);spi_writedata(pdev, 0x05);
	spi_writecomm(pdev, 0xB106);spi_writedata(pdev, 0x43);
	spi_writecomm(pdev, 0xB107);spi_writedata(pdev, 0x08);
	spi_writecomm(pdev, 0xB108);spi_writedata(pdev, 0x07);
	spi_writecomm(pdev, 0xB109);spi_writedata(pdev, 0x60);
	spi_writecomm(pdev, 0xB10A);spi_writedata(pdev, 0x04);
	spi_writecomm(pdev, 0xB10B);spi_writedata(pdev, 0x11);
	spi_writecomm(pdev, 0xB10C);spi_writedata(pdev, 0x10);
	spi_writecomm(pdev, 0xB10D);spi_writedata(pdev, 0x2d);
	spi_writecomm(pdev, 0xB10E);spi_writedata(pdev, 0x34);
	spi_writecomm(pdev, 0xB10F);spi_writedata(pdev, 0x1F);

	//spi_writecomm(pdev, 0xFF00);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xFF00);spi_writedata(pdev, 0x77);
	spi_writecomm(pdev, 0XFF01);spi_writedata(pdev, 0x01);
	spi_writecomm(pdev, 0xFF02);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xFF03);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xFF04);spi_writedata(pdev, 0x11);

	//spi_writecomm(pdev, 0xB000);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xB000);spi_writedata(pdev, 0x67);

	//spi_writecomm(pdev, 0xB100);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xB100);spi_writedata(pdev, 0x66);

	//spi_writecomm(pdev, 0xB200);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xB200);spi_writedata(pdev, 0x07);

	//spi_writecomm(pdev, 0xB300);spi_writedata(pdev, 0x00);	 //0b
	spi_writecomm(pdev, 0xB300);spi_writedata(pdev, 0x80);

	//spi_writecomm(pdev, 0xB500);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xB500);spi_writedata(pdev, 0x45);

	//spi_writecomm(pdev, 0xB700);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xB700);spi_writedata(pdev, 0x85);

	//spi_writecomm(pdev, 0xB800);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xB800);spi_writedata(pdev, 0x21);

	spi_writecomm(pdev, 0xB900);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xB901);spi_writedata(pdev, 0x10);

	//spi_writecomm(pdev, 0xC100);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xC100);spi_writedata(pdev, 0x78);

	//spi_writecomm(pdev, 0xC200);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xC200);spi_writedata(pdev, 0x78);

	//spi_writecomm(pdev, 0xD000);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xD000);spi_writedata(pdev, 0x88);

	//spi_writecomm(pdev, 0xE000);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE000);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE001);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE002);spi_writedata(pdev, 0x02);

	//spi_writecomm(pdev, 0xE100);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE100);spi_writedata(pdev, 0x08);
	spi_writecomm(pdev, 0xE101);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE102);spi_writedata(pdev, 0x0a);
	spi_writecomm(pdev, 0xE105);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE104);spi_writedata(pdev, 0x07);
	spi_writecomm(pdev, 0xE105);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE106);spi_writedata(pdev, 0x09);
	spi_writecomm(pdev, 0xE107);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE108);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE109);spi_writedata(pdev, 0x33);
	spi_writecomm(pdev, 0xE10A);spi_writedata(pdev, 0x33);

	//spi_writecomm(pdev, 0xE200);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE200);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE201);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE202);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE203);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE204);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE205);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE206);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE207);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE208);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE209);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE20A);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE20B);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE20C);spi_writedata(pdev, 0x00);

	//spi_writecomm(pdev, 0xE300);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE300);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE301);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE302);spi_writedata(pdev, 0x33);
	spi_writecomm(pdev, 0xE303);spi_writedata(pdev, 0x33);

	//spi_writecomm(pdev, 0xE400);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE400);spi_writedata(pdev, 0x44);
	spi_writecomm(pdev, 0xE401);spi_writedata(pdev, 0x44);

	//spi_writecomm(pdev, 0xE500);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE500);spi_writedata(pdev, 0x0e);
	spi_writecomm(pdev, 0xE501);spi_writedata(pdev, 0x2d);
	spi_writecomm(pdev, 0xE502);spi_writedata(pdev, 0xa0);
	spi_writecomm(pdev, 0xE503);spi_writedata(pdev, 0xA0);
	spi_writecomm(pdev, 0xE504);spi_writedata(pdev, 0x10);
	spi_writecomm(pdev, 0xE505);spi_writedata(pdev, 0x2d);
	spi_writecomm(pdev, 0xE506);spi_writedata(pdev, 0xa0);
	spi_writecomm(pdev, 0xE507);spi_writedata(pdev, 0xA0);
	spi_writecomm(pdev, 0xE508);spi_writedata(pdev, 0x0a);
	spi_writecomm(pdev, 0xE509);spi_writedata(pdev, 0x2d);
	spi_writecomm(pdev, 0xE50A);spi_writedata(pdev, 0xa0);
	spi_writecomm(pdev, 0xE50B);spi_writedata(pdev, 0xA0);
	spi_writecomm(pdev, 0xE50C);spi_writedata(pdev, 0x0c);
	spi_writecomm(pdev, 0xE50D);spi_writedata(pdev, 0x2d);
	spi_writecomm(pdev, 0xE50E);spi_writedata(pdev, 0xa0);
	spi_writecomm(pdev, 0xE50F);spi_writedata(pdev, 0xA0);

	//spi_writecomm(pdev, 0xE600);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE600);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE601);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE602);spi_writedata(pdev, 0x33);
	spi_writecomm(pdev, 0xE603);spi_writedata(pdev, 0x33);

	//spi_writecomm(pdev, 0xE700);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE700);spi_writedata(pdev, 0x44);
	spi_writecomm(pdev, 0xE701);spi_writedata(pdev, 0x44);

	//spi_writecomm(pdev, 0xE800);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xE800);spi_writedata(pdev, 0x0d);
	spi_writecomm(pdev, 0xE801);spi_writedata(pdev, 0x2d);
	spi_writecomm(pdev, 0xE802);spi_writedata(pdev, 0xa0);
	spi_writecomm(pdev, 0xE803);spi_writedata(pdev, 0xA0);
	spi_writecomm(pdev, 0xE804);spi_writedata(pdev, 0x0f);
	spi_writecomm(pdev, 0xE805);spi_writedata(pdev, 0x2d);
	spi_writecomm(pdev, 0xE806);spi_writedata(pdev, 0xa0);
	spi_writecomm(pdev, 0xE807);spi_writedata(pdev, 0xA0);
	spi_writecomm(pdev, 0xE808);spi_writedata(pdev, 0x09);
	spi_writecomm(pdev, 0xE809);spi_writedata(pdev, 0x2d);
	spi_writecomm(pdev, 0xE80A);spi_writedata(pdev, 0xa0);
	spi_writecomm(pdev, 0xE80B);spi_writedata(pdev, 0xA0);
	spi_writecomm(pdev, 0xE80C);spi_writedata(pdev, 0x0b);
	spi_writecomm(pdev, 0xE80D);spi_writedata(pdev, 0x2d);
	spi_writecomm(pdev, 0xE80E);spi_writedata(pdev, 0xa0);
	spi_writecomm(pdev, 0xE80F);spi_writedata(pdev, 0xA0);

	//spi_writecomm(pdev, 0xEB00);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xEB00);spi_writedata(pdev, 0x02);
	spi_writecomm(pdev, 0xEB01);spi_writedata(pdev, 0x01);
	spi_writecomm(pdev, 0xEB02);spi_writedata(pdev, 0xe4);
	spi_writecomm(pdev, 0xEB03);spi_writedata(pdev, 0xe4);
	spi_writecomm(pdev, 0xEB04);spi_writedata(pdev, 0x44);
	spi_writecomm(pdev, 0xEB05);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xEB06);spi_writedata(pdev, 0x40);

	//spi_writecomm(pdev, 0xEC00);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xEC00);spi_writedata(pdev, 0x02);
	spi_writecomm(pdev, 0xEC01);spi_writedata(pdev, 0x01);

	//spi_writecomm(pdev, 0xED00);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xED00);spi_writedata(pdev, 0xab);
	spi_writecomm(pdev, 0xED01);spi_writedata(pdev, 0x89);
	spi_writecomm(pdev, 0xED02);spi_writedata(pdev, 0x76);
	spi_writecomm(pdev, 0xED03);spi_writedata(pdev, 0x54);
	spi_writecomm(pdev, 0xED04);spi_writedata(pdev, 0x01);
	spi_writecomm(pdev, 0xED05);spi_writedata(pdev, 0xFF);
	spi_writecomm(pdev, 0xED06);spi_writedata(pdev, 0xFF);
	spi_writecomm(pdev, 0xED07);spi_writedata(pdev, 0xFF);
	spi_writecomm(pdev, 0xED08);spi_writedata(pdev, 0xFF);
	spi_writecomm(pdev, 0xED09);spi_writedata(pdev, 0xFF);
	spi_writecomm(pdev, 0xED0A);spi_writedata(pdev, 0xFf);
	spi_writecomm(pdev, 0xED0B);spi_writedata(pdev, 0x10);
	spi_writecomm(pdev, 0xED0C);spi_writedata(pdev, 0x45);
	spi_writecomm(pdev, 0xED0D);spi_writedata(pdev, 0x67);
	spi_writecomm(pdev, 0xED0E);spi_writedata(pdev, 0x98);
	spi_writecomm(pdev, 0xED0F);spi_writedata(pdev, 0xba);
	/*
	spi_writecomm(pdev, 0xFF00);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xFF01);spi_writedata(pdev, 0x77);
	spi_writecomm(pdev, 0XFF02);spi_writedata(pdev, 0x01);
	spi_writecomm(pdev, 0xFF03);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xFF04);spi_writedata(pdev, 0x00);
	spi_writecomm(pdev, 0xFF05);spi_writedata(pdev, 0x00);

	spi_writecomm(pdev, 0xE500);spi_writedata(pdev, 0xE4);
	*/
    //spi_writecomm(pdev, 0xFF00);spi_writedata(pdev, 0x00);
    spi_writecomm(pdev, 0xFF00);spi_writedata(pdev, 0x77);
    spi_writecomm(pdev, 0XFF01);spi_writedata(pdev, 0x01);
    spi_writecomm(pdev, 0xFF02);spi_writedata(pdev, 0x00);
    spi_writecomm(pdev, 0xFF03);spi_writedata(pdev, 0x00);
    spi_writecomm(pdev, 0xFF04);spi_writedata(pdev, 0x00);

    spi_writecomm(pdev, 0x3a00);spi_writedata(pdev, 0x77);
    spi_writecomm(pdev, 0x3600);spi_writedata(pdev, 0x00);
    spi_writecomm(pdev, 0x1100); spi_writedata(pdev, 0x00);
    m_msleep(150);
    spi_writecomm(pdev, 0x2900); spi_writedata(pdev, 0x00);
}



static struct spi_device *spi;

static int st7701_power_on(struct lcdc *lcdc)
{

    gpio_direction_output(gpio_lcd_rst, 1);
    gpio_direction_output(gpio_lcd_power_en, 0);

    m_msleep(100);

    if (gpio_lcd_power_en >= 0) {
        gpio_direction_output(gpio_lcd_power_en, 1);
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

    st7701s_spiinit(spi);

    return 0;
}

static int st7701_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0)
        gpio_direction_output(gpio_lcd_power_en, 1);

    return 0;
}

struct lcdc_data lcdc_data = {
    .name = "st7701",
    .refresh = 60,
    .xres = 480,
    .yres = 800,
    .pixclock = 0, // 自动计算
    .left_margin = 42,
    .right_margin = 16,
    .upper_margin = 28,
    .lower_margin = 12,
    .hsync_len = 2,
    .vsync_len = 2,
    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = TFT_24BITS,
    .out_format = OUT_FORMAT_RGB888,
    .tft = {
        .even_line_order = ORDER_RGB,
        .odd_line_order = ORDER_RGB,
        .pix_clk_polarity = AT_RISING_EDGE,
        .de_active_level = AT_HIGH_LEVEL,
        .hsync_active_level = AT_HIGH_LEVEL,
        .vsync_active_level = AT_HIGH_LEVEL,
    },
    .power_on = st7701_power_on,
    .power_off = st7701_power_off,
};


static int st7701s_probe(struct spi_device *pdev)
{
    spi = pdev;

    jzfb_register_lcd(&lcdc_data);

    return 0;
}

static int st7701s_remove(struct spi_device *pdev)
{
    jzfb_unregister_lcd(&lcdc_data);

    return 0;
}

static struct spi_device_id id_table[] = {
    {
        .name = "md_st7701s_tft",
        .driver_data = 1,
    },
};

static struct spi_driver st7701s_driver = {
    .driver = {
        .name   = "md_st7701s_tft",
        .bus    = &spi_bus_type,
        .owner  = THIS_MODULE,
    },
    .id_table = id_table,
    .probe    = st7701s_probe,
    .remove   = st7701s_remove,
};

static struct spi_board_info st7701s_device = {
    .modalias = "md_st7701s_tft",
    .max_speed_hz = 1*1000*1000,
    .mode = SPI_MODE_0,
};

static int __init st7701s_init(void)
{
    int ret;

    if (spi_bus_num < 0) {
        printk(KERN_ERR "st7701: spi_bus_num must set\n");
        return -EINVAL;
    }

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

    ret = spi_register_driver(&st7701s_driver);
    if (ret) {
        printk(KERN_ERR "st7701: failed to register spi driver\n");
        goto err_register_driver;
    }

    st7701s_device.controller_data = (void *)-1;
    struct spi_device *dev = spi_register_device(&st7701s_device, spi_bus_num);
    if (dev == NULL) {
        printk(KERN_ERR "st7701: failed to register spi device\n");
        ret = -1;
        goto err_register_device;
    }

    return 0;
err_register_device:
    spi_unregister_driver(&st7701s_driver);
err_register_driver:
    if (gpio_lcd_rst >= 0)
        gpio_free(gpio_lcd_rst);
err_lcd_rst:
    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);
    return ret;
}


static void __exit st7701s_exit(void)
{
    if (spi)
        spi_unregister_device(spi);
    spi_unregister_driver(&st7701s_driver);
    if (gpio_lcd_rst >= 0)
        gpio_free(gpio_lcd_rst);
    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);
}

module_init(st7701s_init);
module_exit(st7701s_exit);

MODULE_DESCRIPTION("st7701s lcd panel driver");
MODULE_LICENSE("GPL");
