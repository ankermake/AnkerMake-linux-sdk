/*
 *  Copyright (C) 2010 Ingenic Semiconductor Inc.
 *
 *   In this file, here are some macro/device/function to
 * to help the board special file to organize resources
 * on the chip.
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

/* devio define list */

/*******************************************************************************************************************/
#define UART0_PORTB							\
	{ .name = "uart0", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0xF << 19, }
#define UART1_PORTB							\
	{ .name = "uart1", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x3 << 23, }
/*******************************************************************************************************************/

#define MSC0_PORTB_4BIT							\
	{ .name = "msc0-pb-4bit",	.port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = (0x3f<<0), }
#define MSC1_PORTC							\
	{ .name = "msc1-pC",		.port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x3f<<2), }
#define I2S_PORTC                           \
	{.name = "i2s",              .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x7f << 18),}
#define I2S_DAC_PORTC                           \
	{.name = "i2s-dac",     .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x3 << 18) | (0x1 << 24),}
#define I2S_ADC_PORTC                           \
	{.name = "i2s-adc",     .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x7 << 20),}
#define I2S_MCLK_PORTC                           \
	{.name = "i2s-mclk",    .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x1 << 22),}

/*******************************************************************************************************************/
/*****************************************************************************************************************/

#define SSI0_PORTC							\
	{ .name = "ssi0-pa",	       .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 0x1F800, }
#define SFC_PORTA							\
	{ .name = "sfc",	.port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = (0x3f << 23), }

/*****************************************************************************************************************/

#define I2C0_PORTA							\
	{ .name = "i2c0-pa", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x3 << 12, }
#define I2C1_PORTB							\
	{ .name = "i2c1-pb", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x3 << 25, }
#define I2C2_PORTC							\
	{ .name = "i2c2-pc", .port = GPIO_PORT_C, .func = GPIO_FUNC_1, .pins = 0x3 << 27, }

/*******************************************************************************************************************/

/*******************************************************************************************************************/
#define PWM_PORTB_BIT17					\
	{ .name = "pwm0", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 1 << 17, }
#define PWM_PORTB_BIT18							\
	{ .name = "pwm1", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 1 << 18, }
#define PWM_PORTC_BIT8							\
	{ .name = "pwm2", .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 1 << 8, }
#define PWM_PORTC_BIT9							\
	{ .name = "pwm3", .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 1 << 9, }

#define MCLK_PORTA                 \
{ .name = "mclk", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 1 << 15, }

/*******************************************************************************************************************/

#define MII_PORTBDF							\
	{ .name = "mii-0", .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = 0x00000010, }, \
	{ .name = "mii-1", .port = GPIO_PORT_D, .func = GPIO_FUNC_1, .pins = 0x3c000000, }, \
	{ .name = "mii-2", .port = GPIO_PORT_F, .func = GPIO_FUNC_0, .pins = 0x0000fff0, }

/*******************************************************************************************************************/

#define OTG_DRVVUS							\
	{ .name = "otg-drvvbus", .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 1 << 17, }

/*******************************************************************************************************************/

#define ISP_PORTD							\
	{ .name = "isp", .port = GPIO_PORT_D, .func = GPIO_FUNC_2, .pins = 0x000003ff, }

#define ISP_I2C							\
	{ .name = "isp-i2c",    .port = GPIO_PORT_B,  .func = GPIO_FUNC_2, .pins = 3 << 7, }

/*******************************************************************************************************************/

#define DVP_PORTA_LOW_10BIT							\
	{ .name = "dvp-pa-10bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x000343ff, }

#define DVP_PORTA_HIGH_10BIT							\
	{ .name = "dvp-pa-10bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x00034ffc, }

#define DVP_PORTA_12BIT							\
	{ .name = "dvp-pa-12bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x00034fff, }

/*******************************************************************************************************************/

#define DMIC_PORTC							\
	{ .name = "dmic_pc",	.port = GPIO_PORT_C, .func = GPIO_FUNC_2, .pins = 0x1<<8 | 0x3 << 25}

/*******************************************************************************************************************/
#define GMAC_PORTB							\
	{ .name = "gmac_pb",	.port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x0001efc0}

/*******************************************************************************************************************/

/*******************************************************************************************************************/
#define DPU_PORTD_TFT_18BIT                                                     \
	{ .name = "dpu_tft_18bit",  .port = GPIO_PORT_D, .func = GPIO_FUNC_0, .pins = (0xff << 2 | 0xff << 12 | 0x3f << 22)}
/*******************************************************************************************************************/



/* JZ SoC on Chip devices list */
extern struct platform_device jz_adc_device;

#ifdef CONFIG_JZ_VPU_IRQ_TEST
extern struct platform_device jz_vpu_irq_device;
#elif defined(CONFIG_SOC_VPU)
#ifdef CONFIG_VPU_HELIX
#if (CONFIG_VPU_HELIX_NUM >= 1)
extern struct platform_device jz_vpu_helix0_device;
#endif
#endif
#ifdef CONFIG_VPU_RADIX
#if (CONFIG_VPU_RADIX_NUM >= 1)
extern struct platform_device jz_vpu_radix0_device;
#endif
#endif
#endif

extern struct platform_device jz_uart0_device;
extern struct platform_device jz_uart1_device;

extern struct platform_device jz_ssi0_device;

extern struct platform_device jz_i2c0_device;
extern struct platform_device jz_i2c1_device;
extern struct platform_device jz_i2c2_device;

extern struct platform_device jz_sfc_device;
extern struct platform_device jz_msc0_device;
extern struct platform_device jz_msc1_device;
extern struct platform_device jz_ipu_device;
extern struct platform_device jz_pdma_device;

extern struct platform_device jz_dwc_otg_device;
#endif
#if defined(CONFIG_SND_ASOC_INGENIC)
extern struct platform_device jz_aic_device;
extern struct platform_device jz_aic_dma_device;
#ifdef CONFIG_SND_ASOC_JZ_DMIC_V13
extern struct platform_device jz_dmic_device;
extern struct platform_device jz_dmic_dma_device;
extern struct platform_device jz_dmic_dump_cdc_device;
#endif
#ifdef CONFIG_SND_ASOC_JZ_ICDC_D4
extern struct platform_device jz_icdc_device;
#endif
extern struct platform_device snd_alsa_device;
#endif
extern struct platform_device jz_rtc_device;
extern struct platform_device jz_efuse_device;
extern struct platform_device jz_tcu_device;

int jz_device_register(struct platform_device *pdev, void *pdata);
void *get_driver_common_interfaces(void);

/* __PLATFORM_H__ */
