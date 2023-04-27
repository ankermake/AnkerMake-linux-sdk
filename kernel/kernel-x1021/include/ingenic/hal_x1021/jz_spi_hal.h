#ifndef _JZ_SPI_HAL_H_
#define _JZ_SPI_HAL_H_

#include "ingenic_bit_field.h"
#include "ingenic_common.h"

enum jzspi_data_endian {
    Endian_mode0 = 0,   //Most significant 「byte」 in a word first, most significant 「bit」 in a byte first.
    Endian_mode1 = 1,   //Most significant 「byte」 in a word first, least significant 「bit」 in a byte first.
    Endian_mode2 = 2,   //Least significant 「byte」 in a word first, least significant 「bit」 in a byte first.
    Endian_mode3 = 3,   //Least significant 「byte」 in a word first, most significant 「bit」 in a byte first.
};

enum jzspi_interrupt_type {
    JZSPI_irq_rx_fifo_overrun = 0,
    JZSPI_irq_tx_fifo_under_run = 1,
    JZSPI_irq_rx_fifo_half = 2,
    JZSPI_irq_tx_fifo_half = 3,
};

enum jzspi_pull_cs {
    HAND_PULL_CS = 0,   //当FIFO空时，需要手动控制CS引脚
    AUTO_PULL_CS = 1,   //当FIFO空时，CS引脚由控制器自动拉高
 };

struct jzspi_hal_config {
    int cs;
    int clk_rate;
    int src_clk_rate;
    struct clk * clk;

    enum jzspi_pull_cs cs_mode;   //当FIFO空时，CS引脚工作模式
    enum jzspi_data_endian tx_endian;   //发送数据大小端格式
    enum jzspi_data_endian rx_endian;   //接收数据大小端格式
    unsigned int bits_per_word;

    unsigned int spi_pha;
    unsigned int spi_pol;
    unsigned int loop_mode;
};

void jzspi_hal_init_setting(void *base_addr);

void jzspi_hal_enable_transfer(void *base_addr, struct jzspi_hal_config *config);

void jzspi_hal_disable_transfer(void *base_addr);

void jzspi_hal_enable_fifo_empty_finish_transfer(void *base_addr);

void jzspi_hal_disable_fifo_empty_finish_transfer(void *base_addr);

void jzspi_hal_set_tx_fifo_threshold(void *base_addr, int len);

void jzspi_hal_set_rx_fifo_threshold(void *base_addr, int len);

unsigned int jzspi_hal_get_interrupts(void *base_addr);

unsigned int jzspi_hal_get_tx_fifo_num(void *base_addr);

unsigned int jzspi_hal_get_rx_fifo_num(void *base_addr);

int jzspi_hal_get_busy(void *base_addr);

int jzspi_hal_get_end(void *base_addr);

void jzspi_hal_clear_underrun(void *base_addr);

void jzspi_hal_clear_interrupt(void *base_addr, enum jzspi_interrupt_type irq_type);

int jzspi_hal_check_interrupt(unsigned int flags, enum jzspi_interrupt_type irq_type);

void jzspi_hal_write_tx_fifo(void *base_addr, int bits_per_word, const void *buf, int len);

void jzspi_hal_read_rx_fifo(void *base_addr, int bytes_per_word, void *buf, int len);

void jzspi_hal_write_tx_fifo_only(void *base_addr, unsigned int len, unsigned int val);

void jzspi_hal_read_rx_fifo_only(void *base_addr, int len);

int to_bytes(int bits_per_word);

static inline void jzspi_write_reg(void *base_addr, unsigned int reg, unsigned int value)
{
    writel(value, base_addr + reg);
}

static inline unsigned int jzspi_read_reg(void *base_addr, unsigned int reg)
{
    return readl(base_addr + reg);
}

static inline void jzspi_set_bit(void *base_addr, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(base_addr + reg, start, end, val);
}

static inline unsigned int jzspi_get_bit(void *base_addr, unsigned int reg, int start, int end)
{
    return get_bit_field(base_addr + reg, start, end);
}

#endif /* _JZ_SPI_HAL_H_ */