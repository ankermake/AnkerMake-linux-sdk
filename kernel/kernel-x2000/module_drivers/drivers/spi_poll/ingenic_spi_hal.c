#include "ingenic_spi_hal.h"
#include "bit_field.h"
#include <linux/slab.h>

#define SSIDR   0x0000
#define SSICR0  0x0004
#define SSICR1  0x0008
#define SSISR   0x000C
#define SSIITR  0x0010
#define SSIICR  0x0014
#define SSIGR   0x0018
#define SSIRCNT 0x001C

#define SSIDR_D31_17 17, 31
#define SSIDR_GPC    16, 16
#define SSIDR_D15_0  0, 15

#define SSICR0_Reserved  20, 31
#define SSICR0_TENDIAN   18, 19
#define SSICR0_RENDIAN   16, 17
#define SSICR0_SSIE      15, 15
#define SSICR0_TIE       14, 14
#define SSICR0_RIE       13, 13
#define SSICR0_TEIE      12, 12
#define SSICR0_REIE      11, 11
#define SSICR0_LOOP      10, 10
#define SSICR0_RFINE     9, 9
#define SSICR0_RFINC     8, 8
#define SSICR0_EACLRUN   7, 7
#define SSICR0_FSEL      6, 6
#define SSICR0_Reserved1 5, 5
#define SSICR0_VRCNT     4, 4
#define SSICR0_TFMODE    3, 3
#define SSICR0_TFLUSH    2, 2
#define SSICR0_RFLUSH    1, 1
#define SSICR0_DISREV    0, 0

#define SSICR1_FRMHL1   31, 31
#define SSICR1_FRMHL0   30, 30
#define SSICR1_TFVCK    28, 29
#define SSICR1_TCKFI    26, 27
#define SSICR1_GPCMD    25, 25
#define SSICR1_ITFRM    24, 24
#define SSICR1_UNFIN    23, 23
#define SSICR1_FMAT     20, 21
#define SSICR1_TTRG     16, 19
#define SSICR1_MCOM     12, 15
#define SSICR1_RTRG     8, 11
#define SSICR1_FLEN     3, 7
#define SSICR1_GPCHL    2, 2
#define SSICR1_PHA      1, 1
#define SSICR1_POL      0, 0

#define SSISR_Reserved  24, 31
#define SSISR_TFIFO_NUM 16, 23
#define SSISR_RFIFO_NUM 8, 15
#define SSISR_END       7, 7
#define SSISR_BUSY      6, 6
#define SSISR_TFF       5, 5
#define SSISR_RFE       4, 4
#define SSISR_TFHE      3, 3
#define SSISR_RFHF      2, 2
#define SSISR_TUNDR     1, 1
#define SSISR_ROVER     0, 0

#define SSIITR_Reserved 16, 31
#define SSIITR_CNTCLK   15, 15
#define SSIITR_IVLTM    0, 14

#define SSIICR_Reserved 3, 31
#define SSIICR_ICC      0, 2

#define SSIGR_Reserved 8, 31
#define SSIGR_CGV      0, 7

#define SSIRCNT_Reserved 16, 31
#define SSIRCNT_RCNT     0, 15

#define SSI0_IOBASE     0x10043000

static const unsigned long iobase[] = {
    SSI0_IOBASE,
};

#define SPI_ADDR(id, reg) ((volatile unsigned long *)(KSEG1ADDR(iobase[id]) + reg))

static inline void spi_write_reg(int id, unsigned int reg, unsigned int value)
{
    *SPI_ADDR(id, reg) = value;
}

static inline unsigned int spi_read_reg(int id, unsigned int reg)
{
    return *SPI_ADDR(id, reg);
}

static inline void spi_set_bits(int id, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(SPI_ADDR(id, reg), start, end, val);
}

static inline unsigned int spi_get_bits(int id, unsigned int reg, int start, int end)
{
    return get_bit_field(SPI_ADDR(id, reg), start, end);
}

/////////////////////////////////////////////////////////////////////////////////////

unsigned long spi_hal_get_dma_ssidr_addr(int id)
{
    return iobase[id] + SSIDR;
}

/*SSIDR*/
unsigned int spi_hal_read_fifo(int id)
{
    spi_hal_clear_transmit_fifo_underrrun_flag(id);

    return spi_read_reg(id, SSIDR);
}

void spi_hal_write_fifo(int id, unsigned int data)
{
    spi_hal_clear_transmit_fifo_underrrun_flag(id);

    spi_write_reg(id, SSIDR, data);
}


/*SSICR0*/
inline void spi_hal_enable_receive(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_DISREV, 0);
}

inline void spi_hal_disable_receive(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_DISREV, 1);
}

inline void spi_hal_clear_receive_fifo(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_RFLUSH, 1);
}

inline void spi_hal_clear_transmit_fifo(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_TFLUSH, 1);
}

inline void spi_hal_set_transmit_fifo_empty_mode(int id, int mode)
{
    spi_set_bits(id, SSICR0, SSICR0_TFMODE, mode);
}

inline void spi_hal_receive_counter_unvalid(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_VRCNT, 0);
}

inline void spi_hal_receive_counter_valid(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_VRCNT, 1);
}

inline void spi_hal_select_slave(int id, int num)
{
    spi_set_bits(id, SSICR0, SSICR0_FSEL, num);
}

inline void spi_hal_set_unauto_clear_transmit_fifo_empty_flag(int id)
{
    // underrun auto 1 , unauto 0
    spi_set_bits(id, SSICR0, SSICR0_EACLRUN, 0);
}

inline void spi_hal_set_auto_clear_transmit_fifo_empty_flag(int id)
{
    // underrun auto 1 , unauto 0
    spi_set_bits(id, SSICR0, SSICR0_EACLRUN, 1);
}

inline void spi_hal_set_receive_finish(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_RFINC, 1);
}

inline void spi_hal_set_receive_continue(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_RFINC, 0);
}

inline void spi_hal_enable_receive_finish_control(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_RFINE, 1);
}

inline void spi_hal_disable_receive_finish_control(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_RFINE, 0);
}

inline void spi_hal_set_loop_mode(int id, unsigned int mode)
{
    spi_set_bits(id, SSICR0, SSICR0_LOOP, mode);
}

inline void spi_hal_enable_receive_error_interrupt(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_REIE, 1);
}

inline void spi_hal_disable_receive_error_interrupt(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_REIE, 0);
}

inline unsigned int spi_hal_get_receive_error_interrupt_control_value(int id)
{
    return spi_get_bits(id, SSICR0, SSICR0_REIE);
}

inline void spi_hal_enable_transmit_error_interrupt(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_TEIE, 1);
}

inline void spi_hal_disable_transmit_error_interrupt(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_TEIE, 0);
}

inline unsigned int spi_hal_get_transmit_error_interrupt_control_value(int id)
{
    return spi_get_bits(id, SSICR0, SSICR0_TEIE);
}

inline void spi_hal_enable_receive_interrupt(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_RIE, 1);
}

inline void spi_hal_disable_receive_interrupt(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_RIE, 0);
}

inline unsigned int spi_hal_get_receive_interrupt_control_value(int id)
{
    return spi_get_bits(id, SSICR0, SSICR0_RIE);
}

inline void spi_hal_enable_transmit_interrupt(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_TIE, 1);
}

inline void spi_hal_disable_transmit_interrupt(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_TIE, 0);
}

inline unsigned int spi_hal_get_transmit_interrupt_control_value(int id)
{
    return spi_get_bits(id, SSICR0, SSICR0_TIE);
}

inline void spi_hal_enable(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_SSIE, 1);
}

inline void spi_hal_disable(int id)
{
    spi_set_bits(id, SSICR0, SSICR0_SSIE, 0);
}

inline void spi_hal_set_transfer_endian(int id, unsigned int tx_endian, unsigned int rx_endian)
{
    spi_set_bits(id, SSICR0, SSICR0_TENDIAN, tx_endian);
    spi_set_bits(id, SSICR0, SSICR0_RENDIAN, rx_endian);
}


/*SSICR1*/
inline void spi_hal_set_clk_pol(int id, unsigned int pol)
{
    spi_set_bits(id, SSICR1, SSICR1_POL, pol);
}

inline void spi_hal_set_clk_pha(int id, unsigned int pha)
{
    spi_set_bits(id, SSICR1, SSICR1_PHA, pha);
}

inline void spi_hal_set_gpc_high(int id)
{
    spi_set_bits(id, SSICR1, SSICR1_GPCHL, 1);
}

inline void spi_hal_set_gpc_low(int id)
{
    spi_set_bits(id, SSICR1, SSICR1_GPCHL, 0);
}

inline void spi_hal_set_data_unit_bits_len(int id, unsigned int bits_per_word)
{
    spi_set_bits(id, SSICR1, SSICR1_FLEN, bits_per_word - 2);
}

inline void spi_hal_set_receive_threshold(int id, int len)
{
    if (len < 0)
        len = 0;
    spi_set_bits(id, SSICR1, SSICR1_RTRG, len / 8);
}

inline void spi_hal_set_command_len(int id, int len)
{
    spi_set_bits(id, SSICR1, SSICR1_MCOM, len - 1);
}

inline void spi_hal_set_transmit_threshold(int id, int len)
{
    if (len < 0)
        len = 0;
    spi_set_bits(id, SSICR1, SSICR1_TTRG, len / 8);
}

inline void spi_hal_set_standard_transfer_format(int id)
{
    spi_set_bits(id, SSICR1, SSICR1_FMAT, 0);
}

inline void spi_hal_set_ssp_transfer_format(int id)
{
    spi_set_bits(id, SSICR1, SSICR1_FMAT, 1);
}

inline void spi_hal_set_national1_transfer_format(int id)
{
    spi_set_bits(id, SSICR1, SSICR1_FMAT, 2);
}

inline void spi_hal_set_national2_transfer_format(int id)
{
    spi_set_bits(id, SSICR1, SSICR1_FMAT, 3);
}

inline void spi_hal_set_transmit_fifo_empty_transfer_finish(int id)
{
    spi_set_bits(id, SSICR1, SSICR1_UNFIN, 0);
}

inline void spi_hal_set_transmit_fifo_empty_transfer_unfinish(int id)
{
    spi_set_bits(id, SSICR1, SSICR1_UNFIN, 1);
}

inline void spi_hal_set_gpc_normal_mode(int id)
{
    spi_set_bits(id, SSICR1, SSICR1_GPCMD, 0);
}

inline void spi_hal_set_gpc_special_mode(int id)
{
    spi_set_bits(id, SSICR1, SSICR1_GPCMD, 1);
}

inline void spi_hal_set_stop_delay_clk(int id, unsigned int cycle)
{
    spi_set_bits(id, SSICR1, SSICR1_TCKFI, cycle);
}

inline void spi_hal_set_start_delay_clk(int id, unsigned int cycle)
{
    spi_set_bits(id, SSICR1, SSICR1_TFVCK, cycle);
}

inline void spi_hal_select_frame_valid_level(int id, unsigned int cs, unsigned int cs_valid_level)
{
    spi_set_bits(id, SSICR0, SSICR0_FSEL, cs);
    if (cs == 0)
        spi_set_bits(id, SSICR1, SSICR1_FRMHL0, cs_valid_level);
    if (cs == 1)
        spi_set_bits(id, SSICR1, SSICR1_FRMHL1, cs_valid_level);
}

/*SSISR*/
inline unsigned int spi_hal_get_receive_fifo_overrun_flag(int id)
{
    return spi_get_bits(id, SSISR, SSISR_ROVER);
}

inline void spi_hal_clear_receive_fifo_overrun_flag(int id)
{
    if (spi_hal_get_receive_fifo_overrun_flag(0))
        spi_set_bits(id, SSISR, SSISR_ROVER, 0);
}

inline unsigned int spi_hal_get_transmit_fifo_underrun_flag(int id)
{
    return spi_get_bits(id, SSISR, SSISR_TUNDR);
}

inline void spi_hal_clear_transmit_fifo_underrrun_flag(int id)
{
    if (spi_hal_get_transmit_fifo_underrun_flag(id))
        spi_set_bits(id, SSISR, SSISR_TUNDR, 0);
}

inline unsigned int spi_hal_get_receive_fifo_more_threshold_flag(int id)
{
    return spi_get_bits(id, SSISR, SSISR_RFHF);
}

inline unsigned int spi_hal_get_transmit_fifo_less_threshold_flag(int id)
{
    return spi_get_bits(id, SSISR, SSISR_TFHE);
}

inline unsigned int spi_hal_get_receive_fifo_empty_flag(int id)
{
    return spi_get_bits(id, SSISR, SSISR_RFE);
}

inline unsigned int spi_hal_get_transmit_fifo_full_flag(int id)
{
    return spi_get_bits(id, SSISR, SSISR_TFF);
}

inline unsigned int spi_hal_get_working_flag(int id)
{
    return spi_get_bits(id, SSISR, SSISR_BUSY);
}

inline unsigned int spi_hal_get_transfer_end_flag(int id)
{
    return spi_get_bits(id, SSISR, SSISR_END);
}

inline unsigned int spi_hal_get_receive_fifo_number(int id)
{
    return spi_get_bits(id, SSISR, SSISR_RFIFO_NUM);
}

inline unsigned int spi_hal_get_transmit_fifo_number(int id)
{
    return spi_get_bits(id, SSISR, SSISR_TFIFO_NUM);
}

/*SSIITR*/
inline void spi_hal_set_interval_time(int id, unsigned int time)
{
    spi_set_bits(id, SSIITR, SSIITR_IVLTM, time);
}

inline void spi_hal_interval_counter_select_spi_clk_source(int id)
{
    spi_set_bits(id, SSIITR, SSIITR_CNTCLK, 0);
}

inline void spi_hal_interval_counter_select_32k_clk_source(int id)
{
    spi_set_bits(id, SSIITR, SSIITR_CNTCLK, 1);
}

/*SSiGR*/
inline void spi_hal_set_clock_dividing_number(int id, int cgv)
{
    spi_set_bits(id, SSIGR, SSIGR_CGV, cgv);
}

inline unsigned int spi_hal_get_clock_dividing_number(int id)
{
    return spi_get_bits(id, SSIGR, SSIGR_CGV);
}

/*SSIRCNT*/
inline unsigned int spi_hal_get_receive_count(int id)
{
    return spi_get_bits(id, SSIRCNT, SSIRCNT_RCNT);
}

inline void spi_hal_set_receive_count(int id, unsigned int count)
{
    spi_set_bits(id, SSIRCNT, SSIRCNT_RCNT, count);
}
