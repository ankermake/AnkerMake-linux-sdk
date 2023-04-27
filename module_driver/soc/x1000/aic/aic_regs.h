#ifndef _AIC_REGS_H_
#define _AIC_REGS_H_

#define TX_FIFO_SIZE 64
#define RX_FIFO_SIZE 32
#define RX_FIFO_TRIGGER (RX_FIFO_SIZE / 4)
#define TX_FIFO_TRIGGER (TX_FIFO_SIZE / 2)

#define AICFR 0x00
#define AICCR 0x04
#define I2SCR 0x10
#define AICSR 0x14
#define I2SDIV 0x30
#define AICDR 0x34

#define rg_codec_srst 31, 31
#define RFTH 24, 27
#define TFTH 16, 20
#define MSB 12, 12
#define IBCKD 10, 10
#define ISYNCD 9, 9
#define DMODE 8, 8
#define CDC_MASTER 7, 7
#define LSMP 6, 6
#define ICDC 5, 5
#define AUSEL 4, 4
#define RST 3, 3
#define BCKD 2, 2
#define SYNCD 1, 1
#define ENB 0, 0

#define PACK16 28, 28
#define CHANNEL 24, 26
#define OSS 19, 21
#define ISS 16, 18
#define RDMS 15, 15
#define TDMS 14, 14
#define M2S 11, 11
#define ENDSW 10, 10
#define ASVTSU 9, 9
#define TFLUSH 8, 8
#define RFLUSH 7, 7
#define EROR 6, 6
#define ETUR 5, 5
#define ERFS 4, 4
#define ETFS 3, 3
#define ENLBF 2, 2
#define ERPL 1, 1
#define EREC 0, 0

#define RFIRST 17, 17
#define SWLH 16, 16
#define ISTPBK 13, 13
#define STPBK 12, 12
#define ESCLK 4, 4
#define AMSL 0, 0

#define RFL 24, 29
#define TFL 8, 13
#define ROR 6, 6
#define TUR 5, 5
#define RFS 4, 4
#define TFS 3, 3

#define CHBSY 5, 5
#define TBSY 4, 4
#define RBSY 3, 3
#define BSY 2, 2

#define IDV 16, 24
#define DV 0, 8

#endif /* _AIC_REGS_H_ */
