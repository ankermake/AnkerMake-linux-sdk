#ifndef DDR_H
#define DDR_H

#define PHY_BASE						(0x13011000)
#define APB_BASE						(0x13012000)
#define AHB_BASE						(0x134f0000)

/**********          PHY ADDR  ********************/
#define INNO_CHANNEL_EN		0x0
#define INNO_MEM_CFG		0x04
#define INNO_TRAINING_CTRL	0x08
#define INNO_WL_MODE1		0x0C
#define INNO_WL_MODE2		0x10
#define INNO_CL			0x14
#define INNO_AL			0x18
#define INNO_CWL		0x1C
#define INNO_DQ_WIDTH		0x7C

#define INNO_PLL_FBDIV		0x80
#define INNO_PLL_CTRL		0x84
#define INNO_PLL_PDIV		0x88

#define INNO_WL_DONE		0xC0
#define INNO_PLL_LOCK		0xC8
#define INNO_CALIB_DONE		0xCC

#define INNO_INIT_COMP		0xD0

#define DDRC_STATUS		0x0
#define DDRC_CFG		0x4
#define DDRC_CTRL		0x8
#define DDRC_LMR		0xc
#define DDRC_TIMING(n)	(0x60 + 4 * (n - 1))
#define DDRC_REFCNT		0x18
#define DDRC_MMAP0		0x24
#define DDRC_MMAP1		0x28
#define DDRC_DLP		0xbc
#define DDRC_REMAP(n)	(0x9c + 4 * (n - 1))
#define DDRC_STRB		0x34
#define DDRC_WCMDCTRL1	0x100
#define DDRC_RCMDCTRL0	0x104
#define DDRC_RCMDCTRL1	0x108
#define DDRC_WDATTHD0	0x114
#define DDRC_WDATTHD1	0x118
#define DDRC_IPORTPRI	0x128
#define DDRC_IPORTWPRI	0x240
#define DDRC_IPORTRPRI	0x244

#define DDRC_AUTOSR_CNT             0x308
#define DDRC_AUTOSR_EN		        0x304


#define DDRC_APB_PHY_INIT	(0x8c)


#define ddr_phy_writel(value, reg)  outl((value), PHY_BASE+reg)
#define ddr_phy_readl(reg)          inl(PHY_BASE+reg)

#define ddr_ahb_writel(value, reg)  outl((value), AHB_BASE + (reg))
#define ddr_ahb_readl(reg)          inl(AHB_BASE + (reg))

#define ddr_apb_writel(value, reg)  outl((value), APB_BASE + (reg))
#define ddr_apb_readl(reg)          inl(APB_BASE + (reg))
#endif /* DDR_H */
