#ifndef __VIC_REG_H__
#define __VIC_REG_H__

#include <soc/base.h>
#include <linux/io.h>



#define VIC_SUPPORT_MIPI                1


#define VIC_HCNT0                       0x04
#define VIC_HCNT1                       0x08

#define VIC_CONTROL			(0x0)
#define GLB_RST				(1<<2)
#define REG_ENABLE			(1<<1)
#define VIC_START			(1<<0)


#define VIC_RESOLUTION	 	        0x04
#define H_RESOLUTION_OFF			(16)
#define V_RESOLUTION_OFF			(0)


#define VIC_FRM_ECC                     0x08
#define FRAME_ECC_EN		       (1<<0)
#define FRAME_ECC_MODE		       (1<<1)

#define VIC_INTF_TYPE                   0x0C
#define INTF_TYPE_BT656			0x0
#define INTF_TYPE_BT601			0x1
#define INTF_TYPE_MIPI			0x2
#define INTF_TYPE_DVP			0x3
#define INTF_TYPE_BT1120		0x4

#define VIC_DB_CFG		        0x10
#define DVP_DATA_POS			(1<<24)
#define DVP_RGB_ORDER			(1<<21)
#define DVP_RAW_ALIG			(1<<20)
#define DVP_DATA_TYPE			(17)
#define DVP_RAW8			(0<<DVP_DATA_TYPE)
#define DVP_RAW10			(1<<DVP_DATA_TYPE)
#define DVP_RAW12			(2<<DVP_DATA_TYPE)
#define DVP_YUV422_16BIT		(3<<DVP_DATA_TYPE)
#define DVP_RGB565_16BIT		(4<<DVP_DATA_TYPE)
#define DVP_BRG565_16BIT		(5<<DVP_DATA_TYPE)
#define DVP_YUV422_8BIT			(6<<DVP_DATA_TYPE)
#define DVP_RGB565_8BIT			(7<<DVP_DATA_TYPE)
#define DVP_DATA_FMT_MASK       (7 << DVP_DATA_TYPE)
#define DVP_TIMEING_MODE		(1<<15)
#define DVP_SONY_MODE		        (2 << 15)
#define BT_INTF_WIDE			(1<<11)
#define BT_LINE_MODE			(1<<10)
#define BT_601_TIMING_MODE		(1<<9)
#define YUV_DATA_ORDER			(4)
#define UYVY				(0<<YUV_DATA_ORDER)
#define VYUY				(1<<YUV_DATA_ORDER)
#define YUYV				(2<<YUV_DATA_ORDER)
#define YVYU				(3<<YUV_DATA_ORDER)
#define YUV_DATA_ORDER_MASK (3<<YUV_DATA_ORDER)
#define FIRST_FIELD_TYPE		(1<<3)
#define INTERLACE_EN			(1<<2)
#define HSYN_POLAR			(1<<1)
#define VSYN_POLAR			(1<<0)


#define VIC_AB_VALUE		        (0x18)
#define A_VALUE				(1<<16)
#define B_VALUE				(1)

#define VIC_GLOBAL_CFG             	(0x50)
#define ISP_PRESET_MODE1		(0<<5)
#define ISP_PRESET_MODE2		(1<<5)
#define ISP_PRESET_MODE3		(2<<5)
#define VCKE_EN				(1<<4)
#define BLANK_EN			(2)
#define AB_MODE_SELECT			(0)

#define VIC_OUT_ABVAL           (0x54)
#define A_VALUE_OFF             (16)
#define B_VALUE_OFF             (0)


#define VIC_PIXEL			0x94
#define VIC_LINE			0x98
#define VIC_STATE			0x90
#define VIC_OFIFO_COUNT			0x9c
#define VIC_FLASH_STROBE		0x100
#define VIC_FIRST_CB			0xc0
#define VIC_SECOND_CB			0xc4
#define VIC_THIRD_CB			0xc8
#define VIC_FOURTH_CB			0xCC
#define VIC_FIFTH_CB			0xD0
#define VIC_SIXTH_CB			0xD4
#define VIC_SEVENTH_CB			0xD8
#define VIC_EIGHTH_CB			0xDC
#define CB_MODE0			0xb0
#define CB_MODE1			0xa0
#define BK_NUM_CB1			0xb4

#define VIC_INPUT_HSYNC_BLANKING 0x20
#define VIC_INPUT_VSYNC_BLANKING 0x3c

#define VIC_INT_STA         0X100
#define DMA_FIFO_OVF    (1<<8)
#define DMA_FRD             (1<<7)
#define VIC_HVF_ERR     (1<<6)
#define VIC_VRES_ERR    (1<<5)
#define VIC_HRES_ERR    (1<<4)
#define VIC_FIFO_OVF    (1<<3)
#define VIC_FRM_RST     (1<<2)
#define VIC_FRM_START   (1<<1)
#define VIC_FRD             (1<<0)


#define VIC_INT_MASK        0X104
#define DMA_DVP_OVF_MSK    (1<<8)
#define DMA_FRD_MSK             (1<<7)
#define VIC_HVF_ERR_MSK     (1<<6)
#define VIC_VRES_ERR_MSK    (1<<5)
#define VIC_HRES_ERR_MSK    (1<<4)
#define VIC_FIFO_OVF_MSK    (1<<3)
#define VIC_FRM_RST_MSK     (1<<2)
#define VIC_FRM_START_MSK   (1<<1)
#define VIC_FRD_MSK             (1<<0)


#define VIC_INT_CLR         0X108
#define DMA_DVP_OVF_CLR    (1<<8)
#define DMA_FRD_CLR             (1<<7)
#define VIC_HVF_ERR_CLR     (1<<6)
#define VIC_VRES_ERR_CLR    (1<<5)
#define VIC_HRES_ERR_CLR    (1<<4)
#define VIC_FIFO_OVF_CLR    (1<<3)
#define VIC_FRM_RST_CLR     (1<<2)
#define VIC_FRM_START_CLR   (1<<1)
#define VIC_FRD_CLR             (1<<0)


#define VIC_DMA_OUTPUT_MAX_WIDTH 2688

#define VIC_DMA_CONFIG			0x200
#define DMA_EN                          (1<<31)
#define YUV422_OR_OFF                          (8)
#define Y2V1Y1U1                          (0<<YUV422_OR_OFF)
#define Y1U1Y2V1                          (1<<YUV422_OR_OFF)
#define V1Y2U1Y1                          (2<<YUV422_OR_OFF)
#define U1Y1V1Y2                          (3<<YUV422_OR_OFF)
#define UV_SAMPLE                        (1<<7)
#define BUF_NUM(n)	                    ((n - 1)<<3)
#define DMA_OUT_FMT_RAW                          (0<<0)
#define DMA_OUT_FMT_RGB565                     (1<<0)
#define DMA_OUT_FMT_RGB888                          (2<<0)
#define DMA_OUT_FMT_YUV422P                          (3<<0)
#define DMA_OUT_FMT_YUV422_SMP0                          (4<<0)
#define DMA_OUT_FMT_YUV422P_SMP1                          (5<<0)
#define DMA_OUT_FMT_NV12                          (6<<0)
#define DMA_OUT_FMT_NV21                          (7<<0)
#define DMA_OUT_FMT_MASK                (7 << 0)

#define VIC_DMA_RESOLUTION		(0x204)
#define H_RES_OFF                       (16)
#define V_RES_OFF                       (0)

#define VIC_DMA_RESET			(0x208)
#define DMA_RESET                       (1<<0)

#define VIC_DMA_Y_STRID			(0x214)

#define VIC_DMA_Y_BUF0			(0x218)
#define VIC_DMA_Y_BUF1			(0x21c)
#define VIC_DMA_Y_BUF2			(0x220)
#define VIC_DMA_Y_BUF3			(0x224)
#define VIC_DMA_Y_BUF4			(0x228)


#define VIC_DMA_UV_STRID		(0x234)

#define VIC_DMA_UV_BUF0			(0x238)
#define VIC_DMA_UV_BUF1			(0x23c)
#define VIC_DMA_UV_BUF2			(0x240)
#define VIC_DMA_UV_BUF3			(0x244)
#define VIC_DMA_UV_BUF4			(0x248)

#define TX_ISP_TOP_IRQ_CNT		0x20000
#define TX_ISP_TOP_IRQ_CNT1		0x20020
#define TX_ISP_TOP_IRQ_CNT2		0x20024
#define TX_ISP_TOP_IRQ_CLR_1	0x20004
#define TX_ISP_TOP_IRQ_CLR_ALL	0x20008
#define TX_ISP_TOP_IRQ_STA		0x2000C
#define TX_ISP_TOP_IRQ_OVF		0x20010
#define TX_ISP_TOP_IRQ_ENABLE	0x20014
#define TX_ISP_TOP_IRQ_MASK		0x2001c
#define TX_ISP_TOP_IRQ_ISP		0xffff
#define TX_ISP_TOP_IRQ_VIC		0x7f0000
#define TX_ISP_TOP_IRQ_ALL		0x7fffff



#define tx_isp_vic_readl(reg)						\
	__le32_to_cpu(__raw_readl(reg))
#define tx_isp_vic_writel(value, reg)					\
	__raw_writel(__cpu_to_le32(value), reg)


#endif/* __VIC_REG_H__ */
