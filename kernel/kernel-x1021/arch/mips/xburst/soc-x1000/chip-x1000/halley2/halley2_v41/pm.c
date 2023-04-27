/*
 * Copyright (c) 2006-2010  Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <gpio.h>

// default gpio state is input pull;
__initdata int gpio_ss_table[][2] = {
	{32*0+0,	GSS_INPUT_PULL	},	/* SLCD_D0 */
	{32*0+1,	GSS_INPUT_PULL	},	/* SLCD_D1 */
	{32*0+2,	GSS_INPUT_PULL	},	/* SLCD_D2 */
	{32*0+3,	GSS_INPUT_PULL	},	/* SLCD_D3 */
	{32*0+4,	GSS_INPUT_PULL	},	/* SLCD_D4 */
	{32*0+5,	GSS_INPUT_PULL	},	/* SLCD_D5 */
	{32*0+6,	GSS_INPUT_PULL	},	/* SLCD_D6 */
	{32*0+7,	GSS_INPUT_PULL	},	/* SLCD_D7 */
	{32*0+8,	GSS_INPUT_PULL	},	/* SLCD_D8 */
	{32*0+9,	GSS_INPUT_PULL	},	/* SLCD_D9 */
	{32*0+10,	GSS_INPUT_PULL	},	/* SLCD_D10 */
	{32*0+11,	GSS_INPUT_PULL	},	/* SLCD_D11 */
	{32*0+12,	GSS_INPUT_PULL	},	/* SLCD_D12 */
	{32*0+13,	GSS_INPUT_PULL	},	/* SLCD_D13 */
	{32*0+14,	GSS_INPUT_PULL	},	/* SLCD_D14 */
	{32*0+15,	GSS_INPUT_PULL	},	/* SLCD_D15 */
	{32*0+16,	GSS_INPUT_PULL	},	/* CIM_D3 */
	{32*0+17,	GSS_INPUT_PULL	},	/* CIM_D2 */
	{32*0+18,	GSS_INPUT_PULL	},	/* CIM_D1 */
	{32*0+19,	GSS_INPUT_PULL	},	/* CIM_D0 */
	{32*0+20,	GSS_INPUT_PULL	},	/* MSC0_D3 */
	{32*0+21,	GSS_INPUT_PULL	},	/* MSC0_D2 */
	{32*0+22,	GSS_INPUT_PULL	},	/* MSC0_D1 */
	{32*0+23,	GSS_INPUT_PULL	},	/* MSC0_D0 */
	{32*0+24,	GSS_INPUT_PULL	},	/* MSC0_CLK */
	{32*0+25,	GSS_INPUT_PULL	},	/* MSC0_CMD */
	{32*0+26,	GSS_INPUT_PULL	},	/* SFC_CLK */
	{32*0+27,	GSS_OUTPUT_HIGH	},	/* SFC_CE */
	{32*0+28,	GSS_INPUT_PULL	},	/* SFC_DR */
	{32*0+29,	GSS_INPUT_PULL	},	/* SFC_DT */
	{32*0+30,	GSS_INPUT_PULL	},	/* SFC_WP */
	{32*0+31,	GSS_INPUT_PULL	},	/* SFC_HOL */
	{32*1+0,	GSS_INPUT_PULL	},	/* I2S_MCLK */
	{32*1+1,	GSS_INPUT_PULL	},	/* I2S_BCLK */
	{32*1+2,	GSS_INPUT_PULL	},	/* I2S_LRCK */
	{32*1+3,	GSS_INPUT_PULL	},	/* I2S_DI */
	{32*1+4,	GSS_INPUT_PULL	},	/* I2S_DO */
#ifdef CONFIG_JZ_DMIC_WAKEUP_V13
	{32*1+5,	GSS_INPUT_PULL	},	/* DMIC_IN1 */
#else
	{32*1+5,	GSS_INPUT_PULL	},	/* DMIC_IN1 */
#endif
	{32*1+6,	GSS_INPUT_PULL	},	/* MAC_PHY_CLK */
	{32*1+7,	GSS_INPUT_PULL	},	/* MAC_CRS_DV */
	{32*1+8,	GSS_INPUT_PULL	},	/* MAC_RXD1 */
	{32*1+9,	GSS_INPUT_PULL	},	/* MAC_RXD0 */
	{32*1+10,	GSS_INPUT_PULL	},	/* MAC_TXEN */
	{32*1+11,	GSS_INPUT_PULL	},	/* MAC_TXD1 */
	{32*1+12,	GSS_INPUT_PULL	},	/* MAC_TXD0 */
	{32*1+13,	GSS_INPUT_PULL	},	/* MAC_MDC */
	{32*1+14,	GSS_INPUT_PULL	},	/* MAC_MDIO */
	{32*1+15,	GSS_INPUT_PULL	},	/* MAC_REF_CLK */
	{32*1+16,	GSS_INPUT_PULL	},	/* SLCD_RD */
	{32*1+17,	GSS_INPUT_PULL	},	/* SLCD_WR */
	{32*1+18,	GSS_INPUT_PULL	},	/* SLCD_CE */
	{32*1+19,	GSS_INPUT_PULL	},	/* SLCD_TE */
	{32*1+20,	GSS_INPUT_PULL	},	/* SLCD_DC */
#ifdef CONFIG_JZ_DMIC_WAKEUP_V13
	{32*1+21,	GSS_INPUT_PULL	},	/* DMIC_CLK */
	{32*1+22,	GSS_INPUT_PULL	},	/* DMIC_IN0 */
#else
	{32*1+21,	GSS_INPUT_PULL	},	/* DMIC_CLK */
	{32*1+22,	GSS_INPUT_PULL	},	/* DMIC_IN0 */
#endif
	{32*1+23,	GSS_INPUT_PULL	},	/* SMB0_SCK */
	{32*1+24,	GSS_INPUT_PULL	},	/* SMB0_SDA */
	{32*1+25,	GSS_OUTPUT_LOW	},	/* DRVVBUS */
	{32*1+26,	GSS_INPUT_PULL	},	/* CLK32K */
	{32*1+27,	GSS_OUTPUT_LOW	},	/* EXCLK */
	{32*1+28,	GSS_INPUT_NOPULL},	/* BOOT_SEL0 */
	{32*1+29,	GSS_INPUT_NOPULL},	/* BOOT_SEL1 */
	{32*1+30,	GSS_INPUT_NOPULL},	/* BOOT_SEL2 */
	{32*1+31,	GSS_IGNORE	},	/* WAKEUP */
	{32*2+0,	GSS_IGNORE      },	/* MSC1_CLK */
	{32*2+1,	GSS_IGNORE      },	/* MSC1_CMD */
	{32*2+2,	GSS_IGNORE      },	/* MSC1_D0 */
	{32*2+3,	GSS_IGNORE      },	/* MSC1_D1 */
	{32*2+4,	GSS_IGNORE      },	/* MSC1_D2 */
	{32*2+5,	GSS_IGNORE      },	/* MSC1_D3 */
	{32*2+6,	GSS_IGNORE	},	/* PCM_CLK */
	{32*2+7,	GSS_IGNORE	},	/* PCM_DO */
	{32*2+8,	GSS_IGNORE	},	/* PCM_DI */
	{32*2+9,	GSS_IGNORE	},	/* PCM_SYN */
	{32*2+10,	GSS_IGNORE	},	/* UART0_RXD */
	{32*2+11,	GSS_IGNORE	},	/* UART0_TXD */
	{32*2+12,	GSS_IGNORE	},	/* UART0_CTS_N */
	{32*2+13,	GSS_IGNORE	},	/* UART0_RTS_N */
	{32*2+16,	GSS_IGNORE      },	/* WL_WAKE_HOST */
	{32*2+17,	GSS_IGNORE	},	/* WL_REG_EN */
	{32*2+18,	GSS_IGNORE	},	/* BT_REG_EN */
	{32*2+19,	GSS_IGNORE      },	/* HOST_WAKE_BT */
	{32*2+20,	GSS_IGNORE      },	/* BT_WAKE_HOST */
	{32*2+21,	GSS_INPUT_PULL  },	/* OTG_ID */
	{32*2+22,	GSS_INPUT_NOPULL},	/* USB_DETECT */
	{32*2+23,	GSS_INPUT_PULL	},	/* MAC_RST_N */
	{32*2+24,	GSS_INPUT_PULL	},	/* USB_DETE */
	{32*2+25,	GSS_INPUT_PULL	},	/* LCD_PWM */
	{32*2+26,	GSS_INPUT_PULL	},	/* SMB1_SCk */
	{32*2+27,	GSS_INPUT_PULL	},	/* SMB1_SDA */
	{32*2+31,	GSS_IGNORE	},	/* uart2 */
	{32*3+0,	GSS_INPUT_PULL	},	/* SSI0_CLK */
	{32*3+1,	GSS_INPUT_PULL	},	/* SSI0_CE0 */
	{32*3+2,	GSS_INPUT_PULL	},	/* SSI0_DT */
	{32*3+3,	GSS_INPUT_PULL	},	/* SSI0_DR */
#ifdef CONFIG_SERIAL_JZ47XX_UART2_PD
	{32*3+4,	GSS_INPUT_PULL	},	/* UART2_TXD */
	{32*3+5,	GSS_INPUT_PULL	},	/* UART2_RXD */
#else
	{32*3+4,	GSS_INPUT_PULL	},	/* UART2_TXD */
	{32*3+5,	GSS_INPUT_PULL	},	/* UART2_RXD */
#endif
	{GSS_TABLET_END,GSS_TABLET_END	}	/* GPIO Group Set End */
};
