
#include "aic_regs.h"
#include "bit_field.h"
#include <linux/kernel.h>
#include <linux/delay.h>

#include "aic_new.h"

static void *reg_base = (void *)0xB0020000;

#define AIC_ADDR(reg)    ((volatile unsigned long *)(reg_base + reg))

static inline void aic_write_reg(unsigned int reg, int val)
{
    *AIC_ADDR(reg) = val;
}

static inline unsigned int aic_read_reg(unsigned int reg)
{
    return *AIC_ADDR(reg);
}

static inline void aic_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(AIC_ADDR(reg), start, end, val);
}

static inline unsigned int aic_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(AIC_ADDR(reg), start, end);
}

void aic_set_reg_base(void *base)
{
    reg_base = base;
}

void aic_print_state(void)
{
    printk("aic state: %x %x\n", aic_read_reg(AICSR), aic_read_reg(I2SSR));
}

void aic_stop_bit_clk(void)
{
    /* 先关 BIT_CLK,方便设置BIT_CLK相关的位 */
    unsigned long i2scr = aic_read_reg(I2SCR);
    set_bit_field(&i2scr, STPBK, 1);
    set_bit_field(&i2scr, ISTPBK, 1);
    aic_write_reg(I2SCR, i2scr);
}

void aic_start_bit_clk(void)
{
    unsigned long i2scr = aic_read_reg(I2SCR);
    set_bit_field(&i2scr, STPBK, 0);
    set_bit_field(&i2scr, ISTPBK, 0);
    aic_write_reg(I2SCR, i2scr);
}

void aic_enable_sysclk_output(void)
{
    aic_set_bit(AICFR, SYSCKD, 0);
    aic_set_bit(I2SCR, ESCLK, 1);
}

void aic_disable_sysclk_output(void)
{
    aic_set_bit(I2SCR, ESCLK, 0);
    aic_set_bit(AICFR, SYSCKD, 1);
}

void aic_set_div(unsigned int div)
{
    unsigned long i2sdiv = 0;
    set_bit_field(&i2sdiv, IDV, div - 1);
    set_bit_field(&i2sdiv, DV, div - 1);
    aic_write_reg(I2SDIV, i2sdiv);
}

void aic_select_i2s_fmt(void)
{
    aic_set_bit(I2SCR, AMSL, 0);
}

void aic_select_i2s_msb_fmt(void)
{
    aic_set_bit(I2SCR, AMSL, 1);
}

void aic_bclk_input(void)
{
    unsigned long aicfr = aic_read_reg(AICFR);
    set_bit_field(&aicfr, IBCKD, 0);
    set_bit_field(&aicfr, BCKD, 0);
    aic_write_reg(AICFR, aicfr);
}

void aic_bclk_output(void)
{
    unsigned long aicfr = aic_read_reg(AICFR);
    set_bit_field(&aicfr, IBCKD, 1);
    set_bit_field(&aicfr, BCKD, 1);
    aic_write_reg(AICFR, aicfr);
}

void aic_frame_sync_input(void)
{
    unsigned long aicfr = aic_read_reg(AICFR);
    set_bit_field(&aicfr, ISYNCD, 0);
    set_bit_field(&aicfr, SYNCD, 0);
    aic_write_reg(AICFR, aicfr);
}

void aic_frame_sync_output(void)
{
    unsigned long aicfr = aic_read_reg(AICFR);
    set_bit_field(&aicfr, ISYNCD, 1);
    set_bit_field(&aicfr, SYNCD, 1);
    aic_write_reg(AICFR, aicfr);
}

void aic_set_tx_trigger_level(unsigned int level)
{
    aic_set_bit(AICFR, TFTH, level / 2);
}

void aic_set_rx_trigger_level(unsigned int level)
{
    aic_set_bit(AICFR, RFTH, level / 2 - 1);
}

void aic_enable(void)
{
    aic_set_bit(AICFR, ENB, 1);
}

void aic_disable(void)
{
    aic_set_bit(AICFR, ENB, 0);
}

void aic_enable_pack16(void)
{
    aic_set_bit(AICCR, PACK16, 1);
}

void aic_disable_pack16(void)
{
    aic_set_bit(AICCR, PACK16, 0);
}

void aic_enable_m2s(void)
{
    aic_set_bit(AICCR, M2S, 1);
}

void aic_disable_m2s(void)
{
    aic_set_bit(AICCR, M2S, 0);
}

void aic_set_tx_channel(unsigned int channel)
{
    aic_set_bit(AICCR, CHANNEL, channel - 1);
}

void aic_set_tx_sample_bit(enum aic_oss oss)
{
    aic_set_bit(AICCR, OSS, oss);
}

void aic_set_rx_sample_bit(enum aic_iss iss)
{
    aic_set_bit(AICCR, ISS, iss);
}

void aic_set_i2s_frame_LR_mode(void)
{
    aic_set_bit(I2SCR, RFIRST, 0);
}

void aic_set_i2s_frame_RL_mode(void)
{
    aic_set_bit(I2SCR, RFIRST, 1);
}

void aic_clear_tx_underrun(void)
{
    aic_set_bit(AICSR, TUR, 0);
}

int aic_get_tx_underrun(void)
{
    return aic_get_bit(AICSR, TUR);
}

void aic_clear_rx_overrun(void)
{
    aic_set_bit(AICSR, ROR, 0);
}

int aic_get_rx_overrun(void)
{
    return aic_get_bit(AICSR, ROR);
}

unsigned int aic_get_tx_fifo_nums(void)
{
    return aic_get_bit(AICSR, TFL);
}

unsigned int aic_get_rx_fifo_nums(void)
{
    return aic_get_bit(AICSR, RFL);
}

void aic_write_fifo(unsigned int value)
{
    aic_write_reg(AICDR, value);
}

unsigned int aic_read_fifo(void)
{
    return aic_read_reg(AICDR);
}

void aic_enable_replay(void)
{
    aic_set_bit(AICCR, ERPL, 1);
}

void aic_disable_replay(void)
{
    aic_set_bit(AICCR, ERPL, 1);
}

void aic_enable_record(void)
{
    aic_set_bit(AICCR, EREC, 1);
}

void aic_disable_record(void)
{
    aic_set_bit(AICCR, EREC, 1);
}

void aic_enable_tx_dma(void)
{
    aic_set_bit(AICCR, TDMS, 1);
}

void aic_disable_tx_dma(void)
{
    aic_set_bit(AICCR, TDMS, 0);
}

void aic_enable_rx_dma(void)
{
    aic_set_bit(AICCR, RDMS, 1);
}

void aic_disable_rx_dma(void)
{
    aic_set_bit(AICCR, RDMS, 0);
}

void aic_select_internal_codec(void)
{
    aic_set_bit(AICFR, ICDC, 1);
}

void aic_select_external_codec(void)
{
    aic_set_bit(AICFR, ICDC, 0);
}

void aic_internal_codec_master_mode(void)
{
    aic_set_bit(AICFR, CDC_MASTER, 1);
}

void aic_internal_codec_slave_mode(void)
{
    aic_set_bit(AICFR, CDC_MASTER, 0);
}

void aic_flush_rx_fifo(void)
{
    aic_set_bit(AICCR, RFLUSH, 1);
}

void aic_flush_tx_fifo(void)
{
    aic_set_bit(AICCR, TFLUSH, 1);
}

int aic_rx_dma_is_enabled(void)
{
    return aic_get_bit(AICCR, RDMS);
}

int aic_tx_dma_is_enabled(void)
{
    return aic_get_bit(AICCR, TDMS);
}

void aic_init_registers(void)
{
    aic_set_bit(AICCR, TFLUSH, 1);
    aic_set_bit(AICCR, RFLUSH, 1);

    aic_set_bit(AICFR, ENB, 0);

    aic_start_bit_clk();

    unsigned long aicfr = aic_read_reg(AICFR);
    set_bit_field(&aicfr, TFTH, 16); // tfth * 2 触发发送中断
    set_bit_field(&aicfr, RFTH, 7); // (rfth + 1) * 2 触发接收中断
    set_bit_field(&aicfr, MSB, 0);
    set_bit_field(&aicfr, LSMP, 0);
    set_bit_field(&aicfr, AUSEL, 1); // i2s,i2s-MSB
    set_bit_field(&aicfr, IBCKD, 1); // 信号输出
    set_bit_field(&aicfr, ISYNCD, 1); // 信号输出
    set_bit_field(&aicfr, BCKD, 1); // 信号输出
    set_bit_field(&aicfr, SYNCD, 1); // 信号输出
    set_bit_field(&aicfr, ENB, 0);
    aic_write_reg(AICFR, aicfr);

    aic_set_bit(AICFR, RST, 1);

    while(aic_get_bit(AICFR, RST));

    unsigned long aiccr = aic_read_reg(AICCR);
    set_bit_field(&aiccr, PACK16, 0);
    set_bit_field(&aiccr, ENDSW, 0);
    set_bit_field(&aiccr, ASVTSU, 0);
    set_bit_field(&aiccr, EROR, 0);
    set_bit_field(&aiccr, ETUR, 0);
    set_bit_field(&aiccr, ERFS, 0);
    set_bit_field(&aiccr, ETFS, 0);
    set_bit_field(&aiccr, ENLBF, 0);
    set_bit_field(&aiccr, TDMS, 0);
    set_bit_field(&aiccr, RDMS, 0);
    set_bit_field(&aiccr, ERPL, 0);
    set_bit_field(&aiccr, EREC, 0);
    set_bit_field(&aicfr, CHANNEL, 2 - 1); // 输出 2 通道
    set_bit_field(&aiccr, OSS, aic_oss_16bit);
    set_bit_field(&aiccr, ISS, aic_iss_16bit);
    set_bit_field(&aicfr, M2S, 1); // 输出 M2S, 保证单通道的时候LR都有数据
    aic_write_reg(AICCR, aiccr);

    unsigned long i2scr;
    set_bit_field(&i2scr, RFIRST, 0); // LR 模式
    set_bit_field(&i2scr, SWLH, 0);
    set_bit_field(&i2scr, AMSL, 0); // i2s 模式
    aic_write_reg(I2SCR, i2scr);

    aic_write_reg(AICSR, 0);
}
