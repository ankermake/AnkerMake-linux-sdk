#ifndef _CIM_REGS_H_
#define _CIM_REGS_H_

#define CIMCFG     0x0000
#define CIMCR      0x0004
#define CIMST      0x0008
#define CIMIID     0x000C
#define CIMDA      0x0020
#define CIMFA      0x0024
#define CIMFID     0x0028
#define CIMCMD     0x002C
#define CIMWSIZE   0x0030
#define CIMWOFFSET 0x0034
#define CIMCR2     0x0050
#define CIMFS      0x0054
#define CIMIMR     0x0058

#define EEOFEN 31, 31
#define BS3 22, 23
#define BS2 20, 21
#define BS1 18, 19
#define BS0 16, 17
#define INV_DAT 15, 15
#define VSP 14, 14
#define HSP 13, 13
#define PCP 12, 12
#define BURST_TYPE 10, 11
#define DUMMY 9, 9
#define E_VSYNC 8, 8
#define LM 7, 7
#define PACK 4, 6
#define FP 3, 3
#define E_HSYNC 2, 2
#define DSM 0, 1

#define EEOF_LINE 20, 31
#define FRC 16, 19
#define WINE 14, 14
#define FRM_ALIGN 8, 8
#define DMA_SYNC 7, 7
#define STP_REQ 4, 4
#define SW_RST 3, 3
#define DMA_EN 2, 2
#define RF_RST 1, 1
#define ENA 0, 0

#define FSC 23, 23
#define ARIF 22, 22
#define OP 4, 5
#define OPE 2, 2
#define APM 0, 0

#define DEEOF 11, 11
#define DSTOP 10, 10
#define DEOF 9, 9
#define DSOF 8, 8
#define FSE 3, 3
#define RFOF 2, 2
#define RFE 1, 1
#define STP_ACK 0, 0

#define DEEOFM 11, 11
#define DSTPM 10, 10
#define DEOFM 9, 9
#define DSOFM 8, 8
#define FSEM 3, 3
#define RFOFM 2, 2
#define STPM 0, 0

#define SOFINTE 31, 31
#define EOFINTE 30, 30
#define EEOFINTE 29, 29
#define STOP 28, 28
#define OFAR 27, 27
#define LEN 0, 23

#define LPF 16, 18
#define PPL 0, 12

#define V_OFFSET 16, 28
#define H_OFFSET 0, 12

#define FVS 16, 28
#define BPP 14, 15
#define FHS 0, 12

#endif /* _CIM_REGS_H_ */
