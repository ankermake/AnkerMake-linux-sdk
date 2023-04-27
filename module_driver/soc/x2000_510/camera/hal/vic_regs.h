/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Video Interface Control register description
 *
 */

#ifndef __X2000_VIC_REGS_H__
#define __X2000_VIC_REGS_H__

#define IRQ_VIC0                        (19)
#define IRQ_VIC1                        (18)
/*
 * VIC register base address
 */
#define VIC0_IOBASE                     0x13710000
#define VIC1_IOBASE                     0x13810000

/*
 * VIC controller
 */
#define VIC_CONTROL                     0x00
#define VIC_RESOLUTION                  0x04
#define VIC_FRM_ECC                     0x08
#define VIC_INPUT_INTF                  0x0C
#define VIC_INPUT_DVP                   0x10
#define VIC_INPUT_MIPI                  0x14
#define VIC_IN_HOR_PARA0                0x18
#define VIC_IN_HOR_PARA1                0x1C
#define VIC_BK_CB_CTRL                  0x28
#define VIC_BK_CB_BLK                   0x2C
#define VIC_INPUT_VPARA0                0x30
#define VIC_INPUT_VPARA1                0x34
#define VIC_INPUT_VPARA2                0x38
#define VIC_INPUT_VPARA3                0x3C
#define VIC_VLD_LINE_SAV                0x60
#define VIC_VLD_LINE_EAV                0x64
#define VIC_VLD_FRM_SAV                 0x70
#define VIC_VLD_FRM_EAV                 0x74
#define VIC_VC_CONTROL_FSM              0x8C
#define VIC_VC_CONTROL_CH0_PIX          0x90
#define VIC_VC_CONTROL_CH1_PIX          0x94
#define VIC_VC_CONTROL_CH2_PIX          0x98
#define VIC_VC_CONTROL_CH3_PIX          0x9C
#define VIC_VC_CONTROL_CH0_LINE         0xA0
#define VIC_VC_CONTROL_CH1_LINE         0xA4
#define VIC_VC_CONTROL_CH2_LINE         0xA8
#define VIC_VC_CONTROL_CH3_LINE         0xAC
#define VIC_VC_CONTROL_FIFO_USE         0xB0
#define VIC_CB_1ST                      0xC0
#define VIC_CB_2ND                      0xC4
#define VIC_CB_3RD                      0xC8
#define VIC_CB_4TH                      0xCC
#define VIC_CB_5TH                      0xD0
#define VIC_CB_6TH                      0xD4
#define VIC_CB_7TH                      0xD8
#define VIC_CB_8TH                      0xDC
#define VIC_CB2_1ST                     0xE0
#define VIC_CB2_2ND                     0xE4
#define VIC_CB2_3RD                     0xE8
#define VIC_CB2_4TH                     0xEC
#define VIC_CB2_5TH                     0xF0
#define VIC_CB2_6TH                     0xF4
#define VIC_CB2_7TH                     0xF8
#define VIC_CB2_8TH                     0xFC

#define MIPI_ALL_WIDTH_4BYTE            0x100
#define MIPI_VCROP_DEL01                0x104
#define MIPI_SENSOR_CONTROL             0x10C
#define MIPI_HCROP_CH0                  0x110
#define MIPI_VCROP_SHADOW_CFG           0x120
#define VIC_CONTROL_LIMIT               0x1A0
#define VIC_CONTROL_DELAY               0x1A4
#define VIC_CONTROL_TIZIANO_ROUTE       0x1A8
#define VIC_CONTROL_DMA_ROUTE           0x1B0
#define VIC_INT_STA                     0x1E0
#define VIC_INT_MASK                    0x1E8
#define VIC_INT_CLR                     0x1F0

#define VIC_DMA_CONFIGURE               0x300
#define VIC_DMA_RESOLUTION              0x304
#define VIC_DMA_RESET                   0x308
#define VIC_DMA_Y_CH_LINE_STRIDE        0x314
#define VIC_DMA_Y_CH_BUF0_ADDR          0x318
#define VIC_DMA_Y_CH_BUF1_ADDR          0x31C
#define VIC_DMA_Y_CH_BUF2_ADDR          0x320
#define VIC_DMA_Y_CH_BUF3_ADDR          0x324
#define VIC_DMA_Y_CH_BUF4_ADDR          0x328
#define VIC_DMA_UV_CH_LINE_STRIDE       0x334
#define VIC_DMA_UV_CH_BUF0_ADDR         0x338
#define VIC_DMA_UV_CH_BUF1_ADDR         0x33C
#define VIC_DMA_UV_CH_BUF2_ADDR         0x340
#define VIC_DMA_UV_CH_BUF3_ADDR         0x344
#define VIC_DMA_UV_CH_BUF4_ADDR         0x348

/*TIMESTAMP registers*/
#define VIC_TS_ENABLE                   0x360   /* 控制使能*/
    #define TS_COUNTER_EN               (1 << 0)/*使能计数器*/
    #define TS_VIC_DMA_EN               (1 << 4)/*使能VIC DMA添加时间戳使能*/
    #define TS_MS_CH0_EN                (1 << 5)/*使能MSCALER CH0添加时间戳使能*/
    #define TS_MS_CH1_EN                (1 << 6)/*使能MSCALER CH1添加时间戳使能*/
    #define TS_MS_CH2_EN                (1 << 7)/*使能MSCALER CH2添加时间戳使能*/
#define VIC_TS_COUNTER                  0x368   /* 多少个ISP时钟周期计数一次 */
/* timestamp 写入到帧数据中的偏移位置.*/
#define VIC_TS_DMA_OFFSET               0x370
#define VIC_TS_MS_CH0_OFFSET            0x374
#define VIC_TS_MS_CH1_OFFSET            0x378
#define VIC_TS_MS_CH2_OFFSET            0x37c


#define VIC_GLB_RST                     2, 2
#define VIC_REG_ENABLE                  1, 1
#define VIC_START                       0, 0

#define SCALER_CH2_DONE                 17,17
#define SCALER_CH1_DONE                 16,16
#define SCALER_CH0_DONE                 15,15
#define VIC_DONE                        14, 14
#define DVP_HCOMP_ERR                   13, 13
#define MIPI_VCOMP_ERR_CH0              12, 12
#define MIPI_HCOMP_ERR_CH0              11, 11
#define IMAGE_FIFO_OVF                  10, 10
#define OUTPUT_LIMIT_ERR                9, 9
#define DMA_FIFO_OVF                    8, 8
#define DMA_FRD                         7, 7
#define VIC_HVF_ERR                     6, 6
#define VIC_VRES_ERR                    5, 5
#define VIC_HRES_ERR                    4, 4
#define VIC_HVRES_ERR                   4, 5
#define VIC_FIFO_OVF                    3, 3
#define VIC_FRM_RST                     2, 2
#define VIC_FRM_START                   1, 1
#define VIC_FRD                         0, 0

#define HORIZONTAL_RESOLUTION           16, 31
#define VERTICAL_RESOLUTION             0, 15

#define frame_ecc_mode                  1, 1
#define frame_ecc_en                    0, 0

#define dvp_hcomp                       31, 31
#define dvp_img_chk                     28, 28
#define DVP_BUS_SELECT                  24, 27
#define DVP_RGB_ORDER                   21, 23
#define DVP_RAW_ALIGN                   20, 20
#define DVP_DATA_FORMAT                 17, 19
#define DVP_TIMING_MODE                 15, 16
#define BT_INTF_WIDE                    11, 11
#define BT_SAV_EAV                      10, 10
#define BT601_MODE                      9, 9
#define YUV_DATA_ORDER                  4, 5
#define START_FIELD                     3, 3
#define INTERLACE_EN                    2, 2
#define HSYNC_POLAR                     1, 1
#define VSYNC_POLAR                     0, 0

#define HFB_NUM                         16, 31
#define HACT_NUM                        0, 15

#define hbb_num                         0, 15

#define ODD_VFB                         16, 31
#define ODD_VACT                        0, 15

#define ODD_VBB                         16, 31
#define EVEN_VFB                        0, 15

#define EVEN_VACT                       16, 31
#define EVEN_VBB                        0, 15

#define CB_Y_VALUE                      16, 23
#define CB_CB_VALUE                     8, 15
#define CB_CR_VALUE                     0, 7

#define RAW21_VALUE                     12, 23
#define RAW22_VALUE                     0, 11

#define Dma_en                          31, 31
#define Yuv422_order                    8, 9
#define Buffer_number                   3, 6
#define Base_mode                       0, 2

#define VC_TIZIANO_ROUTE_isp_out        0, 0
#define VC_DMA_ROUTE_dma_out            0, 0

#define DMA_HORIZONTAL_RESOLUTION       16, 31
#define DMA_VERTICAL_RESOLUTION         0, 15

#define MIPI_HCROP_CHO_all_image_width  16, 31
#define MIPI_HCROP_CHO_start_pixel      0,  15

#define VC_CONTROL_delay_hdelay         16, 31
#define VC_CONTROL_delay_vdelay         0,  15

#endif /* __X2000_VIC_REGS_H__ */
