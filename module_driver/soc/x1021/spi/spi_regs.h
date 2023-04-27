#ifndef _SPI_REGS_H_
#define _SPI_REGS_H_

#define SSIDR   0x0000
#define SSICR0  0x0004
#define SSICR1  0x0008
#define SSISR   0x000C
#define SSIITR  0x0010
#define SSIICR  0x0014
#define SSIGR   0x0018
#define SSIRCNT 0x001C

#define SSIDR_D31_17 17, 31
#define SSIDR_GPC    16, 16
#define SSIDR_D15_0  0, 15

#define SSICR0_Reserved  20, 31
#define SSICR0_TENDIAN   18, 19
#define SSICR0_RENDIAN   16, 17
#define SSICR0_SSIE      15, 15
#define SSICR0_TIE       14, 14
#define SSICR0_RIE       13, 13
#define SSICR0_TEIE      12, 12
#define SSICR0_REIE      11, 11
#define SSICR0_LOOP      10, 10
#define SSICR0_RFINE     9, 9
#define SSICR0_RFINC     8, 8
#define SSICR0_EACLRUN   7, 7
#define SSICR0_FSEL      6, 6
#define SSICR0_Reserved1 5, 5
#define SSICR0_VRCNT     4, 4
#define SSICR0_TFMODE    3, 3
#define SSICR0_TFLUSH    2, 2
#define SSICR0_RFLUSH    1, 1
#define SSICR0_DISREV    0, 0

#define SSICR1_FRMHL1   31, 31
#define SSICR1_FRMHL0   30, 30
#define SSICR1_TFVCK    28, 29
#define SSICR1_TCKFI    26, 27
#define SSICR1_GPCMD    25, 25
#define SSICR1_ITFRM    24, 24
#define SSICR1_UNFIN    23, 23
#define SSICR1_FMAT     20, 21
#define SSICR1_TTRG     16, 19
#define SSICR1_MCOM     12, 15
#define SSICR1_RTRG     8, 11
#define SSICR1_FLEN     3, 7
#define SSICR1_GPCHL    2, 2
#define SSICR1_PHA      1, 1
#define SSICR1_POL      0, 0

#define SSISR_Reserved  24, 31
#define SSISR_TFIFO_NUM 16, 23
#define SSISR_RFIFO_NUM 8, 15
#define SSISR_END       7, 7
#define SSISR_BUSY      6, 6
#define SSISR_TFF       5, 5
#define SSISR_RFE       4, 4
#define SSISR_TFHE      3, 3
#define SSISR_RFHF      2, 2
#define SSISR_TUNDR     1, 1
#define SSISR_ROVER     0, 0

#define SSIITR_Reserved 16, 31
#define SSIITR_CNTCLK   15, 15
#define SSIITR_IVLTM    0, 14

#define SSIICR_Reserved 3, 31
#define SSIICR_ICC      0, 2

#define SSIGR_Reserved 8, 31
#define SSIGR_CGV      0, 7

#define SSIRCNT_Reserved 16, 31
#define SSIRCNT_RCNT     0, 15

#endif /* _SPI_REGS_H_ */