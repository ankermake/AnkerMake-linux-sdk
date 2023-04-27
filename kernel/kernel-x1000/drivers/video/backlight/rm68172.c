/*
 *  LCD control code for ili6122
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/lcd.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>

struct rm68172_data {
    struct device *dev;
    struct lcd_device *lcd;
    struct lcd_platform_data *ctrl;
    struct regulator *lcd_vcc_reg;
    struct backlight_device *bd;
    int lcd_power;
};
struct spi_device_id jz_id_table[] = {
    {
        .name = "rm68172_tft",
        .driver_data = 0,
    },
};


extern int lcd_display_inited_by_uboot( void );
static void spi_writecomm(struct spi_device *pdev, unsigned short cmd)
{
    unsigned char buf[4]={0x20, cmd>>8, 0x00, cmd};
    spi_write(pdev,buf,4);
}

static void spi_writedata(struct spi_device *pdev, unsigned char value)
{
    unsigned char buf[2]={0x40, value};
    spi_write(pdev,buf,2);
}

static int rm68172_set_power(struct lcd_device *lcd, int power)
{
    struct rm68172_data *dev = lcd_get_data(lcd);

    if (!power && dev->lcd_power) {
        dev->ctrl->power_on(lcd, 1);

    } else if (power && !dev->lcd_power) {
        if (dev->ctrl->reset) {
            dev->ctrl->reset(lcd);
        }
        dev->ctrl->power_on(lcd, 0);
    }
    dev->lcd_power = power;

    return 0;
}

static int rm68172_early_set_power(struct lcd_device *lcd, int power)
{
    return rm68172_set_power(lcd, power);
}

static int rm68172_get_power(struct lcd_device *lcd)
{
    struct rm68172_data *dev= lcd_get_data(lcd);

    return dev->lcd_power;
}

static int rm68172_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
    return 0;
}

static struct lcd_ops rm68172_ops = {
    .early_set_power        = rm68172_early_set_power,
    .set_power              = rm68172_set_power,
    .get_power              = rm68172_get_power,
    .set_mode               = rm68172_set_mode,
};

void rm68172_spiinit(struct spi_device *pdev)
{
    spi_writecomm(pdev,0xF000); spi_writedata(pdev,0x55);

    spi_writecomm(pdev,0xF001); spi_writedata(pdev,0xAA);

    spi_writecomm(pdev,0xF002); spi_writedata(pdev,0x52);

    spi_writecomm(pdev,0xF003); spi_writedata(pdev,0x08);

    spi_writecomm(pdev,0xF004); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xF600); spi_writedata(pdev,0x60);

    spi_writecomm(pdev,0xF601); spi_writedata(pdev,0x40);

    spi_writecomm(pdev,0xFE00); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xFE01); spi_writedata(pdev,0x80);

    spi_writecomm(pdev,0xFE02); spi_writedata(pdev,0x09);

    spi_writecomm(pdev,0xFE03); spi_writedata(pdev,0x09);

    mdelay(20);

    spi_writecomm(pdev,0xF000); spi_writedata(pdev,0x55);

    spi_writecomm(pdev,0xF001); spi_writedata(pdev,0xAA);

    spi_writecomm(pdev,0xF002); spi_writedata(pdev,0x52);

    spi_writecomm(pdev,0xF003); spi_writedata(pdev,0x08);

    spi_writecomm(pdev,0xF004); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xB000); spi_writedata(pdev,0x07);

    spi_writecomm(pdev,0xB100); spi_writedata(pdev,0x07);

    spi_writecomm(pdev,0xB500); spi_writedata(pdev,0x08);

    spi_writecomm(pdev,0xB600); spi_writedata(pdev,0x54);

    spi_writecomm(pdev,0xB700); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xB800); spi_writedata(pdev,0x24);

    spi_writecomm(pdev,0xB900); spi_writedata(pdev,0x34);

    spi_writecomm(pdev,0xBA00); spi_writedata(pdev,0x14);

    spi_writecomm(pdev,0xBC00); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xBC01); spi_writedata(pdev,0x78);

    spi_writecomm(pdev,0xBC02); spi_writedata(pdev,0x13);

    spi_writecomm(pdev,0xBD00); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xBD01); spi_writedata(pdev,0x78);

    spi_writecomm(pdev,0xBD02); spi_writedata(pdev,0x13);

    spi_writecomm(pdev,0xBE00); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xBE01); spi_writedata(pdev,0x1A);//3C

    spi_writecomm(pdev,0xD100); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD101); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD102); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD103); spi_writedata(pdev,0x17);//14

    spi_writecomm(pdev,0xD104); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD105); spi_writedata(pdev,0x3E);//36

    spi_writecomm(pdev,0xD106); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD107); spi_writedata(pdev,0x5E);//53

    spi_writecomm(pdev,0xD108); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD109); spi_writedata(pdev,0x7B);//6C

    spi_writecomm(pdev,0xD10A); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD10B); spi_writedata(pdev,0xA9);//98

    spi_writecomm(pdev,0xD10C); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD10D); spi_writedata(pdev,0xCE);//BB

    spi_writecomm(pdev,0xD10E); spi_writedata(pdev,0x01);//00

    spi_writecomm(pdev,0xD10F); spi_writedata(pdev,0x0A);//F4

    spi_writecomm(pdev,0xD110); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD111); spi_writedata(pdev,0x37);//22

    spi_writecomm(pdev,0xD112); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD113); spi_writedata(pdev,0x7C);//67

    spi_writecomm(pdev,0xD114); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD115); spi_writedata(pdev,0xB0);//9D

    spi_writecomm(pdev,0xD116); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD117); spi_writedata(pdev,0xFF);//EF

    spi_writecomm(pdev,0xD118); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD119); spi_writedata(pdev,0x3D);//32

    spi_writecomm(pdev,0xD11A); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD11B); spi_writedata(pdev,0x3F);//34

    spi_writecomm(pdev,0xD11C); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD11D); spi_writedata(pdev,0x7C);//75

    spi_writecomm(pdev,0xD11E); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD11F); spi_writedata(pdev,0xC4);//C1

    spi_writecomm(pdev,0xD120); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD121); spi_writedata(pdev,0xF6);//F3

    spi_writecomm(pdev,0xD122); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD123); spi_writedata(pdev,0x3A);//36

    spi_writecomm(pdev,0xD124); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD125); spi_writedata(pdev,0x68);//62

    spi_writecomm(pdev,0xD126); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD127); spi_writedata(pdev,0xA0);//99

    spi_writecomm(pdev,0xD128); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD129); spi_writedata(pdev,0xBF);//B8

    spi_writecomm(pdev,0xD12A); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD12B); spi_writedata(pdev,0xE0);//D9

    spi_writecomm(pdev,0xD12C); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD12D); spi_writedata(pdev,0xEC);//E7

    spi_writecomm(pdev,0xD12E); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD12F); spi_writedata(pdev,0xF5);

    spi_writecomm(pdev,0xD130); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD131); spi_writedata(pdev,0xFA);

    spi_writecomm(pdev,0xD132); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD133); spi_writedata(pdev,0xFF);

    spi_writecomm(pdev,0xD200); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD201); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD202); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD203); spi_writedata(pdev,0x17);

    spi_writecomm(pdev,0xD204); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD205); spi_writedata(pdev,0x3E);

    spi_writecomm(pdev,0xD206); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD207); spi_writedata(pdev,0x5E);

    spi_writecomm(pdev,0xD208); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD209); spi_writedata(pdev,0x7B);

    spi_writecomm(pdev,0xD20A); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD20B); spi_writedata(pdev,0xA9);

    spi_writecomm(pdev,0xD20C); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD20D); spi_writedata(pdev,0xCE);

    spi_writecomm(pdev,0xD20E); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD20F); spi_writedata(pdev,0x0A);

    spi_writecomm(pdev,0xD210); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD211); spi_writedata(pdev,0x37);

    spi_writecomm(pdev,0xD212); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD213); spi_writedata(pdev,0x7C);

    spi_writecomm(pdev,0xD214); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD215); spi_writedata(pdev,0xB0);

    spi_writecomm(pdev,0xD216); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD217); spi_writedata(pdev,0xFF);

    spi_writecomm(pdev,0xD218); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD219); spi_writedata(pdev,0x3D);

    spi_writecomm(pdev,0xD21A); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD21B); spi_writedata(pdev,0x3F);

    spi_writecomm(pdev,0xD21C); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD21D); spi_writedata(pdev,0x7C);

    spi_writecomm(pdev,0xD21E); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD21F); spi_writedata(pdev,0xC4);

    spi_writecomm(pdev,0xD220); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD221); spi_writedata(pdev,0xF6);

    spi_writecomm(pdev,0xD222); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD223); spi_writedata(pdev,0x3A);

    spi_writecomm(pdev,0xD224); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD225); spi_writedata(pdev,0x68);

    spi_writecomm(pdev,0xD226); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD227); spi_writedata(pdev,0xA0);

    spi_writecomm(pdev,0xD228); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD229); spi_writedata(pdev,0xBF);

    spi_writecomm(pdev,0xD22A); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD22B); spi_writedata(pdev,0xE0);

    spi_writecomm(pdev,0xD22C); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD22D); spi_writedata(pdev,0xEC);

    spi_writecomm(pdev,0xD22E); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD22F); spi_writedata(pdev,0xF5);

    spi_writecomm(pdev,0xD230); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD231); spi_writedata(pdev,0xFA);

    spi_writecomm(pdev,0xD232); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD233); spi_writedata(pdev,0xFF);

    spi_writecomm(pdev,0xD300); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD301); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD302); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD303); spi_writedata(pdev,0x17);

    spi_writecomm(pdev,0xD304); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD305); spi_writedata(pdev,0x3E);

    spi_writecomm(pdev,0xD306); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD307); spi_writedata(pdev,0x5E);

    spi_writecomm(pdev,0xD308); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD309); spi_writedata(pdev,0x7B);

    spi_writecomm(pdev,0xD30A); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD30B); spi_writedata(pdev,0xA9);

    spi_writecomm(pdev,0xD30C); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD30D); spi_writedata(pdev,0xCE);

    spi_writecomm(pdev,0xD30E); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD30F); spi_writedata(pdev,0x0A);

    spi_writecomm(pdev,0xD310); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD311); spi_writedata(pdev,0x37);

    spi_writecomm(pdev,0xD312); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD313); spi_writedata(pdev,0x7C);

    spi_writecomm(pdev,0xD314); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD315); spi_writedata(pdev,0xB0);

    spi_writecomm(pdev,0xD316); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD317); spi_writedata(pdev,0xFF);

    spi_writecomm(pdev,0xD318); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD319); spi_writedata(pdev,0x3D);

    spi_writecomm(pdev,0xD31A); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD31B); spi_writedata(pdev,0x3F);

    spi_writecomm(pdev,0xD31C); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD31D); spi_writedata(pdev,0x7C);

    spi_writecomm(pdev,0xD31E); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD31F); spi_writedata(pdev,0xC4);

    spi_writecomm(pdev,0xD320); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD321); spi_writedata(pdev,0xF6);

    spi_writecomm(pdev,0xD322); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD323); spi_writedata(pdev,0x3A);

    spi_writecomm(pdev,0xD324); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD325); spi_writedata(pdev,0x68);

    spi_writecomm(pdev,0xD326); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD327); spi_writedata(pdev,0xA0);

    spi_writecomm(pdev,0xD328); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD329); spi_writedata(pdev,0xBF);

    spi_writecomm(pdev,0xD32A); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD32B); spi_writedata(pdev,0xE0);

    spi_writecomm(pdev,0xD32C); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD32D); spi_writedata(pdev,0xEC);

    spi_writecomm(pdev,0xD32E); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD32F); spi_writedata(pdev,0xF5);

    spi_writecomm(pdev,0xD330); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD331); spi_writedata(pdev,0xFA);

    spi_writecomm(pdev,0xD332); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD333); spi_writedata(pdev,0xFF);

    spi_writecomm(pdev,0xD400); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD401); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD402); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD403); spi_writedata(pdev,0x17);

    spi_writecomm(pdev,0xD404); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD405); spi_writedata(pdev,0x3E);

    spi_writecomm(pdev,0xD406); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD407); spi_writedata(pdev,0x5E);

    spi_writecomm(pdev,0xD408); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD409); spi_writedata(pdev,0x7B);

    spi_writecomm(pdev,0xD40A); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD40B); spi_writedata(pdev,0xA9);

    spi_writecomm(pdev,0xD40C); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD40D); spi_writedata(pdev,0xCE);

    spi_writecomm(pdev,0xD40E); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD40F); spi_writedata(pdev,0x0A);

    spi_writecomm(pdev,0xD410); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD411); spi_writedata(pdev,0x37);

    spi_writecomm(pdev,0xD412); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD413); spi_writedata(pdev,0x7C);

    spi_writecomm(pdev,0xD414); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD415); spi_writedata(pdev,0xB0);

    spi_writecomm(pdev,0xD416); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD417); spi_writedata(pdev,0xFF);

    spi_writecomm(pdev,0xD418); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD419); spi_writedata(pdev,0x3D);

    spi_writecomm(pdev,0xD41A); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD41B); spi_writedata(pdev,0x3F);

    spi_writecomm(pdev,0xD41C); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD41D); spi_writedata(pdev,0x7C);

    spi_writecomm(pdev,0xD41E); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD41F); spi_writedata(pdev,0xC4);////////////

    spi_writecomm(pdev,0xD420); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD421); spi_writedata(pdev,0xF6);

    spi_writecomm(pdev,0xD422); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD423); spi_writedata(pdev,0x3A);

    spi_writecomm(pdev,0xD424); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD425); spi_writedata(pdev,0x68);

    spi_writecomm(pdev,0xD426); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD427); spi_writedata(pdev,0xA0);

    spi_writecomm(pdev,0xD428); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD429); spi_writedata(pdev,0xBF);

    spi_writecomm(pdev,0xD42A); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD42B); spi_writedata(pdev,0xE0);

    spi_writecomm(pdev,0xD42C); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD42D); spi_writedata(pdev,0xEC);

    spi_writecomm(pdev,0xD42E); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD42F); spi_writedata(pdev,0xF5);

    spi_writecomm(pdev,0xD430); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD431); spi_writedata(pdev,0xFA);

    spi_writecomm(pdev,0xD432); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD433); spi_writedata(pdev,0xFF);

    spi_writecomm(pdev,0xD500); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD501); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD502); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD503); spi_writedata(pdev,0x17);

    spi_writecomm(pdev,0xD504); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD505); spi_writedata(pdev,0x3E);

    spi_writecomm(pdev,0xD506); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD507); spi_writedata(pdev,0x5E);

    spi_writecomm(pdev,0xD508); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD509); spi_writedata(pdev,0x7B);

    spi_writecomm(pdev,0xD50A); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD50B); spi_writedata(pdev,0xA9);

    spi_writecomm(pdev,0xD50C); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD50D); spi_writedata(pdev,0xCE);

    spi_writecomm(pdev,0xD50E); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD50F); spi_writedata(pdev,0x0A);

    spi_writecomm(pdev,0xD510); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD511); spi_writedata(pdev,0x37);

    spi_writecomm(pdev,0xD512); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD513); spi_writedata(pdev,0x7C);

    spi_writecomm(pdev,0xD514); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD515); spi_writedata(pdev,0xB0);

    spi_writecomm(pdev,0xD516); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD517); spi_writedata(pdev,0xFF);

    spi_writecomm(pdev,0xD518); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD519); spi_writedata(pdev,0x3D);

    spi_writecomm(pdev,0xD51A); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD51B); spi_writedata(pdev,0x3F);

    spi_writecomm(pdev,0xD51C); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD51D); spi_writedata(pdev,0x7C);

    spi_writecomm(pdev,0xD51E); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD51F); spi_writedata(pdev,0xC4);

    spi_writecomm(pdev,0xD520); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD521); spi_writedata(pdev,0xF6);

    spi_writecomm(pdev,0xD522); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD523); spi_writedata(pdev,0x3A);

    spi_writecomm(pdev,0xD524); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD525); spi_writedata(pdev,0x68);

    spi_writecomm(pdev,0xD526); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD527); spi_writedata(pdev,0xA0);

    spi_writecomm(pdev,0xD528); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD529); spi_writedata(pdev,0xBF);

    spi_writecomm(pdev,0xD52A); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD52B); spi_writedata(pdev,0xE0);

    spi_writecomm(pdev,0xD52C); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD52D); spi_writedata(pdev,0xEC);

    spi_writecomm(pdev,0xD52E); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD52F); spi_writedata(pdev,0xF5);

    spi_writecomm(pdev,0xD530); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD531); spi_writedata(pdev,0xFA);

    spi_writecomm(pdev,0xD532); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD533); spi_writedata(pdev,0xFF);

    spi_writecomm(pdev,0xD600); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD601); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD602); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD603); spi_writedata(pdev,0x17);

    spi_writecomm(pdev,0xD604); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD605); spi_writedata(pdev,0x3E);

    spi_writecomm(pdev,0xD606); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD607); spi_writedata(pdev,0x5E);

    spi_writecomm(pdev,0xD608); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD609); spi_writedata(pdev,0x7B);

    spi_writecomm(pdev,0xD60A); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD60B); spi_writedata(pdev,0xA9);

    spi_writecomm(pdev,0xD60C); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xD60D); spi_writedata(pdev,0xCE);

    spi_writecomm(pdev,0xD60E); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD60F); spi_writedata(pdev,0x0A);

    spi_writecomm(pdev,0xD610); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD611); spi_writedata(pdev,0x37);

    spi_writecomm(pdev,0xD612); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD613); spi_writedata(pdev,0x7C);

    spi_writecomm(pdev,0xD614); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD615); spi_writedata(pdev,0xB0);

    spi_writecomm(pdev,0xD616); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xD617); spi_writedata(pdev,0xFF);

    spi_writecomm(pdev,0xD618); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD619); spi_writedata(pdev,0x3D);

    spi_writecomm(pdev,0xD61A); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD61B); spi_writedata(pdev,0x3F);

    spi_writecomm(pdev,0xD61C); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD61D); spi_writedata(pdev,0x7C);

    spi_writecomm(pdev,0xD61E); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD61F); spi_writedata(pdev,0xC4);

    spi_writecomm(pdev,0xD620); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xD621); spi_writedata(pdev,0xF6);

    spi_writecomm(pdev,0xD622); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD623); spi_writedata(pdev,0x3A);

    spi_writecomm(pdev,0xD624); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD625); spi_writedata(pdev,0x68);

    spi_writecomm(pdev,0xD626); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD627); spi_writedata(pdev,0xA0);

    spi_writecomm(pdev,0xD628); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD629); spi_writedata(pdev,0xBF);

    spi_writecomm(pdev,0xD62A); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD62B); spi_writedata(pdev,0xE0);

    spi_writecomm(pdev,0xD62C); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD62D); spi_writedata(pdev,0xEC);

    spi_writecomm(pdev,0xD62E); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD62F); spi_writedata(pdev,0xF5);

    spi_writecomm(pdev,0xD630); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD631); spi_writedata(pdev,0xFA);

    spi_writecomm(pdev,0xD632); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xD633); spi_writedata(pdev,0xFF);

    mdelay(20);

    spi_writecomm(pdev,0xF000); spi_writedata(pdev,0x55);

    spi_writecomm(pdev,0xF001); spi_writedata(pdev,0xAA);

    spi_writecomm(pdev,0xF002); spi_writedata(pdev,0x52);

    spi_writecomm(pdev,0xF003); spi_writedata(pdev,0x08);

    spi_writecomm(pdev,0xF004); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xB000); spi_writedata(pdev,0x05);

    spi_writecomm(pdev,0xB001); spi_writedata(pdev,0x17);

    spi_writecomm(pdev,0xB002); spi_writedata(pdev,0xF9);

    spi_writecomm(pdev,0xB003); spi_writedata(pdev,0x53);

    spi_writecomm(pdev,0xB004); spi_writedata(pdev,0x53);

    spi_writecomm(pdev,0xB005); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB006); spi_writedata(pdev,0x30);

    spi_writecomm(pdev,0xB100); spi_writedata(pdev,0x05);

    spi_writecomm(pdev,0xB101); spi_writedata(pdev,0x17);

    spi_writecomm(pdev,0xB102); spi_writedata(pdev,0xFB);

    spi_writecomm(pdev,0xB103); spi_writedata(pdev,0x55);

    spi_writecomm(pdev,0xB104); spi_writedata(pdev,0x53);

    spi_writecomm(pdev,0xB105); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB106); spi_writedata(pdev,0x30);

    spi_writecomm(pdev,0xB200); spi_writedata(pdev,0xFC);

    spi_writecomm(pdev,0xB201); spi_writedata(pdev,0xFD);

    spi_writecomm(pdev,0xB202); spi_writedata(pdev,0xFE);

    spi_writecomm(pdev,0xB203); spi_writedata(pdev,0xFF);

    spi_writecomm(pdev,0xB204); spi_writedata(pdev,0xF0);

    spi_writecomm(pdev,0xB205); spi_writedata(pdev,0xED);

    spi_writecomm(pdev,0xB206); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB207); spi_writedata(pdev,0xC4);

    spi_writecomm(pdev,0xB208); spi_writedata(pdev,0x08);

    spi_writecomm(pdev,0xB300); spi_writedata(pdev,0x5B);

    spi_writecomm(pdev,0xB301); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB302); spi_writedata(pdev,0xFC);

    spi_writecomm(pdev,0xB303); spi_writedata(pdev,0x5A);

    spi_writecomm(pdev,0xB304); spi_writedata(pdev,0x5A);

    spi_writecomm(pdev,0xB305); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xB400); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB401); spi_writedata(pdev,0x01);

    spi_writecomm(pdev,0xB402); spi_writedata(pdev,0x02);

    spi_writecomm(pdev,0xB403); spi_writedata(pdev,0x03);

    spi_writecomm(pdev,0xB404); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB405); spi_writedata(pdev,0x40);

    spi_writecomm(pdev,0xB406); spi_writedata(pdev,0x04);

    spi_writecomm(pdev,0xB407); spi_writedata(pdev,0x08);

    spi_writecomm(pdev,0xB408); spi_writedata(pdev,0xED);

    spi_writecomm(pdev,0xB409); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB40A); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB500); spi_writedata(pdev,0x40);

    spi_writecomm(pdev,0xB501); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB502); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB503); spi_writedata(pdev,0x80);

    spi_writecomm(pdev,0xB504); spi_writedata(pdev,0x5F);

    spi_writecomm(pdev,0xB505); spi_writedata(pdev,0x5E);

    spi_writecomm(pdev,0xB506); spi_writedata(pdev,0x50);

    spi_writecomm(pdev,0xB507); spi_writedata(pdev,0x50);

    spi_writecomm(pdev,0xB508); spi_writedata(pdev,0x33);

    spi_writecomm(pdev,0xB509); spi_writedata(pdev,0x33);

    spi_writecomm(pdev,0xB50A); spi_writedata(pdev,0x55);

    spi_writecomm(pdev,0xB600); spi_writedata(pdev,0xBC);

    spi_writecomm(pdev,0xB601); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB602); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB603); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB604); spi_writedata(pdev,0x2A);

    spi_writecomm(pdev,0xB605); spi_writedata(pdev,0x80);

    spi_writecomm(pdev,0xB606); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB700); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB701); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB702); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB703); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB704); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB705); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB706); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB707); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB800); spi_writedata(pdev,0x11);

    spi_writecomm(pdev,0xB801); spi_writedata(pdev,0x60);

    spi_writecomm(pdev,0xB802); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB900); spi_writedata(pdev,0x90);

    spi_writecomm(pdev,0xBA00); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBA01); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBA02); spi_writedata(pdev,0x08);

    spi_writecomm(pdev,0xBA03); spi_writedata(pdev,0xAC);

    spi_writecomm(pdev,0xBA04); spi_writedata(pdev,0xE2);

    spi_writecomm(pdev,0xBA05); spi_writedata(pdev,0x64);

    spi_writecomm(pdev,0xBA06); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBA07); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBA08); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBA09); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBA0A); spi_writedata(pdev,0x47);

    spi_writecomm(pdev,0xBA0B); spi_writedata(pdev,0x3F);

    spi_writecomm(pdev,0xBA0C); spi_writedata(pdev,0xDB);

    spi_writecomm(pdev,0xBA0D); spi_writedata(pdev,0x91);

    spi_writecomm(pdev,0xBA0E); spi_writedata(pdev,0x54);

    spi_writecomm(pdev,0xBA0F); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBB00); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBB01); spi_writedata(pdev,0x43);

    spi_writecomm(pdev,0xBB02); spi_writedata(pdev,0x79);

    spi_writecomm(pdev,0xBB03); spi_writedata(pdev,0xFD);

    spi_writecomm(pdev,0xBB04); spi_writedata(pdev,0xB5);

    spi_writecomm(pdev,0xBB05); spi_writedata(pdev,0x14);

    spi_writecomm(pdev,0xBB06); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBB07); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBB08); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBB09); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBB0A); spi_writedata(pdev,0x40);

    spi_writecomm(pdev,0xBB0B); spi_writedata(pdev,0x4A);

    spi_writecomm(pdev,0xBB0C); spi_writedata(pdev,0xCE);

    spi_writecomm(pdev,0xBB0D); spi_writedata(pdev,0x86);

    spi_writecomm(pdev,0xBB0E); spi_writedata(pdev,0x24);

    spi_writecomm(pdev,0xBB0F); spi_writedata(pdev,0x44);

    spi_writecomm(pdev,0xBC00); spi_writedata(pdev,0xE0);

    spi_writecomm(pdev,0xBC01); spi_writedata(pdev,0x1F);

    spi_writecomm(pdev,0xBC02); spi_writedata(pdev,0xF8);

    spi_writecomm(pdev,0xBC03); spi_writedata(pdev,0x07);

    spi_writecomm(pdev,0xBD00); spi_writedata(pdev,0xE0);

    spi_writecomm(pdev,0xBD01); spi_writedata(pdev,0x1F);

    spi_writecomm(pdev,0xBD02); spi_writedata(pdev,0xF8);

    spi_writecomm(pdev,0xBD03); spi_writedata(pdev,0x07);

    mdelay(20);

    spi_writecomm(pdev,0xF000); spi_writedata(pdev,0x55);

    spi_writecomm(pdev,0xF001); spi_writedata(pdev,0xAA);

    spi_writecomm(pdev,0xF002); spi_writedata(pdev,0x52);

    spi_writecomm(pdev,0xF003); spi_writedata(pdev,0x08);

    spi_writecomm(pdev,0xF004); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB000); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0xB001); spi_writedata(pdev,0x10);

    //spi_writecomm(pdev,0xB400); spi_writedata(pdev,0x10);

    spi_writecomm(pdev,0xB500); spi_writedata(pdev,0x6B);

    spi_writecomm(pdev,0xBC00); spi_writedata(pdev,0x00);


    spi_writecomm(pdev,0x3500); spi_writedata(pdev,0x00);

    spi_writecomm(pdev,0x3600); spi_writedata(pdev,0x0b);

    // spi_writecomm(pdev,0x2100);

    spi_writecomm(pdev,0x1100);

    mdelay(20);

    spi_writecomm(pdev,0x2900);

    //spi_writecomm(pdev,0x2300);spi_writedata_CS(); /* display all on white */

    while(0) {

        spi_writecomm(pdev,0x2300);/* display all on white */

        mdelay(500);

        spi_writecomm(pdev,0x2200);/* display all off black */

        mdelay(500);

        printk("==============\n");

    }

}


static int rm68172_probe(struct spi_device *pdev)
{
    int ret;
    struct rm68172_data *dev;
    dev = kzalloc(sizeof(struct rm68172_data), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->dev = &pdev->dev;
    dev->ctrl = pdev->dev.platform_data;
    if (dev->ctrl == NULL) {
        dev_info(&pdev->dev, "no platform data!\n");
        return -EINVAL;
    }

    dev_set_drvdata(&pdev->dev, dev);

    dev->lcd = lcd_device_register("rm68172_tft", &pdev->dev, dev, &rm68172_ops);
    if (IS_ERR(dev->lcd)) {
        ret = PTR_ERR(dev->lcd);
        dev->lcd = NULL;
        dev_info(&pdev->dev, "lcd device(rm68172) register error: %d\n", ret);
    } else {
        dev_info(&pdev->dev, "lcd device(rm68172) register success\n");
    }
    dev->ctrl->power_on(dev->lcd,1);
    dev->ctrl->reset(dev->lcd);
    /* default the bachlight poweroff */
    rm68172_spiinit(pdev);
    dev->lcd_power = FB_BLANK_POWERDOWN;

    return 0;
}

static int rm68172_remove(struct spi_device *pdev)
{
    struct rm68172_data *dev = dev_get_drvdata(&pdev->dev);

    if (dev->lcd_power)
        dev->ctrl->power_on(dev->lcd, 0);

    lcd_device_unregister(dev->lcd);
    dev_set_drvdata(&pdev->dev, NULL);
    kfree(dev);

    return 0;
}

#ifdef CONFIG_PM

static int rm68172_suspend(struct spi_device *pdev, pm_message_t state)
{
    return 0;
}

static int rm68172_resume(struct spi_device *pdev)
{
    return 0;
}

#else

#define rm68172_suspend                 NULL
#define rm68172_resume                  NULL

#endif

static struct spi_driver rm68172_driver = {
    .driver     = {
        .name   = "rm68172_tft",
        .bus        = &spi_bus_type,
        .owner  = THIS_MODULE,
    },
    .id_table	= jz_id_table,
    .probe              = rm68172_probe,
    .remove             = rm68172_remove,
    .suspend            = rm68172_suspend,
    .resume             = rm68172_resume,
};

static int __init rm68172_init(void)
{
    return spi_register_driver(&rm68172_driver);
}


static void __exit rm68172_exit(void)
{
    spi_unregister_driver(&rm68172_driver);
}

rootfs_initcall(rm68172_init);
module_exit(rm68172_exit);

MODULE_DESCRIPTION("rm68172 lcd panel driver");
MODULE_LICENSE("GPL");
