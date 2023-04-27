#ifndef __INGENIC_SPI_H__
#define __INGENIC_SPI_H__

enum jzspi_transfer_mode {
    Transfer_int_mode,
    Transfer_poll_mode,
};

enum jzspi_cs_valid_level {
    Valid_low,
    Valid_high,
};

enum jzspi_data_endian {
    Endian_msb_first = 0,
    Endian_lsb_first = 2,
};

struct jzspi_config_data {
    int id;
    int cs;
    int clk_rate;
    int src_clk_rate;

    enum jzspi_transfer_mode transfer_mode; // do not care now
    enum jzspi_cs_valid_level cs_valid_level;
    enum jzspi_data_endian tx_endian;
    enum jzspi_data_endian rx_endian;
    unsigned int bits_per_word;

    unsigned int spi_pha:1;
    unsigned int spi_pol:1;
    unsigned int loop_mode:1;
};
void spi_start_config(struct jzspi_config_data *config);
int spi_write_read(struct jzspi_config_data *config, void *tx_buf, int tx_len, void *rx_buf, int rx_len);
void spi_stop_config(struct jzspi_config_data *config);

#endif