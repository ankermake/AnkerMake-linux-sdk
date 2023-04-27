#ifndef __MD_SPI_H__
#define __MD_SPI_H__

enum spi_data_endian {
    /* 传输数据时，最高有效位先传输 */
    Spi_endian_msb_first = 0,
    /* 传输数据时，最低有效位先传输 */
    Spi_endian_lsb_first = 2,
};

enum spi_cs_valid_level {
    Spi_valid_low,  /* 表示传输过程的CS有效电平为低，即当CS被拉低时启动传输 */
    Spi_valid_high, /* 表示传输过程的CS有效电平为高，即当CS被拉高时启动传输 */
};

struct spi_config_data {
    unsigned int clk_rate;  /* 时钟频率(默认为 1*1000*1000) */

    /* 数据传输的大小端模式选择,具体可选类型为 spi_data_endian 所定义的类型 */
    enum spi_data_endian tx_endian;
    enum spi_data_endian rx_endian;

    /* bits_per_word 代表数据的位宽.
        例如:bits_per_word = 32 时,SPI传输过程中的最小数据单位为32bit. */
    unsigned int bits_per_word;

    /**
     * 极性 spi_pol :
     * 当spi_pol=0，在时钟空闲即无数据传输时,clk电平为低电平
     * 当spi_pol=1，在时钟空闲即无数据传输时,clk电平为高电平
     * 相位 spi_pha :
     * 当spi_pha=0，表示在第一个跳变沿开始传输数据，下一个跳变沿完成传输
     * 当spi_pha=1，表示在第二个跳变沿开始传输数据，下一个跳变沿完成传输
     */
    unsigned int spi_pol;
    unsigned int spi_pha;

    unsigned int loop_mode; /* 循环模式，可用于测试 */
};

#endif