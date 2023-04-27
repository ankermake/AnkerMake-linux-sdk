#ifndef _VIC_REGS_H_
#define _VIC_REGS_H_

#define VIC_CONTROL           0x00
#define VIC_RESOLUTION        0x04
#define VIC_FRM_ECC           0x08
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
#define VIC_INT_STA           0x100
#define VIC_INT_MASK          0x104
#define VIC_INT_CLR           0x108


/* DMA register
 * 基地址与vic 寄存器基地址相同
 */
#define DMA_CONFIGURE          0x200
#define DMA_RESOLUTION         0x204
#define DMA_RESET              0x208
#define DMA_Y_CH_BANK_CTRL     0x210
#define DMA_Y_CH_LINE_STRIDE   0x214
#define DMA_Y_CH_BANK0_ADDR    0x218
#define DMA_Y_CH_BANK1_ADDR    0x21C
#define DMA_Y_CH_BANK2_ADDR    0x220
#define DMA_Y_CH_BANK3_ADDR    0x224
#define DMA_Y_CH_BANK4_ADDR    0x228
#define DMA_UV_CH_BANK_CTRL    0x230
#define DMA_UV_CH_LINE_STRIDE  0x234
#define DMA_UV_CH_BANK0_ADDR   0x238
#define DMA_UV_CH_BANK1_ADDR   0x23C
#define DMA_UV_CH_BANK2_ADDR   0x240
#define DMA_UV_CH_BANK3_ADDR   0x244
#define DMA_UV_CH_BANK4_ADDR   0x248

#define VIC_GLB_SAFE_RST 3, 3
#define VIC_GLB_RST 2, 2
#define VIC_REG_ENABLE   1, 1
#define VIC_START        0, 0

/* irq signal */
#define DMA_FIFO_OVF 8, 8
#define DMA_FRD 7, 7
#define VIC_HVF_ERR 6, 6
#define VIC_VRES_ERR 5, 5
#define VIC_HRES_ERR 4, 4
#define VIC_FIFO_OVF 3, 3
#define VIC_FRM_RST 2, 2
#define VIC_FRM_START 1, 1
#define VIC_FRD 0, 0

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

#endif /* _VIC_REGS_H_ */
