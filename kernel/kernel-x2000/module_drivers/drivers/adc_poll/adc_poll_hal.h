#ifndef _ADC_POLL_HAL_H_
#define _ADC_POLL_HAL_H_

/*关闭控制器*/
void adc_hal_disable_controller(void);
/*打开控制器*/
void adc_hal_enable_controller(void);
/*打开重复采样*/
void adc_hal_enable_repeat_sampling(void);
/*关闭重复采样*/
void adc_hal_disable_repeat_sampling(void);
/*打开功耗优化*/
void adc_hal_enable_power_optimize(void);
/*关闭功耗优化*/
void adc_hal_disable_power_optimize(void);
/*打开指定通道*/
int adc_hal_enable_channel(unsigned int channel, int try_count);
/*关闭指定通道*/
void adc_hal_disable_channel(unsigned int channel);

unsigned int adc_hal_get_channel_status(unsigned int channel);
unsigned int adc_hal_get_all_channel_status(void);
unsigned int adc_hal_enable_bits_channels(unsigned int bits_channels, int try_count);

/*关闭软件复位*/
void adc_hal_disable_soft_reset(void);
/*打开软件复位*/
void adc_hal_enable_soft_reset(void);

/*关闭中断*/
void adc_hal_mask_all_interrupt(void);
/*打开中断*/
void adc_hal_enable_all_interrupt(void);
/*打开指定通道中断*/
void adc_hal_enable_interrupt(unsigned int channel);
/*关闭指定通道中断*/
void adc_hal_disable_interrupt(unsigned int channel);
/*判断是否设置中断*/
unsigned int adc_hal_get_interrupt_value(int channel);

/*清除中断标志位*/
void adc_hal_clean_all_interrupt_flag(void);
/*清除指定通道标志位*/
void adc_hal_clean_interrupt_flag(unsigned int channel);
/*获取中断标志位*/
unsigned int adc_hal_get_all_interrupt_flag(void);
/*获取指定通道中断标志位*/
unsigned int adc_hal_get_interrupt_flag(unsigned int channel);

/*读取通道数据*/
int adc_hal_read_channel_data(unsigned int channel);

/*设置时钟分频*/
void adc_hal_set_clkdiv(unsigned int clkidiv, unsigned int clkdiv_us, unsigned int clkdiv_ms);

/*设置等待采样稳定时间*/
void adc_hal_set_wait_sampling_stable_time(unsigned int stable_time);

/*设置重复采样时间间隔*/
void adc_hal_set_repeat_sampling_time(unsigned int repeat_time);

#endif