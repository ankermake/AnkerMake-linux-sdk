/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Mulitiple Channel Scaler register description
 *
 */

#ifndef __X2000_MSCALER_REGS_H__
#define __X2000_MSCALER_REGS_H__

/*
 * MScaler register base address
 */
#define MSCALER0_IOBASE                 0x13702300
#define MSCALER1_IOBASE                 0x13802300

/*
 * MScaler controller
 */
#define MSCA_BASE                       0
#define MSCA_CTRL                       (MSCA_BASE + 0x000)
#define MSCA_CH_EN                      (MSCA_BASE + 0x004)
#define MSCA_CH_STA                     (MSCA_BASE + 0x008)
#define MSCA_DMAOUT_ARB                 (MSCA_BASE + 0x018)
#define MSCA_CLK_GATE_EN                (MSCA_BASE + 0x01C)
#define MSCA_CLK_DIS                    (MSCA_BASE + 0x020)
#define MSCA_SRC_IN                     (MSCA_BASE + 0x030)
#define MSCA_GLO_RSZ_COEF_WR            (MSCA_BASE + 0x040)
#define MSCA_SYS_PRO_CLK_EN             (MSCA_BASE + 0x050)
#define MSCA_DS0_CLK_NUM                (MSCA_BASE + 0x054)
#define MSCA_DS1_CLK_NUM                (MSCA_BASE + 0x058)
#define MSCA_DS2_CLK_NUM                (MSCA_BASE + 0x05C)

#define MSCA_CH_XXX(n, offset)      (MSCA_BASE + (0x0100*(n)) + offset)

#define RSZ_OSIZE(n)                (MSCA_BASE + (0x0100*(n)) + 0x100)
#define RSZ_STEP(n)                 (MSCA_BASE + (0x0100*(n)) + 0x104)

#define CROP_OPOS(n)                (MSCA_BASE + (0x0100*(n)) + 0x128)
#define CROP_OSIZE(n)               (MSCA_BASE + (0x0100*(n)) + 0x12c)
#define FRA_CTRL_LOOP(n)            (MSCA_BASE + (0x0100*(n)) + 0x130)
#define FRA_CTRL_MASK(n)            (MSCA_BASE + (0x0100*(n)) + 0x134)
#define MS0_POS(n)                  (MSCA_BASE + (0x0100*(n)) + 0x138)
#define MS0_SIZE(n)                 (MSCA_BASE + (0x0100*(n)) + 0x13c)
#define MS0_VALUE(n)                (MSCA_BASE + (0x0100*(n)) + 0x140)
#define MS1_POS(n)                  (MSCA_BASE + (0x0100*(n)) + 0x144)
#define MS1_SIZE(n)                 (MSCA_BASE + (0x0100*(n)) + 0x148)
#define MS1_VALUE(n)                (MSCA_BASE + (0x0100*(n)) + 0x14c)
#define MS2_POS(n)                  (MSCA_BASE + (0x0100*(n)) + 0x150)
#define MS2_SIZE(n)                 (MSCA_BASE + (0x0100*(n)) + 0x154)
#define MS2_VALUE(n)                (MSCA_BASE + (0x0100*(n)) + 0x158)
#define MS3_POS(n)                  (MSCA_BASE + (0x0100*(n)) + 0x15c)
#define MS3_SIZE(n)                 (MSCA_BASE + (0x0100*(n)) + 0x160)
#define MS3_VALUE(n)                (MSCA_BASE + (0x0100*(n)) + 0x164)
#define OUT_FMT(n)                  (MSCA_BASE + (0x0100*(n)) + 0x168)
#define DMAOUT_Y_ADDR(n)            (MSCA_BASE + (0x0100*(n)) + 0x16c)
#define Y_ADDR_FIFO_STA(n)          (MSCA_BASE + (0x0100*(n)) + 0x170)
#define DMAOUT_Y_LAST_ADDR(n)       (MSCA_BASE + (0x0100*(n)) + 0x174)
#define DMAOUT_Y_LAST_STATS_NUM(n)  (MSCA_BASE + (0x0100*(n)) + 0x178)
#define Y_LAST_ADDR_FIFO_STA(n)     (MSCA_BASE + (0x0100*(n)) + 0x17c)
#define DMAOUT_Y_STRI(n)            (MSCA_BASE + (0x0100*(n)) + 0x180)
#define DMAOUT_UV_ADDR(n)           (MSCA_BASE + (0x0100*(n)) + 0x184)
#define UV_ADDR_FIFO_STA(n)         (MSCA_BASE + (0x0100*(n)) + 0x188)
#define DMAOUT_UV_LAST_ADDR(n)      (MSCA_BASE + (0x0100*(n)) + 0x18c)
#define DMAOUT_UV_LAST_STATS_NUM(n) (MSCA_BASE + (0x0100*(n)) + 0x190)
#define UV_LAST_ADDR_FIFO_STA(n)    (MSCA_BASE + (0x0100*(n)) + 0x194)
#define DMAOUT_UV_STRI(n)           (MSCA_BASE + (0x0100*(n)) + 0x198)
#define DMAOUT_Y_ADDR_CLR(n)        (MSCA_BASE + (0x0100*(n)) + 0x19c)
#define DMAOUT_UV_ADDR_CLR(n)       (MSCA_BASE + (0x0100*(n)) + 0x1a0)
#define DMAOUT_Y_LAST_ADDR_CLR(n)   (MSCA_BASE + (0x0100*(n)) + 0x1a4)
#define DMAOUT_UV_LAST_ADDR_CLR(n)  (MSCA_BASE + (0x0100*(n)) + 0x1a8)
#define DMAOUT_Y_ADDR_SEL(n)        (MSCA_BASE + (0x0100*(n)) + 0x1ac)
#define DMAOUT_UV_ADDR_SEL(n)       (MSCA_BASE + (0x0100*(n)) + 0x1b0)
#define COE_ZERO_VRSZ_H(n)          (MSCA_BASE + (0x0100*(n)) + 0x1c0)
#define COE_ZERO_VRSZ_L(n)          (MSCA_BASE + (0x0100*(n)) + 0x1c4)
#define COE_ZERO_HRSZ_H(n)          (MSCA_BASE + (0x0100*(n)) + 0x1c8)
#define COE_ZERO_HRSZ_L(n)          (MSCA_BASE + (0x0100*(n)) + 0x1cc)

#define MSCA_CHx_Y_ADDR_FIFO_STA_FULL       (1 << 1)
#define MSCA_CHx_Y_ADDR_FIFO_STA_EMPTY      (1 << 0)

#define MSCA_CHx_UV_ADDR_FIFO_STA_FULL      (1 << 1)
#define MSCA_CHx_UV_ADDR_FIFO_STA_EMPTY     (1 << 0)

#define MSCA_CH0_FRM_DONE_INT           (0)
#define MSCA_CH1_FRM_DONE_INT           (1)
#define MSCA_CH2_FRM_DONE_INT           (2)
#define MSCA_CH0_CROP_ERR_INT           (3)
#define MSCA_CH1_CROP_ERR_INT           (4)
#define MSCA_CH2_CROP_ERR_INT           (5)

#define OUT_FMT_NV12                0x0
#define OUT_FMT_NV21                0x1
#define OUT_FMT_ARGB8888            0x2
#define OUT_FMT_RGB565              0x3
#define OUT_FMT_Y_OUT_ONLY          (1 << 6)

#define RESIZE_OSIZE_WIDTH          16, 31
#define RESIZE_OSIZE_HEIGHT         0, 15
#define RESIZW_STEP_WIDTH           16, 31
#define RESIZW_STEP_HEIGHT          0, 15
#define CROP_OPOS_START_X           16, 31
#define CROP_OPOS_START_Y           0, 15
#define CROP_OSIZE_WIDTH            16, 31
#define CROP_OSIZE_HEIGHT           0, 15

#endif /* __X2000_MSCALER_REGS_H__ */
