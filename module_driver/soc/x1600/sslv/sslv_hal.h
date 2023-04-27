#ifndef _SSLV_HAL_H_
#define _SSLV_HAL_H_

/*获取DMA传输的目标地址*/
unsigned long sslv_hal_get_dma_slvdr_addr(int id);

/* 设置帧格式- Motorola SPI */
void sslv_hal_set_standard_transfer_format(int id);
/* 设置帧格式- Texas Instruments SSP */
void sslv_hal_set_ssp_transfer_format(int id);
/* 设置帧格式- National Semiconductors Microwire */
void sslv_hal_set_national_transfer_format(int id);
/* 设置时钟相位 */
void sslv_hal_set_clk_scph(int id, unsigned int scph);
/* 设置时钟极性 */
void sslv_hal_set_clk_scpol(int id, unsigned int scpol);
/* 设置传输模式 transfer_mode */
void sslv_hal_set_transfer_mode(int id, unsigned int mode);
/* 设置从模式输出使能 */
void sslv_hal_set_slv_transmit_enable(int id, unsigned int mode);
/* 设置循环测试模式 */
void sslv_hal_set_loop_mode(int id, unsigned int mode);
/* microwire frame format设置控制字长度
 * size: 1 - 16 */
void sslv_hal_set_control_frame_size(int id, unsigned int size);
/* 32-bit mode设置数据长度
 * size: 4 - 32 */
void sslv_hal_set_data_frame_size(int id, unsigned int size);
/* 设置Motorola SPI Frame Format */
void sslv_hal_set_standard_format(int id);

/* 使能sslv */
void sslv_hal_enable(int id);
/* 失能sslv */
void sslv_hal_disable(int id);
unsigned int sslv_hal_get_isenable(int id);

/* 设置sequential_transfer */
void sslv_hal_enable_microwire_sequential_transfer(int id);
/* 设置non_sequential_transfer */
void sslv_hal_disable_microwire_sequential_transfer(int id);
/* 设置串行receive */
void sslv_hal_set_microwire_receive(int id);
/* 设置串行transfer */
void sslv_hal_set_microwire_transfer(int id);

/* 设置发送fifo阈值
 * len: 0 - 63 */
void sslv_hal_set_transmit_threshold(int id, int len);
unsigned char sslv_hal_get_transmit_threshold(int id);
/* 设置接收fifo阈值
 * len: 1 - 64 */
void sslv_hal_set_receive_threshold(int id, int len);
unsigned char sslv_hal_get_receive_threshold(int id);

/* TXFLR 只读 */
/* 获取发送fifo有效数据数量 */
unsigned int sslv_hal_get_transmit_fifo_number(int id);
/* RXFLR 只读 */
/* 获取接收fifo有效数据数量 */
unsigned int sslv_hal_get_receive_fifo_number(int id);

/* 获取sslv工作(busy)标志
 * return: 0 idle  1 busy */
unsigned int sslv_hal_get_busy_flag(int id);
/* 获取发送fifo未满标志
 * return: 0 full  1 not */
unsigned int sslv_hal_get_transmit_fifo_full_flag(int id);
/* 获取发送fifo空标志
 * return: 0 not  1 empty */
unsigned int sslv_hal_get_transmit_fifo_empty_flag(int id);
/* 获取接收fifo非空标志
 * return: 0 empty  1 not */
unsigned int sslv_hal_get_receive_fifo_empty_flag(int id);
/* 获取接收fifo满标志
 * return: 0 not  1 full */
unsigned int sslv_hal_get_receive_fifo_full_flag(int id);
/* 获取发送错误标志
 * return: 0 no error  1 transmit error */
unsigned int sslv_hal_get_transmit_error_flag(int id);

/* IMR */
/* 使能发送fifo阈值中断 */
void sslv_hal_enable_transmit_threshold_interrupt(int id);
/* 失能发送fifo阈值中断 */
void sslv_hal_disable_transmit_threshold_interrupt(int id);
/* 是否使能发送fifo阈值中断 */
unsigned int sslv_hal_transmit_threshold_interrupt_is_enabled(int id);
/* 使能发送overflow溢出中断 */
void sslv_hal_enable_transmit_overflow_interrupt(int id);
/* 失能发送overflow溢出中断 */
void sslv_hal_disable_transmit_overflow_interrupt(int id);
/* 是否使能发送overflow溢出中断 */
unsigned int sslv_hal_transmit_overflow_interrupt_is_enabled(int id);
/* 使能接收underflow溢出中断 */
void sslv_hal_enable_receive_underflow_interrupt(int id);
/* 失能接收underflow溢出中断 */
void sslv_hal_disable_receive_underflow_interrupt(int id);
/* 是否使能接收underflow溢出中断 */
unsigned int sslv_hal_receive_underflow_interrupt_is_enabled(int id);
/* 使能接收overflow溢出中断 */
void sslv_hal_enable_receive_overflow_interrupt(int id);
/* 失能接收overflow溢出中断 */
void sslv_hal_disable_receive_overflow_interrupt(int id);
/* 是否使能接收overflow溢出中断 */
unsigned int sslv_hal_receive_overflow_interrupt_is_enabled(int id);
/* 使能接收fifo阈值中断 */
void sslv_hal_enable_receive_threshold_interrupt(int id);
/* 失能接收fifo阈值中断 */
void sslv_hal_disable_receive_threshold_interrupt(int id);
/* 是否使能接收fifo阈值中断 */
unsigned int sslv_hal_receive_threshold_interrupt_is_enabled(int id);
/* 使能全部中断 */
void sslv_hal_enable_all_interrupt(int id);
/* 失能全部中断 */
void sslv_hal_disable_all_interrupt(int id);

/* 获取发送fifo阈值中断状态 */
unsigned int sslv_hal_get_transmit_threshold_interrupt_status(int id);
/* 获取发送overflow溢出中断状态 */
unsigned int sslv_hal_get_transmit_overflow_interrupt_status(int id);
/* 获取接收underflow溢出中断状态 */
unsigned int sslv_hal_get_receive_underflow_interrupt_status(int id);
/* 获取接收overflow溢出中断状态 */
unsigned int sslv_hal_get_receive_overflow_interrupt_status(int id);
/* 获取接收fifo阈值中断状态 */
unsigned int sslv_hal_get_receive_threshold_interrupt_status(int id);

/* 获取发送fifo阈值raw中断状态 */
unsigned int sslv_hal_get_transmit_threshold_raw_interrupt_statues(int id);
/* 获取发送overflow溢出raw中断状态 */
unsigned int sslv_hal_get_transmit_overflow_raw_interrupt_statues(int id);
/* 获取接收underflow溢出raw中断状态 */
unsigned int sslv_hal_get_receive_underflow_raw_interrupt_statues(int id);
/* 获取接收overflow溢出raw中断状态 */
unsigned int sslv_hal_get_receive_overflow_raw_interrupt_statues(int id);
/* 获取接收fifo阈值raw中断状态 */
unsigned int sslv_hal_get_receive_threshold_raw_interrupt_statues(int id);

/* 读取发送fifo阈值中断状态(中断使能且中断置位return1) */
unsigned int sslv_hal_get_transmit_threshold_intr_status(int id);
/* 读取发送overflow溢出中断状态(中断使能且中断置位return1) */
unsigned int sslv_hal_get_transmit_overflow_intr_status(int id);
/* 读取接收underflow溢出中断状态(中断使能且中断置位return1) */
unsigned int sslv_hal_get_receive_underflow_intr_status(int id);
/* 读取接收overflow溢出中断状态(中断使能且中断置位return1) */
unsigned int sslv_hal_get_receive_overflow_intr_status(int id);
/* 读取接收fifo阈值中断状态(中断使能且中断置位return1) */
unsigned int sslv_hal_get_receive_threshold_intr_status(int id);

/* 清除发送overflow溢出中断 */
void sslv_hal_clear_transmit_overflow_interrupt(int id);
/* 清除接收overflow溢出中断 */
void sslv_hal_clear_receive_overflow_interrupt(int id);
/* 清除接收underflow溢出中断 */
void sslv_hal_clear_receive_underflow_interrupt(int id);
/* 清除txo,rxu,rxo中断 */
unsigned int sslv_hal_clear_all_interrupt(int id);

/* 使能接收DMA */
void sslv_hal_enable_receive_dma(int id);
/* 失能接收DMA */
void sslv_hal_disable_receive_dma(int id);
/* 使能发送DMA */
void sslv_hal_enable_transmit_dma(int id);
/* 失能发送DMA */
void sslv_hal_disable_transmit_dma(int id);

/* 设置dma发送阈值
 * len: 0 - 63 */
void sslv_hal_set_transmit_dma_threshold(int id, int len);
unsigned char sslv_hal_get_transmit_dma_threshold(int id);
/* 设置dma接收阈值
 * len: 1 - 64 */
void sslv_hal_set_receive_dma_threshold(int id, int len);
unsigned char sslv_hal_get_receive_dma_threshold(int id);

/* 读取ID */
unsigned int sslv_hal_get_id_code(int id);

/* 读寄存器 */
unsigned int sslv_hal_read_fifo(int id);
/* 写寄存器 */
void sslv_hal_write_fifo(int id, unsigned int data);
#endif
