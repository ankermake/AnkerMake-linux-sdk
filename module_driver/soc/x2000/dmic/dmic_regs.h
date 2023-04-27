#ifndef _DMIC_REGS_H_
#define _DMIC_REGS_H_

#define DMIC_IOBASE 0x134da000

#define DMIC_CR0        0x00 //  DMIC Control register
#define DMIC_GCR        0x04 //  DMIC Gain Control Register
#define DMIC_IER        0x08 //  DMIC Interrupt Enable Register
#define DMIC_ICR        0x0C //  DMIC Interrupt Control Register
#define DMIC_TRI_TCR    0x10 //  DMIC Trigger Control Register
#define DMIC_THR_L1     0x14 //  DMIC Trigger 1 Low Threshold
#define DMIC_THR_H2     0x18 //  DMIC Trigger High Threshold
#define DMIC_THR_L2     0x1C //  DMIC Trigger 2 Low Threshold
#define DMIC_TRI_M_MAX  0x20 //  DMIC Trigger number M Max value
#define DMIC_TRI_N_MAX  0x24 //  DMIC Trigger number N Max value
#define DMIC_THR_L3     0x28 //  DMIC Trigger 3 Low Threshold
#define DMIC_THR_TM     0x2C //  DMIC Voice Trigger timer
#define DMIC_FFGDIS     0x30 //  DMIC clock disable Control Register
#define DMIC_SFGDIS     0x34 //  DMIC clock disable Control Register
#define DMIC_TFGDIS     0x38 //  DMIC clock disable Control Register
#define DMIC_CGDIS      0x3C //  DMIC clock disable Control Register
#define DMIC_VTSR       0x40 //  DMIC Voice Trigger status

#define D_RESET       31, 31
#define D_RESET_TRI   30, 30// ???
#define D_HPF2_EN     22, 22
#define D_LPF_EN      21, 21
#define D_HPF1_EN     20, 20
#define D_CHNUM       16, 19
#define D_OSS         12, 13
#define D_SW_LR       11, 11
#define D_SR          6, 8
#define D_DMIC_EN     0, 0

#define D_DGAIN       0, 4

#define D_TRI_FAIL_INT_EN 4, 4
#define D_WAKEUP_INT_EN   0, 0

#define D_FIFO_FAIL_FLAG  4, 4
#define D_WAKEUP_FLAG     0, 0

#define D_TRI2_MODE       16, 19
#define D_TRI1_CLR        11, 11
#define D_TRI2_CLR        10, 10
#define D_TRI3_CLR        9, 9
#define D_TRI3_EN         6, 6
#define D_TRI2_EN         5, 5
#define D_TRI1_EN         4, 4
#define D_TRI1_RESET      0, 0

#define D_THR_L1          0, 23
#define D_THR_H2          0, 23
#define D_THR_L2          0, 23

#define D_M_MAX 0, 23
#define D_N_MAX 0, 15

#define D_THR_L3          0, 23

#define D_TM_LEN          0, 21

#define D_CH3_LPF_GDIS    31, 31
#define D_CH3_HPF2_GDIS   30, 30
#define D_CH3_HPF1_GDIS   29, 29
#define D_CH3_SRC3_GDIS   28, 28
#define D_CH3_SRC2_GDIS   27, 27
#define D_CH3_SRC1_GDIS   26, 26
#define D_CH3_FIR2_GDIS   25, 25
#define D_CH3_FIR3_GDIS   24, 24
#define D_CH2_LPF_GDIS    23, 23
#define D_CH2_HPF2_GDIS   22, 22
#define D_CH2_HPF1_GDIS   21, 21
#define D_CH2_SRC3_GDIS   20, 20
#define D_CH2_SRC2_GDIS   19, 19
#define D_CH2_SRC1_GDIS   18, 18
#define D_CH2_FIR2_GDIS   17, 17
#define D_CH2_FIR3_GDIS   16, 16
#define D_CH1_LPF_CGDIS   15, 15
#define D_CH1_HPF2_GDIS   14, 14
#define D_CH1_HPF1_GDIS   13, 13
#define D_CH1_SRC3_GDIS   12, 12
#define D_CH1_SRC2_GDIS   11, 11
#define D_CH1_SRC1_GDIS   10, 10
#define D_CH1_FIR2_GDIS   9, 9
#define D_CH1_FIR3_GDIS   8, 8
#define D_CH0_LPF_GDIS    7, 7
#define D_CH0_HPF2_GDIS   6, 6
#define D_CH0_HPF1_GDIS   5, 5
#define D_CH0_SRC3_GDIS   4, 4
#define D_CH0_SRC2_GDIS   3, 3
#define D_CH0_SRC1_GDIS   2, 2
#define D_CH0_FIR2_GDIS   1, 1
#define D_CH0_FIR3_GDIS   0, 0

#define D_CH7_LPF_GDIS    31, 31
#define D_CH7_HPF2_GDIS   30, 30
#define D_CH7_HPF1_GDIS   29, 29
#define D_CH7_SRC3_GDIS   28, 28
#define D_CH7_SRC2_GDIS   27, 27
#define D_CH7_SRC1_GDIS   26, 26
#define D_CH7_FIR2_GDIS   25, 25
#define D_CH7_FIR3_GDIS   24, 24
#define D_CH6_LPF_GDIS    23, 23
#define D_CH6_HPF2_GDIS   22, 22
#define D_CH6_HPF1_GDIS   21, 21
#define D_CH6_SRC3_GDIS   20, 20
#define D_CH6_SRC2_GDIS   19, 19
#define D_CH6_SRC1_GDIS   18, 18
#define D_CH6_FIR2_GDIS   17, 17
#define D_CH6_FIR3_GDIS   16, 16
#define D_CH5_LPF_CGDIS   15, 15
#define D_CH5_HPF2_GDIS   14, 14
#define D_CH5_HPF1_GDIS   13, 13
#define D_CH5_SRC3_GDIS   12, 12
#define D_CH5_SRC2_GDIS   11, 11
#define D_CH5_SRC1_GDIS   10, 10
#define D_CH5_FIR2_GDIS   9, 9
#define D_CH5_FIR3_GDIS   8, 8
#define D_CH4_LPF_GDIS    7, 7
#define D_CH4_HPF2_GDIS   6, 6
#define D_CH4_HPF1_GDIS   5, 5
#define D_CH4_SRC3_GDIS   4, 4
#define D_CH4_SRC2_GDIS   3, 3
#define D_CH4_SRC1_GDIS   2, 2
#define D_CH4_FIR2_GDIS   1, 1
#define D_CH4_FIR3_GDIS   0, 0

#define D_CH11_LPF_GDIS    31, 31
#define D_CH11_HPF2_GDIS   30, 30
#define D_CH11_HPF1_GDIS   29, 29
#define D_CH11_SRC3_GDIS   28, 28
#define D_CH11_SRC2_GDIS   27, 27
#define D_CH11_SRC1_GDIS   26, 26
#define D_CH11_FIR2_GDIS   25, 25
#define D_CH11_FIR3_GDIS   24, 24
#define D_CH10_LPF_GDIS    23, 23
#define D_CH10_HPF2_GDIS   22, 22
#define D_CH10_HPF1_GDIS   21, 21
#define D_CH10_SRC3_GDIS   20, 20
#define D_CH10_SRC2_GDIS   19, 19
#define D_CH10_SRC1_GDIS   18, 18
#define D_CH10_FIR2_GDIS   17, 17
#define D_CH10_FIR3_GDIS   16, 16
#define D_CH9_LPF_CGDIS   15, 15
#define D_CH9_HPF2_GDIS   14, 14
#define D_CH9_HPF1_GDIS   13, 13
#define D_CH9_SRC3_GDIS   12, 12
#define D_CH9_SRC2_GDIS   11, 11
#define D_CH9_SRC1_GDIS   10, 10
#define D_CH9_FIR2_GDIS   9, 9
#define D_CH9_FIR3_GDIS   8, 8
#define D_CH8_LPF_GDIS    7, 7
#define D_CH8_HPF2_GDIS   6, 6
#define D_CH8_HPF1_GDIS   5, 5
#define D_CH8_SRC3_GDIS   4, 4
#define D_CH8_SRC2_GDIS   3, 3
#define D_CH8_SRC1_GDIS   2, 2
#define D_CH8_FIR2_GDIS   1, 1
#define D_CH8_FIR3_GDIS   0, 0

#define D_DAT_MASK_DIS    4, 4
#define D_TRI_GDIS        0, 0

#define D_VT3_TRI_ST      2, 2
#define D_VT2_TRI_ST      1, 1
#define D_VT1_TRI_ST      0, 0

#endif /* _DMIC_REGS_H_ */
