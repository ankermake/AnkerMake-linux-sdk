

#include "jz_spi_regs.h"

#include <hal_x1021/jz_spi_hal.h>

#define SPI_ADDR(id, reg) (jz_get_reg_base(id, reg))

void jzspi_hal_init_setting(void *base_addr)
{
    unsigned int ssicr0 = 0;
    unsigned int ssicr1 = 0;

    set_bit_field(&ssicr0, SSICR0_SSIE, 0);
    set_bit_field(&ssicr0, SSICR0_EACLRUN, 0);
    set_bit_field(&ssicr0, SSICR0_TFLUSH, 1);
    set_bit_field(&ssicr0, SSICR0_RFLUSH, 1);

    jzspi_write_reg(base_addr, SSICR0, ssicr0);
    jzspi_write_reg(base_addr, SSISR, 0);

    ssicr0 = 0;
    set_bit_field(&ssicr0, SSICR0_TIE, 0);

    set_bit_field(&ssicr0, SSICR0_RFINE, 0);
    set_bit_field(&ssicr0, SSICR0_RFINC, 0);
    set_bit_field(&ssicr0, SSICR0_VRCNT, 0);
    set_bit_field(&ssicr0, SSICR0_TFMODE, 0);

    set_bit_field(&ssicr0, SSICR0_DISREV, 0);

    //set TTRG RTRG
    set_bit_field(&ssicr1, SSICR1_TTRG, 0);
    set_bit_field(&ssicr1, SSICR1_RTRG, 0);

    set_bit_field(&ssicr1, SSICR1_TFVCK, 0);
    set_bit_field(&ssicr1, SSICR1_TCKFI, 0);

    // standard spi format
    set_bit_field(&ssicr1, SSICR1_FMAT, 0);

    // disable interval mode
    jzspi_set_bit(base_addr, SSIITR, SSIITR_IVLTM, 0);

    jzspi_write_reg(base_addr, SSICR0, ssicr0);
    jzspi_write_reg(base_addr, SSICR1, ssicr1);

}

void jzspi_hal_enable_transfer(void *base_addr, struct jzspi_hal_config *config)
{
    int cgv;
    unsigned int ssicr0 = jzspi_read_reg(base_addr, SSICR0);
    unsigned int ssicr1 = jzspi_read_reg(base_addr, SSICR1);

    assert_range(config->bits_per_word, 2, 32);
    set_bit_field(&ssicr0, SSICR0_SSIE, 0);
    set_bit_field(&ssicr0, SSICR0_TIE, 0);
    set_bit_field(&ssicr0, SSICR0_TEIE, 0);
    set_bit_field(&ssicr0, SSICR0_RIE, 0);
    set_bit_field(&ssicr0, SSICR0_REIE, 0);

    set_bit_field(&ssicr0, SSICR0_TENDIAN, config->tx_endian);
    set_bit_field(&ssicr0, SSICR0_RENDIAN, config->rx_endian);
    set_bit_field(&ssicr0, SSICR0_FSEL, config->cs);

    set_bit_field(&ssicr1, SSICR1_PHA, config->spi_pha);
    set_bit_field(&ssicr1, SSICR1_POL, config->spi_pol);
    set_bit_field(&ssicr0, SSICR0_LOOP, config->loop_mode);

    set_bit_field(&ssicr0, SSICR0_TFLUSH, 1);
    set_bit_field(&ssicr0, SSICR0_RFLUSH, 1);

    if (config->cs_mode == AUTO_PULL_CS)
        set_bit_field(&ssicr1, SSICR1_UNFIN, 0);
    else
        set_bit_field(&ssicr1, SSICR1_UNFIN, 1);

    set_bit_field(&ssicr1, SSICR1_FLEN, (config->bits_per_word - 2));

    jzspi_write_reg(base_addr, SSICR0, ssicr0);
    jzspi_write_reg(base_addr, SSICR1, ssicr1);

    cgv = config->src_clk_rate / 2 / config->clk_rate / 2 - 1;

    if (cgv < 0)
        cgv = 0;
    if (cgv > 255)
        cgv = 255;

    jzspi_set_bit(base_addr, SSIGR, SSIGR_CGV, cgv);

    jzspi_set_bit(base_addr, SSICR0, SSICR0_SSIE, 1);

}

void jzspi_hal_disable_transfer(void *base_addr)
{
    jzspi_set_bit(base_addr, SSICR0, SSICR0_SSIE, 0);
}

void jzspi_hal_enable_fifo_empty_finish_transfer(void *base_addr)
{
    jzspi_set_bit(base_addr, SSICR1, SSICR1_UNFIN, 0);
    jzspi_set_bit(base_addr, SSISR, SSISR_TUNDR, 0);
}

void jzspi_hal_clear_underrun(void *base_addr)
{
    jzspi_set_bit(base_addr, SSISR, SSISR_TUNDR, 0);
}

void jzspi_hal_disable_fifo_empty_finish_transfer(void *base_addr)
{
    jzspi_set_bit(base_addr, SSICR1, SSICR1_UNFIN, 1);
}

void jzspi_hal_set_tx_fifo_threshold(void *base_addr, int len)
{
    // assert_range(len, 0, 128);
    len = len / 8;
    jzspi_set_bit(base_addr, SSICR1, SSICR1_TTRG, len);
}

void jzspi_hal_set_rx_fifo_threshold(void *base_addr, int len)
{
    //assert_range(len, 0, 128);
    len = len / 8;
    jzspi_set_bit(base_addr, SSICR1, SSICR1_RTRG, len);
}

unsigned int jzspi_hal_get_interrupts(void *base_addr)
{
    return jzspi_read_reg(base_addr, SSISR);
}

void jzspi_hal_clear_interrupt(void *base_addr, enum jzspi_interrupt_type irq_type)
{
    jzspi_set_bit(base_addr, SSISR, irq_type, irq_type, 0);
}

int jzspi_hal_check_interrupt(unsigned int flags, enum jzspi_interrupt_type irq_type)
{
    return get_bit_field(&flags, irq_type, irq_type);
}

unsigned int jzspi_hal_get_tx_fifo_num(void *base_addr)
{
    return jzspi_get_bit(base_addr, SSISR, SSISR_TFIFO_NUM);
}

unsigned int jzspi_hal_get_rx_fifo_num(void *base_addr)
{
    return jzspi_get_bit(base_addr, SSISR, SSISR_RFIFO_NUM);
}

int jzspi_hal_get_busy(void *base_addr)
{
    return jzspi_get_bit(base_addr, SSISR, SSISR_BUSY) != 0;
}

int jzspi_hal_get_end(void *base_addr)
{
    return jzspi_get_bit(base_addr, SSISR, SSISR_END) != 0;
}

int to_bytes(int bits_per_word)
{
    if (bits_per_word <= 8)
        return 1;
    else if (bits_per_word <= 16)
        return 2;
    else
        return 4;
}

void jzspi_hal_write_tx_fifo(void *base_addr, int bits_per_word, const void *buf, int len)
{
    int i = 0;
    unsigned char *p8 = (unsigned char *)buf;
    unsigned short *p16 = (unsigned short *)buf;
    unsigned int *p32 = (unsigned int *)buf;
    unsigned int bytes_per_word = to_bytes(bits_per_word);

    switch (bytes_per_word)
    {
        case 1:
            for (i = 0; i < len; i++)
                jzspi_write_reg(base_addr, SSIDR, p8[i]);
            break;

        case 2:
            for (i = 0; i < len; i++)
                jzspi_write_reg(base_addr, SSIDR, p16[i]);
            break;

        case 4:
            for (i = 0; i < len; i++)
                jzspi_write_reg(base_addr, SSIDR, p32[i]);
            break;

        default:
            assert(0);
            break;
    }
}

void jzspi_hal_read_rx_fifo(void *base_addr, int bits_per_word, void *buf, int len)
{
    int i = 0;
    unsigned char *p8 = buf;
    unsigned short *p16 = buf;
    unsigned int *p32 = buf;
    unsigned int bytes_per_word = to_bytes(bits_per_word);

    switch (bytes_per_word)
    {
        case 1:
            for (i = 0; i < len; i++)
                p8[i] = jzspi_read_reg(base_addr, SSIDR) & 0xff;
            break;

        case 2:
            for (i = 0; i < len; i++)
                p16[i] = jzspi_read_reg(base_addr, SSIDR) & 0xffff;
            break;

        case 4:
            for (i = 0; i < len; i++)
                p32[i] = jzspi_read_reg(base_addr, SSIDR);
            break;

        default:
            assert(0);
            break;
    }
}

void jzspi_hal_write_tx_fifo_only(void *base_addr, unsigned int len, unsigned int val)
{
    int i = 0;

    for (i = 0; i < len; i++)
        jzspi_write_reg(base_addr, SSIDR, val);
}

volatile unsigned int val_xxxx;

void jzspi_hal_read_rx_fifo_only(void *base_addr, int len)
{
    int i = 0;

    for (i = 0; i < len; i++)
        val_xxxx = jzspi_read_reg(base_addr, SSIDR);
}
