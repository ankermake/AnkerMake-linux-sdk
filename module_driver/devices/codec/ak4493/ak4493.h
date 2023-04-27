/*
 * ak4493.h  --  audio driver for ak4493
 *
 * Copyright (C) 2018 Asahi Kasei Microdevices Corporation
 *  Author                Date        Revision
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                      2018/02/07       1.0
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */


#define SND_SOC_DAIFMT_DSD           0x101

#ifndef _AK4493_H
#define _AK4493_H

#define AK4493_00_CONTROL1          0x00
#define AK4493_01_CONTROL2          0x01
#define AK4493_02_CONTROL3          0x02
#define AK4493_03_LCHATT            0x03
#define AK4493_04_RCHATT            0x04
#define AK4493_05_CONTROL4          0x05
#define AK4493_06_DSD1              0x06
#define AK4493_07_CONTROL5          0x07
#define AK4493_08_SOUNDCONTROL      0x08
#define AK4493_09_DSD2              0x09
#define AK4493_0A_CONTROL7          0x0A
#define AK4493_0B_CONTROL8          0x0B
#define AK4493_0C_RESERVED          0x0C
#define AK4493_0D_RESERVED          0x0D
#define AK4493_0E_RESERVED          0x0E
#define AK4493_0F_RESERVED          0x0F
#define AK4493_10_RESERVED          0x10
#define AK4493_11_RESERVED          0x11
#define AK4493_12_RESERVED          0x12
#define AK4493_13_RESERVED          0x13
#define AK4493_14_RESERVED          0x14
#define AK4493_15_DFSREAD           0x15

/* AK4493_00_CONTROL1 (0x00) D0 bit */
#define AK4493_RSTN_MASK    GENMASK(0, 0)
#define AK4493_RSTN        (0x1 << 0)

#define AK4493_MAX_REGISTERS    (AK4493_15_DFSREAD + 1)

/* Bitfield Definitions */

/* AK4493_00_CONTROL1 (0x00) Fields */
#define AK4493_DIF                    0x0E
#define AK4493_DIF_MSB_MODE        (2 << 1)
#define AK4493_DIF_I2S_MODE     (3 << 1)
#define AK4493_DIF_32BIT_MODE    (4 << 1)

/* AK4493_02_CONTROL3 (0x02) Fields */
#define AK4493_DIF_DSD                0x80
#define AK4493_DIF_DSD_MODE        (1 << 7)


/* AK4493_01_CONTROL2 (0x01) Fields */
/* AK4493_05_CONTROL4 (0x05) Fields */
#define AK4493_DFS                0x18
#define AK4493_DFS_48KHZ        (0x0 << 3)  //  30kHz to 54kHz
#define AK4493_DFS_96KHZ        (0x1 << 3)  //  54kHz to 108kHz
#define AK4493_DFS_192KHZ        (0x2 << 3)  //  120kHz  to 216kHz
#define AK4493_DFS_384KHZ        (0x0 << 3)
#define AK4493_DFS_768KHZ        (0x1 << 3)

#define AK4493_DFS2                0x2
#define AK4493_DFS2_48KHZ        (0x0 << 1)  //  30kHz to 216kHz
#define AK4493_DFS2_384KHZ        (0x1 << 1)  //  384kHz, 768kHkHz to 108kHz

#endif
