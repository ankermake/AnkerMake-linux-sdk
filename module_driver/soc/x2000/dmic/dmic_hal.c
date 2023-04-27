#include <bit_field.h>
#include <linux/slab.h>
#include "dmic_regs.h"
#include "../audio_dma/audio_regs.h"

#define DMIC_ADDR(reg) ((volatile unsigned long *)KSEG1ADDR(DMIC_IOBASE + (reg)))

void dmic_write_reg(unsigned int reg, int val)
{
    *DMIC_ADDR(reg) = val;
}

unsigned int dmic_read_reg(unsigned int reg)
{
    return *DMIC_ADDR(reg);
}

void dmic_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(DMIC_ADDR(reg), start, end, val);
}

unsigned int dmic_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(DMIC_ADDR(reg), start, end);
}

static inline unsigned int sub_pos(unsigned int size, unsigned int pos, unsigned int delta)
{
    return (pos + size - delta) % size;
}

static unsigned int add_pos(unsigned int size, unsigned int pos, unsigned int delta)
{
    return (pos + delta) % size;
}

static inline int to_sr(unsigned int rate)
{
    if (rate == 8000) return 0;
    if (rate == 16000) return 1;
    if (rate == 48000) return 2;
    if (rate == 96000) return 3;
    return 1;
}

static void dmic_config(int channels, int sample_rate, int gain, int fmt_width)
{
    dmic_set_bit(DMIC_CR0, D_HPF1_EN, 1);
    dmic_set_bit(DMIC_CR0, D_HPF2_EN, 1);
    dmic_set_bit(DMIC_CR0, D_LPF_EN, 1);
    dmic_set_bit(DMIC_CR0, D_SW_LR, 1);

    dmic_set_bit(DMIC_GCR, D_DGAIN, gain);

    dmic_set_bit(DMIC_CR0, D_RESET, 1);
    while (dmic_get_bit(DMIC_CR0, D_RESET));

    dmic_set_bit(DMIC_CR0, D_CHNUM, channels - 1);
    dmic_set_bit(DMIC_CR0, D_OSS, fmt_width == 16 ? 0 : 1);
    dmic_set_bit(DMIC_CR0, D_SR, to_sr(sample_rate));
}

static void dmic_enable(void)
{
    dmic_set_bit(DMIC_CR0, D_DMIC_EN, 1);
}

static void dmic_disable(void)
{
    dmic_set_bit(DMIC_CR0, D_DMIC_EN, 0);
}

static int dmic_capture_get_volume(void)
{
    return dmic_get_bit(DMIC_GCR, D_DGAIN);
}

static int dmic_capture_set_volume(int val)
{
    dmic_set_bit(DMIC_GCR, D_DGAIN, val);

    return 0;
}
