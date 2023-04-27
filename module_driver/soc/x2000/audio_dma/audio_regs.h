#ifndef _AUDIO_REGS_H_
#define _AUDIO_REGS_H_

#include <linux/io.h>

#define AUDIO_BASE 0x134d0000
#define FIFO_OFF 0x1000
#define DFMT_OFF 0x2000
#define FIFO_DATA_OFF 0x3000
#define IBUS_OFF 0x4000
#define AIC0_OFF 0x5000
#define AIC1_OFF 0x6000
#define AIC2_OFF 0x7000
#define AIC3_OFF 0x8000
#define AIC4_OFF 0x9000


/*
 * DMA regs
 */
#define DBA(n)   (0x00 + (n)*0x18)
#define DTC(n)   (0x04 + (n)*0x18)
#define DCR(n)   (0x08 + (n)*0x18)
#define DSR(n)   (0x0c + (n)*0x18)
#define DCM(n)   (0x10 + (n)*0x18)
#define DDA(n)   (0x14 + (n)*0x18)
#define DGRR 0x100
#define DGER 0x104
#define AEER 0x108
#define AESR 0x10c
#define AIPR 0x110

#define DFS 1, 1
#define DCR_RESET 1, 1 // 参考内核中驱动,手册里面没有
#define CTE 0, 0

#define CDOA  8, 15
#define TT_INT 5, 5
#define LTT_INT 4, 4
#define TT 3, 3
#define LTT 2, 2
#define RST_EN 1, 1  // 参考内核中驱动,手册里面没有
#define LINK 0, 0

#define NDES 31, 31
#define BAI 8, 8
#define TSZ 4, 6
#define TIE 1, 1
#define LTIE 0, 0

#define DMA_RESET 0, 0

#define DMA_EN 0, 0

#define EXP_EN 0, 0

#define EXP_STATUS 0, 0

#define EXP_INT 11, 11
#define DMIC_INT 10, 10
#define DMA_INT9 9, 9
#define DMA_INT8 8, 8
#define DMA_INT7 7, 7
#define DMA_INT6 6, 6
#define DMA_INT5 5, 5
#define DMA_INT4 4, 4
#define DMA_INT3 3, 3
#define DMA_INT2 2, 2
#define DMA_INT1 1, 1
#define DMA_INT0 0, 0

#define D_BAI 8, 8
#define D_TSZ 4, 6
/*
 * FIFO regs
 */
#define FAS(n) (FIFO_OFF + 0x00 + (n)*0x10)
#define FCR(n) (FIFO_OFF + 0x04 + (n)*0x10)
#define FFR(n) (FIFO_OFF + 0x08 + (n)*0x10)
#define FSR(n) (FIFO_OFF + 0x0c + (n)*0x10)

#define FAD 0, 12

#define FLUSH 2, 2
#define FULL_EN 1, 1
#define FIFO_EN 0, 0

#define FTH 16, 28
#define TUR_ROR_E 9, 9
#define TFS_RFS_E 8, 8
#define FIFO_TD 7, 7

#define FLEVEL 16, 28
#define TUR_ROR_INPRO 5, 5
#define TFS_RFS_INPRO 4, 4
#define TUR_ROR 2, 2
#define TFS_RFS 0, 0

/*
 * DATA FORMAT regs
 */
#define DFCR(n) (DFMT_OFF + 0x00 + (n)*0x04)

#define CHNUM 12, 15
#define SS 8, 10
#define PACK_EN 2, 2
#define RESET 1, 1
#define ENABLE 0, 0
#define FMT_ENABLE 1

/*
 * FIFO DATA regs
 */
#define FDR(n)  (FIFO_DATA_OFF + 0x00 + (n)*0x04)

/*
 * IBUS regs
 */
#define BTSET (IBUS_OFF + 0x0)
#define BTCLR (IBUS_OFF + 0x4)
#define BTSR (IBUS_OFF + 0x8)
#define BFSR (IBUS_OFF + 0xc)
#define BFCR0 (IBUS_OFF + 0x10)
#define BFCR1 (IBUS_OFF + 0x14)
#define BFCR2 (IBUS_OFF + 0x18)

#define BST0 (IBUS_OFF + 0x1c)
#define BST1 (IBUS_OFF + 0x20)
#define BST2 (IBUS_OFF + 0x24)
#define BTT0 (IBUS_OFF + 0x28)
#define BTT1 (IBUS_OFF + 0x2c)

#define BST0_OFF (0x1c)
#define BST1_OFF (0x20)
#define BST2_OFF (0x24)
#define BTT0_OFF (0x28)
#define BTT1_OFF (0x2c)

#define TSLOT_SET 1, 31
#define TSLOT_CLR 1, 31
#define TSLOT_EN 1, 31

#define DEV11_TUR 27, 27
#define DEV10_TUR 26, 26
#define DEV9_TUR 25, 25
#define DEV8_TUR 24, 24
#define DEV7_TUR 23, 23
#define DEV6_TUR 22, 22
#define DEV5_TUR 21, 21
#define DEV4_TUR 20, 20
#define DEV3_TUR 19, 19
#define DEV2_TUR 18, 18
#define DEV1_TUR 17, 17
#define DEV0_TUR 16, 16
#define DEV14_ROR 14, 14
#define DEV13_ROR 13, 13
#define DEV12_ROR 12, 12
#define DEV11_ROR 11, 11
#define DEV10_ROR 10, 10
#define DEV9_ROR 9, 9
#define DEV8_ROR 8, 8
#define DEV7_ROR 7, 7
#define DEV6_ROR 6, 6
#define DEV5_ROR 5, 5
#define DEV4_ROR 4, 4
#define DEV3_ROR 3, 3
#define DEV2_ROR 2, 2
#define DEV1_ROR 1, 1
#define DEV0_ROR 0, 0

#define DEV11_LSMP 11, 11
#define DEV10_LSMP 10, 10
#define DEV9_LSMP 9, 9
#define DEV8_LSMP 8, 8
#define DEV7_LSMP 7, 7
#define DEV6_LSMP 6, 6
#define DEV5_LSMP 5, 5
#define DEV4_LSMP 4, 4
#define DEV3_LSMP 3, 3
#define DEV2_LSMP 2, 2
#define DEV1_LSMP 1, 1
#define DEV0_LSMP 0, 0

#define DEV14_DBE 14, 14
#define DEV13_DBE 13, 13
#define DEV12_DBE 12, 12
#define DEV11_DBE 11, 11
#define DEV10_DBE 10, 10
#define DEV9_DBE 9, 9
#define DEV8_DBE 8, 8
#define DEV7_DBE 7, 7
#define DEV6_DBE 6, 6
#define DEV5_DBE 5, 5
#define DEV4_DBE 4, 4
#define DEV3_DBE 3, 3
#define DEV2_DBE 2, 2
#define DEV1_DBE 1, 1
#define DEV0_DBE 0, 0


#define DEV5_SUR 26, 30
#define DEV4_SUR 21, 25
#define DEV3_SUR 16, 20
#define DEV2_SUR 10, 14
#define DEV1_SUR 5, 9
#define DEV0_SUR 0, 4

#define DEV10_SUR 21, 25
#define DEV9_SUR 16, 20
#define DEV8_SUR 10, 14
#define DEV7_SUR 5, 9
#define DEV6_SUR 0, 4

#define DEV14_SUR 16, 20
#define DEV13_SUR 10, 14
#define DEV12_SUR 5, 9
#define DEV11_SUR 0, 4

#define DEV5_TAR 26, 30
#define DEV4_TAR 21, 25
#define DEV3_TAR 16, 20
#define DEV2_TAR 10, 14
#define DEV1_TAR 5, 9
#define DEV0_TAR 0, 4

#define DEV11_TAR 26, 30
#define DEV10_TAR 21, 25
#define DEV9_TAR 16, 20
#define DEV8_TAR 10, 14
#define DEV7_TAR 5, 9
#define DEV6_TAR 0, 4


#define AUDIO_ADDR(reg) ((volatile unsigned long *)KSEG1ADDR(AUDIO_BASE + (reg)))

static inline void audio_write_reg(unsigned int reg, unsigned int value)
{
    *AUDIO_ADDR(reg) = value;
}

static inline unsigned int audio_read_reg(unsigned int reg)
{
    return *AUDIO_ADDR(reg);
}

static inline void audio_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(AUDIO_ADDR(reg), start, end, val);
}

static inline unsigned int audio_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(AUDIO_ADDR(reg), start, end);
}

#endif /* _AUDIO_REGS_H_ */
