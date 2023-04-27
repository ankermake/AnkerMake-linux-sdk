#ifndef _DMIC_REGS_H_
#define _DMIC_REGS_H_

#define DMIC_CR0    0x00 //  DMIC Control register
#define DMIC_GCR    0x04 //  DMIC Gain Control Register
#define DMIC_IMR    0x08 //  DMIC Interrupt Mask Register
#define DMIC_ICR    0x0C //  DMIC Interrupt Control Register
#define TRI_CR      0x10 //  DMIC Trigger Control Register
#define THR_H       0x14 //  DMIC Trigger High Threshold
#define THR_L       0x18 //  DMIC Trigger Low Threshold
#define TRI_M_MAX   0x1C //  DMIC Trigger number M Max value
#define TRI_N_MAX   0x20 //  DMIC Trigger number N Max value
#define DMIC_DR     0x30 //  DMIC Data Register
#define DMIC_FCR    0x34 //  DMIC FIFO Control register
#define DMIC_FSR    0x38 //  DMIC FIFO State Register

#define RESET 31, 31
#define RESET_TRI 30, 30
#define CHNUM 16, 18
#define UNPACK_MSB 13, 13
#define UNPACK_DIS 12, 12
#define SW_LR 11, 11
#define PACK_EN 8, 8
#define SR 6, 7
#define LP_MODE 3, 3
#define HPF1_EN 2, 2
#define TRI_EN 1, 1
#define DMIC_EN 0, 0

#define DGAIN 0, 4

#define FIFO_TRIG_MASK 5, 5
#define WAKEUP_MASK 4, 4
#define EMPTY_MASK 3, 3
#define FULL_MASK 2, 2
#define PRE_READ_MASK 1, 1
#define TRI_MASK 0, 0

#define FIFO_TRIG_FLAG 5, 5
#define WAKEUP_FLAG 4, 4
#define EMPTY_FLAG 3, 3
#define FULL_FLAG 2, 2
#define PRE_READ_FLAG 1, 1
#define TRI_FLAG 0, 0

#define TRI_MODE 16, 19
#define TRI_DEBUG 4, 4
#define HPF2_EN 3, 3
#define PREFETCH 1, 2
#define TRI_CLR 0, 0

#define THR_H_DATA 0, 19

#define THR_L_DATA 0, 19

#define M_MAX 0, 23

#define N_MAX 0, 15

#define RDMS 31, 31
#define FIFO_THR 0, 5

#define FULL_S 19, 19
#define TRIG_S 18, 18
#define PRERD_S 17, 17
#define EMPTY_S 16, 16
#define FIFO_LVL 0, 5

#define HPF2_CGDIS 31, 31
#define HPF1B_CGDIS 29, 29
#define SRC3B_CGDIS 28, 28
#define SRC2B_CGDIS 27, 27
#define SRC1B_CGDIS 26, 26
#define FIR2B_CGDIS 25, 25
#define FIR3B_CGDIS 24, 24
#define TRI_CGDIS 23, 23
#define HPF1F_CGDIS 21, 21
#define SRC3F_CGDIS 20, 20
#define SRC2F_CGDIS 19, 19
#define SRC1F_CGDIS 18, 18
#define FIR2F_CGDIS 17, 17
#define FIR3F_CGDIS 16, 16
#define TRI_LPF_CGDIS 15, 15
#define HPF1R_CGDIS 13, 13
#define SRC3R_CGDIS 12, 12
#define SRC2R_CGDIS 11, 11
#define SRC1R_CGDIS 10, 10
#define FIR2R_CGDIS 9, 9
#define FIR3R_CGDIS 8, 8
#define INV_DAT_MASK_DIS 7, 7
#define HPF1L_CGDIS 5, 5
#define SRC3L_CGDIS 4, 4
#define SRC2L_CGDIS 3, 3
#define SRC1L_CGDIS 2, 2
#define FIR2L_CGDIS 1, 1
#define FIR3L_CGDIS 0, 0

#endif /* _DMIC_REGS_H_ */
