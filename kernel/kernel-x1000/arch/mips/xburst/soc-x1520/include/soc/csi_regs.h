#ifndef _CSI_REGS_H_
#define _CSI_REGS_H_

#define MIPI_PHY_VERSION                0x00
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

#define CSI_PHY_STATE_DATA_LANE0        4
#define CSI_PHY_STATE_DATA_LANE1        5
#define CSI_PHY_STATE_DATA_LANE2        6
#define CSI_PHY_STATE_DATA_LANE3        7
#define CSI_PHY_STATE_CLK_LANE_STOP     10


#define CSI_PHY_phy_shutdown            0, 0
#define CSI_PHY_dphy_reset              0, 0
#define CSI_PHY_csi2_reset              0, 0
#define CSI_PHY_n_lanes                 0, 1
#define CSI_PHY_test_ctrl0_testclr      0, 0
#define CSI_PHY_test_ctrl0_testclk      1, 1
#define CSI_PHY_test_ctrl1_testen       16, 16

typedef enum {
    CSI_ERR_MASK_REGISTER1              = 0x1,
    CSI_ERR_MASK_REGISTER2              = 0x2,
} csi_err_mask_reg_t;

typedef enum {
    CSI_ERR_NOT_INIT                    = 0xFE,
    CSI_ERR_ALREADY_INIT                = 0xFD,
    CSI_ERR_NOT_COMPATIBLE              = 0xFC,
    CSI_ERR_UNDEFINED                   = 0xFB,
    CSI_ERR_OUT_OF_BOUND                = 0xFA,
    CSI_SUCCESS                         = 0
} csi_error_t;



#endif /* _VIC_REGS_H_ */
