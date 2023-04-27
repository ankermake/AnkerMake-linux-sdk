#include <bit_field.h>
#include "icodec_regs.h"

#define CODEC_IOBASE 0x10021000

#define ICODEC_REG_BASE  KSEG1ADDR(CODEC_IOBASE)

#define CODEC_ADDR(reg) ((volatile unsigned long *)(ICODEC_REG_BASE + (reg)))

void codec_write_reg(unsigned int reg, int val)
{
    *CODEC_ADDR(reg) = val;
}

unsigned int codec_read_reg(unsigned int reg)
{
    return *CODEC_ADDR(reg);
}

void codec_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(CODEC_ADDR(reg), start, end, val);
}

unsigned int codec_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(CODEC_ADDR(reg), start, end);
}

void icodec_power_on(void)
{
    // codec reset
    codec_set_bit(CGR, srst, 0);
    msleep(10);
    codec_set_bit(CGR, srst, 1);
    codec_set_bit(CGR, dcrst, 1);
    msleep(10);

    // codec i2s master mode
    codec_set_bit(CMCR, Masteren, 1);
    codec_set_bit(CMCR, adcdacmaster, 1);

    // precharge
    codec_set_bit(CHCR, hpoutpop, 1);
    msleep(20);
    codec_set_bit(CCR, dacprecharge, 1);
    msleep(20);

    int i;
    for (i = 0; i < 6; i++) {
        codec_set_bit(CCR, chargeselect, 0x3f >> (6-i));
        msleep(20);
    }

    msleep(20);
    codec_set_bit(CCR, chargeselect, 0x1);
    msleep(20);

    codec_set_bit(CMICCR, Micmute, 1); // 影响播放的音量大小
    msleep(1);
}

void icodec_power_off(void)
{
    // discharge
    codec_set_bit(CHCR, hpoutpop, 1);
    codec_set_bit(CCR, chargeselect, 0x3f);
    codec_set_bit(CCR, dacprecharge, 0);

    int i;
    for (i = 0; i < 6; i++) {
        codec_set_bit(CCR, chargeselect, 0x3f >> (6-i));
        msleep(20);
    }
}

void icodec_enable_dac(void)
{
    codec_set_bit(CDCR2, dacdalength, 3); // dac 32bit 长的i2s信号
    codec_set_bit(CDCR1, dacvaldalength, 2); // 24 bit dac 数据
    codec_set_bit(CDCR1, daci2smode, 2); // i2s 模式
    codec_set_bit(CDCR1, dacswap, 0); // not swap left-right data
    codec_set_bit(CDCR1, dacenI2S, 0);
    msleep(10);

    codec_set_bit(CAACR, adcsrcen, 1);
    msleep(1);

    codec_set_bit(CHR, hpoutmute, 0);
    msleep(1);

    codec_set_bit(CAR, Audiodacen, 1);
    msleep(1);

    codec_set_bit(CAR, Dacrefvolen, 1);
    msleep(1);

    codec_set_bit(CHCR, hpoutpop, 2);
    msleep(1);

    codec_set_bit(CAR, dacrefvolen, 1);
    msleep(1);

    codec_set_bit(CAR, daclclken, 1);
    msleep(1);

    codec_set_bit(CAR, dacen, 1);
    msleep(1);

    codec_set_bit(CAR, dacinit, 1);
    msleep(10);

    codec_set_bit(CHR, hpouten, 1);
    msleep(1);

    codec_set_bit(CHR, hpoutinit, 1);
    msleep(1);

    codec_set_bit(CHR, hpoutmute, 1);
    msleep(1);

    codec_set_bit(CHCR, hpoutgain, hpout_gain);
    msleep(1);

    codec_set_bit(CASR, daclechsrc, 0); // i2s 作为dac输出
}

void icodec_disable_dac(void)
{
    codec_set_bit(CHCR, hpoutgain, 0);
    codec_set_bit(CHR, hpoutmute, 0);
    codec_set_bit(CHR, hpoutinit, 0);
    codec_set_bit(CHR, hpouten, 0);
    codec_set_bit(CAR, dacen, 0);
    codec_set_bit(CAR, daclclken, 0);
    codec_set_bit(CAR, dacrefvolen, 0);
    codec_set_bit(CHCR, hpoutpop, 0);
    codec_set_bit(CAR, Dacrefvolen, 0);
    codec_set_bit(CAR, Audiodacen, 0);
    codec_set_bit(CAR, dacinit, 0);
}

void icodec_enable_adc(void)
{
    codec_set_bit(CMCR, adcdalength, 3); // adc 32bit 长的i2s信号
    codec_set_bit(CACR, adcvaldalength, 2); // adc 有效数据长度 24bit
    codec_set_bit(CACR, adci2smode, 2); // adc i2s 模式
    codec_set_bit(CACR, adcswap, 1); // swap left-right data
    codec_set_bit(CACR, adcintf, 1); // 0 stereo 1 mono
    codec_set_bit(CACR, adcen, 1);

    codec_set_bit(CACR2, Alcsel, 0); // 差分模式
    msleep(10);

    codec_set_bit(CAACR, adcsrcen, 1);
    msleep(1);

    if (icodec.is_bais) {
        codec_set_bit(CAACR, Micbiasen, 1);
        msleep(1);
        codec_set_bit(CAACR, Micbaisctr, 7);
        msleep(10);
    } else {
        codec_set_bit(CAACR, Micbiasen, 0);
        msleep(1);
    }
    codec_set_bit(CAMPCR, adcrefvolen, 1);
    msleep(1);
    codec_set_bit(CAACR, 4, 4, 1); // 以前的驱动这么弄,手册上没有描述
    msleep(1);
    codec_set_bit(CMICCR, Alcmute, 1);
    msleep(1);
    codec_set_bit(CAMPCR, adclclken, 1);
    msleep(1);
    codec_set_bit(CAACR, 3, 3, 1); // 以前的驱动这么弄,手册上没有描述
    msleep(1);
    codec_set_bit(CAMPCR, adclrst, 1);
    msleep(1);
    codec_set_bit(CAMPCR, adclrst, 0);
    msleep(1);

    codec_set_bit(CMICCR, Micen, 1);
    msleep(1);
    codec_set_bit(CAMPCR, adclampen, 1);
    msleep(1);
    codec_set_bit(CMICCR, Alcen, 1);
    msleep(1);

    codec_set_bit(CAACR, adczeroen, 1);
    msleep(1);
    codec_set_bit(CACR2, Alcgain, mic_gain);
    msleep(1);
    codec_set_bit(CMICCR, Micgain, 1);
    msleep(1);

    codec_set_bit(CMICCR, 3, 3, 1); // 以前的驱动这么弄,手册上没有描述
    msleep(1);

    codec_set_bit(CADR, adcloopdac, 0);
    msleep(10);
}

void icodec_disable_adc(void)
{
    codec_set_bit(CACR2, Alcgain, 12); // 为啥不是0

    codec_set_bit(CMICCR, Micen, 0);
    codec_set_bit(CAMPCR, adclclken, 0);
    codec_set_bit(CAMPCR, adcrefvolen, 0);
    codec_set_bit(CAMPCR, adclrst, 0);
}

int icodec_playback_pcm_set_mute(int mute)
{
    codec_set_bit(CHR, hpoutmute, !mute);
    return 0;
}

int icodec_playback_pcm_get_mute(void)
{
    return !codec_get_bit(CHR, hpoutmute);
}

int icodec_playback_pcm_set_volume(int val)
{
    // 当音量等于0时,选择0作为dac的输出
    codec_set_bit(CASR, daclechsrc, val ? 0 : 3);

    // 增益是 0 时也有音量
    codec_set_bit(CHCR, hpoutgain, val);

    return 0;
}

int icodec_playback_pcm_get_volume(void)
{
    int val;

    if (codec_get_bit(CASR, daclechsrc) == 3)
        val = 0;
    else
        val = codec_get_bit(CHCR, hpoutgain);

    return val;
}

int icodec_capture_pcm_set_volume(int val)
{
    codec_set_bit(CACR2, Alcgain, val);
    return 0;
}

int icodec_capture_pcm_get_volume(void)
{
    return codec_get_bit(CACR2, Alcgain);
}