#ifndef _DPU_REGS_H_
#define _DPU_REGS_H_

#define FRM_CFG_ADDR   0x0000
#define FRM_CFG_CTRL   0x0004
#define SRD_CHAIN_ADDR 0x1000
#define SRD_CHAIN_CTRL 0x1004
#define CTRL           0x2000
#define ST             0x2004
#define CLR_ST         0x2008
#define INTC           0x200C
#define INT_FLAG       0x2010
#define COM_CFG        0x2014
#define PCFG_RD_CTRL   0x2018
#define PCFG_WB_CTRL   0x201C
#define PCFG_OFIFO     0x2020
#define PCFG_WDMA      0x2024
#define PCFG_CMPW      0x2028
#define PCFG0_CMPW     0x202C
#define PCFG1_CMPW     0x2030
#define PCFG2_CMPW     0x2034

#define FRM_DES            0x2100
#define LAY0_DES_READ      0x2104
#define LAY1_DES_READ      0x2108
#define LAY2_DES_READ      0x210c
#define LAY3_DES_READ      0x2110
#define RDMA_DES_READ      0x2104
#define FRM_CHAIN_SITE     0x2200
#define RDMA_CHAIN_SITE    0x2204
#define LAY0_SITE          0x3100
#define LAY1_SITE          0x3104
#define LAY2_SITE          0x3108
#define LAY3_SITE          0x310c
#define RDMA_SITE          0x3110
#define WDMA_SITE          0x221c

#define TLB_GLBC          0x3000
#define TLB_TLBA          0x3010
#define TLB_TLBC          0x3020
#define TLB0_VPN          0x3030
#define TLB1_VPN          0x3034
#define TLB2_VPN          0x3038
#define TLB3_VPN          0x303C
#define TLB_TLBV          0x3040
#define TLB_STAT          0x3050

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
#define LAY2_CSC_MULT_YRV  0x3220
#define LAY2_CSC_MULT_GUGV 0x3224
#define LAY2_CSC_MULT_BU   0x3228
#define LAY2_CSC_SUB_YUV   0x322C
#define LAY3_CSC_MULT_YRV  0x3230
#define LAY3_CSC_MULT_GUGV 0x3234
#define LAY3_CSC_MULT_BU   0x3238
#define LAY3_CSC_SUB_YUV   0x323C

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

#define f_WB_Dither_DW 20, 25
#define f_WB_Format 16, 18
#define f_WB_DitherAuto 5, 5
#define f_WB_DitherEn 4, 4
#define f_Change2RDMA 3, 3
#define f_DirectEn 2, 2
#define f_WriteBack 1, 1
#define f_stop 0, 0

#define f_layer3order 14, 15
#define f_layer2order 12, 13
#define f_layer1order 10, 11
#define f_layer0order 8, 9
#define f_layer3En 7, 7
#define f_layer2En 6, 6
#define f_layer1En 5, 5
#define f_layer0En 4, 4
#define f_lay3ScaleEn 3, 3
#define f_lay2ScaleEn 2, 2
#define f_lay1ScaleEn 1, 1
#define f_lay0ScaleEn 0, 0

#define f_EOD_MSK 17, 17
#define f_EOW_MSK 15, 15
#define f_SOC_MSK 14, 14
#define f_EOC_MSK 9, 9

#define l_Height 16, 27
#define l_Width 0, 11

#define l_SHARPL 20, 21
#define l_Format 16, 19
#define l_PREMULT 14, 14
#define l_GAlpha_en 13, 13
#define l_Color 10, 12
#define l_GAlpha 0, 7

#define l_TargetHeight 16, 27
#define l_TargetWidth 0, 11

#define l_YPos 16, 27
#define l_XPos 0, 11

#define s_Format 19, 22
#define s_Color 16, 18
#define s_Change2Comp 1, 1
#define s_CHAIN_END 0, 0

#define s_EOD_MSK 17, 17
#define s_SOS_MSK 2, 2
#define s_EOS_MSK 1, 1

#define CHANGE_2_SRD 1, 1
#define FRM_CFG_START 0, 0

#define SRD_CHAIN_START 0, 0

#define TFT_START 6, 6
#define SLCD_START 5, 5
#define GEN_STP_SRD 4, 4
#define GEN_STP_CMP 3, 3
#define DES_CNT_RST 2, 2
#define QCK_STP_SRD 1, 1
#define QCK_STP_CMP 0, 0

#define CMP_W_SLOW 31, 31
#define DISP_END 17, 17
#define WDMA_OVER 16, 16
#define WDMA_END 15, 15
#define CMP_START 14, 14
#define LAY3_END 13, 13
#define LAY2_END 12, 12
#define LAY1_END 11, 11
#define LAY0_END 10, 10
#define CMP_END 9, 9
#define TFT_UNDR 8, 8
#define STOP_SRD_ACK 7, 7
#define STOP_CMP_ACK 6, 6
#define WRBK_WORKING 5, 5
#define DIRECET_WORKING 4, 4
#define SRD_WORKING 3, 3
#define SRD_START 2, 2
#define SRD_END 1, 1
#define WORKING 0, 0

#define CLR_CMP_W_SLOW 31, 31
#define CLR_DISP_END 17, 17
#define CLR_WDMA_OVER 16, 16
#define CLR_WDMA_END 15, 15
#define CLR_CMP_START 14, 14
#define CLR_LAY3_END 13, 13
#define CLR_LAY2_END 12, 12
#define CLR_LAY1_END 11, 11
#define CLR_LAY0_END 10, 10
#define CLR_CMP_END 9, 9
#define CLR_TFT_UNDR 8, 8
#define CLR_STOP_SRD_ACK 7, 7
#define CLR_STOP_CMP_ACK 6, 6
#define CLR_SRD_START 2, 2
#define CLR_SRD_END 1, 1

#define CWS_MSK 31, 31
#define EOD_MSK 17, 17
#define OOW_MSK 16, 16
#define EOW_MSK 15, 15
#define SOC_MSK 14, 14
#define EOL3_MSK 13, 13
#define EOL2_MSK 12, 12
#define EOL1_MSK 11, 11
#define EOL0_MSK 10, 10
#define EOC_MSK 9, 9
#define UOT_MSK 8, 8
#define SSA_MSK 7, 7
#define SCA_MSK 6, 6
#define SOS_MSK 2, 2
#define EOS_MSK 1, 1


#define INT_CWS 31, 31
#define INT_EOD 17, 17
#define INT_OOW 16, 16
#define INT_EOW 15, 15
#define INT_SOC 14, 14
#define INT_EOL3 13, 13
#define INT_EOL2 12, 12
#define INT_EOL1 11, 11
#define INT_EOL0 10, 10
#define INT_EOC 9, 9
#define INT_UOT 8, 8
#define INT_SSA 7, 7
#define INT_SCA 6, 6
#define INT_SOS 2, 2
#define INT_EOS 1, 1

#define SCALE1_CLKGATE_EN 21, 21
#define SCALE0_CLKGATE_EN 20, 20
#define LAYER3_CLKGATE_EN 19, 19
#define LAYER2_CLKGATE_EN 18, 18
#define LAYER1_CLKGATE_EN 17, 17
#define LAYER0_CLKGATE_EN 16, 16
#define BURST_LEN_WDMA 6, 7
#define BURST_LEN_RDMA 4, 5
#define BURST_LEN_BDMA 2, 3
#define CH_SEL 1, 1

#define Arqos_val 1, 2
#define Arqos_ctrl 0, 0

#define Pcfg2 18, 26
#define Pcfg1 9, 17
#define Pcfg0 0, 8

#define wrbk_frm_rate_ctrl 10, 10
#define num_mclk 0, 9

#define Pcfg_count 0, 20
#define Pcfg_cycle 21, 24


#define CSC_MULT_RV 16, 26
#define CSC_MULT_Y 0, 10

#define CSC_MULT_GV 16, 25
#define CSC_MULT_GU 0, 8

#define CSC_MULT_BU 0, 11

#define CSC_SUB_UV 16, 23
#define CSC_SUB_Y 0, 4

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
#define SYNC_DL 8, 8
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
#define TE_SWITCH_MIPI 7,7
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

#endif /* _DPU_REGS_H_ */
