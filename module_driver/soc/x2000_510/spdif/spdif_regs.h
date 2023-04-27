#ifndef __SPDIF_H__
#define __SPDIF_H__

#define SPDIF_OUT_BASE 0x134db000
#define SPDIF_IN_BASE 0x134db100

/* SPDIF OUT */
#define SPENA               (0x0)
#define SPO_SPEN            1

#define SPCTRL              (0x4)
#define SPO_SIGN_N          13, 13
#define SPO_INVALID         12, 12
#define SPO_SFT_RST         11, 11


#define SPCFG1              (0x8)
#define SPO_CFG1            0,11
#define SPO_SCR_NUM_SFT     (8)
#define SPO_SET_SCR_NUM(x)  ((x) << SPO_SCR_NUM_SFT)
#define SPO_CH1_NUM_SFT     (4)
#define SPO_SET_CH1_NUM(x)  ((x) << SPO_CH1_NUM_SFT)
#define SPO_CH2_NUM_SFT     (0)
#define SPO_SET_CH2_NUM(x)  ((x) << SPO_CH2_NUM_SFT)

#define SPCFG2              (0xc)
#define SPO_CFG2_CATCODE    8,15
#define SPO_CFG2_CH_MD      6,7

#define SPO_CFG2_FRQ        22,29
#define SPO_CFG2_SAMPL_WL   19,21
#define SPO_CFG2_MAX_WL     18,18

#define SPO_AUDIO_N         1<<1
#define SPO_CON_PRO         1<<0
#define SPO_PCM_MODE        0, 1

#define SPDIV               (0x10)
#define SPO_DV_SFT          (0)
#define SPO_SET_DV(x)       (((x) > ((1 << 7) - 1) ? ((1 << 7) - 1) : (x)) << SPO_DV_SFT)

/* SPDIF IN */
#define SPIENA              (0x0)
#define SPI_RESET            1
#define SPI_RESET_MASK       1, 1

#define SPI_SPIEN            1
#define SPI_SPIEN_MASK       0, 0

#define SPICFG1             (0x4)
#define SPICFG2             (0x8)
#define SPICFG3             (0xc)
#define SPIDIV              (0x10)
#define SPI_DV_SFT          (0)
#define SPI_SET_DV(x)       SPO_SET_DV(x)

#endif //__AS_SPDIF_H__
