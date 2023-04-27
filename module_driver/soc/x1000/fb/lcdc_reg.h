#ifndef _LCDC_X1000_REGS_H_
#define _LCDC_X1000_REGS_H_

#define LCDCFG      0x0000
#define LCDCTRL     0x0030
#define LCDSTATE    0x0034
#define LCDOSDC     0x0100
#define LCDOSDCTRL  0x0104
#define LCDOSDS     0x0108
#define LCDBGC0     0x010c
#define LCDBGC1     0x02c4
#define LCDKEY0     0x0110
#define LCDKEY1     0x0114
#define LCDALPHA    0x0118
#define LCDRGBC     0x0090
#define LCDVAT      0x000c
#define LCDDAH      0x0010
#define LCDDAV      0x0014
#define LCDXYP0     0x0120
#define LCDXYP1     0x0124
#define LCDSIZE0    0x0128
#define LCDSIZE1    0x012C
#define LCDVSYNC    0x0004
#define LCDHSYNC    0x0008
#define LCDIID      0x0038
#define LCDDA0      0x0040
#define LCDSA0      0x0044
#define LCDFID0     0x0048
#define LCDCMD0     0x004C
#define LCDOFFS0    0x0060
#define LCDPW0      0x0064
#define LCDCNUM0    0x0068
#define LCDPOS0     0x0068
#define LCDDESSIZE0 0x006C
#define LCDDA1      0x0050
#define LCDSA1      0x0054
#define LCDFID1     0x0058
#define LCDCMD1     0x005C
#define LCDOFFS1    0x0070
#define LCDPW1      0x0074
#define LCDCNUM1    0x0078
#define LCDPOS1     0x0078
#define LCDDESSIZE1 0x007C
#define LCDPCFG     0x02C0
#define MCFG        0x00A0
#define MCFG_NEW    0x00B8
#define MCTRL       0x00A4
#define MSTATE      0x00A8
#define MDATA       0x00AC
#define WTIME       0x00B0
#define TASH        0x00B4
#define SMWT        0x00BC

#define LCDCFG_LCDPIN    31, 31    /* LCD pins selection 1: SLCD */
#define LCDCFG_NEWDES    28, 28    /* use new descripter. 0: 4words, 1:8words */
#define LCDCFG_PALBP     27, 27    /* bypass data format and alpha blending */
#define LCDCFG_RECOVER   25, 25    /* Auto recover when output fifo underrun */
#define LCDCFG_MODE_TFT  6, 7  /* 0: 16BIT 1: 24BIT 2:18BIT */
#define LCDCFG_MODE    0, 3    /* Display Device Mode Select 13: SLCD */

#define LCDCTRL_BST   28, 30
#define LCDCTRL_EOFM  13, 13
#define LCDCTRL_SOFM  12, 12
#define LCDCTRL_IFUM0 10, 10
#define LCDCTRL_QDM   7, 7
#define LCDCTRL_BEDN  6, 6
#define LCDCTRL_PEDN  5, 5
#define LCDCTRL_ENA   3, 3
#define LCDCTRL_BPP0  0, 2

#define LCDSTATE_QD 7, 7
#define LCDSTATE_EOF 5, 5
#define LCDSTATE_SQF 4, 4
#define LCDSTATE_IFU0 2, 2

#define LCDXYP_YPOS 16, 27
#define LCDXYP_XPOS 0, 11

#define LCDSIZE_Height 16, 27
#define LCDSIZE_Width  0, 11

#define LCDCMD_SOFINT 31, 31
#define LCDCMD_EOFINT 30, 30
#define LCDCMD_CMD 29, 29
#define LCDCMD_FRM_EN 26, 26
#define LCDCMD_LEN 0, 23

#define LCDCPOS_RGB0 30, 30
#define LCDCPOS_BPP0 27, 29
#define LCDCPOS_YPOS0 12, 23
#define LCDCPOS_XPOS0 0, 11

#define LCDDESSIZE_Height 12, 23
#define LCDDESSIZE_Width 0, 11

#define LCDPCFG_Lcd_pri_md 31, 31
#define LCDPCFG_HP_BST 28, 30
#define LCDPCFG_Pcfg2 18, 26
#define LCDPCFG_Pcfg1 9, 17
#define LCDPCFG_Pcfg0 0, 8

#define LCD_PCFG_ARB_Pri_lcd_en 2, 2
#define LCD_PCFG_ARB_Pri_lcd 1, 1

/* RGB Control Register */
#define LCDC_RGBC_RGBDM      15, 15    /* enable RGB Dummy data */
#define LCDC_RGBC_DMM        14, 14    /* RGB Dummy mode */
#define LCDC_RGBC_422        8, 8    /* Change 444 to 422 */
#define LCDC_RGBC_RGBFMT     7, 7    /* RGB format enable */
#define LCDC_RGBC_ODDRGB     4, 6    /* odd line serial RGB data arrangement */
#define LCDC_RGBC_ODD_RGB    0    /* RGB */
#define LCDC_RGBC_ODD_RBG    1    /* RBG */
#define LCDC_RGBC_ODD_GRB    2    /* GRB */
#define LCDC_RGBC_ODD_GBR    3    /* GBR */
#define LCDC_RGBC_ODD_BRG    4    /* BRG */
#define LCDC_RGBC_ODD_BGR    5    /* BGR */
#define LCDC_RGBC_EVENRGB    0, 2    /* even line serial RGB data arrangement */
#define LCDC_RGBC_EVEN_RGB   0    /* RGB */
#define LCDC_RGBC_EVEN_RBG   1    /* RBG */
#define LCDC_RGBC_EVEN_GRB   2    /* GRB */
#define LCDC_RGBC_EVEN_GBR   3    /* GBR */
#define LCDC_RGBC_EVEN_BRG   4    /* BRG */
#define LCDC_RGBC_EVEN_BGR   5    /* BGR */

#define MCFG_CWIDTH 8, 9

#define MCFG_NEW_DWIDTH_NEW 13, 15
#define MCFG_NEW_6800_md 11, 11
#define MCFG_NEW_cmd_9bit 10, 10
#define MCFG_NEW_DTIMES_NEW 8, 9
#define MCFG_NEW_CSPLY_NEW 5, 5
#define MCFG_NEW_RSPLY_NEW 4, 4
#define MCFG_NEW_CLKPLY_NEW 3, 3
#define MCFG_NEW_DTYPE_NEW 2, 2
#define MCFG_NEW_CTYPE_NEW 1, 1
#define MCFG_NEW_FMT_CONV 0, 0

#define MCTRL_NARROW_TE 10, 10
#define MCTRL_TE_INV 9, 9
#define MCTRL_NOT_USE_TE 8, 8
#define MCTRL_DCSI_SEL 7, 7
#define MCTRL_MIPI_SLCD 6, 6
#define MCTRL_FAST_MODE 4, 4
#define MCTRL_GATE_MASK 3, 3
#define MCTRL_DMAMODE 2, 2
#define MCTRL_DMASTART 1, 1
#define MCTRL_DMATXEN 0, 0

#define MSTATE_LCD_ID 16, 31
#define MSTATE_BUSY 0, 0

#define MDATA_PTR 30, 31
#define MDATA_DATA_CMD 0, 23

#define WTIME_DHTIME 24, 31
#define WTIME_DLTIME 16, 23
#define WTIME_CHTIME 8, 15
#define WTIME_CLTIME 0, 7

#define TASH_TAH 8, 15
#define TASH_TAS 0, 7

#endif /* _LCDC_REGS_H_ */