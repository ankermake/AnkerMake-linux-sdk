/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * ISP register description
 *
 */
#ifndef __X2000_ISP_REGS_H__
#define __X2000_ISP_REGS_H__


/*
 * ISP register base address
 */
#define ISP0_IOBASE                     0x13700000
#define ISP1_IOBASE                     0x13800000


#ifndef TIZIANO_BASE
#define TIZIANO_BASE                    0x0
#endif

#ifndef TIZIANO_MAN
#define TIZIANO_MAN                     3
#endif

/*
 * ISP Base Address
 */
#define TOP_BASE                        (TIZIANO_BASE + 0x00000)
#define IP_BASE                         (TIZIANO_BASE + 0x00100)
#define DPC_BASE                        (TIZIANO_BASE + 0x00200)
#define GIB_BASE                        (TIZIANO_BASE + 0x00400)
#define LSC_BASE                        (TIZIANO_BASE + 0x00500)
#define AG_BASE                         (TIZIANO_BASE + 0x00600)
#define AE_BASE                         (TIZIANO_BASE + 0x00700)
#define AWB_BASE                        (TIZIANO_BASE + 0x00800)
#define AF_BASE                         (TIZIANO_BASE + 0x00900)
#define ADR_BASE                        (TIZIANO_BASE + 0x00A00)
#define IRINT_BASE                      (TIZIANO_BASE + 0x00F00)
#define DMSC_BASE                       (TIZIANO_BASE + 0x01000)
#define CCM_BASE                        (TIZIANO_BASE + 0x01200)
#define DEFOG_BASE                      (TIZIANO_BASE + 0x01300)
#define CSC_BASE                        (TIZIANO_BASE + 0x01700)
#define CLM_BASE                        (TIZIANO_BASE + 0x01800)
#define SP_BASE                         (TIZIANO_BASE + 0x01900)
#define MDNS_BASE                       (TIZIANO_BASE + 0x01B00)
#define SDNS_BASE                       (TIZIANO_BASE + 0x02000)
#define HLDC_BASE                       (TIZIANO_BASE + 0x02200)
//#define MSCA_BASE                       (TIZIANO_BASE + 0x02300)

#define DPC_MEM_BASE                    (TIZIANO_BASE + 0x04000)
#define LSC_MEM_BASE                    (TIZIANO_BASE + 0x06000)
#define GIB_MEM_BASE                    (TIZIANO_BASE + 0x08000)
#define AE_HIST_MEM_BASE                (TIZIANO_BASE + 0x08200)
#define R_GAMMA_MEM_BASE                (TIZIANO_BASE + 0x08A00)
#define G_GAMMA_MEM_BASE                (TIZIANO_BASE + 0x08C00)
#define B_GAMMA_MEM_BASE                (TIZIANO_BASE + 0x08E00)
#define DEFOG_MEM_BASE                  (TIZIANO_BASE + 0x09000)
#define CLM_H_0_MEM_BASE                (TIZIANO_BASE + 0x0A000)
#define CLM_H_1_MEM_BASE                (TIZIANO_BASE + 0x0A800)
#define CLM_S_0_MEM_BASE                (TIZIANO_BASE + 0x0B000)
#define CLM_S_1_MEM_BASE                (TIZIANO_BASE + 0x0B800)


/*
 * ISP controller
 */

#define DPC                             (0 << 0)
#define GIB                             (0 << 1)
#define LSC                             (0 << 2)
#define AWB                             (0 << 3)
#define ADR                             (1 << 4)
#define DMS                             (0 << 5)
#define CCM                             (0 << 6)
#define GAMMA                           (0 << 7)
#define DEFOG                           (1 << 8)
#define CLM                             (0 << 9)
#define Y_SHARPEN                       (0 << 10)
#define MDNS                            (0 << 11)
#define SDNS                            (0 << 12)
#define HLDC                            (1 << 13)
#define TP                              (1 << 14)
#define FONT                            (1 << 15)
#define SEL                             (0 << 31)
#define TIZIANO_BYPASS_INIT             (DPC|GIB|LSC|AWB|ADR|DMS|CCM|GAMMA|DEFOG|CLM|Y_SHARPEN|MDNS|SDNS|HLDC|TP|FONT|SEL)

#define TIZIANO_FM_SIZE                 (TOP_BASE + 0X0004)
#define TIZIANO_BATER_TYPE              (TOP_BASE + 0X0008)
#define TIZAINO_BYPASS_CON              (TOP_BASE + 0X000C)
#define TIZIANO_TOP_CON                 (TOP_BASE + 0X0010)
#define TIZIANO_REG_CON                 (TOP_BASE + 0X001C)
#define TIZAINO_INT_EN                  (TOP_BASE + 0X0080)

/*
 * TOP_CTRL_ADDR Register
 */

/*
 * TOP_CTRL_ADDR_INT_EN 各位描述
 *
 * [31]  MDNS_FRA_DONE_INT_EN
 * [30]  Y_SP_FRA_DONE_INT_EN
 * [29]  CLM_FRA_DONE_INT_EN
 * [28]  DEFOG_FRA_DONE_INT_EN
 * [27]  ADR_FRA_DONE_INT_EN
 * [26]  LSC_FRA_DONE_INT_EN
 * [25]  GIB_FRA_DONE_INT_EN
 * [24]  DPC_FRA_DONE_INT_EN
 * [23]  reserved
 * [22]  reserved
 * [21]  DEFOG_STATIC_INT_EN
 * [20]  ADR_STATIC_INT_EN
 * [19]  AF_STATIC_INT_EN
 * [18]  AWB_STATIC_INT_EN
 * [17]  AE_HIST_INT_EN
 * [16]  AE_STATIC_INT_EN
 * [15]  IP_FRA_START_INT_EN
 * [14]  SDNS_FRA_DONE_INT_EN
 * [13]  DMSC_FRA_DONE_INT_EN
 * [12]  IP_FRA_DONE_INT_EN
 * [11]  DMA_REG_CONFIG_DONE_ONT_EN
 * [10]  STOP_FINISH_INT_EN
 * [9]   IP_OF_ERR_INT_EN
 * [8]   BROKE_FRAME_INT_EN
 * [7]   STATIC_DMA_ERR_INT_EN
 * [6]   FRA_START_ERR_INT_EN
 * [5]   CH2_CROPSIZE_ERR_EN
 * [4]   CH1_CROPSIZE_ERR_EN
 * [3]   CH0_CROPSIZE_ERR_EN
 * [2]   CH2_FRM_DONE_INT_EN
 * [1]   CH1_FRM_DONE_INT_EN
 * [0]   CH0_FRM_DONE_INT_EN
 */
#define TOP_CTRL_ADDR_VERSION           (TOP_BASE + 0x0000)
#define TOP_CTRL_ADDR_FM_SIZE           (TOP_BASE + 0x0004)
#define TOP_CTRL_ADDR_BAYER_TYPE        (TOP_BASE + 0x0008)
#define TOP_CTRL_ADDR_BYPASS_CON        (TOP_BASE + 0x000C)
#define TOP_CTRL_ADDR_TOP_CON           (TOP_BASE + 0x0010)
#define TOP_CTRL_ADDR_TOP_STATE         (TOP_BASE + 0x0014)
#define TOP_CTRL_ADDR_LINE_SPACE        (TOP_BASE + 0x0018)
#define TOP_CTRL_ADDR_REG_CON           (TOP_BASE + 0x001C)
#define TOP_CTRL_ADDR_DMA_RC_TRIG       (TOP_BASE + 0x0030)
#define TOP_CTRL_ADDR_DMA_RC_CON        (TOP_BASE + 0x0034)
#define TOP_CTRL_ADDR_DMA_RC_ADDR       (TOP_BASE + 0x0038)
#define TOP_CTRL_ADDR_DMA_RC_STATE          (TOP_BASE + 0x003C)
#define TOP_CTRL_ADDR_DMA_RC_APB_WR_DATA    (TOP_BASE + 0x0040)
#define TOP_CTRL_ADDR_DMA_RC_APB_WR_ADDR    (TOP_BASE + 0x0044)
#define TOP_CTRL_ADDR_DMA_RD_CON            (TOP_BASE + 0x0050)
#define TOP_CTRL_ADDR_DMA_WR_CON            (TOP_BASE + 0x0054)
#define TOP_CTRL_ADDR_DMA_FR_WR_CON         (TOP_BASE + 0x0058)
#define TOP_CTRL_ADDR_DMA_STA_WR_CON        (TOP_BASE + 0x005C)
#define TOP_CTRL_ADDR_DMA_RD_DEBUG          (TOP_BASE + 0x0060)
#define TOP_CTRL_ADDR_DMA_WR_DEBUG          (TOP_BASE + 0x0064)
#define TOP_CTRL_ADDR_DMA_FR_WR_DEBUG       (TOP_BASE + 0x0068)
#define TOP_CTRL_ADDR_DMA_STA_WR_DEBUG      (TOP_BASE + 0x006C)
#define TOP_CTRL_ADDR_INT_EN                (TOP_BASE + 0x0080)
#define TOP_CTRL_ADDR_INT_REG               (TOP_BASE + 0x0084)
#define TOP_CTRL_ADDR_INT_CLR               (TOP_BASE + 0x0088)
#define TOP_CTRL_ADDR_TP_FREERUN            (TOP_BASE + 0x00A0)
#define TOP_CTRL_ADDR_TP_CON                (TOP_BASE + 0x00A4)
#define TOP_CTRL_ADDR_TP_SIZE               (TOP_BASE + 0x00A8)
#define TOP_CTRL_ADDR_TP_FONT               (TOP_BASE + 0x00AC)
#define TOP_CTRL_ADDR_TP_FLICK              (TOP_BASE + 0x00B0)
#define TOP_CTRL_ADDR_TP_CS_TYPE            (TOP_BASE + 0x00B4)
#define TOP_CTRL_ADDR_TP_CS_FCLO            (TOP_BASE + 0x00B8)
#define TOP_CTRL_ADDR_TP_CS_BCLO            (TOP_BASE + 0x00BC)

/*
 *  Input Interface
 */
#define INPUT_CTRL_ADDR_IP_TRIG         (IP_BASE + 0x0000)
#define INPUT_CTRL_ADDR_CONTROL         (IP_BASE + 0x0004)
#define INPUT_CTRL_ADDR_CHK_TRIG        (IP_BASE + 0x0008)
#define INPUT_CTRL_ADDR_CHK_TIME        (IP_BASE + 0x000C)
#define INPUT_CTRL_ADDR_CHK_CNT         (IP_BASE + 0x0010)
#define INPUT_CTRL_ADDR_CHK_FR_CNT      (IP_BASE + 0x0014)
#define INPUT_CTRL_ADDR_IP_STATE        (IP_BASE + 0x0018)
#define INPUT_CTRL_ADDR_IN_CNT          (IP_BASE + 0x001C)
#define INPUT_CTRL_ADDR_OUT_CNT         (IP_BASE + 0x0020)


#endif /* __X2000_ISP_REGS_H__ */
