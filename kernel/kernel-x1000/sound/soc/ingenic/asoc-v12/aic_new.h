#ifndef _AIC_NEW_H_
#define _AIC_NEW_H_

enum aic_iss {
    aic_iss_8bit,
    aic_iss_16bit,
    aic_iss_18bit,
    aic_iss_20bit,
    aic_iss_24bit,
};

enum aic_oss {
    aic_oss_8bit,
    aic_oss_16bit,
    aic_oss_18bit,
    aic_oss_20bit,
    aic_oss_24bit,
};

void aic_set_reg_base(void *base);

void aic_stop_bit_clk(void);

void aic_start_bit_clk(void);

void aic_enable_sysclk_output(void);

void aic_disable_sysclk_output(void);

void aic_set_div(unsigned int div);

void aic_select_i2s_fmt(void);

void aic_select_i2s_msb_fmt(void);

void aic_bclk_input(void);

void aic_bclk_output(void);

void aic_frame_sync_input(void);

void aic_frame_sync_output(void);

void aic_set_tx_trigger_level(unsigned int level);

void aic_set_rx_trigger_level(unsigned int level);

void aic_enable(void);

void aic_disable(void);

void aic_enable_pack16(void);

void aic_disable_pack16(void);

void aic_enable_m2s(void);

void aic_disable_m2s(void);

void aic_set_tx_channel(unsigned int channel);

void aic_set_tx_sample_bit(enum aic_oss oss);

void aic_set_rx_sample_bit(enum aic_iss iss);

void aic_set_i2s_frame_LR_mode(void);

void aic_set_i2s_frame_RL_mode(void);

void aic_clear_tx_underrun(void);

void aic_clear_rx_overrun(void);

unsigned int aic_get_tx_fifo_nums(void);

unsigned int aic_get_rx_fifo_nums(void);

void aic_write_fifo(unsigned int value);

unsigned int aic_read_fifo(void);

void aic_enable_replay(void);

void aic_disable_replay(void);

void aic_enable_record(void);

void aic_disable_record(void);

void aic_enable_tx_dma(void);

void aic_disable_tx_dma(void);

void aic_enable_rx_dma(void);

void aic_disable_rx_dma(void);

void aic_select_internal_codec(void);

void aic_select_external_codec(void);

void aic_internal_codec_master_mode(void);

void aic_internal_codec_slave_mode(void);

void aic_flush_rx_fifo(void);

void aic_flush_tx_fifo(void);

void aic_init_registers(void);

int aic_rx_dma_is_enabled(void);

int aic_tx_dma_is_enabled(void);

int aic_get_tx_underrun(void);

int aic_get_rx_overrun(void);

void aic_print_state(void);

#endif /* _AIC_NEW_H_ */