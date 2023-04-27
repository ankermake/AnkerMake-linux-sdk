#ifndef _ROTATOR_REG_H_
#define _ROTATOR_REG_H_

#define ROTATOR_FRM_CFG_ADDR    0x0000
#define ROTATOR_FRM_SIZE        0x0004
#define ROTATOR_GLB_CFG         0x0008
#define ROTATOR_CTRL            0x000c
#define ROTATOR_STATUS          0x0010
#define ROTATOR_CLR_STATUS      0x0014
#define ROTATOR_INT_MASK        0x0018
#define ROTATOR_RDMA_SITE       0x0020
#define ROTATOR_WDMA_SITE       0x0024
#define ROTATOR_DS_FRM_DES      0x0028
#define ROTATOR_QOS_CTRL        0x0030
#define ROTATOR_QOS_CFG         0x0034

#define f_SrcStride     0, 10
#define f_FrameStop     0, 0
#define f_TargetStride  0, 10
#define f_SOF_MASK      3, 3
#define f_EOF_MASK      2, 2

#define ROTATORFRMSIZE_Reserved     27, 31
#define ROTATORFRMSIZE_FRAM_HEIGHT  16, 26
#define ROTATORFRMSIZE_Reserved1    11, 15
#define ROTATORFRMSIZE_FRAM_WIDTH   0,  10

#define ROTATORGLBCFG_Reserved          31, 31
#define ROTATORGLBCFG_DS_ROT_EN         30, 30
#define ROTATORGLBCFG_DS_RDMA_EN        29, 29
#define ROTATORGLBCFG_DS_WDMA_EN        28, 28
#define ROTATORGLBCFG_RDMA_FMT          24, 27
#define ROTATORGLBCFG_Reserved1         19, 23
#define ROTATORGLBCFG_RDMA_ORDER        16, 18
#define ROTATORGLBCFG_Reserved2         15, 15
#define ROTATORGLBCFG_WDMA_FMT          12, 14
#define ROTATORGLBCFG_Reserved3         8,  11
#define ROTATORGLBCFG_HORIZONTAL_MIRROR 7,  7
#define ROTATORGLBCFG_VERTICAL_MIRROR   6,  6
#define ROTATORGLBCFG_ROT_ANGLE         4,  5
#define ROTATORGLBCFG_WDMA_BURST_LEN    2,  3
#define ROTATORGLBCFG_RDMA_BURST_LEN    0,  1

#define ROTATORCTRL_Reserved    4, 31
#define ROTATORCTRL_DES_CNT_RST 3, 3
#define ROTATORCTRL_GEN_STOP    2, 2
#define ROTATORCTRL_QUICK_STOP  1, 1
#define ROTATORCTRL_START       0, 0

#define ROTATORSTATUS_Reserved      20, 31
#define ROTATORSTATUS_SOF_MASK_ST   19, 19
#define ROTATORSTATUS_EOF_MASK_ST   18, 18
#define ROTATORSTATUS_GSA_MASK_ST   17, 17
#define ROTATORSTATUS_Reserved1     4,  16
#define ROTATORSTATUS_SOF           3,  3
#define ROTATORSTATUS_EOF           2,  2
#define ROTATORSTATUS_GEN_STOP_ACK  1,  1
#define ROTATORSTATUS_WORKING       0,  0

#define ROTATORCLRSTATUS_Reserved           4, 31
#define ROTATORCLRSTATUS_CLR_SOF            3, 3
#define ROTATORCLRSTATUS_CLR_EOF            2, 2
#define ROTATORCLRSTATUS_CLR_GEN_STP_ACK    1, 1
#define ROTATORCLRSTATUS_Reserved1          0, 0

#define ROTATORINTMASK_Reserved     4, 31
#define ROTATORINTMASK_SOF_MASK     3, 3
#define ROTATORINTMASK_EOF_MASK     2, 2
#define ROTATORINTMASK_GSA_MASK     1, 1
#define ROTATORINTMASK_Reserved1    0, 0

#define ROTATORQOSCTRL_Reserved     3, 31
#define ROTATORQOSCTRL_FIX          2, 1
#define ROTATORQOSCTRL_FIX_EN       0, 0

#define ROTATORQOSCFG_STD_THR2      24, 31
#define ROTATORQOSCFG_STD_THR1      16, 23
#define ROTATORQOSCFG_STD_THR0      8,  15
#define ROTATORQOSCFG_STD_CLK       0,  7

#endif
