#ifndef __X1800_CODEC_H__
#define __X1800_CODEC_H__

#include <linux/gpio.h>
#include <mach/jzsnd.h>
#include <linux/bitops.h>

#define BASE_ADDR_CODEC         0x10021000

#define TS_CODEC_CGR_00         0x00
#define TS_CODEC_CACR_02        0x08
#define TS_CODEC_CMCR_03        0x0c
#define TS_CODEC_CDCR1_04       0x10
#define TS_CODEC_CDCR2_05       0x14
#define TS_CODEC_CADR_07        0x1c
#define TS_CODEC_CGAINR_0a      0x28
#define TS_CODEC_CASR           0x34
#define TS_CODEC_CDPR_0e        0x38
#define TS_CODEC_CDDPR2_0f      0x3c
#define TS_CODEC_CDDPR1_10      0x40
#define TS_CODEC_CDDPR0_11      0x44
#define TS_CODEC_CAACR_21       0x84
#define TS_CODEC_CMICCR_22      0x88
#define TS_CODEC_CACR2_23       0x8c
#define TS_CODEC_CAMPCR_24      0x90
#define TS_CODEC_CAR_25         0x94
#define TS_CODEC_CHR_26         0x98
#define TS_CODEC_CHCR_27        0x9c
#define TS_CODEC_CCR_28         0xa0
#define TS_CODEC_CMR_40         0x100
#define TS_CODEC_CTR_41         0x104
#define TS_CODEC_CAGCCR_42      0x108
#define TS_CODEC_CPGR_43        0x10c
#define TS_CODEC_CSRR_44        0x110
#define TS_CODEC_CALMR_45       0x114
#define TS_CODEC_CAHMR_46       0x118
#define TS_CODEC_CALMINR_47     0x11c
#define TS_CODEC_CAHMINR_48     0x120
#define TS_CODEC_CAFG_49        0x124
#define TS_CODEC_CCAGVR_4c      0x130

#define ADC_VALID_DATA_LEN_16BIT    0x0
#define ADC_VALID_DATA_LEN_20BIT    0x1
#define ADC_VALID_DATA_LEN_24BIT    0x2
#define ADC_VALID_DATA_LEN_32BIT    0x3

#define ADC_I2S_INTERFACE_RJ_MODE   0x0
#define ADC_I2S_INTERFACE_LJ_MODE   0x1
#define ADC_I2S_INTERFACE_I2S_MODE  0x2
#define ADC_I2S_INTERFACE_PCM_MODE  0x3

#define CHOOSE_ADC_CHN_STEREO       0x0
#define CHOOSE_ADC_CHN_MONO         0x1

#define SAMPLE_RATE_8K      0x7
#define SAMPLE_RATE_12K     0x6
#define SAMPLE_RATE_16K     0x5
#define SAMPLE_RATE_24K     0x4
#define SAMPLE_RATE_32K     0x3
#define SAMPLE_RATE_44_1K   0x2
#define SAMPLE_RATE_48K     0x1
#define SAMPLE_RATE_96K     0x0

struct codec_operation {
    struct resource *res;
    void __iomem *iomem;
    struct device *dev;
    char name[16];
    void *priv;
};

static inline unsigned int codec_reg_read(struct codec_operation * ope, int offset)
{
    return readl(ope->iomem + offset);
}

static inline void codec_reg_write(struct codec_operation * ope, int offset, int data)
{
    writel(data, ope->iomem + offset);
}

int jz_codec_init(void);
void jz_codec_exit(void);

/**
 * sound device
 **/
enum snd_device_t {
    SND_DEVICE_DEFAULT = 0,

    SND_DEVICE_CURRENT,
    SND_DEVICE_HANDSET,
    SND_DEVICE_HEADSET, //fix for mid pretest
    SND_DEVICE_SPEAKER, //fix for mid pretest
    SND_DEVICE_HEADPHONE,

    SND_DEVICE_BT,
    SND_DEVICE_BT_EC_OFF,
    SND_DEVICE_HEADSET_AND_SPEAKER,
    SND_DEVICE_TTY_FULL,
    SND_DEVICE_CARKIT,

    SND_DEVICE_FM_SPEAKER,
    SND_DEVICE_FM_HEADSET,
    SND_DEVICE_BUILDIN_MIC,
    SND_DEVICE_HEADSET_MIC,
    SND_DEVICE_HDMI = 15,   //fix for mid pretest

    SND_DEVICE_LOOP_TEST = 16,

    SND_DEVICE_CALL_START,                          //call route start mark
    SND_DEVICE_CALL_HEADPHONE = SND_DEVICE_CALL_START,
    SND_DEVICE_CALL_HEADSET,
    SND_DEVICE_CALL_HANDSET,
    SND_DEVICE_CALL_SPEAKER,
    SND_DEVICE_HEADSET_RECORD_INCALL,
    SND_DEVICE_BUILDIN_RECORD_INCALL,
    SND_DEVICE_CALL_END = SND_DEVICE_BUILDIN_RECORD_INCALL, //call route end mark

    SND_DEVICE_LINEIN_RECORD,
    SND_DEVICE_LINEIN1_RECORD,
    SND_DEVICE_LINEIN2_RECORD,
    SND_DEVICE_LINEIN3_RECORD,

    SND_DEVICE_COUNT
};

void codec_set_record_volume(int * vol);
int codec_get_record_volume(void);
void codec_set_play_volume(int * vol);
int codec_get_play_volume(void);

#endif
