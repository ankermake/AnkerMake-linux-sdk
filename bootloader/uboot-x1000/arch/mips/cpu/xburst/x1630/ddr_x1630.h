#ifndef X1630_PHY
#define X1630_PHY
#include <asm/io.h>

#define PHY_BASE						(0xb3011000)
#define APB_BASE						(0xb3012000)
#define AHB_BASE						(0xb34f0000)

/**********          PHY ADDR  ********************/
#define INNO_CHANNEL_EN 0x0
#define INNO_MEM_CFG    0x04
#define INNO_TRAINING_CTRL 0x08
#define INNO_WL_MODE1   0x0C
#define INNO_WL_MODE2   0x10
#define INNO_CL         0x14
#define INNO_AL         0x18

#define INNO_CWL        0x1C
#define INNO_DQ_WIDTH   0x7C

#define INNO_PLL_FBDIV  0x80
#define INNO_PLL_CTRL   0x84
#define INNO_PLL_PDIV   0x88

#define INNO_WL_DONE    0xC0
#define INNO_CALIB_DONE 0xCC

#define INNO_INIT_COMP  0xD0

#define X1630_CHANNEL_EN	(PHY_BASE + 0x0)
#define X1630_MEM_CFG           (PHY_BASE + 0x04)
#define X1630_TRANING_CTRL      (PHY_BASE + 0x08)
#define X1630_WRITE_LEVEL_MODE1 (PHY_BASE + 0x0C)
#define X1630_WRITE_LEVEL_MODE2 (PHY_BASE + 0x10)
#define X1630_CL                (PHY_BASE + 0x14)
#define X1630_AL                (PHY_BASE + 0x18)

#define X1630_CWL		(PHY_BASE + 0x1C)
#define X1630_DQ_WIDTH          (PHY_BASE + 0x7C)

#define X1630_PLL_FBDIV         (PHY_BASE + 0x80)
#define X1630_PLL_CTRL          (PHY_BASE + 0x84)
#define X1630_PLL_PDIV          (PHY_BASE + 0x88)

#define X1630_WL_DONE           (PHY_BASE + 0xc0)
#define X1630_DDR_PLL_LOCK      (PHY_BASE + 0xc8)
#define X1630_CALIB_DONE        (PHY_BASE + 0xcc)

#define X1630_INIT_COMP         (PHY_BASE + 0xd0)
/**************        PHY ADDR  END    ****************/

/**************        PHY 附加寄存器   ****************/
#define X1630_DQS_DELAY_L	(PHY_BASE + 0x120)
#define X1630_DQS_DELAY_H	(PHY_BASE + 0x160)
#define X1630_REG46		(PHY_BASE + 0x118)
#define X1630_REG56		(PHY_BASE + 0x158)
#define DDR_APB_PHY_INIT	(APB_BASE + 0x8c)
#define REG_DDR_CTRL		(AHB_BASE + 0x008)
#define REG_DDR_CFG		(AHB_BASE + 0x004)
#define REG_DDR_LMR		(AHB_BASE + 0x00c)
/*************       PHY END         *******************/

/***********     PHY REGISTER  BEGIN     ****************/
#define PHY_CHANNEL_EN          (*(volatile unsigned int *)X1630_CHANNEL_EN       )
#define PHY_RST                 (*(volatile unsigned int *)X1630_PHY_RST          )
#define PHY_MEM_CFG             (*(volatile unsigned int *)X1630_MEM_SEL          )
#define PHY_TRANING_CTRL        (*(volatile unsigned int *)X1630_TRANING_CTRL     )
#define PHY_WL_MODE1            (*(volatile unsigned int *)X1630_WRITE_LEVEL_MODE1)
#define PHY_WL_MODE2            (*(volatile unsigned int *)X1630_WRITE_LEVEL_MODE2)

#define PHY_CL                  (*(volatile unsigned int *)X1630_CL               )
#define PHY_AL                  (*(volatile unsigned int *)X1630_AL               )
#define PHY_CWL                 (*(volatile unsigned int *)X1630_CWL             )

#define PHY_DQ_WIDTH            (*(volatile unsigned int *)X1630_DQ_WIDTH         )
#define PHY_PLL_FBDIV           (*(volatile unsigned int *)X1630_PLL_FBDIV        )
#define PHY_PLL_CTRL            (*(volatile unsigned int *)X1630_PLL_CTRL         )
#define PHY_PLL_PDIV            (*(volatile unsigned int *)X1630_PLL_PDIV         )

#define PHY_WL_DONE             (*(volatile unsigned int *)X1630_WL_DONE          )
#define PHY_CALIB_DONE          (*(volatile unsigned int *)X1630_CALIB_DONE   )

#define PHY_INIT_COMP           (*(volatile unsigned int *)X1630_INIT_COMP   )
/*********** PHY REGISTER  END          ****************/
#define phy_writel(value, reg)  writel((value), PHY_BASE+reg)
#define phy_readl(reg)          readl(PHY_BASE+reg)
#endif

