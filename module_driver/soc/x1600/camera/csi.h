/*
 * Copyright (C) 2021 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Ingenic MIPI-CSI
 *
 */
#ifndef __X1600_MIPI_CSI_H__
#define __X1600_MIPI_CSI_H__

#include <soc/base.h>
#include "camera_sensor.h"

/*
 * MIPI CSI controller
 */
#define MIPI_VERSION                    0x000
#define MIPI_N_LANES                    0x004
#define MIPI_PHY_SHUTDOWNZ              0x008
#define MIPI_DPHY_RSTZ                  0x00C
#define MIPI_CSI2_RESETN                0x010
#define MIPI_PHY_STATE                  0x014
#define MIPI_DATA_IDS_1                 0x018
#define MIPI_DATA_IDS_2                 0x01C
#define MIPI_ERR1                       0x020
#define MIPI_ERR2                       0x024
#define MIPI_MASK1                      0x028
#define MIPI_MASK2                      0x02C
#define MIPI_PHY_TST_CTRL0              0x030
#define MIPI_PHY_TST_CTRL1              0x034
#define MIPI_DEBUG                      0x040


/*
 * MIPI PHY controller(inno D-PHY)
 */
#define MIPI_PHY_RXPHY_REG_0_00         0x000
#define MIPI_PHY_RXPHY_REG_2_0A         0x128
#define MIPI_PHY_RXPHY_REG_2_18         0x160    /* clk Ths-settle */
#define MIPI_PHY_RXPHY_REG_3_18         0x1E0    /* data lane0 Ths-settle */
#define MIPI_PHY_RXPHY_REG_4_18         0x260    /* data lane1 Ths-settle */



/* MIPI_PHY_STATE */
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

/* MIPI_PHY_RXPHY_REG_0_00 */
#define MIPI_PHY_RXPHY_power_enable         0, 1
#define MIPI_PHY_RXPHY_data_lane0_enable    2, 2
#define MIPI_PHY_RXPHY_data_lane1_enable    3, 3
#define MIPI_PHY_RXPHY_clk_lane_enable      6, 6

/* MIPI_PHY_RXPHY_REG_2_0A */
#define MIPI_PHY_RXPHY_clk_continue_mode    4, 3

#define MIPI_PHY_RXPHY_clk_settle_time      0, 7   /* Tclk-settle */
#define MIPI_PHY_RXPHY_data0_settle_time    0, 7   /* Ths-settle */
#define MIPI_PHY_RXPHY_data1_settle_time    0, 7   /* Ths-settle */


typedef enum {
    CSI_ERR_MASK_REGISTER1              = 0x1,
    CSI_ERR_MASK_REGISTER2              = 0x2,
} csi_err_mask_reg_t;

typedef enum {
    CSI_LANES_FREQ_80_110MHz            = 0x02,
    CSI_LANES_FREQ_110_150MHz           = 0x03,
    CSI_LANES_FREQ_150_300MHz           = 0x06,
    CSI_LANES_FREQ_300_400MHz           = 0x08,
    CSI_LANES_FREQ_400_500MHz           = 0x0B,
    CSI_LANES_FREQ_500_600MHz           = 0x0E,
    CSI_LANES_FREQ_600_700MHz           = 0x10,
    CSI_LANES_FREQ_700_800MHz           = 0x12,
    CSI_LANES_FREQ_800_1000MHz          = 0x16,
    CSI_LANES_FREQ_1000_1200MHz         = 0x1E,
    CSI_LANES_FREQ_1200_1400MHz         = 0x23,
    CSI_LANES_FREQ_1400_1600MHz         = 0x2D,
    CSI_LANES_FREQ_1600_1800MHz         = 0x32,
    CSI_LANES_FREQ_1800_2000MHz         = 0x37,
    CSI_LANES_FREQ_2000_2200MHz         = 0x3C,
    CSI_LANES_FREQ_2200_2400MHz         = 0x41,
    CSI_LANES_FREQ_2400_2500MHz         = 0x46,
} dphy_receiver_lanes_freq;

int mipi_csi_phy_stop(struct mipi_csi_bus *mipi_info);
int mipi_csi_phy_initialization(struct mipi_csi_bus *mipi_info);


#endif /* __X1600_MIPI_CSI_H__ */
