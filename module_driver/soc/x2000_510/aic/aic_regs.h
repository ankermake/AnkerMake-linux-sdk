#ifndef _AIC_REGS_H_
#define _AIC_REGS_H_

#define AUDIO_AIC0_BASE 0x134d5000
#define AUDIO_AIC1_BASE 0x134d6000
#define AUDIO_AIC2_BASE 0x134d7000
#define AUDIO_AIC3_BASE 0x134d8000
#define AUDIO_AIC4_BASE 0x134d9000

#define BAICRCTL 0x00
#define BAICRCFG 0x04
#define BAICRDIV 0x08
#define BAICRCGR 0x0c
#define BAICTCTL 0x10
#define BAICTCFG 0x14
#define BAICTDIV 0x18
#define BAICTCGR 0x1c
#define BAICTLCR 0x20
#define BAICCCR 0x24
#define BAICISR 0x28
#define BAICIMR 0x2c

#define R_RST 0, 0

#define R_SLOT 24, 27
#define R_SWLR 20, 20
#define R_CHANNEL 17, 19
#define R_SLS 13, 14
#define R_ISYNC 12, 12
#define R_ASVTSU 11, 11
#define ISS 8, 10
#define R_MODE 2, 5
#define R_NEG 1, 1
#define R_MASTER 0, 0

#define R_BCLKDIV 16, 23
#define R_SYNL 8, 15
#define R_SYNC_DIV 0, 7

#define R_GFCLK 4, 4
#define R_GTCLK 3, 3
#define R_GSCLK 2, 2
#define R_GHCLK 1, 1
#define R_GCLK 0, 0

#define CODE_RST 4, 4
#define T_RST 0, 0

#define T_SLOT 24, 27
#define T_SWLR 20, 20
#define T_CHANNEL 17, 19
#define T_SLS 13, 14
#define T_ISYNC 12, 12
#define T_ASVTSU 11, 11
#define OSS 8, 10
#define T_MODE 2, 5
#define T_NEG 1, 1
#define T_MASTER 0, 0

#define T_BCLKDIV 16, 23
#define T_SYNL 8, 15
#define T_SYNC_DIV 0, 7

#define T_GFCLK 4, 4
#define T_GTCLK 3, 3
#define T_GSCLK 2, 2
#define T_GHCLK 1, 1
#define T_GCLK 0, 0

#define CLK_SPLIT_EN 0, 0

#define TEN 3, 3
#define REN 2, 2


enum aic_data_mode {
    AIC_PCM_A,
    AIC_PCM_B,
    AIC_DSP_A,
    AIC_DSP_B,
    AIC_I2S,
    AIC_LEFT_J,
    AIC_RIGHT_J,
    AIC_RESERVED,
    AIC_TDM1_A,
    AIC_TDM1_B,
    AIC_TDM2_A,
    AIC_TDM2_B,
};

#endif /* _AIC_REGS_H_ */
