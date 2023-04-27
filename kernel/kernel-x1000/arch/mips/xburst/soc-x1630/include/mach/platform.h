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
#define MSC0_PORTB_1BIT							\
	{ .name = "msc0-pb-1bit",	.port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = (0x31<<0), }

#define MSC1_PORTC_4BIT							\
	{ .name = "msc1-pc-4bit",	.port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x3f<<2), }

#define I2S_PORTC                           \
	{.name = "i2s",              .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x7f << 18),}

/*******************************************************************************************************************/
/*****************************************************************************************************************/

#define SSI0_PORTC							\
	{ .name = "ssi0-pa",	       .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 0x1F800, }
#define SFC_PORTA							\
	{ .name = "sfc",	.port = GPIO_PORT_A, .func = GPIO_FUNC_1 | GPIO_PULL_UP, .pins = (0x3f << 23), }

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

#define DPU_PORTD_TFT_18BIT                                                     \
	{ .name = "dpu_tft_18bit",  .port = GPIO_PORT_D, .func = GPIO_FUNC_0, .pins = (0xff << 2 | 0xff << 12 | 0x3f << 22)}

#ifdef CONFIG_LCD_V14_SLCD_CS
#define LCD_V14_SLCD_CS_FLAG 0x40
#else
#define LCD_V14_SLCD_CS_FLAG 0x00
#endif

#ifdef CONFIG_LCD_V14_SLCD_TE
#define LCD_V14_SLCD_TE_FLAG 0x80
#else
#define LCD_V14_SLCD_TE_FLAG 0x00
#endif

#ifdef CONFIG_LCD_V14_SLCD_RDY
#define LCD_V14_SLCD_RDY_FLAG 0x10
#else
#define LCD_V14_SLCD_RDY_FLAG 0x00
#endif

#define DPU_PORTD_SLCD_8BIT                                                     \
	{ .name = "dpu_slcd_8bit",  .port = GPIO_PORT_D, .func = GPIO_FUNC_1, \
	.pins = (0xbf << 2 | (0x3 | LCD_V14_SLCD_CS_FLAG | LCD_V14_SLCD_TE_FLAG) << 12 | (0x20 | LCD_V14_SLCD_RDY_FLAG) << 22)}
#define DPU_PORTD_SLCD_9BIT                                                     \
	{ .name = "dpu_slcd_9bit",  .port = GPIO_PORT_D, .func = GPIO_FUNC_1, \
	.pins = (0xbf << 2 | (0x7 | LCD_V14_SLCD_CS_FLAG | LCD_V14_SLCD_TE_FLAG) << 12 | (0x20 | LCD_V14_SLCD_RDY_FLAG) << 22)}
#define DPU_PORTD_SLCD_16BIT                                                     \
	{ .name = "dpu_slcd_16bit",  .port = GPIO_PORT_D, .func = GPIO_FUNC_1, \
	.pins = (0xbf << 2 | (0x3f | LCD_V14_SLCD_CS_FLAG | LCD_V14_SLCD_TE_FLAG) << 12 | (0x2f | LCD_V14_SLCD_RDY_FLAG) << 22)}

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
extern struct platform_device jz_i2s_device;
extern struct platform_device jz_dmic_device;
extern struct platform_device jz_codec_device;
extern struct platform_device es8374_codec_device;

/* alsa */
#ifdef CONFIG_SND_ALSA_INGENIC
extern struct platform_device jz_aic_device;
extern struct platform_device jz_aic_dma_device;
extern struct platform_device jz_alsa_device;
#endif
extern struct platform_device jz_mixer0_device;
extern struct platform_device jz_mixer1_device;
extern struct platform_device jz_mixer2_device;
extern struct platform_device jz_mixer3_device;
extern struct platform_device jz_rtc_device;
extern struct platform_device jz_efuse_device;
extern struct platform_device jz_tcu_device;

int jz_device_register(struct platform_device *pdev, void *pdata);
void *get_driver_common_interfaces(void);

#endif
/* __PLATFORM_H__ */
