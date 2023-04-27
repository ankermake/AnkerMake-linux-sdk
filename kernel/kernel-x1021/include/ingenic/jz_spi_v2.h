#ifndef _JZ_SPI_V2_H_
#define _JZ_SPI_V2_H_

#include <hal_x1021/jz_spi_hal.h>

#define TEST_JZ_SPI_INTERFACE

#ifdef TEST_JZ_SPI_INTERFACE

enum jzspi_cs_valid_level {
    Valid_low,
    Valid_high,
};

enum jzspi_transfer_mode {
    Transfer_irq_callback_mode = 1,
    Transfer_poll_mode = 2,
 };

struct jzspi_config_data {
    unsigned int id;
    unsigned int cs;
    unsigned int clk_rate;
    unsigned int src_clk_rate;

    enum jzspi_cs_valid_level cs_valid_level;
    enum jzspi_transfer_mode transfer_mode;
    enum jzspi_data_endian tx_endian;
    enum jzspi_data_endian rx_endian;
    unsigned int bits_per_word;
    unsigned int auto_cs;

    unsigned int spi_pha;
    unsigned int spi_pol;
    unsigned int loop_mode;

    void (*finish_irq_callback)(struct jzspi_config_data *config);
};

void spi_start_config(struct jzspi_config_data *config);

void spi_stop_config(struct jzspi_config_data *config);

void spi_write_read(struct jzspi_config_data *config,
    void *tx_buf, int tx_len, void *rx_buf, int rx_len);

#endif

#endif /* _JZ_SPI_V2_H_ */