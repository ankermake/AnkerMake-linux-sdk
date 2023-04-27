/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Video Interface Control register description
 *
 */

#ifndef __X2000_CIM_REGS_H__
#define __X2000_CIM_REGS_H__


#define IRQ_CIM                         30
/*
 * CIM register base address
 */
#define CIM_IOBASE                      0x13060000

/*
 * CIM controller
 */
#define CIM_GLB_CFG                     0x0000
#define CIM_CROP_SIZE                   0x0004
#define CIM_CROP_SITE                   0x0008
#define CIM_RESIZE_CFG                  0x000C
#define CIM_RESIZE_COEF_X               0x0010
#define CIM_RESIZE_COEF_Y               0x0014
#define CIM_SCAN_CFG                    0x0018
#define CIM_DLY_CFG                     0x001C
#define CIM_QOS_CTRL                    0x0020
#define CIM_QOS_CFG                     0x0024
#define CIM_DES_ADDR                    0x1000
#define CIM_CTRL                        0x2000
#define CIM_ST                          0x2004
#define CIM_CLR_ST                      0x2008
#define CIM_INTC                        0x200C
#define CIM_INT_FLAG                    0x2010
#define CIM_FRM_ID                      0x2014
#define CIM_ACT_SIZE                    0x2018

#define CIM_DBG_DES                     0x3000
#define CIM_DBG_DMA                     0x3004
#define CIM_DBG_CGC                     0x3008

#define FRM_WRBK_FMT_RGB888             (6)
#define FRM_WRBK_FMT_MONO               (4)
#define FRM_WRBK_FMT_YUV422             (3)
#define FRM_WRBK_FMT_RGB565             (1)

/* CIM Globle cfg */
#define GLB_CFG_COLOR_ORDER             20,23
#define GLB_CFG_FRM_FORMAT              16,18
#define GLB_CFG_EDGE_PCLK               15,15
#define GLB_CFG_LEVEL_VSYNC             14,14
#define GLB_CFG_LEVEL_HSYNC             13,13
#define GLB_CFG_EXPO_WIDTH              8, 10
#define GLB_CFG_SIZE_CHK                5, 5
#define GLB_CFG_DAT_IF_SEL              4, 4
#define GLB_CFG_SNAPSHOT_MODE           3, 3
#define GLB_CFG_BURST_LEN               1, 2
#define GLB_CFG_AUTO_RECOVERY           0, 0


/* CIM CTRL */
#define CTRL_SOFT_RESET                 3, 3
#define CTRL_GEN_STOP                   2, 2
#define CTRL_QUICK_STOP                 1, 1
#define CTRL_START                      0, 0

/* CIM ST */
#define ST_SRA                          7, 7    /* soft reset acknowledge */

/* Crop */
#define CROP_SIZE_HEIGHT                16,26
#define CROP_SIZE_WIDTH                 0, 10
#define CROP_SITE_Y                     16,26
#define CROP_SITE_X                     0 ,10

/* interrupt controller(中断控制) */
#define INTC_MSK_SZ_ERR                 6 ,6    /* size check error */
#define INTC_MSK_EOW                    5 ,5    /* end of normal work */
#define INTC_MSK_OVER                   4 ,4    /* over run */
#define INTC_MSK_GSA                    3 ,3    /* general stop acknowledge */
#define INTC_MSK_SOF                    2 ,2    /* start of frame */
#define INTC_MSK_EOF                    1 ,1    /* end of frame */

/* interrupt flag(中断标志只读) */
#define INT_FLAG_SZ_ERR                 6 ,6    /* size check error */
#define INT_FLAG_EOW                    5 ,5    /* end of normal work */
#define INT_FLAG_OVER                   4 ,4    /* over run */
#define INT_FLAG_GSA                    3 ,3    /* general stop acknowledge */
#define INT_FLAG_SOF                    2 ,2    /* start of frame */
#define INT_FLAG_EOF                    1 ,1    /* end of frame */

/* Clear Status */
#define CLR_ST_SOFT_RESET               7, 7    /* soft reset acknowledge */
#define CLR_ST_SIZE_ERR                 6, 6    /* size check error */
#define CLR_ST_EOW                      5, 5    /* end of normal work */
#define CLR_ST_OVER                     4, 4    /* over run */
#define CLR_ST_GSA                      3, 3    /* general stop acknowledge */
#define CLR_ST_SOF                      2, 2    /* start of frame */
#define CLR_ST_EOF                      1, 1    /* end of frame */

/* Delay Config */
#define DLY_CFG_EN                      31,31
#define DLY_CFG_MD                      30,30
#define DLY_CFG_NUM                     0, 23

enum {
    CIM_PCLK_SAMPLE_FALLING = 0,
    CIM_PCLK_SAMPLE_RISING  = 1,
} cim_pclk_sample_edge;

enum {
    CIM_VSYNC_ACTIVE_HIGH = 0,
    CIM_VSYNC_ACTIVE_LOW  = 1,
} cim_vsync_active_level;

enum {
    CIM_HSYNC_ACTIVE_HIGH = 0,
    CIM_HSYNC_ACTIVE_LOW  = 1,
} cim_hsync_active_level;

enum {
    CIM_FRAME_ORDER_RGB    = 0,
    CIM_FRAME_ORDER_RBG    = 1,
    CIM_FRAME_ORDER_GRB    = 2,
    CIM_FRAME_ORDER_GBR    = 3,
    CIM_FRAME_ORDER_BRG    = 4,
    CIM_FRAME_ORDER_BGR    = 5,
    CIM_FRAME_ORDER_YUYV   = 8,
    CIM_FRAME_ORDER_YVYU   = 9,
    CIM_FRAME_ORDER_UYVY   = 10,
    CIM_FRAME_ORDER_VYUY   = 11,
} cim_frame_color_order;

enum {
    CIM_FRAME_FMT_RGB565    = 0,
    CIM_FRAME_FMT_RGB888    = 1,
    CIM_FRAME_FMT_YUV422    = 2,
    CIM_FRAME_FMT_ITU656    = 3,
    CIM_FRAME_FMT_MONO      = 4,
} cim_frame_format;

enum {
    CIM_DMA_BURST_LEN_4     = 0,
    CIM_DMA_BURST_LEN_8     = 1,
    CIM_DMA_BURST_LEN_16    = 2,
    CIM_DMA_BURST_LEN_32    = 3,
} cim_dma_burst_len;

#endif /* __X2000_CIM_REGS_H__ */
