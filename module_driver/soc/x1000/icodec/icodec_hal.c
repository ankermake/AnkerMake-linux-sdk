#include <bit_field.h>
#include <utils/clock.h>
#include "icodec_regs.h"

#define RGADW   ((volatile unsigned long *)KSEG1ADDR(0x100200A4))
#define RGDATA  ((volatile unsigned long *)KSEG1ADDR(0x100200A8))

static void wait_ready(void)
{
    int timeout = 0xfffff;

    uint64_t old = local_clock_us();

    while (get_bit_field(RGADW, RGWR)) {
        if (timeout-- == 0)
            panic("ICODEC: failed to wait RGADW ready\n");
    }

    uint64_t delta = local_clock_us() - old;
    if (delta > 1)
    printk(KERN_DEBUG "ICODEC:delta %lld\n", delta);
}

unsigned char icodec_read(unsigned int reg)
{
    wait_ready();

    unsigned long rgadw = *RGADW;
    set_bit_field(&rgadw, RGWR, 0);
    set_bit_field(&rgadw, RGADDR, reg);
    *RGADW = rgadw;
    *RGADW = rgadw;

    unsigned char value;
    value = get_bit_field(RGDATA, RGDOUT);
    value = get_bit_field(RGDATA, RGDOUT);
    value = get_bit_field(RGDATA, RGDOUT);
    value = get_bit_field(RGDATA, RGDOUT);
    value = get_bit_field(RGDATA, RGDOUT);

    return value;
}

void icodec_write(unsigned int reg, int value)
{
    wait_ready();

    unsigned long rgadw = *RGADW;
    set_bit_field(&rgadw, RGWR, 0);
    set_bit_field(&rgadw, RGADDR, reg);
    set_bit_field(&rgadw, RGDIN, value);
    *RGADW = rgadw;

    set_bit_field(&rgadw, RGWR, 1);
    *RGADW = rgadw;

    if (reg != IFR && reg != IFR2) {
        unsigned char tmp = icodec_read(reg);
        if (value != tmp);
            // panic("icodec: reg: %d not check ok: %x %x\n",
            //     (int)reg, (int)value, (int)tmp);
    }
}

void icodec_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    unsigned long tmp = icodec_read(reg);
    set_bit_field(&tmp, start, end, val);
    icodec_write(reg, tmp);
}

unsigned int icodec_get_bit(unsigned int reg, int start, int end)
{
    unsigned long value = icodec_read(reg);
    return get_bit_field(&value, start, end);
}

#define CR_LOAD 6, 6
#define CR_ADD 0, 5

static void icodec_ex_write(unsigned char reg, unsigned char value)
{
    int creg, dreg, index;

    switch (reg) {
    case MIX_0 ... MIX_4:
        creg = CR_MIX;
        dreg = DR_MIX;
        index = reg - MIX_0;
        break;
    case DAC_AGC_0 ... DAC_AGC_3:
        creg = CR_DAC_AGC;
        dreg = DR_DAC_AGC;
        index = reg - DAC_AGC_0;
        break;
    case DAC2_AGC_0 ... DAC2_AGC_3:
        creg = CR_DAC2_AGC;
        dreg = DR_DAC2_AGC;
        index = reg - DAC2_AGC_0;
        break;
    case ADC_AGC_0 ... ADC_AGC_4:
        creg = CR_ADC_AGC;
        dreg = DR_ADC_AGC;
        index = reg - ADC_AGC_0;
        break;
    default:
        return;
    }

    icodec_write(dreg, value);

    unsigned long cdata = icodec_read(creg);
    set_bit_field(&cdata, CR_LOAD, 1);
    set_bit_field(&cdata, CR_ADD, index);
    icodec_write(creg, cdata);

}

static unsigned char icodec_ex_read(unsigned char reg)
{
    int creg, dreg, index;

    switch (reg) {
    case MIX_0 ... MIX_4:
        creg = CR_MIX;
        dreg = DR_MIX;
        index = reg - MIX_0;
        break;
    case DAC_AGC_0 ... DAC_AGC_3:
        creg = CR_DAC_AGC;
        dreg = DR_DAC_AGC;
        index = reg - DAC_AGC_0;
        break;
    case DAC2_AGC_0 ... DAC2_AGC_3:
        creg = CR_DAC2_AGC;
        dreg = DR_DAC2_AGC;
        index = reg - DAC2_AGC_0;
        break;
    case ADC_AGC_0 ... ADC_AGC_4:
        creg = CR_ADC_AGC;
        dreg = DR_ADC_AGC;
        index = reg - ADC_AGC_0;
        break;
    default:
        return 0;
    }

    unsigned long cdata = icodec_read(creg);
    set_bit_field(&cdata, CR_LOAD, 0);
    set_bit_field(&cdata, CR_ADD, index);
    icodec_write(creg, cdata);

    return icodec_read(dreg);
}

static inline unsigned char icodec_ex_get_bit(unsigned char reg, int start, int end)
{
    unsigned long value = icodec_ex_read(reg);
    return get_bit_field(&value, start, end);
}

static inline void icodec_ex_set_bit(unsigned char reg, int start, int end, unsigned char value)
{
    unsigned long tmp = icodec_ex_read(reg);
    set_bit_field(&tmp, start, end, value);
    icodec_ex_write(reg, tmp);
}

static int is_enable;
static int dac_mute;
static int dac_volume_mute;
static int dac_mute_count;

void icodec_power_on(void)
{
    if (is_enable++ == 0) {
        icodec_set_bit(CR_VIC, SB, 0);
        msleep(250);
        icodec_set_bit(CR_VIC, SB_SLEEP, 0);
        msleep(10);
    }
}

void icodec_power_off(void)
{
    if (--is_enable == 0) {
        icodec_set_bit(CR_VIC, SB_SLEEP, 1);
        icodec_set_bit(CR_VIC, SB, 1);
    }
}

static void icodec_dac_mute(void)
{
    if (dac_mute_count++ == 0)
        icodec_set_bit(CR_DAC, DAC_SOFT_MUTE, 1);
}

static void icodec_dac_unmute(void)
{
    if (--dac_mute_count == 0)
        icodec_set_bit(CR_DAC, DAC_SOFT_MUTE, 0);
}

void icodec_enable_dac(void)
{
    icodec_write(FCR_DAC, icodec.dac_rate);

    unsigned char bit_sel = icodec.dac_format == SNDRV_PCM_FORMAT_S16_LE ? 0 : 3;
    icodec_set_bit(AICR_DAC, DAC_ADWL, bit_sel);

    icodec_set_bit(CR_DAC, SB_DAC, 0);
    icodec_set_bit(CR_DAC, DAC_ZERO_N, 0);
    icodec_set_bit(CR_DAC, DAC_SOFT_MUTE, 0);
    icodec_set_bit(GCR_DACL, GODL, 31 - hpout_gain);

    dac_volume_mute = 0;
    dac_mute = 0;
    dac_mute_count = 0;
}

void icodec_disable_dac(void)
{
    icodec_set_bit(CR_DAC, DAC_SOFT_MUTE, 1);
    icodec_set_bit(CR_DAC, SB_DAC, 1);
}

void icodec_enable_adc(void)
{
    icodec_write(FCR_ADC, icodec.adc_rate);

    unsigned char bit_sel = icodec.adc_format == SNDRV_PCM_FORMAT_S16_LE ? 0 : 3;
    icodec_set_bit(AICR_ADC, ADC_ADWL, bit_sel);

    icodec_set_bit(CR_MIC1, SB_MICBIAS1, !icodec.is_bais);
    icodec_set_bit(CR_MIC1, SB_MIC1, 0);

    icodec_set_bit(CR_ADC, SB_ADC, 0);
    icodec_set_bit(CR_ADC, ADC_DMIC_SEL, 0);
    icodec_set_bit(CR_ADC, ADC_SOFT_MUTE, 0);
    icodec_set_bit(GCR_ADCL, GIDL, 31 - mic_gain);
}

void icodec_disable_adc(void)
{
    icodec_set_bit(CR_MIC1, SB_MICBIAS1, 1);

    icodec_set_bit(CR_MIC1, SB_MIC1, 1);

    icodec_set_bit(CR_ADC, SB_ADC, 1);
}

int icodec_playback_pcm_set_mute(int mute)
{
    if (mute) {
        if (!dac_mute) {
            icodec_dac_mute();
            dac_mute = 1;
        }
    } else {
        if (dac_mute) {
            icodec_dac_unmute();
            dac_mute = 0;
        }
    }

    return 0;
}

int icodec_playback_pcm_get_mute(void)
{
    return !icodec_get_bit(CR_DAC, DAC_SOFT_MUTE);
}

int icodec_playback_pcm_set_volume(int val)
{
    icodec_set_bit(GCR_DACL, GODL, 31 - val);

    if (val) {
        if (dac_volume_mute) {
            icodec_dac_unmute();
            dac_volume_mute = 0;
        }
    } else {
        if (!dac_volume_mute) {
            icodec_dac_mute();
            dac_volume_mute = 1;
        }
    }

    return 0;
}

int icodec_playback_pcm_get_volume(void)
{
    int val = 0;

    if (dac_volume_mute) {
        val = 0;
    } else {
        /*
         * 实际测试出来的效果是 0 - 31 是增益逐渐变小, 32以上突然声音变大
         */
        val = icodec_get_bit(GCR_DACL, GODL);
    }

    return 31 - val;
}

int icodec_capture_pcm_set_volume(int val)
{
    icodec_set_bit(GCR_ADCL, GIDL, 31 - val);
    return 0;
}

int icodec_capture_pcm_get_volume(void)
{
    return 31 - icodec_get_bit(GCR_ADCL, GIDL);
}

void icodec_hal_init(void)
{
    /*
     * 打开 12M MCLK
     */
    unsigned long cr_ck = icodec_read(CR_CK);
    set_bit_field(&cr_ck, MCLK_DIV, 1);
    set_bit_field(&cr_ck, SHUTDOWN_CLOCK, 0);
    set_bit_field(&cr_ck, CRYSTAL, 0);
    icodec_write(CR_CK, cr_ck);

    /*
     * 使能DAC i2s 接口
     * 并设置 codec 为 master 模式
     */
    unsigned long aicr_dac = icodec_read(AICR_DAC);
    set_bit_field(&aicr_dac, DAC_SLAVE, 0);
    set_bit_field(&aicr_dac, DAC_AUDIOIF, 3);
    set_bit_field(&aicr_dac, SB_AICR_DAC, 0);
    icodec_write(AICR_DAC, aicr_dac);

    /*
     * 使能ADC i2s 接口
     */
    unsigned long aicr_adc = icodec_read(AICR_ADC);
    set_bit_field(&aicr_adc, ADC_AUDIOIF, 3);
    set_bit_field(&aicr_adc, SB_AICR_ADC, 0);
    icodec_write(AICR_ADC, aicr_adc);

    /*
     * DAC/ADC 左右通道共用一个增益
     * 默认增益: 20
     */
    icodec_set_bit(GCR_DACL, LRGOD, 1);
    icodec_set_bit(GCR_ADCL, LRGID, 1);
    icodec_set_bit(GCR_DACL, GODL, 20);
    icodec_set_bit(GCR_ADCL, GIDL, 20);

    /*
     * 屏蔽所有中断,清除中断标志
     */
    icodec_write(IMR, 0xff);
    icodec_write(IMR2, 0xff);
    icodec_write(IFR, 0xff);
    icodec_write(IFR2, 0xff);
}