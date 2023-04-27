#include "adc_hal.h"

#include <bit_field.h>
#include <linux/delay.h>

#define ADENA              0x00
#define ADCFG              0x04
#define ADCTRL                    0x08
#define ADSTATE              0x0c
#define ADADAT0              0x10
#define ADADAT1              0x14
#define ADCLK              0x20
#define ADSTB              0x24
#define ADRETM     0x28

#define AUX0EN    0
#define AUX1EN    1
#define AUX2EN    2
#define REPTEN    9
#define POW_OPT   14
#define POWER    15

#define RST      0

#define ARDYM0   0
#define ARDYM1   1
#define ARDYM2   2

#define ARDY0   0
#define ARDY1   1
#define ARDY2   2

#define ADATAL   0, 9
#define ADATAH   16, 25

#define CLKDIV    0, 7
#define CLKDIV_US 8, 15
#define CLKDIV_MS 16, 31

#define STABLE    0, 15

#define RETIME    0, 31

#define ALL_INTERRUPT       7

#define SADC_REG_BASE   0xB0070000

#define SADC_ADDR(reg) ((volatile unsigned long *)(SADC_REG_BASE + reg))

static inline void adc_write_reg(unsigned long reg, unsigned int value)
{
    *SADC_ADDR(reg) = value;
}

static inline unsigned int adc_read_reg(unsigned long reg)
{
    return *SADC_ADDR(reg);
}

static inline void adc_set_bit(unsigned long reg, int bit, unsigned int val)
{
    set_bit_field(SADC_ADDR(reg), bit, bit, val);
}

static inline unsigned int adc_get_bit(unsigned long reg, int bit)
{
    return get_bit_field(SADC_ADDR(reg), bit, bit);
}

static inline void adc_set_bits(unsigned long reg, int start, int end, unsigned int val)
{
    set_bit_field(SADC_ADDR(reg), start, end, val);
}

static inline unsigned int adc_get_bits(unsigned long reg, int start, int end)
{
    return get_bit_field(SADC_ADDR(reg), start, end);
}

//////////////////////////////////////////////////////////////

inline void adc_hal_disable_controller(void)
{
    unsigned int val;
    do {
        adc_set_bit(ADENA, POWER, 1);
        val = adc_get_bit(ADENA, POWER);
    } while (!val);
}

inline void adc_hal_enable_controller(void)
{
    unsigned int val;
    do {
        adc_set_bit(ADENA, POWER, 0);
        mdelay(2);
        val = adc_get_bit(ADENA, POWER);
    } while (val);
}

inline void adc_hal_enable_channel(unsigned int channel)
{
    unsigned int val;
    unsigned int auxen = AUX0EN + channel;

    do {
        adc_set_bit(ADENA, auxen, 1);
        val = adc_get_bit(ADENA, auxen);
    } while (!val);
}

inline void adc_hal_disable_channel(unsigned int channel)
{
    adc_set_bits(ADENA, AUX0EN, AUX2EN, 0);
}

inline unsigned int adc_hal_get_channel_status(unsigned int channel)
{
    return adc_get_bit(ADENA, AUX0EN + channel);
}

inline void adc_hal_disable_repeat_sampling(void)
{
    adc_set_bit(ADENA, REPTEN, 0);
}

inline void adc_hal_enable_repeat_sampling(void)
{
    adc_set_bit(ADENA, REPTEN, 1);
}

inline void adc_hal_enable_power_optimize(void)
{
    adc_set_bit(ADENA, POW_OPT, 1);
}

inline void adc_hal_disable_power_optimize(void)
{
    adc_set_bit(ADENA, POW_OPT, 0);
}

inline void adc_hal_enable_soft_reset(void)
{
    adc_set_bit(ADCFG, RST, 0);
}

inline void adc_hal_disable_soft_reset(void)
{
    adc_set_bit(ADCFG, RST, 1);
}

inline void adc_hal_enable_all_interrupt(void)
{
    adc_set_bits(ADCTRL, ARDYM0, ARDYM2, 0);
}

inline void adc_hal_mask_all_interrupt(void)
{
    adc_set_bits(ADCTRL, ARDYM0, ARDYM2, ALL_INTERRUPT);
}

inline void adc_hal_disable_interrupt(unsigned int channel)
{
    unsigned int bit = ARDYM0 + channel;
    adc_set_bit(ADCTRL, bit, 1);
}

inline void adc_hal_enable_interrupt(unsigned int channel)
{
    unsigned int bit = ARDYM0 + channel;
    adc_set_bit(ADCTRL, bit, 0);
}

inline unsigned int adc_hal_get_interrupt_value(int channel)
{
    return adc_get_bit(ADCTRL, ARDYM0 + channel);
}

inline void adc_hal_clean_all_interrupt_flag(void)
{
    adc_set_bits(ADSTATE, ARDY0, ARDY2, ALL_INTERRUPT);
}

inline unsigned int adc_hal_get_all_interrupt_flag(void)
{
    return adc_get_bits(ADSTATE, ARDY0, ARDY2);
}

inline void adc_hal_clean_interrupt_flag(unsigned int channel)
{
    unsigned int bit = ARDYM0 + channel;

    adc_set_bit(ADSTATE, bit, 1);
}

inline unsigned int adc_hal_get_interrupt_flag(unsigned int channel)
{
    return adc_get_bit(ADSTATE, ARDYM0 + channel);
}

inline unsigned int adc_hal_read_channel_data(unsigned int channel)
{
    if (channel == 0)
        return adc_get_bits(ADADAT0, ADATAL);
    else if (channel == 1)
        return adc_get_bits(ADADAT0, ADATAH);
    else if (channel == 2)
        return adc_get_bits(ADADAT1, ADATAL);
    else
        return 0;
}

inline void adc_hal_set_clkdiv(unsigned int clkidiv, unsigned int clkdiv_us, unsigned int clkdiv_ms)
{
    adc_set_bits(ADCLK, CLKDIV, clkidiv);
    adc_set_bits(ADCLK, CLKDIV_US, clkdiv_us);
    adc_set_bits(ADCLK, CLKDIV_MS, clkdiv_ms);
}

inline void adc_hal_set_wait_sampling_stable_time(unsigned int ADSTB_time)
{
    adc_set_bits(ADSTB, STABLE, ADSTB_time);
}

inline void adc_hal_set_repeat_sampling_time(unsigned int retime)
{
    adc_set_bits(ADRETM, RETIME, retime);
}

