#ifndef _SPI_HAL_H_
#define _SPI_HAL_H_

/*获取DMA传输的目标地址*/
unsigned long spi_hal_get_dma_ssidr_addr(int id);

/*从fifo读数据*/
unsigned int spi_hal_read_fifo(int id);
/*向fifo写数据*/
void spi_hal_write_fifo(int id, unsigned int data);

/*使能接收*/
void spi_hal_enable_receive(int id);
/*失能接收*/
void spi_hal_disable_receive(int id);
/*清除接收fifo*/
void spi_hal_clear_receive_fifo(int id);
/*清除发送fifo*/
void spi_hal_clear_transmit_fifo(int id);
/*设置发送fifo为空模式模式（新模式mode = 0 旧模式mode = 1，推荐使用新模式）*/
void spi_hal_set_transmit_fifo_empty_mode(int id, int mode);
/*接收数据数量计数器无效*/
void spi_hal_receive_counter_unvalid(int id);
/*接收数据数量计数器有效*/
void spi_hal_receive_counter_valid(int id);
/*选择总线上的从机*/
void spi_hal_select_slave(int id, int number);
/*设置不自动清除发送fifo空标志位*/
void spi_hal_set_unauto_clear_transmit_fifo_empty_flag(int id);
/*设置自动清除发送fifo空标志位*/
void spi_hal_set_auto_clear_transmit_fifo_empty_flag(int id);
/*设置接收完成(在间隔模式下)*/
void spi_hal_set_receive_finish(int id);
/*设置接收继续（在间隔模式下）*/
void spi_hal_set_receive_continue(int id);
/*失能接收完成控制*/
void spi_hal_disable_receive_finish_control(int id);
/*使能接收完成控制*/
void spi_hal_enable_receive_finish_control(int id);
/*设置模式(0 正常   1 自旋)*/
void spi_hal_set_loop_mode(int id, unsigned int mode);
/*使能接收错误中断*/
void spi_hal_enable_receive_error_interrupt(int id);
/*失能接收错误中断*/
void spi_hal_disable_receive_error_interrupt(int id);
/*获取接收错误中断控制值*/
unsigned int spi_hal_get_receive_error_interrupt_control_value(int id);
/*使能发送错误中断*/
void spi_hal_enable_transmit_error_interrupt(int id);
/*失能发送错误中断*/
void spi_hal_disable_transmit_error_interrupt(int id);
/*获取发送错误中断控制值*/
unsigned int spi_hal_get_transmit_error_interrupt_control_value(int id);
/*使能接收中断*/
void spi_hal_enable_receive_interrupt(int id);
/*失能接收中断*/
void spi_hal_disable_receive_interrupt(int id);
/*获取接收中断控制值*/
unsigned int spi_hal_get_receive_interrupt_control_value(int id);
/*使能发送中断*/
void spi_hal_enable_transmit_interrupt(int id);
/*失能发送中断*/
void spi_hal_disable_transmit_interrupt(int id);
/*获取接收中断控制值*/
unsigned int spi_hal_get_transmit_interrupt_control_value(int id);
/*使能spi*/
void spi_hal_enable(int id);
/*失能spi*/
void spi_hal_disable(int id);
/*
设置数据大小端传输格式
0：字高位在前，字节高位在前
1：字高位在前，字节低位在前
2：字低位在前，字节高位在前
3：字低位在前，字节低位在前
*/
void spi_hal_set_transfer_endian(int id, unsigned int tx_endian, unsigned int rx_endian);

/*设置时钟极性*/
void spi_hal_set_clk_pol(int id, unsigned int pol);
/*设置时钟相位*/
void spi_hal_set_clk_pha(int id, unsigned int pha);
/*设置dpc为高电平(在gpc在特殊模式有效)*/
void spi_hal_set_gpc_high(int id);
/*设置dpc为低电平(在gpc在特殊模式有效)*/
void spi_hal_set_gpc_low(int id);
/*设置传输过程中最小数据单位*/
void spi_hal_set_data_unit_bits_len(int id, unsigned int bits_per_word);
/*设置接收的阀值*/
void spi_hal_set_receive_threshold(int id, int len);
/*设置命令长度(仅仅使用在National Microwire format 1 or 2)*/
void spi_hal_set_command_len(int id, int len);
/*设置发送的阀值*/
void spi_hal_set_transmit_threshold(int id, int len);
/*设置标准传输格式*/
void spi_hal_set_standard_transfer_format(int id);
/*设置ssp传输格式*/
void spi_hal_set_ssp_transfer_format(int id);
/*设置National Microwire 1传输格式*/
void spi_hal_set_national1_transfer_format(int id);
/*设置National Microwire 2传输格式*/
void spi_hal_set_national2_transfer_format(int id);
/*设置发送fifo空时传输完成*/
void spi_hal_set_transmit_fifo_empty_transfer_finish(int id);
/*设置发送fifo空时传输未完成*/
void spi_hal_set_transmit_fifo_empty_transfer_unfinish(int id);
/*设置gpc正常模式*/
void spi_hal_set_gpc_normal_mode(int id);
/*设置gpc特殊模式*/
void spi_hal_set_gpc_special_mode(int id);
/*设置停止时延时时钟周期*/
void spi_hal_set_stop_delay_clk(int id, unsigned int cycle);
/*设置开始时延时时钟周期*/
void spi_hal_set_start_delay_clk(int id, unsigned int cycle);
/*设置帧的有效电平*/
void spi_hal_select_frame_valid_level(int id, unsigned int cs, unsigned int cs_valid_level);


/*清除接收fifo溢出标志*/
void spi_hal_clear_receive_fifo_overrun_flag(int id);
/*获取接收fifo溢出标志*/
unsigned int spi_hal_get_receive_fifo_overrun_flag(int id);
/*清除发送fifo空标志*/
void spi_hal_clear_transmit_fifo_underrrun_flag(int id);
/*获取发送fifo空标志*/
unsigned int spi_hal_get_transmit_fifo_underrun_flag(int id);
/*获取接收fifo数量超过阀值标志*/
unsigned int spi_hal_get_receive_fifo_more_threshold_flag(int id);
/*获取发送fifo数量低于阀值标志*/
unsigned int spi_hal_get_transmit_fifo_less_threshold_flag(int id);
/*获取接收fifo为空标志*/
unsigned int spi_hal_get_receive_fifo_empty_flag(int id);
/*获取发送fifo满标志*/
unsigned int spi_hal_get_transmit_fifo_full_flag(int id);
/*获取spi工作标志*/
unsigned int spi_hal_get_working_flag(int id);
/*获取传输结束标志*/
unsigned int spi_hal_get_transfer_end_flag(int id);
/*获取接收fifo的数量*/
unsigned int spi_hal_get_receive_fifo_number(int id);
/*获取发送fifo的数量*/
unsigned int spi_hal_get_transmit_fifo_number(int id);

/*设置间隔时间(间隔模式下)*/
void spi_hal_set_interval_time(int id, unsigned int time);
/*选择spi时钟作为间隔计数器时钟*/
void spi_hal_interval_counter_select_spi_clk_source(int id);
/*选择32k作为间隔计数器时钟*/
void spi_hal_interval_counter_select_32k_clk_source(int id);

/*设置时钟分频系数*/
void spi_hal_set_clock_dividing_number(int id, int cgv);
/*获取时钟分频系数*/
unsigned int  spi_hal_get_clock_dividing_number(int id);

/*获取接收数据的数量*/
unsigned int spi_hal_get_receive_count(int id);
/*设置接收数据的数量*/
void spi_hal_set_receive_count(int id, unsigned int count);
#endif