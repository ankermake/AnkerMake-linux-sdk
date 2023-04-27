#ifndef _LCDC_REGS_H_
#define _LCDC_REGS_H_

#define SRD_CHAIN_ADDR 0x1000
#define SRD_CHAIN_CTRL 0x1004
#define CTRL           0x2000
#define ST             0x2004
#define CLR_ST         0x2008
#define INTC           0x200C
#define INT_FLAG       0x2010
#define COM_CFG        0x2014
#define PCFG_RD_CTRL   0x2018
#define PCFG_OFIFO     0x2020

#define RDMA_DES           0x2114
#define RDMA_CHAIN_SITE    0x2204
#define RDMA_SITE          0x3110

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

#define SRD_CHAIN_START 0, 0

#define GEN_STP_SRD 4, 4
#define DES_CNT_RST 2, 2
#define QCK_STP_SRD 1, 1

#define DISP_END 17, 17
#define TFT_UNDR 8, 8
#define STOP_SRD_ACK 7, 7
#define SRD_WORKING 3, 3
#define SRD_START 2, 2
#define SRD_END 1, 1
#define WORKING 0, 0

#define CLR_DISP_END 17, 17
#define CLR_TFT_UNDR 8, 8
#define CLR_STOP_SRD_ACK 7, 7
#define CLR_SRD_START 2, 2
#define CLR_SRD_END 1, 1

#define EOD_MSK 17, 17
#define UOT_MSK 8, 8
#define SSA_MSK 7, 7
#define SOS_MSK 2, 2
#define EOS_MSK 1, 1

#define INT_EOD 17, 17
#define INT_UOT 8, 8
#define INT_SSA 7, 7
#define INT_SOS 2, 2
#define INT_EOS 1, 1

#define BURST_LEN_RDMA 4, 5

#define Arqos_val 1, 2
#define Arqos_ctrl 0, 0

#define Pcfg2 18, 26
#define Pcfg1 9, 17
#define Pcfg0 0, 8

#define DP_DITHER_DW 16, 21
#define DITHER_CLKGATE_EN 7, 7
#define SLCD_CLKGATE_EN 6, 6
#define TFT_CLKGATE_EN 5, 5
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

#define COLOR_EVEN 19, 21
#define COLOR_ODD 16, 18
#define PIX_CLK_INV 10, 10
#define DE_DL 9, 9
#define HSYNC_DL 8, 8
#define VSYNC_DL 7, 7
#define MODE 0, 2

#define TFT_WORKING 1, 1
#define TFT_UNDER 0, 0

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

#define s_Format 19, 22
#define s_Color 16, 18
#define s_CHAIN_END 0, 0

#define s_EOD_MSK 17, 17
#define s_SOS_MSK 2, 2
#define s_EOS_MSK 1, 1

#endif