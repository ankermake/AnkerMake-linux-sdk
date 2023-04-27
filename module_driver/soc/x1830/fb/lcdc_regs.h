#ifndef _LCDC_REGS_H_
#define _LCDC_REGS_H_

#define FRM_CFG_ADDR 0x0000
#define FRM_CFG_CTRL 0x0004
#define CTRL         0x2000
#define ST           0x2004
#define CLR_ST       0x2008
#define INTC         0x200C
#define INT_FLAG     0x2010
#define COM_CFG      0x2014
#define PCFG_RD_CTRL 0x2018
#define PCFG_OFIFO   0x2020

#define FRM_DES            0x2100
#define LAY0_DES_READ      0x2104
#define LAY1_DES_READ      0x2108
#define FRM_CHAIN_SITE     0x2200
#define LAY0_RGB_SITE      0x3100
#define LAY1_RGB_SITE      0x3104
#define LAY0_Y_SITE        0x3120
#define LAY0_UV_SITE       0x3124
#define LAY1_Y_SITE        0x3128
#define LAY1_UV_SITE       0x312C
#define LAY0_CSC_MULT_YRV  0x3200
#define LAY0_CSC_MULT_GUGV 0x3204
#define LAY0_CSC_MULT_BU   0x3208
#define LAY0_CSC_SUB_YUV   0x320C
#define LAY1_CSC_MULT_YRV  0x3210
#define LAY1_CSC_MULT_GUGV 0x3214
#define LAY1_CSC_MULT_BU   0x3218
#define LAY1_CSC_SUB_YUV   0x321C

#define DISP_COM 0x8000

#define TFT_HSYNC 0x9000
#define TFT_VSYNC 0x9004
#define TFT_HDE   0x9008
#define TFT_VDE   0x900C
#define TFT_CFG   0x9010
#define TFT_ST    0x9014

#define SLCD_CFG       0xA000
#define SLCD_WR_DUTY   0xA004
#define SLCD_TIMING    0xA008
#define SLCD_FRM_SIZE  0xA00C
#define SLCD_SLOW_TIME 0xA010
#define SLCD_REG_IF    0xA014
#define SLCD_ST        0xA018
#define SLCD_REG_CTRL  0xA01C

#define f_Height 16, 26
#define f_Width 0, 10

#define f_stop 0, 0

#define f_layer1order 10, 11
#define f_layer0order 8, 9
#define f_layer1En 5, 5
#define f_layer0En 4, 4

#define f_EOD_MSK 17, 17
#define f_SOF_MSK 2, 2
#define f_EOF_MSK 1, 1

#define l_Height 16, 27
#define l_Width 0, 11

#define l_Format 16, 19
#define l_PREMULT 14, 14
#define l_GAlpha_en 13, 13
#define l_Color 10, 12
#define l_GAlpha 0, 7

#define l_YPos 16, 27
#define l_XPos 0, 11

#define FRM_CFG_START 0, 0

#define TFT_START 6, 6
#define SLCD_START 5, 5
#define GEN_STP_DISP 3, 3
#define DES_CNT_RST 2, 2
#define QCK_STP_DISP 0, 0

#define CMP_END 23, 23
#define DISP_END 17, 17
#define BDMA1_END 11, 11
#define BDMA0_END 10, 10
#define BDMA_END 9, 9
#define TFT_UNDR 8, 8
#define STOP_DISP_ACK 6, 6
#define DIRECET_WORKING 4, 4
#define FRM_START 2, 2
#define FRM_END 1, 1
#define WORKING 0, 0

#define CLR_CMP_W_SLOW 31, 31
#define CLR_CMP_END 23, 23
#define CLR_DISP_END 17, 17
#define CLR_BDMA1_END 11, 11
#define CLR_BDMA0_END 10, 10
#define CLR_BDMA_END 9, 9
#define CLR_TFT_UNDR 8, 8
#define CLR_STP_DISP_ACK 6, 6
#define CLR_FRM_START 2, 2
#define CLR_FRM_END 1, 1

#define CWS_MSK 31, 31
#define EOC_MSK 23, 23
#define EOD_MSK 17, 17
#define EOB1_MSK 11, 11
#define EOB0_MSK 10, 10
#define EOB_MSK 9, 9
#define UOT_MSK 8, 8
#define SDA_MSK 6, 6
#define SOF_MSK 2, 2
#define EOF_MSK 1, 1

#define INT_CWS 31, 31
#define INT_EOC 23, 23
#define INT_EOD 17, 17
#define INT_EOB1 11, 11
#define INT_EOB0 10, 10
#define INT_EOB 9, 9
#define INT_UOT 8, 8
#define INT_SDA 6, 6
#define INT_SOF 2, 2
#define INT_EOF 1, 1

#define LAYER1_YUVDMA_4K 21, 21
#define LAYER0_YUVDMA_4K 20, 20
#define LAYER1_CLKGATE_EN 17, 17
#define LAYER0_CLKGATE_EN 16, 16
#define BURST_LEN_BDMA 2, 3

#define Arqos_val 1, 2
#define Arqos_ctrl 0, 0

#define Pcfg2 18, 26
#define Pcfg1 9, 17
#define Pcfg0 0, 8

#define CSC_MULT_RV 16, 26
#define CSC_MULT_Y 0, 10

#define CSC_MULT_GV 16, 25
#define CSC_MULT_GU 0, 8

#define CSC_MULT_BU 0, 11

#define CSC_SUB_UV 16, 23
#define CSC_SUB_Y 0, 4

#define DP_DITHER_DW 16, 21
#define CGC_DITHER 7, 7
#define CGC_SLCD 6, 6
#define CGC_TFT 5, 5
#define DP_DITHER_EN 4, 4
#define DP_IF_SEL 0, 1

#define HPS 16, 27
#define HPE 0, 11

#define VPS 16, 27
#define VPE 0, 11

#define HDS 16, 27
#define HDE 0, 11

#define VDS 16, 27
#define VDE 0, 11

#define PIX_CLK_INV 10, 10
#define DE_DL 9, 9
#define SYNC_DL 8, 8
#define COLOR_EVEN 5, 7
#define COLOR_ODD 2, 4
#define MODE 0, 1

#define TFT_WORKING 1, 1
#define TFT_UNDER 0, 0

#define FRM_MD 31, 31
#define RDY_ANTI_JIT 27, 27
#define FMT_EN 26, 26
#define DBI_TYPE 23, 25
#define PIX_FMT 21, 22
#define TE_ANTI_JIT 20, 20
#define TE_MD 19, 19
#define TE_SWITCH 18, 18
#define RDY_SWITCH 17, 17
#define CS_EN 16, 16
#define CS_DP 11, 11
#define RDY_DP 10, 10
#define DC_MD 9, 9
#define WR_MD 8, 8
#define TE_DP 6, 6
#define DWIDTH 3, 5
#define CWIDTH 0, 2

#define DSTIME 24, 31
#define DDTIME 16, 23
#define CSTIME 8, 15
#define CDTIME 0, 7

#define TCH 24, 31
#define TCS 16, 23
#define TAH 8, 15
#define TAS 0, 7

#define V_SIZE 16, 31
#define H_SIZE 0, 15

#define SLOW_TIME 0, 15

#define FLAG 30, 31
#define CMD_END 29, 29
#define CONTENT 0, 23

#define BUSY 0, 0

#define RST_3LINE 0, 0

#endif /* _LCDC_REGS_H_ */
