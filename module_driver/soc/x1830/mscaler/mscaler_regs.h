#ifndef __MSCALER_REGS_H__
#define __MSCALER_REGS_H__

#define MSCA_CTRL                       0x0000
#define MSCA_CH_EN                      0x0004
#define MSCA_CH_STAT                    0x0008
#define MSCA_IRQ_STAT                   0x000c
#define MSCA_IRQ_MASK                   0x0010
#define MSCA_CLR_IRQ                    0x0014
#define DMA_OUT_ARB                     0x0018
#define CLK_GATE_EN                     0x001c
#define CLK_DIS                         0x0020

#define GLO_RSZ_COEF_WR                 0x0080
#define MSCA_SRC_IN                     0x00a0
#define MSCA_SRC_SIZE                   0x00a4
#define MSCA_SRC_Y_ADDR                 0x00a8
#define MSCA_SRC_Y_STRI                 0x00ac
#define MSCA_SRC_UV_ADDR                0x00b0
#define MSCA_SRC_UV_STRI                0x00b4
#define CHx_BASE(n)                     0x0100 * n
#define CHx_RSZ_OSIZE(n)                (CHx_BASE(n)+0x100)
#define CHx_RSZ_STEP(n)                 (CHx_BASE(n)+0x104)
#define CHx_Y_HRSZ_COEF_WR(n)           (CHx_BASE(n)+0x108)
#define CHx_Y_HRSZ_COEF_RD(n)           (CHx_BASE(n)+0x10c)
#define CHx_Y_VRSZ_COEF_WR(n)           (CHx_BASE(n)+0x110)
#define CHx_Y_VRSZ_COEF_RD(n)           (CHx_BASE(n)+0x114)
#define CHx_UV_HRSZ_COEF_WR(n)          (CHx_BASE(n)+0x118)
#define CHx_UV_HRSZ_COEF_RD(n)          (CHx_BASE(n)+0x11c)
#define CHx_UV_VRSZ_COEF_WR(n)          (CHx_BASE(n)+0x120)
#define CHx_UV_VRSZ_COEF_RD(n)          (CHx_BASE(n)+0x124)
#define CHx_CROP_OPOS(n)                (CHx_BASE(n)+0x128)
#define CHx_CROP_OSIZE(n)               (CHx_BASE(n)+0x12c)
#define CHx_FRA_CTRL_LOOP(n)            (CHx_BASE(n)+0x130)
#define CHx_FRA_CTRL_MASK(n)            (CHx_BASE(n)+0x134)
#define CHx_MS0_POS(n)                  (CHx_BASE(n)+0x138)
#define CHx_MS0_SIZE(n)                 (CHx_BASE(n)+0x13c)
#define CHx_MS0_VALUE(n)                (CHx_BASE(n)+0x140)
#define CHx_MS1_POS(n)                  (CHx_BASE(n)+0x144)
#define CHx_MS1_SIZE(n)                 (CHx_BASE(n)+0x148)
#define CHx_MS1_VALUE(n)                (CHx_BASE(n)+0x14c)
#define CHx_MS2_POS(n)                  (CHx_BASE(n)+0x150)
#define CHx_MS2_SIZE(n)                 (CHx_BASE(n)+0x154)
#define CHx_MS2_VALUE(n)                (CHx_BASE(n)+0x158)
#define CHx_MS3_POS(n)                  (CHx_BASE(n)+0x15c)
#define CHx_MS3_SIZE(n)                 (CHx_BASE(n)+0x160)
#define CHx_MS3_VALUE(n)                (CHx_BASE(n)+0x164)
#define CHx_OUT_FMT(n)                  (CHx_BASE(n)+0x168)
#define CHx_DMAOUT_Y_ADDR(n)            (CHx_BASE(n)+0x16c)
#define CHx_Y_ADDR_FIFO_STA(n)          (CHx_BASE(n)+0x170)
#define CHx_DMAOUT_Y_LAST_ADDR(n)       (CHx_BASE(n)+0x174)
#define CHx_DMAOUT_Y_LAST_STATS_NUM(n)  (CHx_BASE(n)+0x178)
#define CHx_Y_LAST_ADDR_FIFO_STA(n)     (CHx_BASE(n)+0x17c)
#define CHx_DMAOUT_Y_STRI(n)            (CHx_BASE(n)+0x180)
#define CHx_DMAOUT_UV_ADDR(n)           (CHx_BASE(n)+0x184)
#define CHx_UV_ADDR_FIFO_STA(n)         (CHx_BASE(n)+0x188)
#define CHx_DMAOUT_UV_LAST_ADDR(n)      (CHx_BASE(n)+0x18c)
#define CHx_DMAOUT_UV_LAST_STATS_NUM(n) (CHx_BASE(n)+0x190)
#define CHx_UV_LAST_ADDR_FIFO_STA(n)    (CHx_BASE(n)+0x194)
#define CHx_DMAOUT_UV_STRI(n)           (CHx_BASE(n)+0x198)
#define CHx_DMAOUT_Y_ADDR_CLR(n)        (CHx_BASE(n)+0x19c)
#define CHx_DMAOUT_UV_ADDR_CLR(n)       (CHx_BASE(n)+0x1a0)
#define CHx_DMAOUT_Y_LAST_ADDR_CLR(n)   (CHx_BASE(n)+0x1a4)
#define CHx_DMAOUT_UV_LAST_ADDR_CLR(n)  (CHx_BASE(n)+0x1a8)

#define CSC_C0_COEF                     0x00400
#define CSC_C1_COEF                     0x00404
#define CSC_C2_COEF                     0x00408
#define CSC_C3_COEF                     0x0040c
#define CSC_C4_COEF                     0x00410
#define CSC_OFFSET_PARA                 0x00414
#define CSC_GLO_ALPHA                   0x00418
#define SYS_PRO_CLK_EN                  0x00430
#define CH0_CLK_NUM                     0x00434
#define CH1_CLK_NUM                     0x00438
#define CH2_CLK_NUM                     0x0043c

#define MSCALER_INPUT_MIN_WIDTH         8
#define MSCALER_INPUT_MIN_HEIGHT        8

#define MSCALER_INPUT_MAX_WIDTH         2592
#define MSCALER_INPUT_MAX_HEIGHT        2048

#define MSCALER_OUTPUT_MIN_WIDTH        8
#define MSCALER_OUTPUT_MIN_HEIGHT       8

#define MSCALER_OUTPUT0_MAX_WIDTH       2592
#define MSCALER_OUTPUT0_MAX_HEIGHT      2048

#define MSCALER_OUTPUT1_MAX_WIDTH       1280
#define MSCALER_OUTPUT1_MAX_HEIGHT      1280

#define MSCALER_OUTPUT2_MAX_WIDTH       800
#define MSCALER_OUTPUT2_MAX_HEIGHT      800



/* interrupt mask bit */
#define MS_IRQ_MAX_BIT                  0x09
#define MS_IRQ_OVF_BIT                  0x08
#define MS_IRQ_CSC_BIT                  0x07
#define MS_IRQ_FRM_BIT                  0x06
#define MS_IRQ_CH2_CROP_BIT             0x05
#define MS_IRQ_CH1_CROP_BIT             0x04
#define MS_IRQ_CH0_CROP_BIT             0x03
#define MS_IRQ_CH2_DONE_BIT             0x02
#define MS_IRQ_CH1_DONE_BIT             0x01
#define MS_IRQ_CH0_DONE_BIT             0x0

/* the state of the address of dma buffer fifo */
#define CH_ADDR_FIFO_EMPTY              (0x01 << 0)
#define CH_ADDR_FIFO_FULL               (0x01 << 1)
#define CH_ADDR_FIFO_ENTRYS(n)          (((n) & 0x07 << 2)) >> 2)
#endif
