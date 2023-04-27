#include <bit_field.h>
#include "dmic_regs.h"

#define DMIC_IOBASE 0x10023000

#define DMIC_REG_BASE  KSEG1ADDR(DMIC_IOBASE)

#define DMIC_ADDR(reg) ((volatile unsigned long *)(DMIC_REG_BASE + (reg)))

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

static unsigned int sub_pos(unsigned int size, unsigned int pos, unsigned int delta)
{
    return (pos + size - delta) % size;
}

static unsigned int add_pos(unsigned int size, unsigned int pos, unsigned int delta)
{
    return (pos + delta) % size;
}

static void dmic_enable(int channels, int sample_rate, int gain)
{
    dmic_set_bit(DMIC_CR0, RESET, 1);
    while (dmic_get_bit(DMIC_CR0, RESET));

    int sr = 0;
    if (sample_rate == 8000)
        sr = 0;
    if (sample_rate == 16000)
        sr = 1;
    if (sample_rate == 48000)
        sr = 2;

    unsigned long dmic_cr0 = 0;
    set_bit_field(&dmic_cr0, CHNUM, channels - 1);
    set_bit_field(&dmic_cr0, UNPACK_MSB, 0);
    set_bit_field(&dmic_cr0, UNPACK_DIS, 1);
    set_bit_field(&dmic_cr0, SW_LR, 0);
    set_bit_field(&dmic_cr0, PACK_EN, 1);
    set_bit_field(&dmic_cr0, SR, sr);
    set_bit_field(&dmic_cr0, LP_MODE, 0);
    set_bit_field(&dmic_cr0, HPF1_EN, 1);
    dmic_write_reg(DMIC_CR0, dmic_cr0);

    dmic_write_reg(DMIC_GCR, gain);

    dmic_set_bit(TRI_CR, HPF2_EN, 1);
    dmic_write_reg(THR_H, 32);
    dmic_write_reg(THR_L, 16);

    unsigned long dmic_fcr = 0;
    set_bit_field(&dmic_fcr, RDMS, 1);
    set_bit_field(&dmic_fcr, FIFO_THR, 32);
    dmic_write_reg(DMIC_FCR, dmic_fcr);

    unsigned long dmic_imr = bit_field_mask(0, 5);
    set_bit_field(&dmic_imr, PRE_READ_MASK, 0);
    dmic_write_reg(DMIC_IMR, dmic_imr);

    dmic_write_reg(DMIC_ICR, bit_field_mask(0, 5));

    enable_irq(IRQ_DMIC);
}

static void dmic_start(void)
{
    unsigned long dmic_cr0 = dmic_read_reg(DMIC_CR0);
    set_bit_field(&dmic_cr0, TRI_EN, 1);
    set_bit_field(&dmic_cr0, DMIC_EN, 1);
    dmic_write_reg(DMIC_CR0, dmic_cr0);
}

static void dmic_stop(void)
{
    dmic_set_bit(DMIC_CR0, DMIC_EN, 0);
}

static void dmic_disable(void)
{
    disable_irq(IRQ_DMIC);
    dmic_stop();
}

static int dmic_capture_get_volume(void)
{
    int val = dmic_get_bit(DMIC_GCR, DGAIN);

    return val;
}

static int dmic_capture_set_volume(int val)
{
    dmic_write_reg(DMIC_GCR, val);

    return 0;
}