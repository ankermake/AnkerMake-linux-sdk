#ifndef _ICODEC_REGS_H_
#define _ICODEC_REGS_H_

#define Reset 0x00
#define BIST_RST 7, 7
#define DIGITAL_RST 1, 1
#define SYS_RST 0, 0

#define ADC_DAC_Configure1 0x08
#define ADC_LRC_POL 7, 7
#define ADC_VALID_LEN 5, 6
#define ADC_MODE 3, 4
#define ADC_SWAP 1, 1

#define ADC_DAC_Configure2 0x0C
#define DAC_IO_Master 7, 7
#define DAC_inner_Master 6, 6
#define ADC_IO_Master 5, 5
#define ADC_inner_Master 4, 4
#define ADC_LENGTH 2, 3
#define ADC_RESET 1, 1
#define ADC_BIT_POLARITY 0, 0

#define ADC_DAC_Configure3 0x10
#define DAC_LRC_POLARITY 7, 7
#define DAC_VALID_LENGTH 5, 6
#define DAC_MODE 3, 4
#define DAC_SWAP 2, 2

#define ADC_DAC_Configure4 0x14
#define DAC_LENGTH 2, 3
#define DAC_RESET 1, 1
#define DAC_BIT_POLARITY 0, 0

#define Chooose_ALC_Gain 0X28
#define PGA_GAIN_SEL 5, 5

#define Select_charge 0x84
#define CHOOSE2 6, 6
#define CHOOSE4 5, 5
#define CHOOSE8 4, 4
#define CHOOSE16 3, 3
#define CHOOSE32 2, 2
#define CHOOSE64 1, 1
#define CHOOSE128 0, 0

#define Control1 0x88
#define REF_VOL_EN 5, 5
#define ADC_CUR_EN 4, 4
#define BIAS_VOL_EN 3, 3
#define BIAS_VOL 0, 2

#define Control2 0x8C
#define ADCL_MUTE 7, 7 
#define ADCL_INITIAL 6, 6
#define ADCL_VOL_EN 5, 5
#define ADCL_ZERO 4, 4

#define Control3 0x90
#define ADCL_MIC_GAIN 6, 7
#define ADCL_EN 5, 5
#define ADCL_MIC_EN 4, 4

#define Control4 0x94
#define ALCL_GAIN 0, 4

#define Control5 0x98
#define ADCL_ALC_INIT 7, 7
#define ADCL_CLK_EN 6, 6
#define ADCL_ADC_EN 5, 5
#define ADCL_ADC_INIT 4, 4
#define DAC_CUR_EN 3, 3
#define DAC_REF_EN 2, 2
#define POP_CTL 0, 1

#define Control6 0x9C
#define DACL_REF_EN 7, 7
#define DACL_CLK_EN 6, 6
#define DACL_EN 5, 5
#define DACL_INIT 4, 4

#define Control7 0xA0
#define HPOUTL_MUTE 7, 7
#define HPOUTL_EN 6, 6
#define HPOUTL_INIT 5, 5
#define HPOUTL_GAIN 0, 4

#define ALG_CONFIG1 0X110
#define SLOW_CLK_EN 3, 3
#define SAMPLE_RATE 0, 2

#define ALG_MAXL 0X114

#define ALG_MAXH 0X118

#define ALG_MINL 0X11c

#define ALG_MINH 0X120

#define ALG_CONFIG2 0X124
#define ALG_ENABLE 6, 6
#define ALG_MAXGAIN 3, 5
#define ALG_MINGAIN 0, 2

#endif /* _ICODEC_REGS_H_ */
