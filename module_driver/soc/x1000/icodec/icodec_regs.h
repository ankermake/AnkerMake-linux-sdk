#ifndef _ICODEC_REGS_H_
#define _ICODEC_REGS_H_

enum {
    SR = 0x0,
    SR2,
    SIGR,
    SIGR2,
    SIGR3,
    SIGR5,
    SIGR7,
    MR,
    AICR_DAC,
    AICR_ADC,
    CR_DMIC,
    CR_MIC1,
    CR_MIC2,
    CR_DAC,
    CR_DAC2,
    CR_ADC,
    CR_MIX,
    DR_MIX,
    CR_VIC,
    CR_CK,
    FCR_DAC,
    SFCCR_DAC,
    SFFCR_DAC,
    FCR_ADC,
    CR_TIMER_MSB,
    CR_TIMER_LSB,
    ICR,
    IMR,
    IFR,
    IMR2,
    IFR2,
    GCR_DACL,
    GCR_DACR,
    GCR_DACL2,
    GCR_DACR2,
    GCR_MIC1,
    GCR_MIC2,
    GCR_ADCL,
    GCR_ADCR,
    GCR_MIXDACL,
    GCR_MIXDACR,
    GCR_MIXADCL,
    GCR_MIXADCR,
    CR_DAC_AGC,
    DR_DAC_AGC,
    CR_DAC2_AGC,
    DR_DAC2_AGC,
    CR_ADC_AGC,
    DR_ADC_AGC,
    SR_ADC_AGCDGL,
    SR_ADC_AGCDGR,
    SR_ADC_AGCAGL,
    SR_ADC_AGCAGR,
    CR_TR,
    DR_TR,
    SR_TR1,
    SR_TR2,
    SR_TR_SRCDAC,

    /*
     * 接下来是需要特殊操作的寄存器
     */
    MIX_0,
    MIX_1,
    MIX_2,
    MIX_3,
    MIX_4,

    DAC_AGC_0,
    DAC_AGC_1,
    DAC_AGC_2,
    DAC_AGC_3,

    DAC2_AGC_0,
    DAC2_AGC_1,
    DAC2_AGC_2,
    DAC2_AGC_3,

    ADC_AGC_0,
    ADC_AGC_1,
    ADC_AGC_2,
    ADC_AGC_3,
    ADC_AGC_4,
    MAX_REG_NUM,
};

#define PON_ACK 7, 7
#define IRQ_ACK 6, 6
#define DAC_LOCKED 4, 4

#define DAC_UNKOWN_FS 4, 4

#define ADC_MUTE 3, 4
#define DAC_MUTE 0, 1

#define DAC_ADWL 6, 7
#define DAC_SLAVE 5, 5
#define SB_AICR_DAC 4, 4
#define DAC_AUDIOIF 0, 1

#define ADC_ADWL 6, 7
#define SB_AICR_ADC 4, 4
#define ADC_AUDIOIF 0, 1

#define DMIC_CLKLON 7, 7

#define MICIDFF1 6, 6
#define SB_MICBIAS1 5, 5
#define SB_MIC1 4, 4
#define MICBIAS1_V 3, 3

#define DAC_SOFT_MUTE 7, 7
#define SB_DAC 4, 4
#define DAC_ZERO_N 0, 0

#define DAC2_LEFT_ONLY 5, 5
#define SB_DAC2 4, 4
#define DAC2_ZERO_N 3, 3
#define DAC2_DIT 0, 2

#define ADC_SOFT_MUTE 7, 7
#define ADC_DMIC_SEL 6, 6
#define SB_ADC 4, 4

#define MIX_EN 7, 7
#define MIX_LOAD 6
#define MIX_ADD 0, 5

#define AIDACL_SEL 6, 7
#define AIDACR_SEL 4, 5
#define DAC_MIX 0, 0

#define MIXDACL_SEL 6, 7
#define MIXDACR_SEL 4, 5

#define AIADCL_SEL 6, 7
#define AIADCR_SEL 4, 5
#define MIX_REC

#define MIXADCL_SEL 6, 7
#define MIXADCR_SEL 4, 5

#define MIXADC2L_SEL 6, 7
#define MIXADC2R_SEL 4, 5


#define SB_SLEEP 1, 1
#define SB 0, 0

#define MCLK_DIV 6, 6
#define SHUTDOWN_CLOCK 4, 4
#define CRYSTAL 0, 3

#define DAC_FREQ 0, 3

#define DACFREQ_VALID 7, 7

#define DAC_FREQ_ADJ_H 0, 6

#define DAC_FREQ_ADJ_L 0, 7

#define ADC_HPF 6, 6
#define ADC_FREQ 0, 3

#define INT_FORM 6, 7

#define ADAS_LOCK_MASK 7, 7
#define ADC_MUTE_MASK 2, 2
#define DAC_MUTE_MASK 0, 0

#define ADAS_LOCK_EVENT 7, 7
#define ADC_MUTE_EVENT 2, 2
#define DAC_MUTE_EVENT 0, 0

#define TIMER_END_MASK 4, 4

#define TIMER_END 4, 4

#define LRGOD 7, 7
#define GODL 0, 5

#define GODR 0, 5

#define GODL2 0, 5

#define GODR2 0, 5

#define GIM1 0, 2

#define LRGID 7, 7
#define GIDL 0, 5

#define LRGOMIX 7, 7
#define GOMIXL 0, 5

#define GOMIXR 0, 5

#define LRGIMIX 7, 7
#define GIMIXL 0, 5

#define GIMIXR 0, 5

#define DAC_AGC_EN 7, 7
#define DAC_AGC_LOAD 6
#define DAC_AGC_ADD 0, 5

#define LR_DRC 7, 7
#define LTHRES 0, 4

#define LCOMP 0, 2

#define RTHRES 0, 4

#define RCOMP 0, 2

#define ADC_AGC_EN 7, 7
#define ADC_AGC_LOAD 6
#define ADC_AGC_ADD 0, 5

#define TARGET 2, 5

#define NG_EN 7, 7
#define NG_THR 4, 6
#define HOLD 0, 3

#define ATK 4, 7
#define DCY 0, 3

#define AGC_MAX 0, 4

#define AGC_MIN 0, 4

#define FAST_ON 7, 7

#define ICRST 31, 31
#define RGWR 16, 16
#define RGADDR 8, 14
#define RGDIN 0, 7

#define IRQ 8, 8
#define RGDOUT 0, 7

#endif /* _ICODEC_REGS_H_ */