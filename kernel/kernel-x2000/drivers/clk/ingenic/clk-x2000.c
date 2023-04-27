/*
 * Copyright (C) 2016 Ingenic Semiconductor Co., Ltd.
 * Author: cli <chen.li@ingenic.com>
 *
 * X2000 Clock (kernel.4.4)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/clk-provider.h>
#include <dt-bindings/clock/ingenic-x2000.h>
#include <linux/syscore_ops.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/mfd/syscon.h>
#include <soc/cpm.h>
#include <linux/regmap.h>
#include <linux/module.h>

#include "clk-comm.h"
#include "clk-pll.h"
#include "clk-cpccr.h"
#include "clk-cgu.h"
#include "clk-gate.h"
#include "clk-x2000-cgu-audio.h"

static const char *clk_name[NR_CLKS] = {
	[CLK_EXT	] = "ext",
	[CLK_RTC_EXT	] = "rtc_ext",
	[CLK_PLL_APLL	] = "apll",
	[CLK_PLL_MPLL	] = "mpll",
	[CLK_PLL_EPLL	] = "epll",
	[CLK_MUX_SCLKA	] = "sclka_mux",
	[CLK_MUX_CPLL	] = "cpu_mux",
	[CLK_MUX_H0PLL	] = "ahb0_mux",
	[CLK_MUX_H2PLL	] = "ahb2_mux",
	[CLK_RATE_CPUCLK] = "cpu",
	[CLK_RATE_L2CCLK] = "l2cache",
	[CLK_RATE_H0CLK	] = "ahb0",
	[CLK_RATE_H2CLK	] = "ahb2",
	[CLK_RATE_PCLK	] = "pclk",
	[CLK_CGU_DDR	] = "cgu_ddr",
	[CLK_CGU_MAC	] = "cgu_mac",
	[CLK_CGU_LCD	] = "cgu_lcd",
	[CLK_CGU_MSC0	] = "cgu_msc0",
	[CLK_CGU_MSC1	] = "cgu_msc1",
	[CLK_CGU_SFC	] = "cgu_sfc",
	[CLK_CGU_CIM	] = "cgu_cim",
	[CLK_CGU_PWM	] = "cgu_pwm",
	[CLK_CGU_I2S0	] = "cgu_i2s0",
	[CLK_CGU_I2S1	] = "cgu_i2s1",
	[CLK_CGU_I2S2	] = "cgu_i2s2",
	[CLK_CGU_I2S3	] = "cgu_i2s3",
	[CLK_CGU_SPDIF	] = "cgu_spdif",
	[CLK_CGU_PCM	] = "cgu_pcm",
	[CLK_CGU_DMIC	] = "cgu_dmic",
	[CLK_GATE_DDR	] = "gate_ddr",
	[CLK_GATE_CPU1	] = "gate_cpu1",
	[CLK_GATE_AHB0	] = "gate_ahb0",
	[CLK_GATE_APB0	] = "gate_apb0",
	[CLK_GATE_RTC	] = "gate_rtc",
	[CLK_GATE_SSI1	] = "gate_ssi1",
	[CLK_GATE_MAC   ] = "gate_mac",
	[CLK_GATE_AES	] = "gate_aes",
	[CLK_GATE_LCD	] = "gate_lcd",
	[CLK_GATE_CIM	] = "gate_cim",
	[CLK_GATE_PDMA	] = "gate_pdma",
	[CLK_GATE_OST	] = "gate_ost",
	[CLK_GATE_SSI0	] = "gate_ssi0",
	[CLK_GATE_TCU	] = "gate_tcu",
	[CLK_GATE_DTRNG	] = "gate_dtrng",
	[CLK_GATE_UART2	] = "gate_uart2",
	[CLK_GATE_UART1	] = "gate_uart1",
	[CLK_GATE_UART0	] = "gate_uart0",
	[CLK_GATE_SADC	] = "gate_sadc",
	[CLK_GATE_JPEG	] = "gate_jpeg",
	[CLK_GATE_AUDIO	] = "gate_audio",
	[CLK_GATE_SMB3	] = "gate_i2c3",
	[CLK_GATE_SMB2	] = "gate_i2c2",
	[CLK_GATE_SMB1	] = "gate_i2c1",
	[CLK_GATE_SMB0	] = "gate_i2c0",
	[CLK_GATE_SCC	] = "gate_scc",
	[CLK_GATE_MSC1	] = "gate_msc1",
	[CLK_GATE_MSC0	] = "gate_msc0",
	[CLK_GATE_OTG	] = "gate_otg",
	[CLK_GATE_SFC	] = "gate_sfc",
	[CLK_GATE_EFUSE	] = "gate_efuse",
	[CLK_GATE_NEMC	] = "gate_nemc",
	[CLK_GATE_ARB	] = "gate_arb",
	[CLK_GATE_MIPI	] = "gate_mipi",
	[CLK_GATE_CPU	] = "gate_cpu",
	[CLK_GATE_INTC	] = "gate_intc",
	[CLK_GATE_GPIO	] = "gate_gpio",
	[CLK_GATE_SPDIF	] = "gate_spdif",
	[CLK_GATE_DMIC	] = "gate_dmic",
	[CLK_GATE_PCM	] = "gate_pcm",
	[CLK_GATE_I2S3	] = "gate_i2s3",
	[CLK_GATE_I2S2	] = "gate_i2s2",
	[CLK_GATE_I2S1	] = "gate_i2s1",
	[CLK_GATE_I2S0	] = "gate_i2s0",
	[CLK_GATE_ROT	] = "gate_rot",
	[CLK_GATE_HASH	] = "gate_hash",
	[CLK_GATE_PWM	] = "gate_pwm",
	[CLK_GATE_UART5	] = "gate_uart5",
	[CLK_GATE_UART4	] = "gate_uart4",
	[CLK_GATE_UART3	] = "gate_uart3",
	[CLK_GATE_SMB5	] = "gate_i2c5",
	[CLK_GATE_SMB4	] = "gate_i2c4",
	[CLK_GATE_USBPHY] = "gate_usbphy",
};

/********************************************************************************
 *	PLL
 ********************************************************************************/
/*PLL HWDESC*/
static const s8 pll_od_encode[7] = {1, 2, 4, 8, 16, 32, 64};
static struct ingenic_pll_hwdesc apll_hwdesc = \
	PLL_DESC(CPM_CPAPCR, 20, 12, 14, 6, 11, 3, 0, 3, -1, pll_od_encode, NULL, 2);

static struct ingenic_pll_hwdesc mpll_hwdesc = \
	PLL_DESC(CPM_CPMPCR, 20, 12, 14, 6, 11, 3, 0, 3, -1, pll_od_encode, NULL, 2);

static struct ingenic_pll_hwdesc epll_hwdesc = \
	PLL_DESC(CPM_CPEPCR, 20, 12, 14, 6, 11, 3, 0, 3, -1, pll_od_encode, NULL, 2);

/********************************************************************************
 *	CPCCR
 ********************************************************************************/
/*CPCCR PARENTS*/
static const int sclk_a_p[] = { DUMMY_STOP, CLK_EXT, CLK_PLL_APLL, DUMMY_UNKOWN };
static const int cpccr_p[] = { DUMMY_STOP, CLK_MUX_SCLKA, CLK_PLL_MPLL, DUMMY_UNKOWN };

/*CPCCR HWDESC*/
#define INDEX_CPCCR_HWDESC(_id)  ((_id) - CLK_ID_CPCCR)
static struct ingenic_cpccr_hwdesc cpccr_hwdesc[] = {
	[INDEX_CPCCR_HWDESC(CLK_MUX_SCLKA)] = CPCCR_MUX_RODESC(CPM_CPCCR, 30, 0x3),	/*sclk a*/
	[INDEX_CPCCR_HWDESC(CLK_MUX_CPLL)] = CPCCR_MUX_RODESC(CPM_CPCCR, 28, 0x3),	/*cpll*/
	[INDEX_CPCCR_HWDESC(CLK_MUX_H0PLL)] = CPCCR_MUX_RODESC(CPM_CPCCR, 26, 0x3),	/*h0pll*/
	[INDEX_CPCCR_HWDESC(CLK_MUX_H2PLL)] = CPCCR_MUX_RODESC(CPM_CPCCR, 24, 0x3),	/*h2pll*/
	[INDEX_CPCCR_HWDESC(CLK_RATE_PCLK)] = CPCCR_RATE_RODESC(CPM_CPCCR, 16, 0xf),	/*pdiv*/
	[INDEX_CPCCR_HWDESC(CLK_RATE_H2CLK)] = CPCCR_RATE_RODESC(CPM_CPCCR, 12, 0xf),	/*h2div*/
	[INDEX_CPCCR_HWDESC(CLK_RATE_H0CLK)] = CPCCR_RATE_RODESC(CPM_CPCCR, 8, 0xf),	/*h0div*/
	[INDEX_CPCCR_HWDESC(CLK_RATE_L2CCLK)] = CPCCR_RATE_RODESC(CPM_CPCCR, 4, 0xf),	/*l2cdiv*/
	[INDEX_CPCCR_HWDESC(CLK_RATE_CPUCLK)] = CPCCR_RATE_RODESC(CPM_CPCCR, 0, 0xf),	/*cdiv*/
};
#define X2000_CPCCR_HWDESC(_id) &cpccr_hwdesc[INDEX_CPCCR_HWDESC((_id))]

/********************************************************************************
 *	cgu
 ********************************************************************************/
/*CGU PARENTS*/
static const int cgu_ddr[] = { DUMMY_STOP, CLK_MUX_SCLKA, CLK_PLL_MPLL, DUMMY_UNKOWN };
static const int cgu_sel_grp[] = { CLK_MUX_SCLKA, CLK_PLL_MPLL };

/*CGU HWDESC*/
#define INDEX_CGU_HWDESC(_id) ((_id) - CLK_ID_CGU)
static struct ingenic_cgu_hwdesc cgu_hwdesc[] = {
	/*reg, sel, selmsk, ce, busy ,stop, cdr, cdrmsk, step*/
	[INDEX_CGU_HWDESC(CLK_CGU_DDR)] = CGU_DESC(CPM_DDRCDR, 30, 0x3, 29, 28, 27, 0, 0xf, 1),
	[INDEX_CGU_HWDESC(CLK_CGU_MAC)] = CGU_DESC(CPM_MACCDR, 30, 0x1, 29, 28, 27, 0, 0xff, 1),
	[INDEX_CGU_HWDESC(CLK_CGU_LCD)] = CGU_DESC(CPM_LPCDR, 30, 0x1, 29, 28, 27, 0, 0xff, 1),
	[INDEX_CGU_HWDESC(CLK_CGU_MSC0)] = CGU_DESC(CPM_MSC0CDR, 30, 0x1, 29, 28, 27, 0, 0xff, 4),
	[INDEX_CGU_HWDESC(CLK_CGU_MSC1)] = CGU_DESC(CPM_MSC1CDR, 30, 0x1, 29, 28, 27, 0, 0xff, 4),
	[INDEX_CGU_HWDESC(CLK_CGU_SFC)] = CGU_DESC(CPM_SSICDR, 30, 0x1, 29, 28, 27, 0, 0xff, 1),
	[INDEX_CGU_HWDESC(CLK_CGU_CIM)] = CGU_DESC(CPM_CIMCDR, 30, 0x1, 29, 28, 27, 0, 0xff, 1),
	[INDEX_CGU_HWDESC(CLK_CGU_PWM)] = CGU_DESC(CPM_PWMCDR, 30, 0x1, 29, 28, 27, 0, 0xf, 1),
};
#define X2000_CGU_HWDESC(_id) &cgu_hwdesc[INDEX_CGU_HWDESC((_id))]

/*CGU PARENTS*/
static const int cgu_i2s[] = { CLK_MUX_SCLKA, CLK_PLL_EPLL };
static const int cgu_audio[] = { CLK_CGU_I2S0, CLK_CGU_I2S1, CLK_CGU_I2S2, CLK_CGU_I2S3 };
static const int cgu_audio_dmic[] = { CLK_EXT, CLK_CGU_I2S3 };

/*CGU HWDESC*/
#define INDEX_CGU_AUDIO_HWDESC(_id) ((_id) - CLK_ID_CGU_AUDIO)
static struct ingenic_cgu_audio_hwdesc cgu_audio_hwdesc[] = {
	[INDEX_CGU_AUDIO_HWDESC(CLK_CGU_I2S0)] = CGU_AUDIO_DESC(CPM_I2S0CDR, 30, 0x1, 29, 20, 0x1ff, 0, 0xfffff),
	[INDEX_CGU_AUDIO_HWDESC(CLK_CGU_I2S1)] = CGU_AUDIO_DESC(CPM_I2S1CDR, 30, 0x1, 29, 20, 0x1ff, 0, 0xfffff),
	[INDEX_CGU_AUDIO_HWDESC(CLK_CGU_I2S2)] = CGU_AUDIO_DESC(CPM_I2S2CDR, 30, 0x1, 29, 20, 0x1ff, 0, 0xfffff),
	[INDEX_CGU_AUDIO_HWDESC(CLK_CGU_I2S3)] = CGU_AUDIO_DESC(CPM_I2S3CDR, 30, 0x1, 29, 20, 0x1ff, 0, 0xfffff),
	[INDEX_CGU_AUDIO_HWDESC(CLK_CGU_SPDIF)] = CGU_AUDIO_DESC(CPM_AUDIOCR, 3, 0x3, -1, -1, -1, -1, -1),
	[INDEX_CGU_AUDIO_HWDESC(CLK_CGU_PCM)] = CGU_AUDIO_DESC(CPM_AUDIOCR, 1, 0x3, -1, -1, -1, -1, -1),
	[INDEX_CGU_AUDIO_HWDESC(CLK_CGU_DMIC)] = CGU_AUDIO_DESC(CPM_AUDIOCR, 0, 0x1, -1, -1, -1, -1, -1),
};
#define X2000_CGU_AUDIO_HWDESC(_id) &cgu_audio_hwdesc[INDEX_CGU_AUDIO_HWDESC((_id))]

/********************************************************************************
 *	GATE
 ********************************************************************************/
#define INDEX_GATE_HWDESC(_id)  ((_id) - CLK_ID_GATE)
static struct ingenic_gate_hwdesc gate_hwdesc[] = {
	[INDEX_GATE_HWDESC(CLK_GATE_DDR	)] = GATE_DESC(CPM_CLKGR, 31),
	[INDEX_GATE_HWDESC(CLK_GATE_CPU1)] = GATE_DESC(CPM_CLKGR, 30),
	[INDEX_GATE_HWDESC(CLK_GATE_AHB0)] = GATE_DESC(CPM_CLKGR, 29),
	[INDEX_GATE_HWDESC(CLK_GATE_APB0)] = GATE_DESC(CPM_CLKGR, 28),
	[INDEX_GATE_HWDESC(CLK_GATE_RTC)] = GATE_DESC(CPM_CLKGR, 27),
	[INDEX_GATE_HWDESC(CLK_GATE_SSI1)] = GATE_DESC(CPM_CLKGR, 26),
	[INDEX_GATE_HWDESC(CLK_GATE_MAC)] = GATE_DESC(CPM_CLKGR, 25),
	[INDEX_GATE_HWDESC(CLK_GATE_AES)] = GATE_DESC(CPM_CLKGR, 24),
	[INDEX_GATE_HWDESC(CLK_GATE_LCD)] = GATE_DESC(CPM_CLKGR, 23),
	[INDEX_GATE_HWDESC(CLK_GATE_CIM)] = GATE_DESC(CPM_CLKGR, 22),
	[INDEX_GATE_HWDESC(CLK_GATE_PDMA)] = GATE_DESC(CPM_CLKGR, 21),
	[INDEX_GATE_HWDESC(CLK_GATE_OST)] = GATE_DESC(CPM_CLKGR, 20),
	[INDEX_GATE_HWDESC(CLK_GATE_SSI0)] = GATE_DESC(CPM_CLKGR, 19),
	[INDEX_GATE_HWDESC(CLK_GATE_TCU)] = GATE_DESC(CPM_CLKGR, 18),
	[INDEX_GATE_HWDESC(CLK_GATE_DTRNG)] = GATE_DESC(CPM_CLKGR, 17),
	[INDEX_GATE_HWDESC(CLK_GATE_UART2)] = GATE_DESC(CPM_CLKGR, 16),
	[INDEX_GATE_HWDESC(CLK_GATE_UART1)] = GATE_DESC(CPM_CLKGR, 15),
	[INDEX_GATE_HWDESC(CLK_GATE_UART0)] = GATE_DESC(CPM_CLKGR, 14),
	[INDEX_GATE_HWDESC(CLK_GATE_SADC)] = GATE_DESC(CPM_CLKGR, 13),
	[INDEX_GATE_HWDESC(CLK_GATE_JPEG)] = GATE_DESC(CPM_CLKGR, 12),
	[INDEX_GATE_HWDESC(CLK_GATE_AUDIO)] = GATE_DESC(CPM_CLKGR, 11),
	[INDEX_GATE_HWDESC(CLK_GATE_SMB3)] = GATE_DESC(CPM_CLKGR, 10),
	[INDEX_GATE_HWDESC(CLK_GATE_SMB2)] = GATE_DESC(CPM_CLKGR, 9),
	[INDEX_GATE_HWDESC(CLK_GATE_SMB1)] = GATE_DESC(CPM_CLKGR, 8),
	[INDEX_GATE_HWDESC(CLK_GATE_SMB0)] = GATE_DESC(CPM_CLKGR, 7),
	[INDEX_GATE_HWDESC(CLK_GATE_SCC)] = GATE_DESC(CPM_CLKGR, 6),
	[INDEX_GATE_HWDESC(CLK_GATE_MSC1)] = GATE_DESC(CPM_CLKGR, 5),
	[INDEX_GATE_HWDESC(CLK_GATE_MSC0)] = GATE_DESC(CPM_CLKGR, 4),
	[INDEX_GATE_HWDESC(CLK_GATE_OTG)] = GATE_DESC(CPM_CLKGR, 3),
	[INDEX_GATE_HWDESC(CLK_GATE_SFC)] = GATE_DESC(CPM_CLKGR, 2),
	[INDEX_GATE_HWDESC(CLK_GATE_EFUSE)] = GATE_DESC(CPM_CLKGR, 1),
	[INDEX_GATE_HWDESC(CLK_GATE_NEMC)] = GATE_DESC(CPM_CLKGR, 0),

	[INDEX_GATE_HWDESC(CLK_GATE_ARB)] = GATE_DESC(CPM_CLKGR1, 19),
	[INDEX_GATE_HWDESC(CLK_GATE_MIPI)] = GATE_DESC(CPM_CLKGR1, 18),
	[INDEX_GATE_HWDESC(CLK_GATE_CPU)] = GATE_DESC(CPM_CLKGR1, 17),
	[INDEX_GATE_HWDESC(CLK_GATE_INTC)] = GATE_DESC(CPM_CLKGR1, 16),
	[INDEX_GATE_HWDESC(CLK_GATE_GPIO)] = GATE_DESC(CPM_CLKGR1, 15),
	[INDEX_GATE_HWDESC(CLK_GATE_SPDIF)] = GATE_DESC(CPM_CLKGR1, 14),
	[INDEX_GATE_HWDESC(CLK_GATE_DMIC)] = GATE_DESC(CPM_CLKGR1, 13),
	[INDEX_GATE_HWDESC(CLK_GATE_PCM)] = GATE_DESC(CPM_CLKGR1, 12),
	[INDEX_GATE_HWDESC(CLK_GATE_I2S3)] = GATE_DESC(CPM_CLKGR1, 11),
	[INDEX_GATE_HWDESC(CLK_GATE_I2S2)] = GATE_DESC(CPM_CLKGR1, 10),
	[INDEX_GATE_HWDESC(CLK_GATE_I2S1)] = GATE_DESC(CPM_CLKGR1, 9),
	[INDEX_GATE_HWDESC(CLK_GATE_I2S0)] = GATE_DESC(CPM_CLKGR1, 8),
	[INDEX_GATE_HWDESC(CLK_GATE_ROT)] = GATE_DESC(CPM_CLKGR1, 7),
	[INDEX_GATE_HWDESC(CLK_GATE_HASH)] = GATE_DESC(CPM_CLKGR1, 6),
	[INDEX_GATE_HWDESC(CLK_GATE_PWM)] = GATE_DESC(CPM_CLKGR1, 5),
	[INDEX_GATE_HWDESC(CLK_GATE_UART5)] = GATE_DESC(CPM_CLKGR1, 4),
	[INDEX_GATE_HWDESC(CLK_GATE_UART4)] = GATE_DESC(CPM_CLKGR1, 3),
	[INDEX_GATE_HWDESC(CLK_GATE_UART3)] = GATE_DESC(CPM_CLKGR1, 2),
	[INDEX_GATE_HWDESC(CLK_GATE_SMB5)] =  GATE_DESC(CPM_CLKGR1, 1),
	[INDEX_GATE_HWDESC(CLK_GATE_SMB4)] =  GATE_DESC(CPM_CLKGR1, 0),
	[INDEX_GATE_HWDESC(CLK_GATE_USBPHY)] =  GATE_DESC(CPM_OPCR, 23),
};
#define X2000_GATE_HWDESC(_id)	\
	 &gate_hwdesc[INDEX_GATE_HWDESC(_id)]

/********************************************************************************
 *	X2000 CLK
 ********************************************************************************/
/*Note: parent num > 1, _pids is parent clock id's array*/
#define X2000_CLK_MUX(_id, _pids, _ops, _flags, _phwdesc)	\
		CLK_INIT_DATA_LOCKED(_id, _ops, _pids, ARRAY_SIZE(_pids), _flags, _phwdesc)

/*Note: parent num == 1, _pid is parent clock id*/
#define X2000_CLK(_id, _pid, _ops, _flags, _phwdesc)	\
	CLK_INIT_DATA_LOCKED(_id, _ops, _pid, 1, _flags, _phwdesc)

static const struct ingenic_clk_init __initdata x2000_clk_init_data[] = {
	/*pll*/
	X2000_CLK(CLK_PLL_APLL, CLK_EXT, &ingenic_pll_ro_ops, CLK_IGNORE_UNUSED, &apll_hwdesc),
	X2000_CLK(CLK_PLL_MPLL, CLK_EXT, &ingenic_pll_ro_ops, CLK_IGNORE_UNUSED, &mpll_hwdesc),
	X2000_CLK(CLK_PLL_EPLL, CLK_EXT, &ingenic_pll_ro_ops, CLK_IGNORE_UNUSED, &epll_hwdesc),
	/*cpccr*/
	X2000_CLK_MUX(CLK_MUX_SCLKA, sclk_a_p, &ingenic_cpccr_mux_ro_ops, 0, X2000_CPCCR_HWDESC(CLK_MUX_SCLKA)),
	X2000_CLK_MUX(CLK_MUX_CPLL, cpccr_p, &ingenic_cpccr_mux_ro_ops, 0, X2000_CPCCR_HWDESC(CLK_MUX_CPLL)),
	X2000_CLK_MUX(CLK_MUX_H0PLL, cpccr_p, &ingenic_cpccr_mux_ro_ops, 0, X2000_CPCCR_HWDESC(CLK_MUX_H0PLL)),
	X2000_CLK_MUX(CLK_MUX_H2PLL, cpccr_p, &ingenic_cpccr_mux_ro_ops, 0, X2000_CPCCR_HWDESC(CLK_MUX_H2PLL)),
	X2000_CLK(CLK_RATE_PCLK, CLK_MUX_H2PLL, &ingenic_cpccr_rate_ro_ops,0, X2000_CPCCR_HWDESC(CLK_RATE_PCLK)),
	X2000_CLK(CLK_RATE_H2CLK, CLK_MUX_H2PLL, &ingenic_cpccr_rate_ro_ops,0, X2000_CPCCR_HWDESC(CLK_RATE_H2CLK)),
	X2000_CLK(CLK_RATE_H0CLK, CLK_MUX_H0PLL, &ingenic_cpccr_rate_ro_ops,0, X2000_CPCCR_HWDESC(CLK_RATE_H0CLK)),
	X2000_CLK(CLK_RATE_CPUCLK, CLK_MUX_CPLL, &ingenic_cpccr_rate_ro_ops,0, X2000_CPCCR_HWDESC(CLK_RATE_CPUCLK)),
	X2000_CLK(CLK_RATE_L2CCLK, CLK_MUX_CPLL, &ingenic_cpccr_rate_ro_ops,0, X2000_CPCCR_HWDESC(CLK_RATE_L2CCLK)),

	/*cgu*/
	X2000_CLK_MUX(CLK_CGU_DDR, cgu_ddr, &ingenic_cgu_ops, CLK_IGNORE_UNUSED, X2000_CGU_HWDESC(CLK_CGU_DDR)),
	X2000_CLK_MUX(CLK_CGU_MAC, cgu_sel_grp, &ingenic_cgu_ops, CLK_SET_PARENT_GATE, X2000_CGU_HWDESC(CLK_CGU_MAC)),
	X2000_CLK_MUX(CLK_CGU_LCD, cgu_sel_grp, &ingenic_cgu_ops, CLK_SET_PARENT_GATE, X2000_CGU_HWDESC(CLK_CGU_LCD)),
	X2000_CLK_MUX(CLK_CGU_MSC0, cgu_sel_grp, &ingenic_cgu_ops, CLK_SET_PARENT_GATE, X2000_CGU_HWDESC(CLK_CGU_MSC0)),
	X2000_CLK_MUX(CLK_CGU_MSC1, cgu_sel_grp, &ingenic_cgu_ops, CLK_SET_PARENT_GATE, X2000_CGU_HWDESC(CLK_CGU_MSC1)),
	X2000_CLK_MUX(CLK_CGU_SFC, cgu_sel_grp, &ingenic_cgu_ops, CLK_SET_PARENT_GATE, X2000_CGU_HWDESC(CLK_CGU_SFC)),
	X2000_CLK_MUX(CLK_CGU_CIM, cgu_sel_grp, &ingenic_cgu_ops, CLK_SET_PARENT_GATE, X2000_CGU_HWDESC(CLK_CGU_CIM)),
	X2000_CLK_MUX(CLK_CGU_PWM, cgu_sel_grp, &ingenic_cgu_ops, CLK_SET_PARENT_GATE, X2000_CGU_HWDESC(CLK_CGU_PWM)),
	X2000_CLK_MUX(CLK_CGU_I2S0, cgu_i2s, &ingenic_cgu_i2s_ops, 0, X2000_CGU_AUDIO_HWDESC(CLK_CGU_I2S0)),
	X2000_CLK_MUX(CLK_CGU_I2S1, cgu_i2s, &ingenic_cgu_i2s_ops, 0, X2000_CGU_AUDIO_HWDESC(CLK_CGU_I2S1)),
	X2000_CLK_MUX(CLK_CGU_I2S2, cgu_i2s, &ingenic_cgu_i2s_ops, 0, X2000_CGU_AUDIO_HWDESC(CLK_CGU_I2S2)),
	X2000_CLK_MUX(CLK_CGU_I2S3, cgu_i2s, &ingenic_cgu_i2s_ops, 0, X2000_CGU_AUDIO_HWDESC(CLK_CGU_I2S3)),
	X2000_CLK_MUX(CLK_CGU_SPDIF, cgu_audio, &ingenic_cgu_audio_ops, CLK_SET_RATE_PARENT, X2000_CGU_AUDIO_HWDESC(CLK_CGU_SPDIF)),
	X2000_CLK_MUX(CLK_CGU_PCM, cgu_audio, &ingenic_cgu_audio_ops, CLK_SET_RATE_PARENT, X2000_CGU_AUDIO_HWDESC(CLK_CGU_PCM)),
	X2000_CLK_MUX(CLK_CGU_DMIC, cgu_audio, &ingenic_cgu_audio_ops, CLK_SET_RATE_PARENT, X2000_CGU_AUDIO_HWDESC(CLK_CGU_DMIC)),

	/*gate*/
	X2000_CLK(CLK_GATE_DDR	, CLK_RATE_H0CLK, &ingenic_gate_ops, CLK_IGNORE_UNUSED, X2000_GATE_HWDESC(CLK_GATE_DDR  )),
	X2000_CLK(CLK_GATE_CPU1	, CLK_RATE_CPUCLK, &ingenic_gate_ops, CLK_IGNORE_UNUSED, X2000_GATE_HWDESC(CLK_GATE_CPU1 )),
	X2000_CLK(CLK_GATE_AHB0	, CLK_RATE_H0CLK, &ingenic_gate_ops, CLK_IGNORE_UNUSED, X2000_GATE_HWDESC(CLK_GATE_AHB0 )),
	X2000_CLK(CLK_GATE_APB0	, CLK_RATE_PCLK, &ingenic_gate_ops, CLK_IGNORE_UNUSED, X2000_GATE_HWDESC(CLK_GATE_APB0 )),
	X2000_CLK(CLK_GATE_RTC	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_RTC	 )),
	X2000_CLK(CLK_GATE_SSI1	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SSI1 )),
	X2000_CLK(CLK_GATE_MAC  , CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_MAC  )),
	X2000_CLK(CLK_GATE_AES	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_AES	 )),
	X2000_CLK(CLK_GATE_LCD	, CLK_RATE_H0CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_LCD	 )),
	X2000_CLK(CLK_GATE_CIM	, CLK_RATE_H0CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_CIM	 )),
	X2000_CLK(CLK_GATE_PDMA	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_PDMA )),
	X2000_CLK(CLK_GATE_OST	, CLK_RATE_CPUCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_OST	 )),
	X2000_CLK(CLK_GATE_SSI0	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SSI0 )),
	X2000_CLK(CLK_GATE_TCU	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_TCU	 )),
	X2000_CLK(CLK_GATE_DTRNG, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_DTRNG)),
	X2000_CLK(CLK_GATE_UART2, CLK_EXT, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_UART2)),
	X2000_CLK(CLK_GATE_UART1, CLK_EXT, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_UART1)),
	X2000_CLK(CLK_GATE_UART0, CLK_EXT, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_UART0)),
	X2000_CLK(CLK_GATE_SADC	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SADC )),
	X2000_CLK(CLK_GATE_JPEG	, CLK_RATE_H0CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_JPEG )),
	X2000_CLK(CLK_GATE_AUDIO, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_AUDIO)),
	X2000_CLK(CLK_GATE_SMB3	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SMB3 )),
	X2000_CLK(CLK_GATE_SMB2	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SMB2 )),
	X2000_CLK(CLK_GATE_SMB1	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SMB1 )),
	X2000_CLK(CLK_GATE_SMB0	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SMB0 )),
	X2000_CLK(CLK_GATE_SCC	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SCC	 )),
	X2000_CLK(CLK_GATE_MSC1	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_MSC1 )),
	X2000_CLK(CLK_GATE_MSC0	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_MSC0 )),
	X2000_CLK(CLK_GATE_OTG	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_OTG	 )),
	X2000_CLK(CLK_GATE_SFC	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SFC	 )),
	X2000_CLK(CLK_GATE_EFUSE, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_EFUSE)),
	X2000_CLK(CLK_GATE_NEMC	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_NEMC )),
	X2000_CLK(CLK_GATE_ARB	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_ARB	 )),
	X2000_CLK(CLK_GATE_MIPI	, CLK_RATE_H0CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_MIPI )),
	X2000_CLK(CLK_GATE_CPU	, CLK_RATE_CPUCLK, &ingenic_gate_ops, CLK_IGNORE_UNUSED, X2000_GATE_HWDESC(CLK_GATE_CPU	 )),
	X2000_CLK(CLK_GATE_INTC	, CLK_RATE_H2CLK, &ingenic_gate_ops, CLK_IGNORE_UNUSED, X2000_GATE_HWDESC(CLK_GATE_INTC )),
	X2000_CLK(CLK_GATE_GPIO	, CLK_RATE_PCLK, &ingenic_gate_ops, CLK_IGNORE_UNUSED, X2000_GATE_HWDESC(CLK_GATE_GPIO )),
	X2000_CLK(CLK_GATE_SPDIF, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SPDIF)),
	X2000_CLK(CLK_GATE_DMIC	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_DMIC )),
	X2000_CLK(CLK_GATE_PCM	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_PCM	 )),
	X2000_CLK(CLK_GATE_I2S3	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_I2S3 )),
	X2000_CLK(CLK_GATE_I2S2	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_I2S2 )),
	X2000_CLK(CLK_GATE_I2S1	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_I2S1 )),
	X2000_CLK(CLK_GATE_I2S0	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_I2S0 )),
	X2000_CLK(CLK_GATE_ROT	, CLK_RATE_H0CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_ROT	 )),
	X2000_CLK(CLK_GATE_HASH	, CLK_RATE_H2CLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_HASH )),
	X2000_CLK(CLK_GATE_PWM	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_PWM )),
	X2000_CLK(CLK_GATE_UART5, CLK_EXT, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_UART5)),
	X2000_CLK(CLK_GATE_UART4, CLK_EXT, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_UART4)),
	X2000_CLK(CLK_GATE_UART3, CLK_EXT, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_UART3)),
	X2000_CLK(CLK_GATE_SMB5	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SMB5 )),
	X2000_CLK(CLK_GATE_SMB4	, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_SMB4 )),
	X2000_CLK(CLK_GATE_USBPHY, CLK_RATE_PCLK, &ingenic_gate_ops, 0, X2000_GATE_HWDESC(CLK_GATE_USBPHY)),
};

static int x2000_clk_suspend(void)
{
	return 0;
}

static void x2000_clk_resume(void)
{
	return;
}

static struct syscore_ops x2000_syscore_ops = {
	.suspend = x2000_clk_suspend,
	.resume = x2000_clk_resume,
};

static void __init x2000_clk_init(struct device_node *np)
{
	struct ingenic_clk_provide *ctx = kzalloc(sizeof(struct ingenic_clk_provide), GFP_ATOMIC);

	ctx->regbase = of_io_request_and_map(np, 0, "cpm");
	if (!ctx->regbase)
		return;

	ctx->np = np;
	ctx->data.clks = (struct clk**)kzalloc(sizeof(struct clk*) * NR_CLKS, GFP_ATOMIC);
	ctx->data.clk_num = NR_CLKS;
	ctx->clk_name = clk_name;
	ctx->pm_regmap = syscon_node_to_regmap(np);
	if (IS_ERR(ctx->pm_regmap)) {
		pr_err("Cannot find regmap for %s: %ld\n", np->full_name,
				PTR_ERR(ctx->pm_regmap));
		return;
	}
	ingenic_fixclk_register(ctx, clk_name[CLK_EXT], CLK_EXT);

	ingenic_fixclk_register(ctx, clk_name[CLK_RTC_EXT], CLK_RTC_EXT);

	ingenic_clks_register(ctx, x2000_clk_init_data, ARRAY_SIZE(x2000_clk_init_data));

	register_syscore_ops(&x2000_syscore_ops);

	if (of_clk_add_provider(np, of_clk_src_onecell_get, &ctx->data))
		panic("could not register clk provider\n");

	{
		char *name[] = {"cpu", "l2cache", "ahb0", "ahb2" , "pclk"};
		unsigned long rate[] = {0, 0, 0, 0, 0};
		int i;
		struct clk *clk;
		for (i = 0; i < 5; i++)  {
			clk = clk_get(NULL, name[i]);
			if (!IS_ERR(clk))
				rate[i] = clk_get_rate(clk);
			clk_put(clk);
			rate[i] /= 1000000;
		}
		printk("CPU:[%dM]L2CACHE:[%dM]AHB0:[%dM]AHB2:[%dM]PCLK:[%dM]\n",
				(int)rate[0], (int)rate[1], (int)rate[2],
				(int)rate[3], (int)rate[4]);
	}
	return;
}
CLK_OF_DECLARE(x2000_clk, "ingenic,x2000-clocks", x2000_clk_init);
