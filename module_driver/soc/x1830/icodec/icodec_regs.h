#ifndef _ICODEC_REGS_H_
#define _ICODEC_REGS_H_

#define CGR     0x00
#define CACR    0x08
#define CMCR    0x0C
#define CDCR1   0x10
#define CDCR2   0x14
#define CADR    0x1C
#define CGAINR  0x28
#define CASR    0x34
#define CDPR    0x38
#define CDDPR2  0x3C
#define CDDPR1  0x40
#define CDDPR0  0x44
#define CAACR   0x84
#define CMICCR  0x88
#define CACR2   0x8C
#define CAMPCR  0x90
#define CAR     0x94
#define CHR     0x98
#define CHCR    0x9C
#define CCR     0xA0
#define CMR     0x100
#define CTR     0x104
#define CAGCCR  0x108
#define CPGR    0x10C
#define CSRR    0x110
#define CALMR   0x114
#define CAHMR   0x118
#define CALMINR 0x11c
#define CAHMINR 0x120
#define CAFR    0x124
#define CCAGVR  0x130

#define bsten 7, 7
#define dcrst 1, 1
#define srst 0, 0

#define adcen 7, 7
#define adcvaldalength 5, 6
#define adci2smode 3, 4
#define adcswap 1, 1
#define adcintf 0, 0

#define Masteren 5, 5
#define adcdacmaster 4, 4
#define adcdalength 2, 3
#define adci2srst 1, 1
#define adcalign 0, 0

#define dacenI2S 7, 7
#define dacvaldalength 5, 6
#define daci2smode 3, 4
#define dacswap 2, 2

#define dacdalength 2, 3
#define daci2srst 1, 1

#define adcloopdac 4, 4
#define adcin 3, 3

#define alclgainen 5, 5

#define daclechsrc 6, 7
#define adclechsrc 2, 3

#define adcdatapath 6, 7
#define adcdatain 3, 3
#define dacindebug3 0, 1

#define adcsrcen 7, 7
#define Micbiasen 6, 6
#define adczeroen 5, 5
#define Micbaisctr 0, 2

#define Micen 6, 6
#define Micgain 5, 5
#define Micmute 4, 4
#define Alcen 1, 1
#define Alcmute 0, 0

#define Alcsel 5, 5
#define Alcgain 0, 4

#define adcrefvolen 7, 7
#define adclclken 6, 6
#define adclampen 5, 5
#define adclrst 4, 4

#define Detdacshort 7, 7
#define Audiodacen 6, 6
#define Dacrefvolen 5, 5
#define dacrefvolen 3, 3
#define daclclken 2, 2
#define dacen 1, 1
#define dacinit 0, 0

#define hpouten 7, 7
#define hpoutinit 6, 6
#define hpoutmute 5, 5

#define hpoutpop 6, 7
#define hpoutgain 0, 4

#define dacprecharge 7, 7
#define chargeselect 0, 6

#define gainmsthod 6, 6
#define ctrmsthod 4, 5
#define gainholdtime 0, 3

#define decaytime 4, 7
#define attacktime 0, 3

#define agcmode 7, 7
#define agczeroen 6, 6
#define ampmode 5, 5
#define fastdecr 4, 4
#define agcnoisegate 3, 3
#define noisethd 0, 2

#define pgalezeroen 5, 5
#define pgalegain 0, 4

#define slowclken 3, 3
#define samplerate 0, 2

#define agcfunsel 6, 6
#define pgamaxgain 3, 5
#define pgamingain 0, 2

#define agcganival 0, 4

#define CODEC_IOBASE 0x10021000

#define ICODEC_REG_BASE  KSEG1ADDR(CODEC_IOBASE)

#define CODEC_ADDR(reg) ((volatile unsigned long *)(ICODEC_REG_BASE + (reg)))

#endif /* _ICODEC_REGS_H_ */