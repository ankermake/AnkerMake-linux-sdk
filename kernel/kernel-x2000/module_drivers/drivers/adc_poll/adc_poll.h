#ifndef __ADC_POLL_H__
#define __ADC_POLL_H__

void test_jz_adc_enable(void);
void test_jz_adc_disable(void);
unsigned int test_jz_adc_read_value(unsigned int channel, unsigned int block);
unsigned int test_jz_adc_channel_sample(unsigned int channel, unsigned int block);
unsigned int test_jz_adc_bit_channels_sample(unsigned int bit_channels, unsigned int block);

#endif