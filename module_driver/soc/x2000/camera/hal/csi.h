/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Ingenic MIPI-CSI
 *
 */
#ifndef __X2000_MIPI_CSI_H__
#define __X2000_MIPI_CSI_H__

#include "camera_sensor.h"
#include "dsys.h"


#define MIPI_CSI0_IOBASE                0x10074000
#define MIPI_CSI1_IOBASE                0x10073000

#define MIPI_PHY_IOBASE                 0x10076000


/*
 * MIPI CSI controller
 */
#define MIPI_VERSION                    0x00
#define MIPI_N_LANES                    0x04
#define MIPI_PHY_SHUTDOWNZ              0x08
#define MIPI_DPHY_RSTZ                  0x0C
#define MIPI_CSI2_RESETN                0x10
#define MIPI_PHY_STATE                  0x14
#define MIPI_DATA_IDS_1                 0x18
#define MIPI_DATA_IDS_2                 0x1C
#define MIPI_ERR1                       0x20
#define MIPI_ERR2                       0x24
#define MIPI_MASK1                      0x28
#define MIPI_MASK2                      0x2C
#define MIPI_PHY_TST_CTRL0              0x30
#define MIPI_PHY_TST_CTRL1              0x34
#define MIPI_DEBUG                      0x40

#define MIPI_VC0_FRAME_NUM              0x40    /* [31:16] FS End,   [15:0] FS Start */
#define MIPI_VC1_FRAME_NUM              0x44
#define MIPI_VC2_FRAME_NUM              0x48
#define MIPI_VC3_FRAME_NUM              0x4c
#define MIPI_VC0_LINE_NUM               0x50    /* [31:16] Line End, [15:0] Line Start */
#define MIPI_VC1_LINE_NUM               0x54
#define MIPI_VC2_LINE_NUM               0x58
#define MIPI_VC3_LINE_NUM               0x60

/*
 * MIPI PHY  controller
 */
#define MIPI_PHY_PRECOUNTER_IN_CLK      0xB8    /* Tclk-settle */
#define MIPI_PHY_PRECOUNTER_IN_DATA     0xBC    /* Ths-settle */
#define MIPI_PHY_1C2C_MODE              0xc4


#define MIPI_PHY_STATE_DATA_LANE0       4
#define MIPI_PHY_STATE_DATA_LANE1       5
#define MIPI_PHY_STATE_DATA_LANE2       6
#define MIPI_PHY_STATE_DATA_LANE3       7
#define MIPI_PHY_STATE_CLK_LANE_STOP    10

#define MIPI_PHY_phy_shutdown           0, 0
#define MIPI_PHY_dphy_reset             0, 0
#define MIPI_PHY_csi2_reset             0, 0
#define MIPI_PHY_n_lanes                0, 1
#define MIPI_PHY_test_ctrl0_testclr     0, 0
#define MIPI_PHY_test_ctrl0_testclk     1, 1
#define MIPI_PHY_test_ctrl1_testen      16, 16

#define MIPI_PHY_precounter_clk0        0, 7   /* Tclk-settle */
#define MIPI_PHY_precounter_clk1        8, 15  /* Tclk-settle */
#define MIPI_PHY_precounter_data0       0, 7   /* Ths-settle */
#define MIPI_PHY_precounter_data1       8, 15  /* Ths-settle */
#define MIPI_PHY_precounter_data2       16,23  /* Ths-settle */
#define MIPI_PHY_precounter_data3       24,31  /* Ths-settle */

typedef enum {
    CSI_ERR_MASK_REGISTER1              = 0x1,
    CSI_ERR_MASK_REGISTER2              = 0x2,
} csi_err_mask_reg_t;


int mipi_csi_phy_stop(int index);
int mipi_csi_phy_initialization(int index, struct mipi_csi_bus *mipi_info);
#ifdef SOC_CAMERA_DEBUG
int dsysfs_mipi_dump_reg(int index, char *buf);
#endif

#endif /* __X2000_MIPI_CSI_H__ */
