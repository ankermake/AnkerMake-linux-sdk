#ifndef _X1021_SLCD_REGS_H_
#define _X1021_SLCD_REGS_H_

#define CHAIN_ADDR      0x0000
#define GLB_CFG         0x0004
#define CSC_MULT_YRV    0x0100
#define CSC_MULT_GUGV   0x0104 
#define CSC_MULT_BU     0x0108 
#define CSC_SUB_YUV     0x010C 
#define CTRL            0x1000 
#define ST              0x1004 
#define CSR             0x1008 
#define INTC            0x100C 
#define PCFG            0x1010 
#define INT_FLAG        0x101C
#define RGB_DMA_SITE    0x2004
#define Y_DMA_SITE      0x2008
#define CHAIN_SITE      0x2000
#define UV_DMA_SITE     0x200c
#define DES_READ        0x3000
#define DISP_COM        0x8000
#define SLCD_CFG        0xA000
#define SLCD_WR_DUTY    0xA004
#define SLCD_TIMING     0xA008
#define SLCD_FRM_SIZE   0xA00C
#define SLCD_SLOW_TIME  0xA010
#define SLCD_REG_IF     0xA014
#define SLCD_ST         0xA018

// GLB_CFG
#define GLB_CFG_COLOR       19, 21
#define GLB_CFG_FORMAT      16, 18
#define GLB_CFG_CLKGATE_CLS 3, 3
#define GLB_CFG_BURST_LEN   1, 1
#define GLB_CFG_DMA_SEL     0, 0

#define GLB_CFG_COLOR_RGB 0
#define GLB_CFG_COLOR_RBG 1
#define GLB_CFG_COLOR_GRB 2
#define GLB_CFG_COLOR_BRG 3
#define GLB_CFG_COLOR_GBR 4
#define GLB_CFG_COLOR_BGR 5

#define GLB_CFG_FORMAT_555 0
#define GLB_CFG_FORMAT_565 1
#define GLB_CFG_FORMAT_888 2
#define GLB_CFG_FORMAT_NV12 4
#define GLB_CFG_FORMAT_NV21 5

// CSC_MULT_YRV
#define CSC_MULT_RV 16, 26
#define CSC_MULT_Y  0, 10

// CSC_MULT_GUGV
#define CSC_MULT_GV 16, 25
#define CSC_MULT_GU 0, 8

// CSC_MULT_BU
#define CSC_MULT_BU_BU 0, 11

// CSC_SUB_YUV,
#define CSC_SUB_Y  0, 4
#define CSC_SUB_UV 16, 23

// CTRL
#define CTRL_SLCD_START  4, 4
#define CTRL_GEN_STOP    3, 3
#define CTRL_DES_CNT_RST 2, 2
#define CTRL_QCK_STOP    1, 1
#define CTRL_START       0, 0

// ST
#define ST_QCK_ACK 6, 6
#define ST_GEN_ACK 5, 5
#define ST_DMA_END 2, 2
#define ST_FRM_END 1, 1
#define ST_WORKIN  0, 0

// CSR
#define CSR_CLR_QCK_ACK 6, 6
#define CSR_CLR_GEN_ACK 5, 5
#define CSR_CLR_DMA_END 2, 2
#define CSR_CLR_FRM_END 1, 1

// INTC
#define INTC_QSA_MASK 6, 6
#define INTC_GSA_MASK 5, 5
#define INTC_EOD_MSK  2, 2
#define INTC_EOF_MSK  1, 1

// PCFG
#define Pcfg2 18, 26
#define Pcfg1 9, 17
#define Pcfg0 0, 8

// INT_FLAG
#define INT_QCK_ACK 6, 6
#define INT_GEN_ACK 5, 5
#define INT_DMA_END 2, 2
#define INT_FRM_END 1, 1

// DISP_COM
#define DISP_COM_DITHER_DW   16, 21
#define DISP_COM_DISP_CG_CLS 2, 2
#define DISP_COM_DITHER_EN   1, 1

// SLCD_CFG
#define SLCD_CFG_FRM_MD       31, 31
#define SLCD_CFG_RDY_ANTI_JIT 27, 27
#define SLCD_CFG_FMT_EN       26, 26
#define SLCD_CFG_DBI_TYPE     23, 25
#define SLCD_CFG_DATA_FMT     21, 22
#define SLCD_CFG_TE_ANTI_JIT  20, 20
#define SLCD_CFG_TE_MD        19, 19
#define SLCD_CFG_TE_SWITCH    18, 18
#define SLCD_CFG_RDY_SWITCH   17, 17
#define SLCD_CFG_CS_EN        16, 16
#define SLCD_CFG_CS_DP        11, 11
#define SLCD_CFG_RDY_DP       10, 10
#define SLCD_CFG_DC_MD        9, 9
#define SLCD_CFG_WR_MD        8, 8
#define SLCD_CFG_CLKPLY       7, 7
#define SLCD_CFG_TE_DP        6, 6
#define SLCD_CFG_DWIDTH       3, 5
#define SLCD_CFG_CWIDTH       0, 2

// SLCD_WR_DUTY
#define SLCD_WR_DUTY_DSTIME 24, 31
#define SLCD_WR_DUTY_DDTIME 16, 23
#define SLCD_WR_DUTY_CSTIME 8, 15
#define SLCD_WR_DUTY_CDTIME 0, 7

// SLCD_TIMING
#define SLCD_TIMING_TCH 24, 31
#define SLCD_TIMING_TCS 16, 23
#define SLCD_TIMING_TAH 8, 15
#define SLCD_TIMING_TAS 0, 7

// SLCD_FRM_SIZE
#define SLCD_FRM_SIZE_V_SIZE 16, 31
#define SLCD_FRM_SIZE_H_SIZE 0, 15

// SLCD_SLOW_TIME
#define SLCD_SLOW_TIME_SLOW_TIME 0, 15

// SLCD_REG_IF
#define SLCD_REG_IF_FLAG    30, 31
#define SLCD_REG_IF_CMD_END 29, 29
#define SLCD_REG_IF_CONTENT 0, 23

// SLCD_ST
#define SLCD_ST_BUSY 0, 0

#endif /* _X1021_SLCD_REGS_H_ */