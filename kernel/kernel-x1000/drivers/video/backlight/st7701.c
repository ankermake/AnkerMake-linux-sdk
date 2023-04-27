/*
 * kernel/drivers/video/panel/jz_st7701s.c -- Ingenic LCD panel device
 *
 * Copyright (C) 2005-2010, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/fb.h>
#include <linux/regulator/consumer.h>
#include <soc/gpio.h>
#include <linux/platform_device.h>

struct st7701s_data {
    struct device *dev;
    struct lcd_device *lcd;
    struct lcd_platform_data *ctrl;
    struct regulator *lcd_vcc_reg;
    struct backlight_device *bd;
    int lcd_power;
};
struct spi_device_id jz_id_table[] = {
    {
        .name = "st7701s_tft",
        .driver_data = 1,
    },
};

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



static void st7701s_spiinit(struct spi_device *pdev)
{
    //--------------------------------------ST7701 Reset Sequence---------------------------------------//
    spi_writecomm (pdev, 0x1100);
    mdelay (120); //Delay 120ms
    //---------------------------------------Bank0 Setting-------------------------------------------------//
    //------------------------------------Display Control setting----------------------------------------------//
    spi_writecomm (pdev,0xFF00); spi_writedata (pdev,0x77);
    spi_writecomm (pdev,0xFF01); spi_writedata (pdev,0x01);
    spi_writecomm (pdev,0xFF02); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xFF03); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xFF04); spi_writedata (pdev,0x10);
    spi_writecomm (pdev,0xC000); spi_writedata (pdev,0xe9);
    spi_writecomm (pdev,0xC001); spi_writedata (pdev,0x03);
    spi_writecomm (pdev,0xC100); spi_writedata (pdev,0x12);
    spi_writecomm (pdev,0xC101); spi_writedata (pdev,0x02);
    spi_writecomm (pdev,0xC200); spi_writedata (pdev,0x37);
    spi_writecomm (pdev,0xC201); spi_writedata (pdev,0x08);
    spi_writecomm (pdev,0xC300); spi_writedata (pdev,0x02);
    spi_writecomm (pdev,0xC301); spi_writecomm (pdev,0xCC);
    spi_writecomm (pdev,0xC302); spi_writedata (pdev,0x10);

    spi_writecomm (pdev,0xc700); spi_writedata (pdev,0x04);
    //----------------------------------Gamma Cluster Setting----------------------------------------//
    spi_writecomm (pdev,0xB000); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xB001); spi_writedata (pdev,0x0E);
    spi_writecomm (pdev,0xB002); spi_writedata (pdev,0x15);
    spi_writecomm (pdev,0xB003); spi_writedata (pdev,0x0F);
    spi_writecomm (pdev,0xB004); spi_writedata (pdev,0x11);
    spi_writecomm (pdev,0xB005); spi_writedata (pdev,0x08);
    spi_writecomm (pdev,0xB006); spi_writedata (pdev,0x08);
    spi_writecomm (pdev,0xB007); spi_writedata (pdev,0x08);
    spi_writecomm (pdev,0xB008); spi_writedata (pdev,0x08);
    spi_writecomm (pdev,0xB009); spi_writedata (pdev,0x23);
    spi_writecomm (pdev,0xB00A); spi_writedata (pdev,0x04);
    spi_writecomm (pdev,0xB00B); spi_writedata (pdev,0x13);
    spi_writecomm (pdev,0xB00C); spi_writedata (pdev,0x12);
    spi_writecomm (pdev,0xB00D); spi_writedata (pdev,0x2B);
    spi_writecomm (pdev,0xB00E); spi_writedata (pdev,0x34);
    spi_writecomm (pdev,0xB00F); spi_writedata (pdev,0x1F);
    spi_writecomm (pdev,0xB100); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xB101); spi_writedata (pdev,0x0E);
    spi_writecomm (pdev,0xB102); spi_writedata (pdev,0x95);
    spi_writecomm (pdev,0xB103); spi_writedata (pdev,0x0F);
    spi_writecomm (pdev,0xB104); spi_writedata (pdev,0x13);
    spi_writecomm (pdev,0xB105); spi_writedata (pdev,0x07);
    spi_writecomm (pdev,0xB106); spi_writedata (pdev,0x09);
    spi_writecomm (pdev,0xB107); spi_writedata (pdev,0x08);
    spi_writecomm (pdev,0xB108); spi_writedata (pdev,0x08);
    spi_writecomm (pdev,0xB109); spi_writedata (pdev,0x22);
    spi_writecomm (pdev,0xB10A); spi_writedata (pdev,0x04);
    spi_writecomm (pdev,0xB10B); spi_writedata (pdev,0x10);
    spi_writecomm (pdev,0xB10C); spi_writedata (pdev,0x0E);
    spi_writecomm (pdev,0xB10D); spi_writedata (pdev,0x2C);
    spi_writecomm (pdev,0xB10E); spi_writedata (pdev,0x34);
    spi_writecomm (pdev,0xB10F); spi_writedata (pdev,0x1F);
    //------------------------------------End Gamma Setting-------------------------------------------//
    //---------------------------------End Display Control setting-------------------------------------//
    //--------------------------------------Bank0 Setting End------------------------------------------//
    //----------------------------------------Bank1 Setting------------------------------------------------//
    //----------------------------- Power Control Registers Initial -----------------------------------//
    spi_writecomm (pdev,0xFF00); spi_writedata (pdev,0x77);
    spi_writecomm (pdev,0xFF01); spi_writedata (pdev,0x01);
    spi_writecomm (pdev,0xFF02); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xFF03); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xFF04); spi_writedata (pdev,0x11);
    spi_writecomm (pdev,0xB000); spi_writedata (pdev,0x4D);
    //----------------------------------------Vcom Setting------------------------------------------------//
    spi_writecomm (pdev,0xB100); spi_writedata (pdev,0x13);
    //--------------------------------------End Vcom Setting--------------------------------------------//
    spi_writecomm (pdev,0xB200); spi_writedata (pdev,0x07);
    spi_writecomm (pdev,0xB300); spi_writedata (pdev,0x80);
    spi_writecomm (pdev,0xB500); spi_writedata (pdev,0x47);
    spi_writecomm (pdev,0xB700); spi_writedata (pdev,0x85);
    spi_writecomm (pdev,0xB800); spi_writedata (pdev,0x20);
    spi_writecomm (pdev,0xB900); spi_writedata (pdev,0x10);
    spi_writecomm (pdev,0xC100); spi_writedata (pdev,0x78);
    spi_writecomm (pdev,0xC200); spi_writedata (pdev,0x78);
    spi_writecomm (pdev,0xD000); spi_writedata (pdev,0x88);
    //------------------------------End Power Control Registers Initial ----------------------------//
    mdelay (100);
    //------------------------------------------GIP Setting-------------------------------------------------//
    spi_writecomm (pdev,0xE000); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE001); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE002); spi_writedata (pdev,0x02);
    spi_writecomm (pdev,0xE100); spi_writedata (pdev,0x0B);
    spi_writecomm (pdev,0xE101); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE102); spi_writedata (pdev,0x0D);
    spi_writecomm (pdev,0xE103); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE104); spi_writedata (pdev,0x0C);
    spi_writecomm (pdev,0xE105); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE106); spi_writedata (pdev,0x0E);
    spi_writecomm (pdev,0xE107); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE108); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE109); spi_writedata (pdev,0x44);
    spi_writecomm (pdev,0xE10A); spi_writedata (pdev,0x44);
    spi_writecomm (pdev,0xE200); spi_writedata (pdev,0x33);
    spi_writecomm (pdev,0xE201); spi_writedata (pdev,0x33);
    spi_writecomm (pdev,0xE202); spi_writedata (pdev,0x44);
    spi_writecomm (pdev,0xE203); spi_writedata (pdev,0x44);
    spi_writecomm (pdev,0xE204); spi_writedata (pdev,0x64);
    spi_writecomm (pdev,0xE205); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE206); spi_writedata (pdev,0x66);
    spi_writecomm (pdev,0xE207); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE208); spi_writedata (pdev,0x65);
    spi_writecomm (pdev,0xE209); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE20A); spi_writedata (pdev,0x67);
    spi_writecomm (pdev,0xE20B); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE20C); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE300); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE301); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE302); spi_writedata (pdev,0x33);
    spi_writecomm (pdev,0xE303); spi_writedata (pdev,0x33);
    spi_writecomm (pdev,0xE400); spi_writedata (pdev,0x44);
    spi_writecomm (pdev,0xE401); spi_writedata (pdev,0x44);
    spi_writecomm (pdev,0xE500); spi_writedata (pdev,0x0C);
    spi_writecomm (pdev,0xE501); spi_writedata (pdev,0x78);
    spi_writecomm (pdev,0xE502); spi_writedata (pdev,0x3C);
    spi_writecomm (pdev,0xE503); spi_writedata (pdev,0xA0);
    spi_writecomm (pdev,0xE504); spi_writedata (pdev,0x0E);
    spi_writecomm (pdev,0xE505); spi_writedata (pdev,0x78);
    spi_writecomm (pdev,0xE506); spi_writedata (pdev,0x3C);
    spi_writecomm (pdev,0xE507); spi_writedata (pdev,0xA0);
    spi_writecomm (pdev,0xE508); spi_writedata (pdev,0x10);
    spi_writecomm (pdev,0xE509); spi_writedata (pdev,0x78);
    spi_writecomm (pdev,0xE50A); spi_writedata (pdev,0x3C);
    spi_writecomm (pdev,0xE50B); spi_writedata (pdev,0xA0);
    spi_writecomm (pdev,0xE50C); spi_writedata (pdev,0x12);
    spi_writecomm (pdev,0xE50D); spi_writedata (pdev,0x78);
    spi_writecomm (pdev,0xE50E); spi_writedata (pdev,0x3C);
    spi_writecomm (pdev,0xE50F); spi_writedata (pdev,0xA0);
    spi_writecomm (pdev,0xE600); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE601); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xE602); spi_writedata (pdev,0x33);
    spi_writecomm (pdev,0xE603); spi_writedata (pdev,0x33);
    spi_writecomm (pdev,0xE700); spi_writedata (pdev,0x44);
    spi_writecomm (pdev,0xE701); spi_writedata (pdev,0x44);
    spi_writecomm (pdev,0xE800); spi_writedata (pdev,0x0d);
    spi_writecomm (pdev,0xE801); spi_writedata (pdev,0x78);
    spi_writecomm (pdev,0xE802); spi_writedata (pdev,0x3C);
    spi_writecomm (pdev,0xE803); spi_writedata (pdev,0xA0);
    spi_writecomm (pdev,0xE804); spi_writedata (pdev,0x0F);
    spi_writecomm (pdev,0xE805); spi_writedata (pdev,0x78);
    spi_writecomm (pdev,0xE806); spi_writedata (pdev,0x3C);
    spi_writecomm (pdev,0xE807); spi_writedata (pdev,0xA0);
    spi_writecomm (pdev,0xE808); spi_writedata (pdev,0x11);
    spi_writecomm (pdev,0xE809); spi_writedata (pdev,0x78);
    spi_writecomm (pdev,0xE80A); spi_writedata (pdev,0x3C);
    spi_writecomm (pdev,0xE80B); spi_writedata (pdev,0xA0);
    spi_writecomm (pdev,0xE80C); spi_writedata (pdev,0x13);
    spi_writecomm (pdev,0xE80D); spi_writedata (pdev,0x78);
    spi_writecomm (pdev,0xE80E); spi_writedata (pdev,0x3C);
    spi_writecomm (pdev,0xE80F); spi_writedata (pdev,0xA0);
    spi_writecomm (pdev,0xEB00); spi_writedata (pdev,0x02);
    spi_writecomm (pdev,0xEB01); spi_writedata (pdev,0x02);
    spi_writecomm (pdev,0xEB02); spi_writedata (pdev,0x39);
    spi_writecomm (pdev,0xEB03); spi_writedata (pdev,0x39);
    spi_writecomm (pdev,0xEB04); spi_writedata (pdev,0xEE);
    spi_writecomm (pdev,0xEB05); spi_writedata (pdev,0x44);
    spi_writecomm (pdev,0xEB06); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xEC00); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xEC01); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xED00); spi_writedata (pdev,0xFF);
    spi_writecomm (pdev,0xED01); spi_writedata (pdev,0xF1);
    spi_writecomm (pdev,0xED02); spi_writedata (pdev,0x04);
    spi_writecomm (pdev,0xED03); spi_writedata (pdev,0x56);
    spi_writecomm (pdev,0xED04); spi_writedata (pdev,0x72);
    spi_writecomm (pdev,0xED05); spi_writedata (pdev,0x3F);
    spi_writecomm (pdev,0xED06); spi_writedata (pdev,0xFF);
    spi_writecomm (pdev,0xED07); spi_writedata (pdev,0xFF);
    spi_writecomm (pdev,0xED08); spi_writedata (pdev,0xFF);
    spi_writecomm (pdev,0xED09); spi_writedata (pdev,0xFF);
    spi_writecomm (pdev,0xED0A); spi_writedata (pdev,0xF3);
    spi_writecomm (pdev,0xED0B); spi_writedata (pdev,0x27);
    spi_writecomm (pdev,0xED0C); spi_writedata (pdev,0x65);
    spi_writecomm (pdev,0xED0D); spi_writedata (pdev,0x40);
    spi_writecomm (pdev,0xED0E); spi_writedata (pdev,0x1F);
    spi_writecomm (pdev,0xED0F); spi_writedata (pdev,0xFF);
    //------------------------------------------End GIP Setting--------------------------------------------//
    //--------------------------- Power Control Registers Initial End--------------------------------//
    //---------------------------------------Bank1 Setting-------------------------------------------------

    //---------------------------------------Bank1 Setting-------------------------------------------------//
    spi_writecomm (pdev,0xFF00); spi_writedata (pdev,0x77);
    spi_writecomm (pdev,0xFF01); spi_writedata(pdev,0x01);
    spi_writecomm (pdev,0xFF02); spi_writedata(pdev,0x00);
    spi_writecomm (pdev,0xFF03); spi_writedata(pdev,0x00);
    spi_writecomm (pdev,0xFF04); spi_writedata(pdev,0x00);
#if 0
    /**************显示st7701内置测试图像********************** */
    spi_writecomm (pdev,0xFF00); spi_writedata (pdev,0x77);
    spi_writecomm (pdev,0xFF01); spi_writedata (pdev,0x01);
    spi_writecomm (pdev,0xFF02); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xFF03); spi_writedata (pdev,0x00);
    spi_writecomm (pdev,0xFF04); spi_writedata (pdev,0x12);
    spi_writecomm (pdev,0xD100); spi_writedata (pdev,0x81);
    spi_writecomm (pdev,0xD101); spi_writedata (pdev,0x20);
    spi_writecomm (pdev,0xD102); spi_writedata (pdev,0x03);
    spi_writecomm (pdev,0xD103); spi_writedata (pdev,0x56);
    spi_writecomm (pdev,0xD104); spi_writedata (pdev,0x08);
    spi_writecomm (pdev,0xD105); spi_writedata (pdev,0x01);
    spi_writecomm (pdev,0xD106); spi_writedata (pdev,0xC0);
    spi_writecomm (pdev,0xD107); spi_writedata (pdev,0x01);
    spi_writecomm (pdev,0xD108); spi_writedata (pdev,0xE0);
    spi_writecomm (pdev,0xD109); spi_writedata (pdev,0xC0);
    spi_writecomm (pdev,0xD10A); spi_writedata (pdev,0x01);
    spi_writecomm (pdev,0xD10B); spi_writedata (pdev,0xE0);
    spi_writecomm (pdev,0xD10C); spi_writedata (pdev,0x03);
    spi_writecomm (pdev,0xD10D); spi_writedata (pdev,0x56);
    spi_writecomm (pdev,0xD200); spi_writedata (pdev,0x08);///灰阶
    /************************************************** */
#endif
    spi_writecomm (pdev,0x3600); spi_writedata (pdev,0x10);
    spi_writecomm (pdev,0x2900);
    //spi_writecomm (pdev,0x3A00); spi_writedata (pdev,0x55);
}


static int st7701s_set_power(struct lcd_device *lcd, int power)
{
    return 0;
}

static int st7701s_get_power(struct lcd_device *lcd)
{
    return 0;
}

static int st7701s_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
    return 0;
}

static struct lcd_ops st7701s_ops = {
    .set_power = st7701s_set_power,
    .get_power = st7701s_get_power,
    .set_mode = st7701s_set_mode,
};

static int st7701s_probe(struct spi_device *pdev)
{
    int ret;
    struct st7701s_data *dev;
    dev = kzalloc(sizeof(struct st7701s_data), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->dev = &pdev->dev;
    dev->ctrl = pdev->dev.platform_data;
    if (dev->ctrl == NULL) {
        dev_info(&pdev->dev, "no platform data!\n");
        return -EINVAL;
    }

    dev_set_drvdata(&pdev->dev, dev);

    dev->lcd = lcd_device_register("st7701s_tft", &pdev->dev, dev, &st7701s_ops);
    if (IS_ERR(dev->lcd)) {
        ret = PTR_ERR(dev->lcd);
        dev->lcd = NULL;
        dev_info(&pdev->dev, "lcd device(st7701s) register error: %d\n", ret);
    } else {
        dev_info(&pdev->dev, "lcd device(st7701s) register success\n");
    }
    dev->ctrl->power_on(dev->lcd,1);

    dev->ctrl->reset(dev->lcd);

    st7701s_spiinit(pdev);

    dev->lcd_power = FB_BLANK_POWERDOWN;

    return 0;
}

static int st7701s_remove(struct spi_device *pdev)
{
    struct st7701s_data *dev = dev_get_drvdata(&pdev->dev);

    if (dev->lcd_power)
        dev->ctrl->power_on(dev->lcd, 0);

    lcd_device_unregister(dev->lcd);
    dev_set_drvdata(&pdev->dev, NULL);
    kfree(dev);

    return 0;
}


#ifdef CONFIG_PM

static int st7701s_suspend(struct spi_device *pdev, pm_message_t state)
{
    return 0;
}

static int st7701s_resume(struct spi_device *pdev)
{
    return 0;
}

#else

#define st7701s_suspend                 NULL
#define st7701s_resume                  NULL

#endif

static struct spi_driver st7701s_driver = {
    .driver     = {
        .name   = "st7701s_tft",
        .bus        = &spi_bus_type,
        .owner  = THIS_MODULE,
    },
    .id_table    = jz_id_table,
    .probe              = st7701s_probe,
    .remove             = st7701s_remove,
    .suspend            = st7701s_suspend,
    .resume             = st7701s_resume,
};

static int __init st7701s_init(void)
{
    return spi_register_driver(&st7701s_driver);
}


static void __exit st7701s_exit(void)
{
    spi_unregister_driver(&st7701s_driver);
}

rootfs_initcall(st7701s_init);
module_exit(st7701s_exit);

MODULE_DESCRIPTION("st7701s lcd panel driver");
MODULE_LICENSE("GPL");