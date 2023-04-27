/*
 * driver/video/fbdev/ingenic/x2000_v12/displays/panel-rm690a0.c
 *
 * Copyright (C) 2016 Ingenic Semiconductor Inc.
 *
 * This program is free software, you can redistribute it and/or modify it
 *
 * under the terms of the GNU General Public License version 2 as published by
 *
 * the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/of_gpio.h>
#include <linux/fb.h>
#include <linux/backlight.h>


#include "../ingenicfb.h"
#include "../jz_dsim.h"

#define CONFIG_DPU_AS_TFT 1     // 1: as mipi tft, 0: as mipi slcd

/*
「学无止境:
#define VSPW  2
#define VBPD  2
#define VFPD  6
#define HSPW  10
#define HBPD  40
#define HFPD  40」
—————————
「学无止境: colorbar是彩条吗，这个要刷图片实现」
—————————
「学无止境: 亮度调节0x51寄存器，参数0~255」
—————————
 */

#if 0
#define ENTER()								\
	do {								\
		printk("%03d panel-rm690a0 ENTER %s()\n", __LINE__, __PRETTY_FUNCTION__); \
	} while (0)

#define my_dbg(sss, aaa...)						\
	do {								\
		printk("%03d panel-rm690a0 debug %s() " sss "\n", __LINE__, __PRETTY_FUNCTION__, ##aaa); \
	} while (0)
#else

#define ENTER()					\
	do {					\
	} while (0)

#define LEAVE()					\
	do {					\
	} while (0)

#define my_dbg(sss, aaa...)			\
	do {					\
	} while (0)

#endif

extern void dump_dsi_reg(struct dsi_device *dsi);

struct board_gpio {
	short gpio;
	short active_level;
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
	struct board_gpio tp_en;
	struct board_gpio rst;
	struct board_gpio lcd_te;
	struct board_gpio oled;
	struct board_gpio lcd_pwm;
	struct board_gpio swire;

	struct mipi_dsim_lcd_device *dsim_dev;
};

struct panel_dev *panel;

#define lcd_to_master(a)	(a->dsim_dev->master)
#define lcd_to_master_ops(a)	((lcd_to_master(a))->master_ops)

struct rm690a0 {
	struct device *dev;
	unsigned int power;
	unsigned int id;

	struct lcd_device *ld;
	struct backlight_device *bd;

	struct mipi_dsim_lcd_device *dsim_dev;

};

#if 0
// original code: RM690A0_CODE_SPI-mipi+on.txt
/*
static struct dsi_cmd_packet Raydium_rm690a0_720p_cmd_list2[] =
{
//	{0x05, 0x29},
	{0x05, 0x29, 0x00},
};

static struct dsi_cmd_packet Raydium_rm690a0_720p_cmd_list11[] =
{
	{0x15, 0XFE, 0X01},
	{0x15, 0X04, 0XA0},  //SPI write ram enable
	{0x15, 0X05, 0X70},  //128RGB, T->B, NW
	{0x15, 0X06, 0X49},  //NL = 292 line (New modify)
	{0x15, 0X09, 0X01},  //NL2 = 1 (New modify)
	{0x15, 0X0E, 0X80},  //AVDD = 6.5V normal mode (New modify)
	{0x15, 0X0F, 0X80},  //AVDD = 6.5V idle mode (New modify)
	{0x15, 0X10, 0X11},  //AVDD = 2xVCI, AVDD regulator enable
	{0x15, 0X11, 0XA2},  //VCL = -1xVCI, -3.3V normal mode
	{0x15, 0X12, 0XA2},  //VCL = -1xVCI, -3.3V idle mode
	{0x15, 0X13, 0X80},  //VGH = AVDD normal mode
	{0x15, 0X14, 0X80},  //VGH = AVDD idle mode
	{0x15, 0X15, 0X81},  //VGL = VCL-VCI normal mode
	{0x15, 0X16, 0X81},  //VGL = VCL-VCI idle mode
	{0x15, 0X18, 0X66},  //VGHR =6V normal/idle mode
	{0x15, 0X19, 0X88},  //VGLR =-6V normal/idle mode
	{0x15, 0X1D, 0X02},  //Switch EQ on
	{0x15, 0X1E, 0X02},  //Switch EQ on
	{0x15, 0X1F, 0X02},  //VSR EQ on
	{0x15, 0X20, 0X02},  //VSR EQ on

	{0x15, 0X25, 0X06},  //Normal mode: gamma1, 24bit, PSELA=2b'01
	{0x15, 0X26, 0X17},  //T1A = 217(535x100ns= 53.5us)
	{0x15, 0X27, 0X0A},  //normal mode VBP= 10
	{0x15, 0X28, 0X0A},  //normal mode VFP= 10
	{0x15, 0X29, 0X01},  //normal mode skip frame off
	{0x15, 0X2A, 0X06},  //idle mode: gamma1, 24bit, PSELA=2b'01
	{0x15, 0X2B, 0X17},  //T1B = 217(535x100ns= 53.5us)
	{0x15, 0X2D, 0X0A},  //idle mode VBP= 10
	{0x15, 0X2F, 0X0A},  //idle mode VFP= 10
	{0x15, 0X30, 0X43},  //idle mode skip frame = 60/4 = 15Hz

	{0x15, 0X36, 0X00},  //RM690A0, AP= 3b'000
	{0x15, 0X37, 0X0C},  //precharge to VGSP, mux 1:6


	{0x15, 0X3A, 0X1E},  //T1_sd=3us
	{0x15, 0X3B, 0X00}, //Tp_sd=0us
	{0x15, 0X3D, 0X0A},  //Th_sd=1us
	{0x15, 0X3F, 0X28},  //Tsw_sd=4us
	{0x15, 0X40, 0X0A},  //Thsw_sd=1us
	{0x15, 0X41, 0X06},  //Thsd_sd

	{0x15, 0X42, 0X36},  //Mux 362514, odd/even line SWAP
	{0x15, 0X43, 0X63},
	{0x15, 0X44, 0X25},
	{0x15, 0X45, 0X52},
	{0x15, 0X46, 0X14},
	{0x15, 0X47, 0X41},
	{0x15, 0X48, 0X36},
	{0x15, 0X49, 0X63},
	{0x15, 0X4A, 0X25},
	{0x15, 0X4B, 0X52},
	{0x15, 0X4C, 0X14},
	{0x15, 0X4D, 0X41},
	{0x15, 0X4E, 0X36},  //Data B1B2G1G2R1R2, odd/even line SWAP
	{0x15, 0X4F, 0X63},
	{0x15, 0X50, 0X25},
	{0x15, 0X51, 0X52},
	{0x15, 0X52, 0X14},
	{0x15, 0X53, 0X41},
	{0x15, 0X54, 0X36},
	{0x15, 0X55, 0X63},
	{0x15, 0X56, 0X25},
	{0x15, 0X57, 0X52},
	{0x15, 0X58, 0X14},
	{0x15, 0X59, 0X41},

	{0x15, 0X5B, 0X10},  //VREFN5 on
	{0x15, 0X62, 0X19},  //VREFN5 = -3V normal mode
	{0x15, 0X63, 0X19},  //VREFN5 = -3V idle mode

	{0x15, 0X66, 0X90},  //idle mode internal power
	{0x15, 0X67, 0X40},  //internal power delay 1 frame off
	{0x15, 0X6A, 0X05},  //swire 05 pulse, -2V for RT4723


	{0x15, 0X6C, 0X80},  //RM690A0, OP setting (sop turn off during sw off)
	{0x15, 0X6D, 0X39},  //skip frame VGMP and VGSP regulator off
	{0x15, 0X6E, 0X00},  //MIPI interface on
	{0x15, 0X70, 0XA8},  //Display on blanking 1: SD to GND; other region: SD to AVDD
	{0x15, 0X72, 0X1A}, //internal OVDD = 4.6V
	{0x15, 0X73, 0X11},  //internal OVSS = -2V
	{0x15, 0X74, 0X0C},  //OVDD power from AVDD, source power from AVDD

	{0x15, 0XFE, 0X02},
	{0x15, 0XA9, 0X30},  //VGMP = 130 ,5.8V (New modify)
	{0x15, 0XAA, 0XB8},  //VGSP = 0B8 ,2.5V
	{0x15, 0XAB, 0X01},  //VGMP VGSP high byte

	{0x15, 0XFE, 0X03},
	{0x15, 0XA9, 0X30},  //VGMP = 130 ,5.8V (New modify)
	{0x15, 0XAA, 0XB8},  //VGSP = 0B8 ,2.5V
	{0x15, 0XAB, 0X01},  //VGMP VGSP high byte

	{0x15, 0XFE, 0X04},

	//SN_STV SE
	{0x15, 0X4c,0X89},
	{0x15, 0X4d,0X00},
	{0x15, 0X4e,0X00},
	{0x15, 0X4f,0X40},
	{0x15, 0X50,0X01},
	{0x15, 0X51,0X01},
	{0x15, 0X52,0X16},
	// SET,
	{0x15, 0X00,0Xcc},
	{0x15, 0X01,0X00},
	{0x15, 0X02,0X02},
	{0x15, 0X03,0X00},
	{0x15, 0X04,0X48},
	{0x15, 0X05,0X03},
	{0x15, 0X06,0X76},
	{0x15, 0X07,0X16},
	{0x15, 0X08,0X08},
	//2 SET,
	{0x15, 0X09,0Xcc},
	{0x15, 0X0a,0X00},
	{0x15, 0X0b,0X02},
	{0x15, 0X0c,0X00},
	{0x15, 0X0d,0X48},
	{0x15, 0X0e,0X02},
	{0x15, 0X0f,0X76},
	{0x15, 0X10,0X16},
	{0x15, 0X11,0X08},
	//1 SET,
	{0x15, 0X12,0Xcc},
	{0x15, 0X13,0X00},
	{0x15, 0X14,0X02},
	{0x15, 0X15,0X00},
	{0x15, 0X16,0X20},
	{0x15, 0X17,0X02},
	{0x15, 0X18,0X32},
	{0x15, 0X19,0Xe4},
	{0x15, 0X1a,0X08},
	//2 SET,
	{0x15, 0X1b,0Xcc},
	{0x15, 0X1c,0X00},
	{0x15, 0X1d,0X02},
	{0x15, 0X1e,0X00},
	{0x15, 0X1f,0X20},
	{0x15, 0X20,0X03},
	{0x15, 0X21,0X32},
	{0x15, 0X22,0Xe4},
	{0x15, 0X23,0X08},

	{0x15, 0X53,0X8a},
	{0x15, 0X54,0X40},
	{0x15, 0X55,0X02},
	{0x15, 0X56,0X01},
	{0x15, 0X58,0X16},
	{0x15, 0X59,0X02},
	{0x15, 0X65,0X02},
	{0x15, 0X66,0X04},
	{0x15, 0X67,0X00},


	//appin,
	{0x15, 0X5E,0X01},
	{0x15, 0X5F,0XC8},
	{0x15, 0X60,0XCC},
	{0x15, 0X61,0XCC},
	{0x15, 0X62,0XCC},
	{0x15, 0X75,0XCC},
	{0x15, 0X76,0X3C},
	{0x15, 0X77,0X92},
	{0x15, 0X78,0XCC},
	{0x15, 0X79,0XCC},
	{0x15, 0X9C,0XF8},
	{0x15, 0X9D,0XC7},
	{0x15, 0X9E,0X33},

	//pping,
	{0x15, 0XA2,0XCC},
	{0x15, 0XA3,0X3C},
	{0x15, 0XA4,0X54},
	{0x15, 0XA5,0X10},
	{0x15, 0XA6,0XC2},
	{0x15, 0XA7,0XCC},
	{0x15, 0X9F,0X3F},
	{0x15, 0XA0,0X3F},


	{0x15, 0X63,0XF8},
	{0x15, 0X7A,0XC7},
	{0x15, 0X8D,0X1F},
	{0x15, 0X8E,0XE3},

	//	     OP s,
	{0x15, 0X64, 0X0E},

	{0x15, 0xFE, 0x00},
	{0x15, 0xC4, 0x80},

	{0x39, 0x05, 0x00, {0x2A, 0x00, 0x00, 0x00,0x7D}},

	{0x15, 0x35, 0x00},
	{0x15, 0x51, 0xff},
//	{0x05, 0x11},
	{0x05, 0x11, 0x00},
};
*/
#else
static struct dsi_cmd_packet Raydium_rm690a0_720p_cmd_list2[] =
{
	{0x05, 0x29, 0x00},
};

static struct dsi_cmd_packet Raydium_rm690a0_720p_cmd_list11[] =
{
	//{0x05, 0X01, 0X00}, //SWRESET(0100h) : Software Reset

	{0x15, 0XFE, 0X01},
	{0x15, 0X04, 0XA0},  //SPI write ram enable
	{0x15, 0X05, 0X70},  //128RGB, T->B, NW
	{0x15, 0X06, 0X49},  //NL = 292 line (New modify)
	{0x15, 0X09, 0X01},  //NL2 = 1 (New modify)
	{0x15, 0X0E, 0X80},  //AVDD = 6.5V normal mode (New modify)
	{0x15, 0X0F, 0X80},  //AVDD = 6.5V idle mode (New modify)
	{0x15, 0X10, 0X11},  //AVDD = 2xVCI, AVDD regulator enable
	{0x15, 0X11, 0XA2},  //VCL = -1xVCI, -3.3V normal mode
	{0x15, 0X12, 0XA2},  //VCL = -1xVCI, -3.3V idle mode
	{0x15, 0X13, 0X80},  //VGH = AVDD normal mode
	{0x15, 0X14, 0X80},  //VGH = AVDD idle mode
	{0x15, 0X15, 0X81},  //VGL = VCL-VCI normal mode
	{0x15, 0X16, 0X81},  //VGL = VCL-VCI idle mode
	{0x15, 0X18, 0X66},  //VGHR =6V normal/idle mode
	{0x15, 0X19, 0X88},  //VGLR =-6V normal/idle mode
	{0x15, 0X1D, 0X02},  //Switch EQ on
	{0x15, 0X1E, 0X02},  //Switch EQ on
	{0x15, 0X1F, 0X02},  //VSR EQ on
	{0x15, 0X20, 0X02},  //VSR EQ on

	{0x15, 0X25, 0X06},  //Normal mode: gamma1, 24bit, PSELA=2b'01
	{0x15, 0X26, 0X17},  //T1A = 217(535x100ns= 53.5us)
	{0x15, 0X27, 0X0A},  //normal mode VBP= 10
	{0x15, 0X28, 0X0A},  //normal mode VFP= 10
	{0x15, 0X29, 0X01},  //normal mode skip frame off
	{0x15, 0X2A, 0X06},  //idle mode: gamma1, 24bit, PSELA=2b'01
	{0x15, 0X2B, 0X17},  //T1B = 217(535x100ns= 53.5us)
	{0x15, 0X2D, 0X0A},  //idle mode VBP= 10
	{0x15, 0X2F, 0X0A},  //idle mode VFP= 10
	{0x15, 0X30, 0X43},  //idle mode skip frame = 60/4 = 15Hz

	{0x15, 0X36, 0X00},  //RM690A0, AP= 3b'000
	{0x15, 0X37, 0X0C},  //precharge to VGSP, mux 1:6


	{0x15, 0X3A, 0X1E},  //T1_sd=3us
	{0x15, 0X3B, 0X00}, //Tp_sd=0us
	{0x15, 0X3D, 0X0A},  //Th_sd=1us
	{0x15, 0X3F, 0X28},  //Tsw_sd=4us
	{0x15, 0X40, 0X0A},  //Thsw_sd=1us
	{0x15, 0X41, 0X06},  //Thsd_sd

	{0x15, 0X42, 0X36},  //Mux 362514, odd/even line SWAP
	{0x15, 0X43, 0X63},
	{0x15, 0X44, 0X25},
	{0x15, 0X45, 0X52},
	{0x15, 0X46, 0X14},
	{0x15, 0X47, 0X41},
	{0x15, 0X48, 0X36},
	{0x15, 0X49, 0X63},
	{0x15, 0X4A, 0X25},
	{0x15, 0X4B, 0X52},
	{0x15, 0X4C, 0X14},
	{0x15, 0X4D, 0X41},
	{0x15, 0X4E, 0X36},  //Data B1B2G1G2R1R2, odd/even line SWAP
	{0x15, 0X4F, 0X63},
	{0x15, 0X50, 0X25},
	{0x15, 0X51, 0X52},
	{0x15, 0X52, 0X14},
	{0x15, 0X53, 0X41},
	{0x15, 0X54, 0X36},
	{0x15, 0X55, 0X63},
	{0x15, 0X56, 0X25},
	{0x15, 0X57, 0X52},
	{0x15, 0X58, 0X14},
	{0x15, 0X59, 0X41},

	{0x15, 0X5B, 0X10},  //VREFN5 on
	{0x15, 0X62, 0X19},  //VREFN5 = -3V normal mode
	{0x15, 0X63, 0X19},  //VREFN5 = -3V idle mode

	{0x15, 0X66, 0X90},  //idle mode internal power
	{0x15, 0X67, 0X40},  //internal power delay 1 frame off
	{0x15, 0X6A, 0X05},  //swire 05 pulse, -2V for RT4723


	{0x15, 0X6C, 0X80},  //RM690A0, OP setting (sop turn off during sw off)
	{0x15, 0X6D, 0X39},  //skip frame VGMP and VGSP regulator off
	{0x15, 0X6E, 0X00},  //MIPI interface on
	{0x15, 0X70, 0XA8},  //Display on blanking 1: SD to GND; other region: SD to AVDD
	{0x15, 0X72, 0X1A}, //internal OVDD = 4.6V
	{0x15, 0X73, 0X11},  //internal OVSS = -2V
	{0x15, 0X74, 0X0C},  //OVDD power from AVDD, source power from AVDD

	{0x15, 0XFE, 0X02},
	{0x15, 0XA9, 0X30},  //VGMP = 130 ,5.8V (New modify)
	{0x15, 0XAA, 0XB8},  //VGSP = 0B8 ,2.5V
	{0x15, 0XAB, 0X01},  //VGMP VGSP high byte

	{0x15, 0XFE, 0X03},
	{0x15, 0XA9, 0X30},  //VGMP = 130 ,5.8V (New modify)
	{0x15, 0XAA, 0XB8},  //VGSP = 0B8 ,2.5V
	{0x15, 0XAB, 0X01},  //VGMP VGSP high byte

	{0x15, 0XFE, 0X04},

	//SN_STV SE
	{0x15, 0X4c,0X89},
	{0x15, 0X4d,0X00},
	{0x15, 0X4e,0X00},
	{0x15, 0X4f,0X40},
	{0x15, 0X50,0X01},
	{0x15, 0X51,0X01},
	{0x15, 0X52,0X16},

	// SET,
	{0x15, 0X00,0Xcc},
	{0x15, 0X01,0X00},
	{0x15, 0X02,0X02},
	{0x15, 0X03,0X00},
	{0x15, 0X04,0X48},
	{0x15, 0X05,0X03},
	{0x15, 0X06,0X76},
	{0x15, 0X07,0X16},
	{0x15, 0X08,0X08},
	//SN_CK2 SET
	{0x15, 0X09,0Xcc},
	{0x15, 0X0a,0X00},
	{0x15, 0X0b,0X02},
	{0x15, 0X0c,0X00},
	{0x15, 0X0d,0X48},
	{0x15, 0X0e,0X02},
	{0x15, 0X0f,0X76},
	{0x15, 0X10,0X16},
	{0x15, 0X11,0X08},

	//EM_CK1 SET
	{0x15, 0X12,0Xcc},
	{0x15, 0X13,0X00},
	{0x15, 0X14,0X02},
	{0x15, 0X15,0X00},
	{0x15, 0X16,0X20},
	{0x15, 0X17,0X02},
	{0x15, 0X18,0X32},
	{0x15, 0X19,0Xe4},
	{0x15, 0X1a,0X08},
	    //EM_CK2 SET
	{0x15, 0X1b,0Xcc},
	{0x15, 0X1c,0X00},
	{0x15, 0X1d,0X02},
	{0x15, 0X1e,0X00},
	{0x15, 0X1f,0X20},
	{0x15, 0X20,0X03},
	{0x15, 0X21,0X32},
	{0x15, 0X22,0Xe4},
	{0x15, 0X23,0X08},

	    //EM SET
	{0x15, 0X53,0X8a},
	{0x15, 0X54,0X40},
	{0x15, 0X55,0X02},
	{0x15, 0X56,0X01},
	{0x15, 0X58,0X16},
	{0x15, 0X59,0X02},
	{0x15, 0X65,0X02},
	{0x15, 0X66,0X04},
	{0x15, 0X67,0X00},


	    //VSR mapping
	{0x15, 0X5E,0X01},
	{0x15, 0X5F,0XC8},
	{0x15, 0X60,0XCC},
	{0x15, 0X61,0XCC},
	{0x15, 0X62,0XCC},
	{0x15, 0X75,0XCC},
	{0x15, 0X76,0X3C},
	{0x15, 0X77,0X92},
	{0x15, 0X78,0XCC},
	{0x15, 0X79,0XCC},
	{0x15, 0X9C,0XF8},
	{0x15, 0X9D,0XC7},
	{0x15, 0X9E,0X33},

	    //SW mapping
	{0x15, 0XA2,0XCC},
	{0x15, 0XA3,0X3C},
	{0x15, 0XA4,0X54},
	{0x15, 0XA5,0X10},
	{0x15, 0XA6,0XC2},
	{0x15, 0XA7,0XCC},
	{0x15, 0X9F,0X3F},
	{0x15, 0XA0,0X3F},


	    //Hi-z
	{0x15, 0X63,0XF8},
	{0x15, 0X7A,0XC7},
	{0x15, 0X8D,0X1F},
	{0x15, 0X8E,0XE3},

	    //Gamma OP setting
	{0x15, 0X64, 0X0E},


	{0x15, 0xFE, 0x00},
	{0x15, 0xC4, 0x80}, // SetDSPIMode (C400h) : set_DSPI Mode

#if 0
	//{0x39, 0x05, 0x00, {0x2A, 0x00, 0x00, 0x00,0x7D}}, // 0x7D(125), CASET(2A00h~2A03h) : Set Column Start Address
	//{0x39, 0x05, 0x00, {0x2B, 0x00, 0x00, 0x00,0xEF}}, // RASET(2B00h~2B03h) : Set Row Start Address
	//{0x39, 0x05, 0x00, {0x30, 0x00, 0x00, 0x00,0xEF}}, // PTLAR (3000h): Partial Area Row
	//{0x39, 0x05, 0x00, {0x31, 0x00, 0x00, 0x00,0x7D}}, // PTLAR (3100h): Vertical Partial Area
#else
	{0x39, 0x05, 0x00, {0x2A, 0x00, 0x00, 0x00,0x7D}}, // 0x7f(127), CASET(2A00h~2A03h) : Set Column Start Address
	//{0x39, 0x05, 0x00, {0x2B, 0x00, 0x00, 0x00,0xEF}}, // RASET(2B00h~2B03h) : Set Row Start Address
	//{0x39, 0x05, 0x00, {0x30, 0x00, 0x00, 0x00,0x7D}}, // PTLAR (3000h): Partial Area Row
	//{0x39, 0x05, 0x00, {0x31, 0x00, 0x00, 0x00,0x7D}}, // PTLAR (3100h): Vertical Partial Area
#endif
	{0x15, 0x36, 0x00}, // MADCTR (3600h): Scan Direction Control, bit3: ‘1’ =BGR, “0”=RGB
	//{0x05, 0x38, 0x00},	// IDMOFF (3800h): Idle Mode Off
	//{0x05, 0x34, 0x00},	// TEOFF (3400h): Tearing Effect Line OFF
	{0x15, 0x35, 0x00},	// TEON (3500h): Tearing Effect Line ON

	//{0x05, 0x21, 0x00}, // test for: INVON (2100H): Display Inversion On
	//{0x05, 0x23, 0x00}, // test for: ALLPON (2300H): All Pixel On
	//{0x05, 0x13, 0x00}, // NORON (1300h): Normal Display Mode On
	//{0x05, 0x12, 0x00}, // PTLON (1200h): Partial Display Mode On
	{0x15, 0x51, 0xff}, // WRDISBV (5100h): Write Display Brightness
	{0x05, 0x11, 0x00}, // SLPOUT (1100h): Sleep Out
	{0x05, 0x29, 0x00}, // display_on
};
#endif

static void jzdsi_send_data(struct panel_dev *lcd, unsigned char * buf, int size)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send; /* 0x2c Memory write */
	int s;
	//my_dbg("size=%d", size);
	while (size>0) {
		s = size;
		if (s>MAX_WORD_COUNT)
			s = MAX_WORD_COUNT;
		data_to_send.packet_type = 0x29; /* 0x29 long packet send */
		data_to_send.cmd0_or_wc_lsb = s;
		data_to_send.cmd1_or_wc_msb = 0;
		memcpy((void*)&data_to_send.cmd_data[0], buf, s);
		data_to_send.cmd_data[0] = 0x39;
		ops->cmd_write(lcd_to_master(lcd), data_to_send);
		buf += s;
		size -= s;
	}
	return;
}

#define LCD_WIDTH (128)
#define LCD_HEIGHT (320)

static void panel_send_data_test(struct panel_dev *lcd)
{
	/* ingenic/fb_v12/jz_dsim.h
	 *  #define MAX_WORD_COUNT     150
	 *  struct dsi_cmd_packet {
	 *  	unsigned char packet_type;
	 *  	unsigned char cmd0_or_wc_lsb;
	 *  	unsigned char cmd1_or_wc_msb;
	 *  	unsigned char cmd_data[MAX_WORD_COUNT];
	 *  };
	*/
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send_mw = {0x05, 0x2c, 0x00}; /* 0x2c Memory write */
	//struct dsi_cmd_packet data_to_send_mode_on = {0x05, 0x13, 0x00}; /* 0x13 normal mode on */
	struct dsi_cmd_packet data_to_send_mode_on = {0x05, 0x39, 0x00}; /* 0x39 display_on */
	unsigned char data[LCD_WIDTH*4];
	int line_length;
	int width, height, h;

	ENTER();
	ops->cmd_write(lcd_to_master(lcd), data_to_send_mw);
	ops->cmd_write(lcd_to_master(lcd), data_to_send_mode_on);

	width = LCD_WIDTH; height=LCD_HEIGHT;
	line_length = width*4;
	for (h=0;h<height;h++) {
		memset(data, 0xff, line_length);
		jzdsi_send_data(lcd, data, line_length);
	}
	msleep(2000);
	ops->cmd_write(lcd_to_master(lcd), data_to_send_mode_on);
}

static void panel_reg_test(struct panel_dev *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	//struct dsi_cmd_packet data_to_send = {0x05, 0x22, 0x00}; // ALLPOFF (2200H): All Pixel Off
	struct dsi_cmd_packet data_to_send = {0x05, 0x23, 0x00}; // ALLPON (2300H): All Pixel On
	ENTER();
	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void panel_test_all_pixel_on(struct panel_dev *lcd, int on)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send_off = {0x05, 0x22, 0x00}; // ALLPOFF (2200H): All Pixel Off
	struct dsi_cmd_packet data_to_send_on = {0x05, 0x23, 0x00}; // ALLPON (2300H): All Pixel On
	my_dbg("on/off: %d\n", on);
	if (on)
		ops->cmd_write(lcd_to_master(lcd), data_to_send_on);
	else
		ops->cmd_write(lcd_to_master(lcd), data_to_send_off);
}

static void panel_dev_sleep_in(struct panel_dev *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x10, 0x00};
	ENTER();
	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void panel_dev_sleep_out(struct panel_dev *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x11, 0x00};
	ENTER();

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void panel_dev_display_on(struct panel_dev *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x29, 0x00};
	//my_dbg("************ skip display on 0x29");	return;
	ENTER();

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void panel_dev_display_off(struct panel_dev *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x28, 0x00};
	ENTER();

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}


static void panel_dev_panel_init(struct panel_dev *lcd)
{
	int  i;
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ENTER();

	for (i = 0; i < ARRAY_SIZE(Raydium_rm690a0_720p_cmd_list11); i++)
	{
		//printk("%d: %02x\n", i, Raydium_rm690a0_720p_cmd_list11[i].cmd0_or_wc_lsb);
		//udelay(100);
		ops->cmd_write(dsi,  Raydium_rm690a0_720p_cmd_list11[i]);
	}
#if 0	
	msleep(120);
	/* display_on */
	for (i = 0; i < ARRAY_SIZE(Raydium_rm690a0_720p_cmd_list2); i++)
	{
		//printk("%d: %02x\n", i, Raydium_rm690a0_720p_cmd_list2[i].cmd0_or_wc_lsb);
		ops->cmd_write(dsi,  Raydium_rm690a0_720p_cmd_list2[i]);
	}
	msleep(80);
#endif
}

static int panel_dev_ioctl(struct mipi_dsim_lcd_device *dsim_dev, int cmd)
{
	ENTER();
	return 0;
}
static void panel_dev_set_sequence_impl(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct rm690a0 *lcd = dev_get_drvdata(&dsim_dev->dev);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ENTER();

	panel_dev_panel_init(panel);
	//msleep(120);
	//panel_dev_sleep_out(panel);
	//msleep(120);
	//panel_dev_display_on(panel);
	//msleep(80);
	//dump_dsi_reg(dsi);
	lcd->power = FB_BLANK_UNBLANK;
	return;
}
static void panel_dev_set_sequence(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct rm690a0 *lcd = dev_get_drvdata(&dsim_dev->dev);
	struct dsi_device *dsi = lcd_to_master(lcd);
	struct board_gpio *vdd_en = &panel->vdd_en;
//	struct board_gpio *tp_en = &panel->tp_en;
	struct board_gpio *rst = &panel->rst;
//	struct board_gpio *lcd_te = &panel->lcd_te;
	int cnt = 0;

	return panel_dev_set_sequence_impl(dsim_dev);

	/* debug */
	panel_dev_set_sequence_impl(dsim_dev);
	my_dbg("panel_test_all_pixel_on(lcd)\n"); msleep(1000);
	panel_test_all_pixel_on(panel, 1); msleep(1000);
	panel_test_all_pixel_on(panel, 0); msleep(1000);
	panel_test_all_pixel_on(panel, 1); msleep(1000);

	while(cnt++<2) {
		my_dbg("power on/off test.");
		gpio_direction_output(vdd_en->gpio, !vdd_en->active_level);
		msleep(3000);

		my_dbg("power_on.");  msleep(1000);
		gpio_direction_output(vdd_en->gpio, vdd_en->active_level);
		msleep(1000);
		my_dbg("reset.");  msleep(1000);
		/* reset test */
		gpio_direction_output(rst->gpio, 1);
		msleep(100);
		gpio_direction_output(rst->gpio, 0);
		msleep(120);
		gpio_direction_output(rst->gpio, 1);
		msleep(100);
		my_dbg("panel_dev_set_sequence_impl start"); msleep(1000);
		panel_dev_set_sequence_impl(dsim_dev);
		//my_dbg("panel_send_data_test(lcd)\n"); msleep(1000);
		panel_send_data_test(panel);
		panel_dev_display_on(panel);
		msleep(3000);
	}
	return;
}

static void panel_dev_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power)
{
	struct board_gpio *vdd_en = &panel->vdd_en;
	struct board_gpio *tp_en = &panel->tp_en;
	struct board_gpio *rst = &panel->rst;
//	struct board_gpio *lcd_te = &panel->lcd_te;
//	struct board_gpio *oled = &panel->oled;
//	struct board_gpio *swire = &panel->swire;
	int cnt;
	ENTER();


	/* must enable both: VDDIO(1.8~3.3V) and VCI/TP-VDD(3V3) */
	my_dbg("vdd_en->gpio=%d, vdd_en->active_level=%d\n", vdd_en->gpio, vdd_en->active_level);
	gpio_direction_output(vdd_en->gpio, vdd_en->active_level);
	my_dbg("tp_en->gpio=%d, tp_en->active_level=%d\n", tp_en->gpio, tp_en->active_level);
	gpio_direction_output(tp_en->gpio, tp_en->active_level);

	cnt = 0;
	while(cnt++<0) {
		my_dbg("power on test.");
		gpio_direction_output(vdd_en->gpio, !vdd_en->active_level);
		msleep(3000);
		gpio_direction_output(vdd_en->gpio, vdd_en->active_level);
		msleep(3000);
	}
//	if (gpio_is_valid(swire->gpio)) {
//		gpio_direction_input(swire->gpio);
//	}
//
//	if (gpio_is_valid(oled->gpio)) {
//		gpio_direction_input(oled->gpio);
//	}
	msleep(10);

	gpio_direction_output(rst->gpio, 1);
	msleep(10);
	gpio_direction_output(rst->gpio, 0);
	msleep(12);
	gpio_direction_output(rst->gpio, 1);
	msleep(1);

	panel->power = power;
}



#if CONFIG_DPU_AS_TFT //tft
static struct fb_videomode panel_modes = {
	.name = "Raydium_rm690a0-lcd",
	/* .xres = 126, */
	/* .yres = 294, */
	.xres = 126,
	.yres = 294,

	.refresh = 60,

	.left_margin = 40,//hbp
	.right_margin = 40,//hfp
	.hsync_len = 10, //hsync

	.upper_margin = 2,//vbp
	.lower_margin = 6,//vfp
	.vsync_len = 2, //vsync

	.sync                   = FB_SYNC_HOR_HIGH_ACT & FB_SYNC_VERT_HIGH_ACT,
	.vmode                  = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

struct jzdsi_data jzdsi_pdata = {
	.modes = &panel_modes,
	.video_config.no_of_lanes = 1,
	.video_config.virtual_channel = 0,
	.video_config.color_coding = COLOR_CODE_24BIT,
	.video_config.video_mode = VIDEO_BURST_WITH_SYNC_PULSES,
	.video_config.receive_ack_packets = 0,	/* enable receiving of ack packets */
	.video_config.is_18_loosely = 0,
	.video_config.data_en_polarity = 1,
	.video_config.byte_clock = 0, // driver will auto calculate byte_clock.
	.video_config.byte_clock_coef = MIPI_PHY_BYTE_CLK_COEF_MUL3_DIV2, // byte_clock *3/2.

	.dsi_config.max_lanes = 2,
	.dsi_config.max_hs_to_lp_cycles = 100,
	.dsi_config.max_lp_to_hs_cycles = 40,
//	.dsi_config.max_hs_to_lp_cycles = 200,
//	.dsi_config.max_lp_to_hs_cycles = 80,
	.dsi_config.max_bta_cycles = 4095,
	.dsi_config.color_mode_polarity = 1,
	.dsi_config.shut_down_polarity = 1,
	.dsi_config.max_bps = 2750,
//	.max_bps = 650,  // 650 Mbps
	.bpp_info = 24,
};
static struct tft_config tft_cfg = {
	.pix_clk_inv = 0,
	.de_dl = 0,
	.sync_dl = 0,
	.color_even = TFT_LCD_COLOR_EVEN_RGB,
	.color_odd = TFT_LCD_COLOR_ODD_RGB,
	.mode = TFT_LCD_MODE_PARALLEL_888,
};
struct lcd_panel lcd_panel = {
	.num_modes = 1,
	.modes = &panel_modes,
	.dsi_pdata = &jzdsi_pdata,

	.lcd_type = LCD_TYPE_MIPI_TFT,
	.tft_config = &tft_cfg,
	.bpp = 24,
	.width = 68,
	.height = 121,
	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,


};

#else //as slcd
static struct fb_videomode panel_modes = {
	.name = "Raydium_rm690a0-lcd",
	.refresh = 60,
	/* .xres = 126, */
	/* .yres = 294, */
	.xres = 128,
	.yres = 300,
	.left_margin = 40,
	.right_margin = 40,
	.upper_margin = 6,//vbp
	.lower_margin = 2,//vfp

	.hsync_len = 40,
	.vsync_len = 40,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

struct jzdsi_data jzdsi_pdata = {
	.modes = &panel_modes,
	.video_config.no_of_lanes = 1,
	.video_config.virtual_channel = 0,
	.video_config.color_coding = COLOR_CODE_24BIT,
	.video_config.video_mode = VIDEO_BURST_WITH_SYNC_PULSES,
	.video_config.receive_ack_packets = 0,	/* enable receiving of ack packets */
	.video_config.is_18_loosely = 0,
	.video_config.data_en_polarity = 1,
	.video_config.byte_clock = 0, // driver will auto calculate byte_clock.
	.video_config.byte_clock_coef = MIPI_PHY_BYTE_CLK_COEF_MUL6_DIV5, //for auto calculate byte clock

	.dsi_config.max_lanes = 1,
	.dsi_config.max_hs_to_lp_cycles = 100,
	.dsi_config.max_lp_to_hs_cycles = 40,
	.dsi_config.max_bta_cycles = 4095,
	.dsi_config.color_mode_polarity = 1,
	.dsi_config.shut_down_polarity = 1,
	.dsi_config.max_bps = 2750,
//	.dsi_config.max_bps = 650,
	.bpp_info = 24,
};
struct smart_config smart_cfg = {
	.dc_md = 0,
	.wr_md = 1,
	.smart_type = SMART_LCD_TYPE_8080,
	.pix_fmt = SMART_LCD_FORMAT_888,
	.dwidth = SMART_LCD_DWIDTH_24_BIT,
	.te_mipi_switch = 1,
	.te_switch = 1,
};

struct lcd_panel lcd_panel = {
	.num_modes = 1,
	.modes = &panel_modes,
	.dsi_pdata = &jzdsi_pdata,
	.smart_config = &smart_cfg,

	.lcd_type = LCD_TYPE_MIPI_SLCD,
	.bpp = 24,
	.width = 68,
	.height = 121,
};

#endif

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)
static int panel_set_power(struct lcd_device *lcd, int power)
{
        return 0;
}

static int panel_get_power(struct lcd_device *lcd)
{
	struct panel_dev *panel = lcd_get_data(lcd);

	return panel->power;
}

/**
* @ pannel_rm690a0_lcd_ops, register to kernel common backlight/lcd.c framworks.
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

	/* must enable both: VDDIO(1.8~3.3V) and VCI/TP-VDD(3V3) */
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
	panel->tp_en.gpio = of_get_named_gpio_flags(np, "ingenic,tp-en-gpio", 0, &flags);
	if(gpio_is_valid(panel->tp_en.gpio)) {
		panel->tp_en.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->tp_en.gpio, GPIOF_DIR_OUT, "tp_en");
		if(ret < 0) {
			dev_err(dev, "Failed to request tp_en pin!\n");
			return ret;
		}
	} else {
		dev_warn(dev, "invalid gpio tp_en.gpio: %d\n", panel->tp_en.gpio);
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
/*
	panel->lcd_te.gpio = of_get_named_gpio_flags(np, "ingenic,lcd-te-gpio", 0, &flags);
	if(gpio_is_valid(panel->lcd_te.gpio)) {
		panel->lcd_te.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(panel->lcd_te.gpio, GPIOF_DIR_IN, "lcd_te");
		if(ret < 0) {
			dev_err(dev, "Failed to request lcd_te pin!\n");
			goto err_request_oled;
		}
		//lcd_te;
		my_dbg("lcd_te->gpio=%d\n", lcd_te->gpio);
		gpio_direction_input(lcd_te->gpio);

	} else {
		dev_warn(dev, "invalid gpio lcd_te.gpio: %d\n", panel->lcd_te.gpio);
	}
*/
//	panel->oled.gpio = of_get_named_gpio_flags(np, "ingenic,oled-gpio", 0, &flags);
//	if(gpio_is_valid(panel->oled.gpio)) {
//		panel->oled.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
//		ret = gpio_request_one(panel->oled.gpio, GPIOF_DIR_OUT, "oled");
//		if(ret < 0) {
//			dev_err(dev, "Failed to request oled pin!\n");
//			goto err_request_oled;
//		}
//	} else {
//		dev_warn(dev, "invalid gpio oled.gpio: %d\n", panel->oled.gpio);
//	}

//	panel->lcd_pwm.gpio = of_get_named_gpio_flags(np, "ingenic,lcd-pwm-gpio", 0, &flags);
//	if(gpio_is_valid(panel->lcd_pwm.gpio)) {
//		panel->lcd_pwm.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
//		ret = gpio_request_one(panel->lcd_pwm.gpio, GPIOF_DIR_OUT, "lcd-pwm");
//		if(ret < 0) {
//			dev_err(dev, "Failed to request lcd-pwm pin!\n");
//			goto err_request_pwm;
//		}
//	} else {
//		dev_warn(dev, "invalid gpio lcd-pwm.gpio: %d\n", panel->lcd_pwm.gpio);
//	}
//
//	panel->swire.gpio = of_get_named_gpio_flags(np, "ingenic,swire-gpio", 0, &flags);
//	if(gpio_is_valid(panel->swire.gpio)) {
//		panel->swire.active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
//		ret = gpio_request_one(panel->swire.gpio, GPIOF_DIR_OUT, "swire");
//		if(ret < 0) {
//			dev_err(dev, "Failed to request swire pin!\n");
//			goto err_request_swire;
//		}
//	} else {
//		dev_warn(dev, "invalid gpio swire.gpio: %d\n", panel->swire.gpio);
//	}
//
	return 0;
err_request_swire:
	if(gpio_is_valid(panel->lcd_pwm.gpio))
		gpio_free(panel->lcd_pwm.gpio);
err_request_pwm:
	if(gpio_is_valid(panel->oled.gpio))
		gpio_free(panel->oled.gpio);
err_request_oled:
	if(gpio_is_valid(panel->rst.gpio))
		gpio_free(panel->rst.gpio);
err_request_rst:
	if(gpio_is_valid(panel->vdd_en.gpio))
		gpio_free(panel->vdd_en.gpio);
	return ret;
}

static int panel_dev_probe(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct rm690a0 *lcd;
	ENTER();
	lcd = devm_kzalloc(&dsim_dev->dev, sizeof(struct rm690a0), GFP_KERNEL);
	if (!lcd)
	{
		dev_err(&dsim_dev->dev, "failed to allocate Raydium_rm690a0 structure.\n");
		return -ENOMEM;
	}

	lcd->dsim_dev = dsim_dev;
	lcd->dev = &dsim_dev->dev;
	my_dbg("lcd_device_register");
	lcd->ld = lcd_device_register("Raydium_rm690a0", lcd->dev, lcd,
	                              &panel_lcd_ops);
	if (IS_ERR(lcd->ld))
	{
		dev_err(lcd->dev, "failed to register lcd ops.\n");
		return PTR_ERR(lcd->ld);
	}

	dev_set_drvdata(&dsim_dev->dev, lcd);
	dev_info(lcd->dev, "probed Raydium_rm690a0 panel driver.\n");
	panel->dsim_dev = dsim_dev;

	return 0;

}

static int panel_suspend(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct board_gpio *vdd_en = &panel->vdd_en;

	ENTER();
	panel_dev_display_off(panel);
	panel_dev_sleep_in(panel);
	gpio_direction_output(vdd_en->gpio, !vdd_en->active_level);

	return 0;
}

static int panel_resume(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct board_gpio *vdd_en = &panel->vdd_en;

	ENTER();
	gpio_direction_output(vdd_en->gpio, vdd_en->active_level);

	return 0;
}

static struct mipi_dsim_lcd_driver panel_dev_dsim_ddi_driver = {
	.name = "Raydium_rm690a0-lcd",
	.id = -1,

	.power_on = panel_dev_power_on,
	.set_sequence = panel_dev_set_sequence,
	.probe = panel_dev_probe,
	.suspend = panel_suspend,
	.resume = panel_resume,
};


struct mipi_dsim_lcd_device panel_dev_device={
	.name		= "Raydium_rm690a0-lcd",
	.id = 0,
};

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
	ENTER();

	panel = kzalloc(sizeof(struct panel_dev), GFP_KERNEL);
	if(panel == NULL) {
		dev_err(&pdev->dev, "Faile to alloc memory!");
		return -ENOMEM;
	}
	panel->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, panel);

	my_dbg();
	ret = of_panel_parse(&pdev->dev);
	if(ret < 0) {
		goto err_of_parse;
	}
	my_dbg();
	mipi_dsi_register_lcd_device(&panel_dev_device);
	my_dbg();
	mipi_dsi_register_lcd_driver(&panel_dev_dsim_ddi_driver);

	my_dbg();
	ret = ingenicfb_register_panel(&lcd_panel);
	if(ret < 0) {
		goto err_of_parse;
	}

	ENTER();
	return 0;

err_of_parse:
	dev_err(&pdev->dev, "Failed to register lcd panel RM690A0!\n");
	kfree(panel);
	return ret;
}

static int panel_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id panel_of_match[] = {
	{ .compatible = "ingenic,rm690a0", },
	{},
};

static struct platform_driver panel_driver = {
	.probe		= panel_probe,
	.remove		= panel_remove,
	.driver		= {
		.name	= "rm690a0",
		.of_match_table = panel_of_match,
	},
};

module_platform_driver(panel_driver);
