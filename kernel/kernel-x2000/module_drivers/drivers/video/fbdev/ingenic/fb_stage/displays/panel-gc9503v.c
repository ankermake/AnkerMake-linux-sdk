/*
 * kernel-4.4.94/module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/panel-gc9503v.c
 *
 * Copyright (C) 2016 Ingenic Semiconductor Inc.
 *
 * This program is free software, you can redistribute it and/or modify it
 *
 * under the terms of the GNU General Public License version 2 as published by
 *
 * the Free Software Foundation.
 */
#if 0
&dpu {
	status = "okay";
	ingenic,disable-rdma-fb = <1>;
	/*Defines the init state of composer fb export infomations.*/
	ingenic,layer-exported = <1 1 0 0>;
	ingenic,layer-frames   = <2 2 2 2>;
	ingenic,layer-framesize = <480 480>, <720 1280>, <320 240>, <320 240>;   /*Max framesize for each layer.*/
	layer,color_mode	= <0 0 0 0>;					/*src fmt,*/
	layer,src-size       	= <480 480>, <720 1280>, <320 240>, <240 200>;	/*Layer src size should smaller than framesize*/
	layer,target-size	= <480 480>, <720 640>, <160 240>, <240 200>;	/*Target Size should smaller than src_size.*/
	layer,target-pos	= <0 0>, <0 640>, <340 480>, <100 980>;	/*target pos , the start point of the target panel.*/
	layer,enable		= <1 0 0 0>;					/*layer enabled or disabled.*/
	ingenic,logo-pan-layer	= <0>;						/*on which layer should init logo pan on.*/
	port {
		dpu_out_ep: endpoint {
			remote-endpoint = <&panel_gc9503v_ep>;
	    };
	};
};

	display-dbi {
		compatible = "simple-bus";
		#interrupt-cells = <1>;
		#address-cells = <1>;
		#size-cells = <1>;
		ranges = <>;
		panel_gc9503v {
			compatible = "ingenic,gc9503v_spirgb";
			status = "okay";
			pinctrl-names = "default";
			pinctrl-0 = <&tft_lcd_pb_rgb666>;
			ingenic,vdd-en-gpio = <&gpc 3 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;
			ingenic,rst-gpio = <&gpc 4 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
			ingenic,lcd-sdo-gpio = <&gpb 30 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
			ingenic,lcd-sck-gpio = <&gpb 31 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
			ingenic,lcd-cs-gpio = <&gpb 28 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
			port {
				panel_gc9503v_ep: endpoint {
					   remote-endpoint = <&dpu_out_ep>;
			   };
			};
		};
	};
backlight {
		compatible = "pwm-backlight";
		pwms = <&pwm 1 1000000>; /* arg1: pwm channel id [0~15]. arg2: period in ns. */
		brightness-levels = <0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15>;
		default-brightness-level = <5>;
	};
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/of_gpio.h>

#include "../ingenicfb.h"

struct board_gpio {
	short gpio;
	short active_level;
};

struct gpio_spi {
	short sdo;
	short sdi;
	short sck;
	short cs;
};

struct panel_dev {
	/* ingenic frame buffer */
	struct device *dev;
	struct lcd_panel *panel;

	/* common lcd framework */
	struct lcd_device *lcd;
	struct backlight_device *backlight;
	int power;

	struct regulator *vcc;
	struct board_gpio vdd_en;
	struct board_gpio rst;
	struct board_gpio pwm;
	struct gpio_spi spi;
};

static struct panel_dev *panel;

#define RESET(n)\
     gpio_direction_output(panel->rst.gpio, n)

#define CS(n)\
     gpio_direction_output(panel->spi.cs, n)

#define SCK(n)\
     gpio_direction_output(panel->spi.sck, n)

#define SDO(n)\
	 gpio_direction_output(panel->spi.sdo, n)

#if 0
#define SDI()\
	gpio_get_value(panel->spi.sdi)
#endif

void SPI_SendData(unsigned char i)
{
   unsigned char n;

   for(n=0; n<8; n++) {
	   if(i&0x80)
		   SDO(1);
	   else
		   SDO(0);
	   i = i << 1;

	   SCK(0);
	   udelay(10);
	   SCK(1);
	   udelay(10);
   }
}

void SPI_WriteComm(unsigned char c)
{
	CS(0);

	SDO(0);

	SCK(0);
	udelay(10);
	SCK(1);
	udelay(10);

	SPI_SendData(c);

	CS(1);
}

void SPI_WriteData(unsigned char d)
{
	CS(0);

	SDO(1);

	SCK(0);
	udelay(10);
	SCK(1);
	udelay(10);

	SPI_SendData(d);

	CS(1);
}

void Initial_IC(void)
{
	RESET(1);
	udelay(100000);
	RESET(0);
	udelay(120000); // Delay 120ms // This delay time is necessary
	RESET(1);
	udelay(120000); // Delay 120ms // This delay time is necessary
	SPI_WriteComm(0x11);//Attention !!! sleep out config or color display error.
        udelay(120000); // Delay 120ms // This delay time is necessary */
	//PAGE1
        SPI_WriteComm(0xF0);
        SPI_WriteData(0x55);
        SPI_WriteData(0xAA);
        SPI_WriteData(0x52);
        SPI_WriteData(0x08);
        SPI_WriteData(0x00);

        SPI_WriteComm(0xF6);
        SPI_WriteData(0x5A);
        SPI_WriteData(0x87);

        SPI_WriteComm(0x3A);
        SPI_WriteData(0x60);
////test de mode,sync mode.
//	SPI_WriteComm(0xB0);//D7,0:de mode[def],1:sync mode
//        SPI_WriteData(0x80);
////test de mode,sync mode.

        SPI_WriteComm(0xC1);
        SPI_WriteData(0x3F);

        SPI_WriteComm(0xC2);
        SPI_WriteData(0x0E);

        SPI_WriteComm(0xC6);
        SPI_WriteData(0xF8);

        SPI_WriteComm(0xC9);
        SPI_WriteData(0x10);

        SPI_WriteComm(0xCD);
        SPI_WriteData(0x25);

        SPI_WriteComm(0xF8);
        SPI_WriteData(0x8A);

        SPI_WriteComm(0xAC);
        SPI_WriteData(0x65);

        SPI_WriteComm(0xA0);
        SPI_WriteData(0xCC);//

        SPI_WriteComm(0xA7);
        SPI_WriteData(0x47);

        SPI_WriteComm(0xFA);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x04);

        SPI_WriteComm(0x86);
        SPI_WriteData(0x99);
        SPI_WriteData(0xa3);
        SPI_WriteData(0xa3);
        SPI_WriteData(0x51);//51

        SPI_WriteComm(0xA3);
        SPI_WriteData(0x22);//22

        SPI_WriteComm(0xFD);
        SPI_WriteData(0x38);//38
        SPI_WriteData(0x38);//38
        SPI_WriteData(0x00);

        SPI_WriteComm(0x71);
        SPI_WriteData(0x48);

        SPI_WriteComm(0x72);
        SPI_WriteData(0x48);

        SPI_WriteComm(0x73);
        SPI_WriteData(0x00);
        SPI_WriteData(0x44);

        SPI_WriteComm(0x97);
        SPI_WriteData(0xEE);

        SPI_WriteComm(0x83);
        SPI_WriteData(0x93);

        SPI_WriteComm(0x9A);
        SPI_WriteData(0x72);

        SPI_WriteComm(0x9B);
        SPI_WriteData(0x5A);

        SPI_WriteComm(0x82);
        SPI_WriteData(0x2C);
        SPI_WriteData(0x2C);

        SPI_WriteComm(0xB1);
        SPI_WriteData(0x10);

        SPI_WriteComm(0x6D);
        SPI_WriteData(0x00);
        SPI_WriteData(0x1F);
        SPI_WriteData(0x19);
        SPI_WriteData(0x1A);
        SPI_WriteData(0x10);
        SPI_WriteData(0x0e);
        SPI_WriteData(0x0c);
        SPI_WriteData(0x0a);
        SPI_WriteData(0x02);
        SPI_WriteData(0x07);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x1E);
        SPI_WriteData(0x08);
        SPI_WriteData(0x01);
        SPI_WriteData(0x09);
        SPI_WriteData(0x0b);
        SPI_WriteData(0x0D);
        SPI_WriteData(0x0F);
        SPI_WriteData(0x1a);
        SPI_WriteData(0x19);
        SPI_WriteData(0x1f);
        SPI_WriteData(0x00);

        SPI_WriteComm(0x64);
        SPI_WriteData(0x38);
        SPI_WriteData(0x05);
        SPI_WriteData(0x01);
        SPI_WriteData(0xdb);
        SPI_WriteData(0x03);
        SPI_WriteData(0x03);
        SPI_WriteData(0x38);
        SPI_WriteData(0x04);
        SPI_WriteData(0x01);
        SPI_WriteData(0xdc);
        SPI_WriteData(0x03);
        SPI_WriteData(0x03);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);

        SPI_WriteComm(0x65);
        SPI_WriteData(0x38);
        SPI_WriteData(0x03);
        SPI_WriteData(0x01);
        SPI_WriteData(0xdd);
        SPI_WriteData(0x03);
        SPI_WriteData(0x03);
        SPI_WriteData(0x38);
        SPI_WriteData(0x02);
        SPI_WriteData(0x01);
        SPI_WriteData(0xde);
        SPI_WriteData(0x03);
        SPI_WriteData(0x03);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);

        SPI_WriteComm(0x66);
        SPI_WriteData(0x38);
        SPI_WriteData(0x01);
        SPI_WriteData(0x01);
        SPI_WriteData(0xdf);
        SPI_WriteData(0x03);
        SPI_WriteData(0x03);
        SPI_WriteData(0x38);
        SPI_WriteData(0x00);
        SPI_WriteData(0x01);
        SPI_WriteData(0xe0);
        SPI_WriteData(0x03);
        SPI_WriteData(0x03);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);

        SPI_WriteComm(0x67);
        SPI_WriteData(0x30);
        SPI_WriteData(0x01);
        SPI_WriteData(0x01);
        SPI_WriteData(0xe1);
        SPI_WriteData(0x03);
        SPI_WriteData(0x03);
        SPI_WriteData(0x30);
        SPI_WriteData(0x02);
        SPI_WriteData(0x01);
        SPI_WriteData(0xe2);
        SPI_WriteData(0x03);
        SPI_WriteData(0x03);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);

        SPI_WriteComm(0x68);
        SPI_WriteData(0x00);
        SPI_WriteData(0x08);
        SPI_WriteData(0x15);
        SPI_WriteData(0x08);
        SPI_WriteData(0x15);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x08);
        SPI_WriteData(0x15);
        SPI_WriteData(0x08);
        SPI_WriteData(0x15);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);

        SPI_WriteComm(0x60);
        SPI_WriteData(0x38);
        SPI_WriteData(0x08);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x38);
        SPI_WriteData(0x09);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);

        SPI_WriteComm(0x63);
        SPI_WriteData(0x31);
        SPI_WriteData(0xe4);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x31);
        SPI_WriteData(0xe5);
        SPI_WriteData(0x7A);
        SPI_WriteData(0x7A);

        SPI_WriteComm(0x69);
        SPI_WriteData(0x14);
        SPI_WriteData(0x22);
        SPI_WriteData(0x14);
        SPI_WriteData(0x22);
        SPI_WriteData(0x14);
        SPI_WriteData(0x22);
        SPI_WriteData(0x08);

        SPI_WriteComm(0x6B);
        SPI_WriteData(0x07);

        SPI_WriteComm(0x7A);
        SPI_WriteData(0x08);
        SPI_WriteData(0x13);

        SPI_WriteComm(0x7B);
        SPI_WriteData(0x08);
        SPI_WriteData(0x13);

        SPI_WriteComm(0xD1);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x04);
        SPI_WriteData(0x00);
        SPI_WriteData(0x12);
        SPI_WriteData(0x00);
        SPI_WriteData(0x18);
        SPI_WriteData(0x00);
        SPI_WriteData(0x21);
        SPI_WriteData(0x00);
        SPI_WriteData(0x2a);
        SPI_WriteData(0x00);
        SPI_WriteData(0x35);
        SPI_WriteData(0x00);
        SPI_WriteData(0x47);
        SPI_WriteData(0x00);
        SPI_WriteData(0x56);
        SPI_WriteData(0x00);
        SPI_WriteData(0x90);
        SPI_WriteData(0x00);
        SPI_WriteData(0xe5);
        SPI_WriteData(0x01);
        SPI_WriteData(0x68);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd5);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd7);
        SPI_WriteData(0x02);
        SPI_WriteData(0x36);
        SPI_WriteData(0x02);
        SPI_WriteData(0xa6);
        SPI_WriteData(0x02);
        SPI_WriteData(0xee);
        SPI_WriteData(0x03);
        SPI_WriteData(0x48);
        SPI_WriteData(0x03);
        SPI_WriteData(0xa0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xba);
        SPI_WriteData(0x03);
        SPI_WriteData(0xc5);
        SPI_WriteData(0x03);
        SPI_WriteData(0xd0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xE0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xea);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFa);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFF);

        SPI_WriteComm(0xD2);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x04);
        SPI_WriteData(0x00);
        SPI_WriteData(0x12);
        SPI_WriteData(0x00);
        SPI_WriteData(0x18);
        SPI_WriteData(0x00);
        SPI_WriteData(0x21);
        SPI_WriteData(0x00);
        SPI_WriteData(0x2a);
        SPI_WriteData(0x00);
        SPI_WriteData(0x35);
        SPI_WriteData(0x00);
        SPI_WriteData(0x47);
        SPI_WriteData(0x00);
        SPI_WriteData(0x56);
        SPI_WriteData(0x00);
        SPI_WriteData(0x90);
        SPI_WriteData(0x00);
        SPI_WriteData(0xe5);
        SPI_WriteData(0x01);
        SPI_WriteData(0x68);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd5);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd7);
        SPI_WriteData(0x02);
        SPI_WriteData(0x36);
        SPI_WriteData(0x02);
        SPI_WriteData(0xa6);
        SPI_WriteData(0x02);
        SPI_WriteData(0xee);
        SPI_WriteData(0x03);
        SPI_WriteData(0x48);
        SPI_WriteData(0x03);
        SPI_WriteData(0xa0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xba);
        SPI_WriteData(0x03);
        SPI_WriteData(0xc5);
        SPI_WriteData(0x03);
        SPI_WriteData(0xd0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xE0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xea);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFa);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFF);

        SPI_WriteComm(0xD3);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x04);
        SPI_WriteData(0x00);
        SPI_WriteData(0x12);
        SPI_WriteData(0x00);
        SPI_WriteData(0x18);
        SPI_WriteData(0x00);
        SPI_WriteData(0x21);
        SPI_WriteData(0x00);
        SPI_WriteData(0x2a);
        SPI_WriteData(0x00);
        SPI_WriteData(0x35);
        SPI_WriteData(0x00);
        SPI_WriteData(0x47);
        SPI_WriteData(0x00);
        SPI_WriteData(0x56);
        SPI_WriteData(0x00);
        SPI_WriteData(0x90);
        SPI_WriteData(0x00);
        SPI_WriteData(0xe5);
        SPI_WriteData(0x01);
        SPI_WriteData(0x68);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd5);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd7);
        SPI_WriteData(0x02);
        SPI_WriteData(0x36);
        SPI_WriteData(0x02);
        SPI_WriteData(0xa6);
        SPI_WriteData(0x02);
        SPI_WriteData(0xee);
        SPI_WriteData(0x03);
        SPI_WriteData(0x48);
        SPI_WriteData(0x03);
        SPI_WriteData(0xa0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xba);
        SPI_WriteData(0x03);
        SPI_WriteData(0xc5);
        SPI_WriteData(0x03);
        SPI_WriteData(0xd0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xE0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xea);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFa);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFF);

        SPI_WriteComm(0xD4);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x04);
        SPI_WriteData(0x00);
        SPI_WriteData(0x12);
        SPI_WriteData(0x00);
        SPI_WriteData(0x18);
        SPI_WriteData(0x00);
        SPI_WriteData(0x21);
        SPI_WriteData(0x00);
        SPI_WriteData(0x2a);
        SPI_WriteData(0x00);
        SPI_WriteData(0x35);
        SPI_WriteData(0x00);
        SPI_WriteData(0x47);
        SPI_WriteData(0x00);
        SPI_WriteData(0x56);
        SPI_WriteData(0x00);
        SPI_WriteData(0x90);
        SPI_WriteData(0x00);
        SPI_WriteData(0xe5);
        SPI_WriteData(0x01);
        SPI_WriteData(0x68);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd5);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd7);
        SPI_WriteData(0x02);
        SPI_WriteData(0x36);
        SPI_WriteData(0x02);
        SPI_WriteData(0xa6);
        SPI_WriteData(0x02);
        SPI_WriteData(0xee);
        SPI_WriteData(0x03);
        SPI_WriteData(0x48);
        SPI_WriteData(0x03);
        SPI_WriteData(0xa0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xba);
        SPI_WriteData(0x03);
        SPI_WriteData(0xc5);
        SPI_WriteData(0x03);
        SPI_WriteData(0xd0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xE0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xea);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFa);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFF);

        SPI_WriteComm(0xD5);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x04);
        SPI_WriteData(0x00);
        SPI_WriteData(0x12);
        SPI_WriteData(0x00);
        SPI_WriteData(0x18);
        SPI_WriteData(0x00);
        SPI_WriteData(0x21);
        SPI_WriteData(0x00);
        SPI_WriteData(0x2a);
        SPI_WriteData(0x00);
        SPI_WriteData(0x35);
        SPI_WriteData(0x00);
        SPI_WriteData(0x47);
        SPI_WriteData(0x00);
        SPI_WriteData(0x56);
        SPI_WriteData(0x00);
        SPI_WriteData(0x90);
        SPI_WriteData(0x00);
        SPI_WriteData(0xe5);
        SPI_WriteData(0x01);
        SPI_WriteData(0x68);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd5);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd7);
        SPI_WriteData(0x02);
        SPI_WriteData(0x36);
        SPI_WriteData(0x02);
        SPI_WriteData(0xa6);
        SPI_WriteData(0x02);
        SPI_WriteData(0xee);
        SPI_WriteData(0x03);
        SPI_WriteData(0x48);
        SPI_WriteData(0x03);
        SPI_WriteData(0xa0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xba);
        SPI_WriteData(0x03);
        SPI_WriteData(0xc5);
        SPI_WriteData(0x03);
        SPI_WriteData(0xd0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xE0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xea);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFa);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFF);

        SPI_WriteComm(0xD6);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x00);
        SPI_WriteData(0x04);
        SPI_WriteData(0x00);
        SPI_WriteData(0x12);
        SPI_WriteData(0x00);
        SPI_WriteData(0x18);
        SPI_WriteData(0x00);
        SPI_WriteData(0x21);
        SPI_WriteData(0x00);
        SPI_WriteData(0x2a);
        SPI_WriteData(0x00);
        SPI_WriteData(0x35);
        SPI_WriteData(0x00);
        SPI_WriteData(0x47);
        SPI_WriteData(0x00);
        SPI_WriteData(0x56);
        SPI_WriteData(0x00);
        SPI_WriteData(0x90);
        SPI_WriteData(0x00);
        SPI_WriteData(0xe5);
        SPI_WriteData(0x01);
        SPI_WriteData(0x68);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd5);
        SPI_WriteData(0x01);
        SPI_WriteData(0xd7);
        SPI_WriteData(0x02);
        SPI_WriteData(0x36);
        SPI_WriteData(0x02);
        SPI_WriteData(0xa6);
        SPI_WriteData(0x02);
        SPI_WriteData(0xee);
        SPI_WriteData(0x03);
        SPI_WriteData(0x48);
        SPI_WriteData(0x03);
        SPI_WriteData(0xa0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xba);
        SPI_WriteData(0x03);
        SPI_WriteData(0xc5);
        SPI_WriteData(0x03);
        SPI_WriteData(0xd0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xE0);
        SPI_WriteData(0x03);
        SPI_WriteData(0xea);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFa);
        SPI_WriteData(0x03);
        SPI_WriteData(0xFF);

	udelay(1000); // Delay 1ms
        SPI_WriteComm(0x11);
        SPI_WriteData(0x00);
	udelay(120000); // Delay 120ms // This delay time is necessary

        SPI_WriteComm(0x29);
        SPI_WriteData(0x00);
	udelay(20000); // Delay 20ms // This delay time is necessary
}


static void panel_enable(struct lcd_panel *panel)
{
}

static void panel_disable(struct lcd_panel *panel)
{
}

static struct lcd_panel_ops panel_ops = {
	.enable  = (void*)panel_enable,
	.disable = (void*)panel_disable,
};

static struct fb_videomode panel_modes[] = {
	[0] = {
		.name                   = "480X480",
		.refresh                = 60,
		.xres                   = 480,
		.yres                   = 480,
		.pixclock               = KHZ2PICOS(25000),//ref 25Mhz.
#if 0
		.left_margin            = 20,//ref config.
		.right_margin           = 4,
		.upper_margin           = 14,//test 1,test 2
		.lower_margin           = 14,//test 1,test 3


		.hsync_len              = 5,
		.vsync_len              = 5,
#else
		.left_margin            = 60,//ref config.
		.right_margin           = 60,
		.upper_margin           = 20,//test 1,test 2
		.lower_margin           = 22,//test 2


		.hsync_len              = 20,
		.vsync_len              = 10,
#endif
		/* .sync                   = ~FB_SYNC_HOR_HIGH_ACT & ~FB_SYNC_VERT_HIGH_ACT, */
		.vmode                  = FB_VMODE_NONINTERLACED,
		.flag                   = 0,
	},
};

static struct tft_config gc9503v_spirgb_cfg = {
	.pix_clk_inv = 0,
	.de_dl = 0,
	.sync_dl = 0,
	.color_even = TFT_LCD_COLOR_EVEN_RGB,
	.color_odd = TFT_LCD_COLOR_ODD_RGB,
//	.color_even = TFT_LCD_COLOR_EVEN_RBG,
//	.color_odd = TFT_LCD_COLOR_ODD_RBG,
	.mode = TFT_LCD_MODE_PARALLEL_666,
};

struct lcd_panel lcd_panel = {
	.name = "gc9503v_spirgb",
	.num_modes = ARRAY_SIZE(panel_modes),
	.modes = panel_modes,
	.bpp = 24,
	.width = 72,
	.height = 70,

	.lcd_type = LCD_TYPE_TFT,

	.tft_config = &gc9503v_spirgb_cfg,
#if 0
	.dither_enable = 1,
	.dither.dither_red = 1,
	.dither.dither_green = 1,
	.dither.dither_blue = 1,
#else
	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,
#endif
	.ops = &panel_ops,
};

#if 0
static int panel_update_status(struct backlight_device *bd)
{
	struct panel_dev *panel = dev_get_drvdata(&bd->dev);
	int brightness = bd->props.brightness;
	unsigned int i;
	int pulse_num = MAX_BRIGHTNESS_STEP - brightness / CONVERT_FACTOR - 1;

	if (bd->props.fb_blank == FB_BLANK_POWERDOWN) {
		return 0;
	}

	if (bd->props.state & BL_CORE_SUSPENDED)
		brightness = 0;

	if (brightness) {
		gpio_direction_output(panel->pwm.gpio,0);
		udelay(5000);
		gpio_direction_output(panel->pwm.gpio,1);
		udelay(100);

		for (i = pulse_num; i > 0; i--) {
			gpio_direction_output(panel->pwm.gpio,0);
			udelay(1);
			gpio_direction_output(panel->pwm.gpio,1);
			udelay(3);
		}
	} else
		gpio_direction_output(panel->pwm.gpio, 0);

	return 0;
}

static struct backlight_ops panel_backlight_ops = {
	.options = BL_CORE_SUSPENDRESUME,
	.update_status = panel_update_status,
};
#endif

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)
static int panel_set_power(struct lcd_device *lcd, int power)
{
	struct panel_dev *panel = lcd_get_data(lcd);
	struct board_gpio *vdd_en = &panel->vdd_en;
	struct board_gpio *rst = &panel->rst;

	if(POWER_IS_ON(power) && !POWER_IS_ON(panel->power)) {
		/* *(unsigned int*)(0xb00101a4) = 0xf000000; */
		/* *(unsigned int*)(0xb00101b4) = 0xf000000; */
		/* *(unsigned int*)(0xb00101c4) = 0xf000000; */
		gpio_direction_output(vdd_en->gpio, 0);
		gpio_direction_output(rst->gpio, 1);
		Initial_IC();
	}
	if(!POWER_IS_ON(power) && POWER_IS_ON(panel->power)) {
		gpio_direction_output(vdd_en->gpio, 1);
	}

	panel->power = power;
        return 0;
}

static int panel_get_power(struct lcd_device *lcd)
{
	struct panel_dev *panel = lcd_get_data(lcd);

	return panel->power;
}

/**
* @ pannel_gc9503v_spirgb_lcd_ops, register to kernel common backlight/lcd.c framworks.
*/
static struct lcd_ops panel_lcd_ops = {
	.early_set_power = panel_set_power,
	.set_power = panel_set_power,
	.get_power = panel_get_power,
};

static int of_panel_parse(struct device *dev)
{
	struct panel_dev *panel = dev_get_drvdata(dev);
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int ret = 0;

	panel->vdd_en.gpio = of_get_named_gpio_flags(np, "ingenic,vdd-en-gpio", 0, &flags);
	if(gpio_is_valid(panel->vdd_en.gpio)) {
		panel->vdd_en.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->vdd_en.gpio, GPIOF_DIR_OUT, "vdd_en");
		if(ret < 0) {
			dev_err(dev, "Failed to request vdd_en pin!\n");
			return ret;
		}
	} else {
		dev_warn(dev, "invalid gpio vdd_en.gpio: %d\n", panel->vdd_en.gpio);
	}

	panel->rst.gpio = of_get_named_gpio_flags(np, "ingenic,rst-gpio", 0, &flags);
	if(gpio_is_valid(panel->rst.gpio)) {
		panel->rst.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->rst.gpio, GPIOF_DIR_OUT, "rst");
		if(ret < 0) {
			dev_err(dev, "Failed to request rst pin!\n");
			goto err_request_rst;
		}
	} else {
		dev_warn(dev, "invalid gpio rst.gpio: %d\n", panel->rst.gpio);
	}
#if 0
	panel->pwm.gpio = of_get_named_gpio_flags(np, "ingenic,lcd-pwm-gpio", 0, &flags);
	if(gpio_is_valid(panel->pwm.gpio)) {
		panel->pwm.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->pwm.gpio, GPIOF_DIR_OUT, "pwm");
		if(ret < 0) {
			dev_err(dev, "Failed to request pwm pin!\n");
			goto err_request_pwm;
		}
	} else {
		dev_warn(dev, "invalid gpio pwm.gpio: %d\n", panel->pwm.gpio);
	}
#endif

	panel->spi.sdo = of_get_named_gpio_flags(np, "ingenic,lcd-sdo-gpio", 0, &flags);
	if(gpio_is_valid(panel->spi.sdo)) {
		ret = gpio_request_one(panel->spi.sdo, GPIOF_DIR_OUT, "sdo");
		if(ret < 0) {
			dev_err(dev, "Failed to request sdo pin!\n");
			goto err_request_sdo;
		}
	} else {
		dev_warn(dev, "invalid gpio spi.sdo: %d\n", panel->spi.sdo);
	}

	panel->spi.sck = of_get_named_gpio_flags(np, "ingenic,lcd-sck-gpio", 0, &flags);
	if(gpio_is_valid(panel->spi.sck)) {
		ret = gpio_request_one(panel->spi.sck, GPIOF_DIR_OUT, "sck");
		if(ret < 0) {
			dev_err(dev, "Failed to request sck pin!\n");
			goto err_request_sck;
		}
	} else {
		dev_warn(dev, "invalid gpio spi.sck: %d\n", panel->spi.sck);
	}

	panel->spi.cs = of_get_named_gpio_flags(np, "ingenic,lcd-cs-gpio", 0, &flags);
	if(gpio_is_valid(panel->spi.cs)) {
		ret = gpio_request_one(panel->spi.cs, GPIOF_DIR_OUT, "cs");
		if(ret < 0) {
			dev_err(dev, "Failed to request cs pin!\n");
			goto err_request_cs;
		}
	} else {
		dev_warn(dev, "invalid gpio spi.cs: %d\n", panel->spi.cs);
	}

	return 0;
err_request_cs:
	if(gpio_is_valid(panel->spi.cs))
		gpio_free(panel->spi.cs);
err_request_sck:
	if(gpio_is_valid(panel->spi.sck))
		gpio_free(panel->spi.sck);
err_request_sdo:
	if(gpio_is_valid(panel->spi.sdo))
		gpio_free(panel->spi.sdo);
err_request_pwm:
	if(gpio_is_valid(panel->rst.gpio))
		gpio_free(panel->rst.gpio);
err_request_rst:
	if(gpio_is_valid(panel->rst.gpio))
		gpio_free(panel->rst.gpio);
	return ret;
}
/**
* @panel_probe
*
* 	1. Register to ingenicfb.
* 	2. Register to lcd.
* 	3. Register to backlight if possible.
*
* @pdev
*
* @Return -
*/
static int panel_probe(struct platform_device *pdev)
{
	int ret = 0;
	/* struct panel_dev *panel; */
	struct backlight_properties props;

	memset(&props, 0, sizeof(props));
	panel = kzalloc(sizeof(struct panel_dev), GFP_KERNEL);
	if(panel == NULL) {
		dev_err(&pdev->dev, "Failed to alloc memory!");
		return -ENOMEM;
	}
	panel->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, panel);

	ret = of_panel_parse(&pdev->dev);
	if(ret < 0) {
		goto err_of_parse;
	}

	panel->lcd = lcd_device_register("panel_lcd", &pdev->dev, panel, &panel_lcd_ops);
	if(IS_ERR_OR_NULL(panel->lcd)) {
		dev_err(&pdev->dev, "Error register lcd!\n");
		ret = -EINVAL;
		goto err_of_parse;
	}

	/* TODO: should this power status sync from uboot */
	panel->power = FB_BLANK_POWERDOWN;
	panel_set_power(panel->lcd, FB_BLANK_UNBLANK);

#if 0
	props.type = BACKLIGHT_RAW;
	props.max_brightness = 255;
	panel->backlight = backlight_device_register("pwm-backlight.0",
						&pdev->dev, panel,
						&panel_backlight_ops,
						&props);
	if (IS_ERR_OR_NULL(panel->backlight)) {
		dev_err(panel->dev, "failed to register 'pwm-backlight.0'.\n");
		goto err_lcd_register;
	}
	panel->backlight->props.brightness = props.max_brightness;
	backlight_update_status(panel->backlight);
#endif

	ret = ingenicfb_register_panel(&lcd_panel);
	if(ret < 0) {
		dev_err(&pdev->dev, "Failed to register lcd panel!\n");
		goto err_lcd_register;
	}

	return 0;

err_lcd_register:
	lcd_device_unregister(panel->lcd);
err_of_parse:
	kfree(panel);
	return ret;
}

static int panel_remove(struct platform_device *pdev)
{
	struct panel_dev *panel = dev_get_drvdata(&pdev->dev);

	panel_set_power(panel->lcd, FB_BLANK_POWERDOWN);
	return 0;
}

#ifdef CONFIG_PM
static int panel_suspend(struct device *dev)
{
	struct panel_dev *panel = dev_get_drvdata(dev);

	panel_set_power(panel->lcd, FB_BLANK_POWERDOWN);
	return 0;
}

static int panel_resume(struct device *dev)
{
	struct panel_dev *panel = dev_get_drvdata(dev);

	panel_set_power(panel->lcd, FB_BLANK_UNBLANK);
	return 0;
}

static const struct dev_pm_ops panel_pm_ops = {
	.suspend = panel_suspend,
	.resume = panel_resume,
};
#endif
static const struct of_device_id panel_of_match[] = {
	{ .compatible = "ingenic,gc9503v_spirgb", },
	{},
};

static struct platform_driver panel_driver = {
	.probe		= panel_probe,
	.remove		= panel_remove,
	.driver		= {
		.name	= "gc9503v_spirgb",
		.of_match_table = panel_of_match,
#ifdef CONFIG_PM
		.pm = &panel_pm_ops,
#endif
	},
};

module_platform_driver(panel_driver);
MODULE_LICENSE("GPL");
