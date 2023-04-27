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
#define UART0_PORTB_FULL_DUPLEX							\
	{ .name = "uart0", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0xF << 19, }
#define UART0_PORTB_NOCTSRTS							\
	{ .name = "uart0", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x9 << 19, }

#define UART1_PORTB								\
	{ .name = "uart1", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x3 << 23, }

#define UART2_PORTC_PC0809							\
	{ .name = "uart2", .port = GPIO_PORT_C, .func = GPIO_FUNC_1, .pins = 0x3 << 8, }
#define UART2_PORTC_PC1314							\
	{ .name = "uart2", .port = GPIO_PORT_C, .func = GPIO_FUNC_2, .pins = 0x3 << 13, }
#define UART2_PORTC_PC1718							\
	{ .name = "uart2", .port = GPIO_PORT_C, .func = GPIO_FUNC_2, .pins = 0x3 << 17, }
#define UART2_PORTC_FULL_DUPLEX							\
	{ .name = "uart2", .port = GPIO_PORT_C, .func = GPIO_FUNC_2, .pins = 0xf << 11, }
/*******************************************************************************************************************/

#define MSC0_PORTB_4BIT							\
	{ .name = "msc0-pb-4bit",	.port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = (0x3f<<0), }
#define MSC1_PORTC							\
	{ .name = "msc1-pC",		.port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x3f<<2), }

/*******************************************************************************************************************/
/*****************************************************************************************************************/

#define SSI0_PORTC_ONLY_DT_CLK							\
	{ .name = "ssi0-pc",	       .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x3 << 12), }
#define SSI1_PORTB_ONLY_DT_CLK							\
	{ .name = "ssi1-pb",	       .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = (0x5 << 8), }

#define SSI0_PORTC_NO_CE							\
	{ .name = "ssi0-pc",	       .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x7 << 11), }
#define SSI1_PORTB_NO_CE							\
	{ .name = "ssi1-pb",	       .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = (0x7 << 8), }

#define SSI0_PORTC							\
	{ .name = "ssi0-pc",	       .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0xf << 11), }
#define SSI1_PORTB							\
	{ .name = "ssi1-pb",	       .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = (0xf << 8), }

#define SSI0_PORTC_CE_ONLY							\
	{ .name = "ssi0-pc",	       .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x1 << 14), }
#define SSI1_PORTB_CE_ONLY							\
	{ .name = "ssi1-pb",	       .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = (0x1 << 11), }

#define SFC_PORTA							\
	{ .name = "sfc",	.port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = (0x3f << 23), }

/*****************************************************************************************************************/

#define I2C0_PORTA							\
	{ .name = "i2c0-pa", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x3 << 12, }
#define I2C1_PORTB							\
	{ .name = "i2c1-pb", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x3 << 25, }
#define I2C1_PORTC							\
	{ .name = "i2c1-pc", .port = GPIO_PORT_C, .func = GPIO_FUNC_2, .pins = 0x3 << 8, }

/*******************************************************************************************************************/

#define MII_PORTBDF							\
	{ .name = "mii-0", .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = 0x00000010, }, \
	{ .name = "mii-1", .port = GPIO_PORT_D, .func = GPIO_FUNC_1, .pins = 0x3c000000, }, \
	{ .name = "mii-2", .port = GPIO_PORT_F, .func = GPIO_FUNC_0, .pins = 0x0000fff0, }

/*******************************************************************************************************************/

#define OTG_DRVVUS							\
	{ .name = "otg-drvvbus", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 1 << 27, }

/*******************************************************************************************************************/

#define MCLK_PORTA                 \
{ .name = "mclk", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 1 << 15, }

#define DVP_PORTA_LOW_8BIT							\
	{ .name = "dvp-pa-8bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x0003c0ff, }

#define DVP_PORTA_LOW_10BIT							\
	{ .name = "dvp-pa-10bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x000343ff, }

#define DVP_PORTA_HIGH_10BIT							\
	{ .name = "dvp-pa-10bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x00034ffc, }

#define DVP_PORTA_12BIT							\
	{ .name = "dvp-pa-12bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x00034fff, }

/*******************************************************************************************************************/
#define DPU_PORTB_SLCD_8BIT                                                     \
        { .name = "dpu_slcd_8bit",  .port = GPIO_PORT_B, .func = GPIO_FUNC_3, .pins = ((0x3f << 6) | (0x2f << 13))}
#define DPU_PORTC_SLCD_8BIT                                                     \
        { .name = "dpu_slcd_8bit",  .port = GPIO_PORT_C, .func = GPIO_FUNC_3, .pins = ((0x7f << 2) | (0xf << 15))}
/*******************************************************************************************************************/
#define GMAC_PORTB							\
	{ .name = "gmac_pb",	.port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x0001efc0}

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
extern struct platform_device jz_uart2_device;

extern struct platform_device jz_ssi1_device;
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
