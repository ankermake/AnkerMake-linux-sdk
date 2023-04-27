#ifndef _VIC_REGS_H_
#define _VIC_REGS_H_

#define VIC_DMA_OFF 0x10000
#define VIC_IRQ_OFF 0x20000
#define VIC_CORE_OFF 0x80000

#define VIC_CONTROL           0x00
#define VIC_RESOLUTION        0x04
#define VIC_INPUT_INTF        0x0C
#define VIC_INPUT_DVP         0x10
#define VIC_INPUT_MIPI        0x14
#define VIC_INPUT_HPARA0      0x20
#define VIC_INPUT_HPARA1      0x24
#define VIC_INPUT_VPARA0      0x30
#define VIC_INPUT_VPARA1      0x34
#define VIC_INPUT_VPARA2      0x38
#define VIC_INPUT_VPARA3      0x3C
#define VIC_OUTPUT_CFG        0x50
#define VIC_OUTPUT_ABVAL      0x54
#define VIC_OUTPUT_HBLK       0x58
#define VIC_OUTPUT_VBLK       0x5C
#define VIC_STATE             0x90
#define VIC_PIXEL_CNT         0x94
#define VIC_LINE_CNT          0x98
#define VIC_OUT_FIFO_CNT      0x9C
#define VIC_FRONT_CB_CTRL     0xA0
#define VIC_FRONT_CB_BLK      0xA4
#define VIC_BACK_CB_CTRL      0xB0
#define VIC_BACK_CB_BLK       0xB4
#define VIC_FIRST_CB          0xC0
#define VIC_SECOND_CB         0xC4
#define VIC_THIRD_CB          0xC8
#define VIC_FOURTH_CB         0xCC
#define VIC_FIFTH_CB          0xD0
#define VIC_SIXTH_CB          0xD4
#define VIC_SEVENTH_CB        0xD8
#define VIC_EIGHTH_CB         0xDC
#define VIC_FIRST_CB2         0xE0
#define VIC_SECOND_CB2        0xE4
#define VIC_THIRD_CB2         0xE8
#define VIC_FOURTH_CB2        0xEC
#define VIC_FIFTH_CB2         0xF0
#define VIC_SIXTH_CB2         0xF4
#define VIC_SEVENTH_CB2       0xF8
#define VIC_EIGHTH_CB2        0xFC
#define ISP_FLASH_STROBE      0x100
#define ISP_CLK_GATE          0x104
#define ISP_AUTO_CLK_GATE     0x110
#define ISP_FEND_INT_SEL      0x114
#define ISP_UVDMA_SEL         0x118
#define ISP_GATE_DELAY        0x11C
#define ISP_CORE_FIFO_FAIL    0x120
#define ISP_CORE_FIFO_MSK     0x124
#define ISP_CORE_FIFO_USE0    0x128
#define ISP_CORE_FIFO_USE1    0x12C
#define ISP_CORE_FIFO_USE2    0x130
#define ISP_CORE_FIFO_USE3    0x134
#define ISP_CORE_FIFO_USE4    0x138

#define DMA_CONFIGURE          (VIC_DMA_OFF + 0x00)
#define DMA_RESOLUTION         (VIC_DMA_OFF + 0x04)
#define DMA_RESET              (VIC_DMA_OFF + 0x08)
#define DMA_Y_CH_BANK_CTRL     (VIC_DMA_OFF + 0x10)
#define DMA_Y_CH_LINE_STRIDE   (VIC_DMA_OFF + 0x14)
#define DMA_Y_CH_BANK0_ADDR    (VIC_DMA_OFF + 0x18)
#define DMA_Y_CH_BANK1_ADDR    (VIC_DMA_OFF + 0x1C)
#define DMA_Y_CH_BANK2_ADDR    (VIC_DMA_OFF + 0x20)
#define DMA_Y_CH_BANK3_ADDR    (VIC_DMA_OFF + 0x24)
#define DMA_Y_CH_BANK4_ADDR    (VIC_DMA_OFF + 0x28)
#define DMA_UV_CH_BANK_CTRL    (VIC_DMA_OFF + 0x30)
#define DMA_UV_CH_LINE_STRIDE  (VIC_DMA_OFF + 0x34)
#define DMA_UV_CH_BANK0_ADDR   (VIC_DMA_OFF + 0x38)
#define DMA_UV_CH_BANK1_ADDR   (VIC_DMA_OFF + 0x3C)
#define DMA_UV_CH_BANK2_ADDR   (VIC_DMA_OFF + 0x40)
#define DMA_UV_CH_BANK3_ADDR   (VIC_DMA_OFF + 0x44)
#define DMA_UV_CH_BANK4_ADDR   (VIC_DMA_OFF + 0x48)

#define ISP_IRQ_CNT0    (VIC_IRQ_OFF + 0x00)
#define ISP_IRQ_CNTC1   (VIC_IRQ_OFF + 0x04)
#define ISP_IRQ_CNTCA   (VIC_IRQ_OFF + 0x08)
#define ISP_IRQ_SRC     (VIC_IRQ_OFF + 0x0c)
#define ISP_IRQ_CNT_OVF (VIC_IRQ_OFF + 0x10)
#define ISP_IRQ_EN      (VIC_IRQ_OFF + 0x14)
#define ISP_IRQ_MSK     (VIC_IRQ_OFF + 0x1C)
#define ISP_IRQ_CNT1    (VIC_IRQ_OFF + 0x20)
#define ISP_IRQ_CNT2    (VIC_IRQ_OFF + 0x24)

#define ISP                (VIC_CORE_OFF + 0x00000)
#define FLASH_TIMER        (VIC_CORE_OFF + 0x02000)
#define GAMMA_FE0_MEM      (VIC_CORE_OFF + 0x02800)
#define GAMMA_FE1_MEM      (VIC_CORE_OFF + 0x03000)
#define RADIAL_SHADING_MEM (VIC_CORE_OFF + 0x03800)
#define SHADING_MEM        (VIC_CORE_OFF + 0x04000)
#define METERING_MEM       (VIC_CORE_OFF + 0x08000)
#define DEFECT_PIXEL_MEM   (VIC_CORE_OFF + 0x0C000)
#define HISTOGRAM_MEM      (VIC_CORE_OFF + 0x10000)
#define FR_GAMMA_RGB_MEM   (VIC_CORE_OFF + 0x10400)
#define FR_SHARPEN_MEM     (VIC_CORE_OFF + 0x10800)
#define DS1_GAMMA_RGB_MEM  (VIC_CORE_OFF + 0x11000)
#define DS1_SHARPEN_MEM    (VIC_CORE_OFF + 0x11800)
#define DS2_GAMMA_RGB_MEM  (VIC_CORE_OFF + 0x12000)
#define DS2_SHARPEN_MEM    (VIC_CORE_OFF + 0x12800)
#define I_HFILT1_MEM       (VIC_CORE_OFF + 0x14000)
#define I_VFILT1_MEM       (VIC_CORE_OFF + 0x16000)
#define I_HFILT2_MEM       (VIC_CORE_OFF + 0x18000)
#define I_VFILT2_MEM       (VIC_CORE_OFF + 0x1A000)

#define VIC_RESET        4, 4
#define VIC_GLB_SAFE_RST 3, 3
#define VIC_GLB_RST 2, 2
#define VIC_REG_ENABLE   1, 1
#define VIC_START        0, 0

#define HORIZONTAL_RESOLUTION 16, 31
#define VERTICAL_RESOLUTION   0, 15

#define frame_ecc_mode 1, 1
#define frame_ecc_en 0, 0

#define dvp_img_chk 28, 28
#define DVP_BUS_SELECT 24, 27
#define DVP_RGB_ORDER 21, 23
#define DVP_RAW_ALIGN 20, 20
#define DVP_DATA_FORMAT 17, 19
#define DVP_TIMING_MODE 15, 16
#define BT_INTF_WIDE 11, 11
#define BT_SAV_EAV 10, 10
#define BT601_MODE 9, 9
#define YUV_DATA_ORDER 4, 5
#define START_FIELD 3, 3
#define INTERLACE_EN 2, 2
#define HSYNC_POLAR 1, 1
#define VSYNC_POLAR 0, 0

#define HFB_NUM 16, 31
#define HACT_NUM 0, 15

#define hbb_num 0, 15

#define ODD_VFB 16, 31
#define ODD_VACT 0, 15

#define ODD_VBB 16, 31
#define EVEN_VFB 0, 15

#define EVEN_VACT 16, 31
#define EVEN_VBB 0, 15

#define ISP_PORT_MOD 5, 6
#define VCKE_ENA_BLE 4, 4
#define BLANK_ENABLE 2, 3
#define AB_MODE_SELECT 0, 1

#define A_VALUE 16, 31
#define B_VALUE 0, 6

#define REMAIN_PIXEL 16, 27
#define COMPLETE_PIXEL 0, 11

#define REMAIN_LINE 16, 27
#define COMPLETE_LINE 0, 11

#define REMAIN_DEPTH 16, 26
#define USED_DEPTH 0, 10

#define CB0_ENABLE 31, 31
#define CB0_FRAME_NUM 0, 15

#define FRONT_CB_HOR_BLANK 16, 31
#define FRONT_CB_VER_BLANK 0, 15

#define BACK_CB_ENABLE 31, 31
#define BACK_CB_INCREMETAL_MODE 30, 30
#define BACK_CB_DIRECTION 29, 29
#define BACK_CB_PIXEL_FORMAT 27, 28
#define BACK_CB_FRAME_NUM 0, 15

#define BACK_CB_HOR_BLANK 16, 31
#define BACK_CB_VER_BLANK 0, 15

#define CB_Y_VALUE 16, 23
#define CB_CB_VALUE 16, 23
#define CB_CR_VALUE 16, 23

#define RAW21_VALUE 12, 23
#define RAW22_VALUE 0, 11

#define DMA_CLK_GATE_EN 1, 1
#define VPCLK_GATE_EN 0, 0

#define Aclk_AGEn 2, 2
#define Vpclk_AGEn 1, 1
#define Vclk_AGEn 0, 0

#define YUV422 3, 3
#define Ds2uv_Dma 2, 2
#define Ds1uv_Dma 1, 1
#define Fruv_Dma 0, 0

#define delay_enable 16, 16
#define delay_number 0, 15

#define Dma_en 31, 31
#define Yuv422_order 8, 9
#define Uv_sample 7, 7
#define Buffer_number 3, 6
#define Base_mode 0, 2


#define DMA_HORIZONTAL_RESOLUTION 16, 31
#define DMA_VERTICAL_RESOLUTION 0, 15

#define CNT_Core_IRQ7 28, 31
#define CNT_Core_IRQ6 24, 27
#define CNT_Core_IRQ5 20, 23
#define CNT_Core_IRQ4 16, 19
#define CNT_Core_IRQ3 12, 15
#define CNT_Core_IRQ2 8,  11
#define CNT_Core_IRQ1 4,  7
#define CNT_Core_IRQ0 0,  3

#define DMA_DVP_OVF 24, 24
#define DMA_FRD 23, 23
#define CORE_FIFO_FAIL 22, 22
#define VIC_HVF 21, 21
#define VIC_IMVE 20, 20
#define VIC_IMHE 19, 19
#define VIC_MIPI_OVF 18, 18
#define VIC_DVP_OVF 17, 17
#define VIC_FRD 16, 16
#define Core_IRQ15 15, 15
#define Core_IRQ14 14, 14
#define Core_IRQ13 13, 13
#define Core_IRQ12 12, 12
#define Core_IRQ11 11, 11
#define Core_IRQ10 10, 10
#define Core_IRQ9  9, 9
#define Core_IRQ8  8, 8
#define Core_IRQ7  7, 7
#define Core_IRQ6  6, 6
#define Core_IRQ5  5, 5
#define Core_IRQ4  4, 4
#define Core_IRQ3  3, 3
#define Core_IRQ2  2, 2
#define Core_IRQ1  1, 1
#define Core_IRQ0  0, 0

#define CNT_Core_IRQ15 28, 31
#define CNT_Core_IRQ14 24, 27
#define CNT_Core_IRQ13 20, 23
#define CNT_Core_IRQ12 16, 19
#define CNT_Core_IRQ11 12, 15
#define CNT_Core_IRQ10 8, 11
#define CNT_Core_IRQ9 4, 7
#define CNT_Core_IRQ8 0, 3

#define CNT_DMA_FRD 28, 31
#define CNT_CORE_FIFO_FAIL 24, 27
#define CNT_HVF_ERR 20, 23
#define CNT_VIC_IMVE 16, 19
#define CNT_VIC_IMHE 12, 15
#define CNT_VIC_MIPI_OVF 8, 11
#define CNT_VIC_DVP_OVF 4, 7
#define CNT_VIC_FRD 0, 3

#define CNT_DMA_FIFO_OVF 0, 3

#define DS1_DMA_source         30, 30
#define Bypass_ds1_cs_conv     29, 29
#define Bypass_ds1_logo        28, 28
#define Bypass_ds1_sharpen     27, 27
#define Bypass_ds1_gamma_RGB   26, 26
#define Bypass_ds1_scaler      25, 25
#define Bypass_ds1_crop        24, 24
#define Bypass_fr_cs_conv      22, 22
#define Bypass_fr_logo         21, 21
#define Bypass_fr_sharpen      20, 20
#define Bypass_fr_gamma_RGB    19, 19
#define Bypass_fr_crop         18, 18
#define Bypass_color_matrix    17, 17
#define Bypass_demosaic        16, 16
#define Bypass_iridix          15, 15
#define Bypass_mesh_shading    14, 14
#define Bypass_radial_shading  13, 13
#define Bypass_white_balance   12, 12
#define Order_Sinter_Temper    11, 11
#define Bypass_temper          10, 10
#define Bypass_sinter          9, 9
#define Bypass_frame_stitch    8, 8
#define Bypass_mirror          7, 7
#define Bypass_defect_pixel    6, 6
#define Bypass_RAW_frontend    5, 5
#define Bypass_gamma_fe        4, 4
#define Bypass_digital_gain    3, 3
#define Bypass_sensor_offset   2, 2
#define Bypass_mirror1         1, 1
#define Bypass_video_test_gen  0, 0

#define ISP_processing_bypass_mode 13, 14
#define ISP_debug_select           10, 11
#define ISP_RAW_bypass             9, 9
#define Bypass_ds2_cs_conv         6, 6
#define Bypass_ds2_logo            5, 5
#define Bypass_ds2_sharpen         4, 4
#define Bypass_ds2_gamma_RGB       3, 3
#define Bypass_ds2_scaler          2, 2
#define Bypass_ds2_crop            1, 1
#define DS2_scaler_source          0, 0

#define Global_FSM_reset 1, 1
#define VCKE_override    0, 0

#endif /* _VIC_REGS_H_ */
