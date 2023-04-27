/*
 * es8374_codec_v13.h  --  ES8374 Soc Audio driver
 *
 * Copyright (C) 2017 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 *
 * Author: dlzhang<daolin.zhang@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ES8374_H
#define _ES8374_H

#define USE_48000_SAMPLE_RATE		1
enum {
	/* Codec Register */
	DAC1_VOL = 0,
	DAC2_VOL,
	ADC_PGA_VOL,
	ADC_VOL,
	CODEC_POWER_CTL,
	ADC_INPUT_SELECT,
	CODEC_REGNUM = ADC_INPUT_SELECT,

	/* Audio Processing Chain */
	ORIGINAL_BASS,
	MAXX_BYPASS,
	MAXX_BASS_INTENSITY,
	MAXX_TREBLE_INTENSITY,
	MAXX_3D_INTENSITY,
	NPCA110P_REGNUM,
};

static unsigned char RegCommandsSetUpCodec[] =
{
	0x00,0x3F,//IC Rst start
//	DELAY_MS(1);//DELAY_MS
	0x00,0x03,//IC Rst stop
	0x01,0x7F,//IC clk on
	0x6F,0xA0,//pll set:mode enable
	0x72,0x41,//pll set:mode set
	0x09,0x01,//pll set:reset on ,set start
	0x0C,0x22,//pll set:k
	0x0D,0x2E,//pll set:k
	0x0E,0xC6,//pll set:k
	0x0A,0x3A,//pll set:
	0x0B,0x07,//pll set:n
	0x09,0x41,//pll set:reset off ,set stop
	0x24,0x08,//adc set
	0x36,0x00,//dac set
	0x12,0x30,//timming set
	0x13,0x20,//timming set
	0x21,0x50,//adc set: SEL LIN1 CH+PGAGAIN=0DB
	0x22,0xFF,//adc set: PGA GAIN=0DB
	0x21,0x10,//adc set: SEL LIN1 CH+PGAGAIN=0DB
	0x00,0x80,// IC START
//	DELAY_MS(50);//DELAY_MS
	0x14,0x8A,// IC START
	0x15,0x40,// IC START
	0x1A,0xA0,// monoout set
	0x1B,0x19,// monoout set
	0x1C,0x90,// spk set
	0x1D,0x02,// spk set
	0x1F,0x00,// spk set
	0x1E,0xA0,// spk on
	0x28,0x00,// alc set
	0x25,0x00,// ADCVOLUME on
	0x38,0x00,// DACVOLUMEL on
	0x37,0x00,// dac set
	0x6D,0x60,//SEL:GPIO1=DMIC CLK OUT+SEL:GPIO2=PLL CLK OUT
};
#endif
