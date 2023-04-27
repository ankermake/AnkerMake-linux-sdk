
#include <linux/kernel.h>
#include <linux/delay.h>
#include "icodec_regs.h"
#include <bit_field.h>
#include <linux/spinlock.h>

static DEFINE_SPINLOCK(icodec_lock);

#define ICODEC_BASE 0xb0020000

#define ICODEC_ADDR(reg) ((volatile unsigned long *)KSEG1ADDR(ICODEC_BASE + (reg)))

static inline void icodec_write_reg(unsigned int reg, unsigned int value)
{
    unsigned long flags;

    spin_lock_irqsave(&icodec_lock, flags);
    *ICODEC_ADDR(reg) = value;
    spin_unlock_irqrestore(&icodec_lock, flags);
}

static inline unsigned int icodec_read_reg(unsigned int reg)
{
    return *ICODEC_ADDR(reg);
}

static inline void icodec_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    unsigned long flags;

    spin_lock_irqsave(&icodec_lock, flags);
    set_bit_field(ICODEC_ADDR(reg), start, end, val);
    spin_unlock_irqrestore(&icodec_lock, flags);

    // printk(KERN_EMERG "icodec: %x*4 (%d %d) %d\n", reg/4, start, end, val);
}

static inline unsigned int icodec_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(ICODEC_ADDR(reg), start, end);
}

static void icodec_power_on(void)
{
    icodec_set_bit(Control5, POP_CTL, 1);

    icodec_write_reg(Select_charge, 0x01);

    icodec_set_bit(Control1, REF_VOL_EN, 1);

    int i;
    for (i = 0; i <= 0xff; i++) {
        icodec_write_reg(Select_charge, i);
        udelay(200);
    }

    icodec_write_reg(Select_charge, 0x7e);
    msleep(20);
}

static void icodec_power_off(void)
{
    icodec_write_reg(Select_charge, 0x01);

    icodec_set_bit(Control1, REF_VOL_EN, 0);

    int i;
    for (i = 0; i <= 0xff; i++) {
        icodec_write_reg(Select_charge, i);
        udelay(200);
    }

    icodec_set_bit(Reset, SYS_RST, 0);
    udelay(200);
    icodec_set_bit(Reset, 0, 1, 3);
}

static void icodec_enable_adc(int bias_enable, int bias_level, int mic_in_gain, int vol)
{
    icodec_set_bit(Control2, ADCL_MUTE, 1);

    icodec_set_bit(Control1, ADC_CUR_EN, 1);

    /* set adc current value*/
    icodec_set_bit(Control3, 0, 3, 8);

    icodec_set_bit(Control2, ADCL_VOL_EN, 1);

    icodec_set_bit(Control3, ADCL_MIC_EN, 1);

    icodec_set_bit(Control3, ADCL_EN, 1);

    icodec_set_bit(Control5, ADCL_CLK_EN, 1);

    icodec_set_bit(Control5, ADCL_ADC_EN, 1);

    icodec_set_bit(Control5, ADCL_ADC_INIT, 1);

    icodec_set_bit(Control5, ADCL_ALC_INIT, 1);

    icodec_set_bit(Control2, ADCL_INITIAL, 1);

    /*set high filter */
    icodec_set_bit(Chooose_ALC_Gain, 0, 2, 1);

    icodec_set_bit(Control3, ADCL_MIC_GAIN, mic_in_gain); // 30b mic gain

    icodec_set_bit(Chooose_ALC_Gain, PGA_GAIN_SEL, 0);

    icodec_set_bit(Control4, ALCL_GAIN, vol); // alc gain

    icodec_set_bit(Control1, BIAS_VOL_EN, bias_enable);

    icodec_set_bit(Control1, BIAS_VOL, bias_level);


    icodec_set_bit(Control2, ADCL_ZERO, 1);

    icodec_set_bit(Control2, ADCL_MUTE, 0);

    udelay(100);

    icodec_set_bit(Control2, ADCL_MUTE, 1);
}

static void icodec_disable_adc(void)
{
    icodec_set_bit(Control2, ADCL_ZERO, 0);

    icodec_set_bit(Control5, ADCL_ADC_EN, 0);

    icodec_set_bit(Control5, ADCL_CLK_EN, 0);

    icodec_set_bit(Control3, ADCL_EN, 0);

    icodec_set_bit(Control2, ADCL_VOL_EN, 0);

    icodec_set_bit(Control1, ADC_CUR_EN, 0);

    icodec_set_bit(Control5, ADCL_ADC_INIT, 0);

    icodec_set_bit(Control5, ADCL_ALC_INIT, 0);

    icodec_set_bit(Control2, ADCL_MUTE, 0);
}

void icodec_enable_dac(int is_mute, int vol)
{
    unsigned long control5 = icodec_read_reg(Control5);
    set_bit_field(&control5, DAC_CUR_EN, 1);
    set_bit_field(&control5, DAC_REF_EN, 1);
    set_bit_field(&control5, POP_CTL, 2);
    icodec_write_reg(Control5, control5);

    unsigned long control7 = icodec_read_reg(Control7);
    set_bit_field(&control7, HPOUTL_INIT, 1);
    set_bit_field(&control7, HPOUTL_EN, 1);
    set_bit_field(&control7, HPOUTL_MUTE, 0);
    icodec_write_reg(Control7, control7);

    unsigned long control6 = icodec_read_reg(Control6);
    set_bit_field(&control6, DACL_REF_EN, 1);
    set_bit_field(&control6, DACL_CLK_EN, 1);
    set_bit_field(&control6, DACL_EN, 1);
    set_bit_field(&control6, DACL_INIT, 1);
    icodec_write_reg(Control6, control6);

    udelay(30*1000);
    icodec_set_bit(Control5, DAC_CUR_EN, 0);
    icodec_set_bit(Control5, DAC_REF_EN, 0);
    udelay(30*1000);

    control5 = icodec_read_reg(Control5);
    set_bit_field(&control5, DAC_CUR_EN, 1);
    set_bit_field(&control5, DAC_REF_EN, 1);
    set_bit_field(&control5, POP_CTL, 2);
    icodec_write_reg(Control5, control5);

    control7 = icodec_read_reg(Control7);
    set_bit_field(&control7, HPOUTL_MUTE, !is_mute);
    set_bit_field(&control7, HPOUTL_GAIN, vol);
    icodec_write_reg(Control7, control7);
}

static void icodec_disable_dac(void)
{
    icodec_set_bit(Control7, HPOUTL_GAIN, 0); // 增益调到最小

    icodec_set_bit(Control7, HPOUTL_MUTE, 0);

    icodec_set_bit(Control7, HPOUTL_INIT, 0);

    icodec_set_bit(Control7, HPOUTL_EN, 0);

    icodec_set_bit(Control6, DACL_EN, 0);

    icodec_set_bit(Control6, DACL_CLK_EN, 0);

    icodec_set_bit(Control6, DACL_REF_EN, 0);

    icodec_set_bit(Control5, POP_CTL, 1);

    icodec_set_bit(Control5, DAC_REF_EN, 0);

    icodec_set_bit(Control5, DAC_CUR_EN, 0);

    icodec_set_bit(Control6, DACL_INIT, 0);
}

static void icodec_config_adc(int data_bits)
{
    int len = 0;

    if (data_bits == 32) len = 3;
    if (data_bits == 24) len = 2;
    if (data_bits == 20) len = 1;
    if (data_bits == 16) len = 0;

    unsigned long adc_dac_cfg2 = icodec_read_reg(ADC_DAC_Configure2);
    set_bit_field(&adc_dac_cfg2, ADC_LENGTH, 3); // 32 bit clk
    set_bit_field(&adc_dac_cfg2, ADC_IO_Master, 0);
    set_bit_field(&adc_dac_cfg2, ADC_inner_Master, 0);
    set_bit_field(&adc_dac_cfg2, ADC_RESET, 1);
    set_bit_field(&adc_dac_cfg2, ADC_BIT_POLARITY, 0);
    icodec_write_reg(ADC_DAC_Configure2, adc_dac_cfg2);

    unsigned long adc_dac_cfg1 = 0;
    set_bit_field(&adc_dac_cfg1, ADC_LRC_POL, 0);
    set_bit_field(&adc_dac_cfg1, ADC_VALID_LEN, len);
    set_bit_field(&adc_dac_cfg1, ADC_MODE, 2); // i2s mode
    set_bit_field(&adc_dac_cfg1, ADC_SWAP, 0);
    icodec_write_reg(ADC_DAC_Configure1, adc_dac_cfg1);
}

static void icodec_config_dac(int data_bits)
{
    int len = 0;

    if (data_bits == 32) len = 3;
    if (data_bits == 24) len = 2;
    if (data_bits == 20) len = 1;
    if (data_bits == 16) len = 0;

    unsigned long adc_dac_cfg4 = 0;
    set_bit_field(&adc_dac_cfg4, DAC_LENGTH, 3); // 32 bit bclk
    set_bit_field(&adc_dac_cfg4, DAC_RESET, 1);
    set_bit_field(&adc_dac_cfg4, DAC_BIT_POLARITY, 0);
    icodec_write_reg(ADC_DAC_Configure4, adc_dac_cfg4);

    unsigned long adc_dac_cfg2 = icodec_read_reg(ADC_DAC_Configure2);
    set_bit_field(&adc_dac_cfg2, DAC_IO_Master, 0);
    set_bit_field(&adc_dac_cfg2, DAC_inner_Master, 0);
    icodec_write_reg(ADC_DAC_Configure2, adc_dac_cfg2);

    unsigned long adc_dac_cfg3 = 0;
    set_bit_field(&adc_dac_cfg3, DAC_LRC_POLARITY, 0);
    set_bit_field(&adc_dac_cfg3, DAC_VALID_LENGTH, len);
    set_bit_field(&adc_dac_cfg3, DAC_MODE, 2); // i2s mode
    set_bit_field(&adc_dac_cfg3, DAC_SWAP, 1);
    icodec_write_reg(ADC_DAC_Configure3, adc_dac_cfg3);

}

static void icodec_set_adc_gain(int gain)
{
    icodec_set_bit(Control4, ALCL_GAIN, gain);
}

static unsigned int icodec_get_adc_gain(void)
{
    return icodec_get_bit(Control4, ALCL_GAIN);
}

static void icodec_set_dac_gain(int gain)
{
    icodec_set_bit(Control7, HPOUTL_GAIN, gain);
}

static unsigned int icodec_get_dac_gain(void)
{
    return icodec_get_bit(Control7, HPOUTL_GAIN);
}

static void icodec_set_dac_mute(int is_mute)
{
    if (icodec_get_bit(Control7, HPOUTL_MUTE) != is_mute)
        icodec_set_bit(Control7, HPOUTL_MUTE, !is_mute);
}

static int icodec_get_dac_mute(void)
{
    return !icodec_get_bit(Control7, HPOUTL_MUTE);
}

static unsigned int reg_tab[] = {0x00, 0x02, 0x03, 0x4, 0x5, 0x0a, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
    0x27, 0x28, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49};


static inline void dump_regs(void)
{
    int i;
    for(i=0; i < ARRAY_SIZE(reg_tab); i++) {
        printk(KERN_EMERG "reg(0x%x) = 0x%x\n", reg_tab[i], icodec_read_reg(4 * reg_tab[i]));
    }
}


