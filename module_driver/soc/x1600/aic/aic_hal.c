#include <bit_field.h>
#include "aic_regs.h"
#include <utils/clock.h>

#define AIC_IOBASE 0x10079000

#define AIC_REG_BASE  KSEG1ADDR(AIC_IOBASE)

#define AIC_ADDR(reg) ((volatile unsigned long *)(AIC_REG_BASE + (reg)))

void aic_write_reg(unsigned int reg, int val)
{
    *AIC_ADDR(reg) = val;
}

unsigned int aic_read_reg(unsigned int reg)
{
    return *AIC_ADDR(reg);
}

void aic_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(AIC_ADDR(reg), start, end, val);
}

unsigned int aic_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(AIC_ADDR(reg), start, end);
}

static unsigned int sub_pos(unsigned int size, unsigned int pos, unsigned int delta)
{
    return (pos + size - delta) % size;
}

static unsigned int add_pos(unsigned int size, unsigned int pos, unsigned int delta)
{
    return (pos + delta) % size;
}

static int to_sample_bits(unsigned int fmt)
{
    if (fmt == SNDRV_PCM_FORMAT_S8) return 0;
    if (fmt == SNDRV_PCM_FORMAT_S16_LE) return 1;
    if (fmt == SNDRV_PCM_FORMAT_S24_LE) return 4;
    return 0;
}

static void aic_reset_div(struct aic_data *data, int is_playback)
{
    unsigned long i2sdiv = 0;
    if (is_playback)
        set_bit_field(&i2sdiv, TDIV, data->clk_div);
    else
        set_bit_field(&i2sdiv, RDIV, data->clk_div);

    aic_write_reg(I2SDIV, i2sdiv);
}

void aic_hal_init_common_setting(struct aic_data *data)
{
    unsigned long aicfr = aic_read_reg(AICFR);

    set_bit_field(&aicfr, TFTH, TX_FIFO_TRIGGER / 2); // tfth * 2 触发发送中断
    set_bit_field(&aicfr, RFTH, RX_FIFO_TRIGGER / 2 - 1); // (rfth + 1) * 2 触发接收中断
    set_bit_field(&aicfr, MSB, 0);
    set_bit_field(&aicfr, LSMP, 0);
    set_bit_field(&aicfr, AUSEL, 1);
    set_bit_field(&aicfr, DMODE, 1);     // 时钟主从模式设置：0共享, 1独立
    set_bit_field(&aicfr, TMASTER, aic_dev.as_master);
    set_bit_field(&aicfr, RMASTER, aic_dev.as_master);
    set_bit_field(&aicfr, ENB, 0);
    aic_write_reg(AICFR, aicfr);

    /* reset aic
     */
    aic_set_bit(AICFR, RST, 1);

    /* 等待aic reset 完成
     */
    uint64_t time = local_clock_us();
    while (aic_get_bit(AICFR, RST)) {
        if (local_clock_us() - time > 100*1000) {
            if (!aic_dev.clk_id)
                panic("AIC:Please enable external codec before enable aic\n");
            else
                panic("AIC:reset aic failure\n");
        }
    }

    unsigned long aiccr = aic_read_reg(AICCR);
    set_bit_field(&aiccr, ENDSW, 0);
    set_bit_field(&aiccr, EROR, 0);
    set_bit_field(&aiccr, ETUR, 0);
    set_bit_field(&aiccr, ERFS, 0);
    set_bit_field(&aiccr, ETFS, 0);
    set_bit_field(&aiccr, ENLBF, 0);
    set_bit_field(&aiccr, ERPL, 0);
    set_bit_field(&aiccr, EREC, 0);
    aic_write_reg(AICCR, aiccr);

    unsigned long i2scr = aic_read_reg(I2SCR);
    set_bit_field(&i2scr, RFIRST, 1);
    set_bit_field(&i2scr, SWLH, 0);
    set_bit_field(&i2scr, AMSL, aic_dev.interface);
    aic_write_reg(I2SCR, i2scr);
}

void aic_init_capture_setting(struct aic_data *data)
{
    if (aic_dev.as_master)
        aic_reset_div(data, 0);
    int channels = data->channels;
    unsigned long aiccr = aic_read_reg(AICCR);
    set_bit_field(&aiccr, ISS, to_sample_bits(data->format));
    set_bit_field(&aiccr, MONOCTR, channels == 2 ? 0 : 1);
    aic_write_reg(AICCR, aiccr);
}

void aic_init_playback_setting(struct aic_data *data)
{

    if (aic_dev.as_master)
        aic_reset_div(data, 1);

    int is_16bit = (data->format == SNDRV_PCM_FORMAT_S16_LE);
    int channels = data->channels;

    unsigned long aiccr = aic_read_reg(AICCR);
    set_bit_field(&aiccr, PACK16, is_16bit && channels == 2);
    set_bit_field(&aiccr, CHANNEL, channels == 2);
    set_bit_field(&aiccr, OSS, to_sample_bits(data->format));
    aic_write_reg(AICCR, aiccr);
}

static void aic_enable(void)
{
    if (aic_dev.is_enabled++ == 0)
        aic_set_bit(AICFR, ENB, 1);
}

static void aic_disable(void)
{
    if (--aic_dev.is_enabled == 0)
        aic_set_bit(AICFR, ENB, 0);
}

static void aic_hal_start_capture(void)
{
    aic_enable();

    /*
     * 清空 rx fifo 数据
     * 开启接收功能,接收dma
     */
    aic_set_bit(AICCR, RFLUSH, 1);

    aic_set_bit(AICCR, EREC, 1);

    uint64_t old = local_clock_us();
    while (!aic_get_bit(AICSR, RFL)) {
        if (local_clock_us() - old > 10000) {
            if (!aic_get_bit(AICSR, RFL)) {
                printk("AIC: failed to wait rx fifo empty: %x\n", aic_read_reg(AICSR));
            }
        }
    }

    aic_set_bit(AICCR, RDMS, 1);
}

static void aic_hal_stop_capture(void)
{
    aic_set_bit(AICCR, RDMS, 0);
    aic_set_bit(AICCR, EREC, 0);

    aic_set_bit(AICCR, RFLUSH, 1);

    aic_disable();
}

static void aic_hal_start_playback(void)
{
    aic_enable();

    /* 开启播放功能
     * 验证播放功能ok
     * 使能播放dma功能
     */
    aic_set_bit(AICCR, ERPL, 1);

    int i;
    for (i = 0; i < 32; i++) {
        int n = aic_get_bit(AICSR, TFL);
        if (n > 32)
            break;
        aic_write_reg(AICDR, 0);
    }

    int timeout = 15;
    while (aic_get_bit(AICSR, TFL)) {
        mdelay(10);
        if (timeout-- == 0)
            panic("AIC: failed to wait tx fifo empty: %x\n", aic_read_reg(AICSR));
    }

    aic_set_bit(AICCR, TDMS, 1);
}

static void aic_hal_stop_playback(void)
{
    aic_set_bit(AICCR, TDMS, 0);
    aic_set_bit(AICCR, ERPL, 0);

    aic_set_bit(AICCR, TFLUSH, 1);

    int timeout = 15;
    while (aic_get_bit(AICSR, TFL)) {
        mdelay(1);
        if (timeout-- == 0)
            panic("AIC: failed to wait tx fifo empty: %x\n", aic_read_reg(AICSR));
    }

    aic_disable();
}