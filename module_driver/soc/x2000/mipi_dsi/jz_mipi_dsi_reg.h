#ifndef _X2000_JZ_MIPI_DSI_REG_H
#define _X2000_JZ_MIPI_DSI_REG_H

#define R_DSI_HOST_VERSION                0x000
#define R_DSI_HOST_PWR_UP                 0x004
#define R_DSI_HOST_CLKMGR_CFG             0x008
#define R_DSI_HOST_DPI_VCID               0x00c
#define R_DSI_HOST_DPI_COLOR_CODING       0x010
#define R_DSI_HOST_DPI_CFG_POL            0x014
#define R_DSI_HOST_DPI_LP_CMD_TIM         0x018
#define R_DSI_HOST_DBI_VCID               0x01c
#define R_DSI_HOST_DBI_CFG                0x020
#define R_DSI_HOST_DBI_PARTITIONING_EN    0x024
#define R_DSI_HOST_DBI_CMDSIZE            0x028
#define R_DSI_HOST_PCKHDL_CFG             0x02c
#define R_DSI_HOST_GEN_VCID               0x030
#define R_DSI_HOST_MODE_CFG               0x034
#define R_DSI_HOST_VID_MODE_CFG           0x038
#define R_DSI_HOST_VID_PKT_SIZE           0x03c
#define R_DSI_HOST_VID_NUM_CHUNKS         0x040
#define R_DSI_HOST_VID_NULL_SIZE          0x044
#define R_DSI_HOST_VID_HSA_TIME           0x048
#define R_DSI_HOST_VID_HBP_TIME           0x04c
#define R_DSI_HOST_VID_HLINE_TIME         0x050
#define R_DSI_HOST_VID_VSA_LINES          0x054
#define R_DSI_HOST_VID_VBP_LINES          0x058
#define R_DSI_HOST_VID_VFP_LINES          0x05c
#define R_DSI_HOST_VID_VACTIVE_LINES      0x060
#define R_DSI_HOST_EDPI_CMD_SIZE          0x064
#define R_DSI_HOST_CMD_MODE_CFG           0x068
#define R_DSI_HOST_GEN_HDR                0x06c
#define R_DSI_HOST_GEN_PLD_DATA           0x070
#define R_DSI_HOST_CMD_PKT_STATUS         0x074
#define R_DSI_HOST_TO_CNT_CFG             0x078
#define R_DSI_HOST_HS_RD_TO_CNT           0x07c
#define R_DSI_HOST_LP_RD_TO_CNT           0x080
#define R_DSI_HOST_HS_WR_TO_CNT           0x084
#define R_DSI_HOST_LP_WR_TO_CNT           0x088
#define R_DSI_HOST_BTA_TO_CNT             0x08c
#define R_DSI_HOST_SDF_3D                 0x090
#define R_DSI_HOST_LPCLK_CTRL             0x094
#define R_DSI_HOST_PHY_TMR_LPCLK_CFG      0x098
#define R_DSI_HOST_PHY_TMR_CFG            0x09c
#define R_DSI_HOST_PHY_RSTZ               0x0a0
#define R_DSI_HOST_PHY_IF_CFG             0x0a4
#define R_DSI_HOST_PHY_ULPS_CTRL          0x0a8
#define R_DSI_HOST_PHY_TX_TRIGGERS        0x0ac
#define R_DSI_HOST_PHY_STATUS             0x0b0
#define R_DSI_HOST_PHY_TST_CTRL0          0x0b4
#define R_DSI_HOST_PHY_TST_CTRL1          0x0b8
#define R_DSI_HOST_INT_ST0                0x0bC
#define R_DSI_HOST_INT_ST1                0x0C0
#define R_DSI_HOST_INT_MSK0               0x0C4
#define R_DSI_HOST_INT_MSK1               0x0C8
#define R_DSI_HOST_INT_FORCE0             0x0D8
#define R_DSI_HOST_INT_FORCE1             0x0DC
#define R_DSI_HOST_VID_SHADOW_CTRL        0x100
#define R_DSI_HOST_DPI_VCID_ACT           0x10C
#define R_DSI_HOST_DPI_COLOR_CODING_ACT   0x110
#define R_DSI_HOST_DPI_LP_CMD_TIM_ACT     0x118
#define R_DSI_HOST_VID_MODE_CFG_ACT       0x138
#define R_DSI_HOST_VID_PKT_SIZE_ACT       0x13C
#define R_DSI_HOST_VID_NUM_CHUNKS_ACT     0x140
#define R_DSI_HOST_VID_NULL_SIZE_ACT      0x144
#define R_DSI_HOST_VID_HSA_TIME_ACT       0x148
#define R_DSI_HOST_VID_HBP_TIME_ACT       0x14C
#define R_DSI_HOST_VID_HLINE_TIME_ACT     0x150
#define R_DSI_HOST_VID_VSA_LINES_ACT      0x154
#define R_DSI_HOST_VID_VBP_LINES_ACT      0x158
#define R_DSI_HOST_VID_VFP_LINES_ACT      0x15C
#define R_DSI_HOST_VID_VACTIVE_LINES_ACT  0x160
#define R_DSI_HOST_SDF_3D_ACT             0x190


#define VERSION                           0, 31

#define SHUTDOWNZ                         0, 0

#define TX_ESC_CLK_DIV                    0, 7
#define TO_CLK_DIV                        8, 15

#define DPI_VCID                          0, 1

#define DIP_COLOR_CODING                  0, 3
#define LOOSELY18_EN                      8, 8

#define DATAEN_ACTIVE_LOW                 0, 0
#define VSYNC_ACTIVE_LOW                  1, 1
#define HSYNC_ACTIVE_LOW                  2, 2
#define SHUTD_ACTIVE_LOW                  3, 3
#define COLORM_ACTIVE_LOW                 4, 4

#define INVACT_LPCMD_TIME                 0, 7
#define OUTVACT_LPCMD_TIME                16, 23

#define ETOP_TX_EN                        0, 0
#define ETOP_RX_EN                        1, 1
#define BAT_EN                            2, 2
#define ECC_RX_EN                         3, 3
#define CRC_RX_EN                         4, 4

#define GEN_VICD_RX                       0, 1

#define CMD_VIDEO_MODE                    0, 0

#define VID_MODE_TYPE                     0, 1
#define LP_VSA_EN                         8, 8
#define LP_VBP_EN                         9, 9
#define LP_VFP_EN                         10, 10
#define LP_VACT_EN                        11, 11
#define LP_HBP_EN                         12, 12
#define LP_HFP_EN                         13, 13
#define FRAME_BTA_ACK_EN                  14, 14
#define LP_CMD_EN                         15, 15
#define VPG_EN                            16, 16
#define VPG_MODE                          20, 20
#define VPG_ORIENTATION                   24, 24

#define VID_PKT_SIZE                      0, 13

#define VID_NUM_CHUNKS                    0, 12

#define VID_NULL_SIZE                     0, 12

#define VID_HSA_TIME                      0, 11

#define VID_HBP_TIME                      0, 11

#define VID_HLINE_TIME                    0, 14

#define VSA_LINES                         0, 9

#define VBP_LINES                         0, 9

#define VFP_LINES                         0, 9

#define V_ACTIVE_LINES                    0, 13

#define EDPI_ALLOWED_CMD_SIZE             0, 15

#define TEAR_FX_EN                        0, 0
#define ACK_RQST_EN                       1, 1
#define GEN_SW_0P_TX                      8, 8
#define GEN_SW_1P_TX                      9, 9
#define GEN_SW_2P_TX                      10, 10
#define GEN_SR_0P_TX                      11, 11
#define GEN_SR_1P_TX                      12, 12
#define GEN_SR_2P_TX                      13, 13
#define GEN_LW_TX                         14, 14
#define DCS_SW_0P_TX                      16, 16
#define DCS_SW_1P_TX                      17, 17
#define DCS_SR_0P_TX                      18, 18
#define DCS_LW_TX                         19, 19
#define MAX_RD_PKT_SIZE                   24, 24

#define GEN_DT                            0, 5
#define GEN_VC                            6, 7
#define GEN_WC_LSBYTE                     8, 15
#define GEN_WC_MSBYTE                     16, 23

#define GEN_PLD_B1                        0, 7
#define GEN_PLD_B2                        8, 15
#define GEN_PLD_B3                        16, 23
#define GEN_PLD_B4                        24, 31

#define GEN_CMD_EMPTY                     0, 0
#define GEN_CMD_FULL                      1, 1
#define GEN_CMD_PLD_WEMPTY                2, 2
#define GEN_PLD_W_FUL                     3, 3
#define GEN_PLD_R_EMPTY                   4, 4
#define GEN_PLD_R_FULL                    5, 5
#define GEN_RD_CMD_BUSY                   6, 6

#define LPRX_TO_CNT                       0, 15
#define HSTX_TO_CNT                       16, 31

#define HS_RD_TO_CNT                      0, 15

#define LP_RD_TO_CNT                      0, 0

#define HS_WR_TO_CNT                      0, 15
#define PRESP_TO_MODE                     24, 24

#define LP_WR_TO_CNT                      0, 15

#define BAT_TO_CNT                        0, 15

#define MODE_3D                           0, 1
#define FORMAT_3D                         2, 3
#define SECOND_VSYNC                      4, 4
#define RIGHT_FIRST                       5, 5
#define SEND_3D_CFG                       16, 16

#define PHY_TXREQUESTCLKHS                0, 0
#define AUTO_CLKLANE_CTRL                 1, 1

#define PHY_CLKLP2HS_TIME                 0, 9
#define PHY_CLKHS2PLP_TIME                16, 25

#define MAX_RD_TIME                       0, 14
#define PHY_LP2HS_TIME                    16, 23
#define PHY_HS2LP_TIME                    24, 31

#define PHY_SHUTDOWMZ                     0, 0
#define PHY_RSTZ                          1, 1
#define PHY_ENABLECLK                     2, 2
#define PHY_FORCEPLL                      3, 3

#define N_LANES                           0, 1
#define PHY_STOP_WAIT_TIME                8, 15

#define PHY_TXREQULPSCLK                  0, 0
#define PHY_TXEXITULPSCLK                 1, 1
#define PHY_TXREQULPSLAN                  2, 2
#define PHY_TXEXITULPSLAN                 3, 3

#define PHY_TX_TRIGGERS                   0, 3

#define PHY_LOCK                          0, 0
#define PHY_DIRECTION                     1, 1
#define PHY_STOPSTATECLKLANE              2, 2
#define PHY_ULPSACTIVENOTCLK              3, 3
#define PHY_STOPSTATE0LANE                4, 4
#define PHY_ULPSACTIVENOT0LANE              5, 5
#define PHY_RXULPSESC0LANE                6, 6
#define PHY_STOPSTATE1LANE                7, 7
#define PHY_ULPSACTIVENOT1LANE            8, 8

#define PHY_TESTCLR                       0, 0
#define PHY_TESTCLK                       1, 1

#define PHY_TESTDIN                       0, 7
#define PHT_TESTDOUT                      8, 15
#define PHY_TESTEN                        16, 16

#define ACK_WITH_ERR_0                    0, 0
#define ACK_WITH_ERR_1                    1, 1
#define ACK_WITH_ERR_2                    2, 2
#define ACK_WITH_ERR_3                    3, 3
#define ACK_WITH_ERR_4                    4, 4
#define ACK_WITH_ERR_5                    5, 5
#define ACK_WITH_ERR_6                    6, 6
#define ACK_WITH_ERR_7                    7, 7
#define ACK_WITH_ERR_8                    8, 8
#define ACK_WITH_ERR_9                    9, 9
#define ACK_WITH_ERR_10                   10, 10
#define ACK_WITH_ERR_11                   11, 11
#define ACK_WITH_ERR_12                   12, 12
#define ACK_WITH_ERR_13                   13, 13
#define ACK_WITH_ERR_14                   14, 14
#define ACK_WITH_ERR_15                   15, 15
#define DPHY_ERRORS_0                     16, 16
#define DPHY_ERRORS_1                     17, 17
#define DPHY_ERRORS_2                     18, 18
#define DPHY_ERRORS_3                     19, 19
#define DPHY_ERRORS_4                     20, 20

#define TO_HS_TX                          0, 0
#define TO_LP_RX                          1, 1
#define ECC_SINGLE_ERR                    2, 2
#define ECC_MULTI_ERR                     3, 3
#define CRC_ERR                           4, 4
#define PKR_SIZE_ERR                      5, 5
#define EOPT_ERR                          6, 6
#define DPI_PID_WR_ERR                    7, 7
#define GEN_CMD_WR_ERR                    8, 8
#define GEN_PLD_WR_ERR                    9, 9
#define GEN_PLD_SEND_WRR                  10, 10
#define GEN_PLD_RD_ERR                    11, 11
#define GEN_PLD_RECEV_ERR                 12, 12

#define VID_SHADOW_EN                     0, 0
#define VID_SHADOW_REQ                    8, 8
#define VID_SHADOW_PIN_REQ                16, 16

#define DPI_VCID                          0, 1
#endif