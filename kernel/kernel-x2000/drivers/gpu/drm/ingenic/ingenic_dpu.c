#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_fourcc.h>
#include "ingenic_drv.h"
#include "dpu_reg.h"
#include "jz_dsim.h"

#define crtc_to_drm_dev(x) (x->base.dev)
#define disport_to_drm_dev(x) (x->encoder.dev)

static void dump_dc_reg(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	printk("-----------------dc_reg------------------\n");
	printk("DC_FRM_CFG_ADDR(0x%04x):    0x%08lx\n", DC_FRM_CFG_ADDR,reg_read(dev, DC_FRM_CFG_ADDR));
	printk("DC_FRM_CFG_CTRL(0x%04x):    0x%08lx\n", DC_FRM_CFG_CTRL,reg_read(dev, DC_FRM_CFG_CTRL));
	printk("DC_CTRL(0x%04x):            0x%08lx\n", DC_CTRL, reg_read(dev, DC_CTRL));
	printk("DC_CSC_MULT_YRV(0x%04x):    0x%08lx\n", DC_CSC_MULT_YRV,reg_read(dev, DC_CSC_MULT_YRV));
	printk("DC_CSC_MULT_GUGV(0x%04x):   0x%08lx\n", DC_CSC_MULT_GUGV,reg_read(dev, DC_CSC_MULT_GUGV));
	printk("DC_CSC_MULT_BU(0x%04x):     0x%08lx\n", DC_CSC_MULT_BU,reg_read(dev, DC_CSC_MULT_BU));
	printk("DC_CSC_SUB_YUV(0x%04x):     0x%08lx\n", DC_CSC_SUB_YUV,reg_read(dev, DC_CSC_SUB_YUV));
	printk("DC_ST(0x%04x):              0x%08lx\n", DC_ST, reg_read(dev, DC_ST));
	printk("DC_INTC(0x%04x):            0x%08lx\n", DC_INTC, reg_read(dev, DC_INTC));
	printk("DC_INT_FLAG(0x%04x):	   0x%08lx\n", DC_INT_FLAG, reg_read(dev, DC_INT_FLAG));
	printk("DC_COM_CONFIG(0x%04x):      0x%08lx\n", DC_COM_CONFIG ,reg_read(dev, DC_COM_CONFIG));
	printk("DC_TLB_GLBC(0x%04x):        0x%08lx\n", DC_TLB_GLBC,reg_read(dev, DC_TLB_GLBC));
	printk("DC_TLB_TLBA(0x%04x):        0x%08lx\n", DC_TLB_TLBA,reg_read(dev, DC_TLB_TLBA));
	printk("DC_TLB_TLBC(0x%04x):        0x%08lx\n", DC_TLB_TLBC,reg_read(dev, DC_TLB_TLBC));
	printk("DC_TLB0_VPN(0x%04x):        0x%08lx\n", DC_TLB0_VPN,reg_read(dev, DC_TLB0_VPN));
	printk("DC_TLB1_VPN(0x%04x):        0x%08lx\n", DC_TLB1_VPN,reg_read(dev, DC_TLB1_VPN));
	printk("DC_TLB2_VPN(0x%04x):        0x%08lx\n", DC_TLB2_VPN,reg_read(dev, DC_TLB2_VPN));
	printk("DC_TLB3_VPN(0x%04x):        0x%08lx\n", DC_TLB3_VPN,reg_read(dev, DC_TLB3_VPN));
	printk("DC_TLB_TLBV(0x%04x):        0x%08lx\n", DC_TLB_TLBV,reg_read(dev, DC_TLB_TLBV));
	printk("DC_TLB_STAT(0x%04x):        0x%08lx\n", DC_TLB_STAT,reg_read(dev, DC_TLB_STAT));
	printk("DC_PCFG_RD_CTRL(0x%04x):    0x%08lx\n", DC_PCFG_RD_CTRL,reg_read(dev, DC_PCFG_RD_CTRL));
	printk("DC_PCFG_WR_CTRL(0x%04x):    0x%08lx\n", DC_PCFG_WR_CTRL,reg_read(dev, DC_PCFG_WR_CTRL));
	printk("DC_OFIFO_PCFG(0x%04x):	   0x%08lx\n", DC_OFIFO_PCFG,reg_read(dev, DC_OFIFO_PCFG));
	printk("DC_WDMA_PCFG(0x%04x):	   0x%08lx\n", DC_WDMA_PCFG,reg_read(dev, DC_WDMA_PCFG));
	printk("DC_CMPW_PCFG_CTRL(0x%04x): 0x%08lx\n", DC_CMPW_PCFG_CTRL,reg_read(dev, DC_CMPW_PCFG_CTRL));
	printk("DC_CMPW_PCFG0(0x%04x):	   0x%08lx\n", DC_CMPW_PCFG0,reg_read(dev, DC_CMPW_PCFG0));
	printk("DC_CMPW_PCFG1(0x%04x):	   0x%08lx\n", DC_CMPW_PCFG1,reg_read(dev, DC_CMPW_PCFG1));
	printk("DC_CMPW_PCFG2(0x%04x):	   0x%08lx\n", DC_CMPW_PCFG2,reg_read(dev, DC_CMPW_PCFG2));
	printk("DC_PCFG_RD_CTRL(0x%04x):    0x%08lx\n", DC_PCFG_RD_CTRL,reg_read(dev, DC_PCFG_RD_CTRL));
	printk("DC_OFIFO_PCFG(0x%04x):	   0x%08lx\n", DC_OFIFO_PCFG,reg_read(dev, DC_OFIFO_PCFG));
	printk("DC_DISP_COM(0x%04x):        0x%08lx\n", DC_DISP_COM,reg_read(dev, DC_DISP_COM));
	printk("-----------------dc_reg------------------\n");
}

static void dump_tft_reg(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	printk("----------------tft_reg------------------\n");
	printk("TFT_TIMING_HSYNC(0x%04x):   0x%08lx\n", DC_TFT_HSYNC, reg_read(dev, DC_TFT_HSYNC));
	printk("TFT_TIMING_VSYNC(0x%04x):   0x%08lx\n", DC_TFT_VSYNC, reg_read(dev, DC_TFT_VSYNC));
	printk("TFT_TIMING_HDE(0x%04x):     0x%08lx\n", DC_TFT_HDE, reg_read(dev, DC_TFT_HDE));
	printk("TFT_TIMING_VDE(0x%04x):     0x%08lx\n", DC_TFT_VDE, reg_read(dev, DC_TFT_VDE));
	printk("TFT_TRAN_CFG(0x%04x):       0x%08lx\n", DC_TFT_CFG, reg_read(dev, DC_TFT_CFG));
	printk("TFT_ST(0x%04x):             0x%08lx\n", DC_TFT_ST, reg_read(dev, DC_TFT_ST));
	printk("----------------tft_reg------------------\n");
}

static void dump_slcd_reg(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	printk("---------------slcd_reg------------------\n");
	printk("SLCD_CFG(0x%04x):           0x%08lx\n", DC_SLCD_CFG, reg_read(dev, DC_SLCD_CFG));
	printk("SLCD_WR_DUTY(0x%04x):       0x%08lx\n", DC_SLCD_WR_DUTY, reg_read(dev, DC_SLCD_WR_DUTY));
	printk("SLCD_TIMING(0x%04x):        0x%08lx\n", DC_SLCD_TIMING, reg_read(dev, DC_SLCD_TIMING));
	printk("SLCD_FRM_SIZE(0x%04x):      0x%08lx\n", DC_SLCD_FRM_SIZE, reg_read(dev, DC_SLCD_FRM_SIZE));
	printk("SLCD_SLOW_TIME(0x%04x):     0x%08lx\n", DC_SLCD_SLOW_TIME, reg_read(dev, DC_SLCD_SLOW_TIME));
	printk("SLCD_REG_IF(0x%04x):	    0x%08lx\n", DC_SLCD_REG_IF, reg_read(dev, DC_SLCD_REG_IF));
	printk("SLCD_ST(0x%04x):            0x%08lx\n", DC_SLCD_ST, reg_read(dev, DC_SLCD_ST));
	printk("---------------slcd_reg------------------\n");
}

static void dump_frm_desc_reg(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	uint32_t ctrl;
	ctrl = reg_read(dev, DC_CTRL);
	ctrl |= DC_DES_CNT_RST;
	reg_write(dev, DC_CTRL, ctrl);

	printk("--------Frame Descriptor register--------\n");
	printk("FrameNextCfgAddr:   %lx\n",reg_read(dev, DC_FRM_DES));
	printk("FrameSize:          %lx\n",reg_read(dev, DC_FRM_DES));
	printk("FrameCtrl:          %lx\n",reg_read(dev, DC_FRM_DES));
	printk("WritebackAddr:      %lx\n",reg_read(dev, DC_FRM_DES));
	printk("WritebackStride:    %lx\n",reg_read(dev, DC_FRM_DES));
	printk("Layer0CfgAddr:      %lx\n",reg_read(dev, DC_FRM_DES));
	printk("Layer1CfgAddr:      %lx\n",reg_read(dev, DC_FRM_DES));
	printk("Layer2CfgAddr:      %lx\n",reg_read(dev, DC_FRM_DES));
	printk("Layer3CfgAddr:      %lx\n",reg_read(dev, DC_FRM_DES));
	printk("LayCfgEn:	    %lx\n",reg_read(dev, DC_FRM_DES));
	printk("InterruptControl:   %lx\n",reg_read(dev, DC_FRM_DES));
	printk("--------Frame Descriptor register--------\n");
}

static void dump_layer_desc_reg(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	uint32_t ctrl;
	ctrl = reg_read(dev, DC_CTRL);
	ctrl |= DC_DES_CNT_RST;
	reg_write(dev, DC_CTRL, ctrl);

	printk("--------layer0 Descriptor register-------\n");
	printk("LayerSize:          %lx\n",reg_read(dev, DC_LAY0_DES));
	printk("LayerCfg:           %lx\n",reg_read(dev, DC_LAY0_DES));
	printk("LayerBufferAddr:    %lx\n",reg_read(dev, DC_LAY0_DES));
	printk("LayerScale:         %lx\n",reg_read(dev, DC_LAY0_DES));
	printk("LayerRotation:      %lx\n",reg_read(dev, DC_LAY0_DES));
	printk("LayerScratch:       %lx\n",reg_read(dev, DC_LAY0_DES));
	printk("LayerPos:           %lx\n",reg_read(dev, DC_LAY0_DES));
	printk("LayerResizeCoef_X:  %lx\n",reg_read(dev, DC_LAY0_DES));
	printk("LayerResizeCoef_Y:  %lx\n",reg_read(dev, DC_LAY0_DES));
	printk("LayerStride:        %lx\n",reg_read(dev, DC_LAY0_DES));
	printk("--------layer0 Descriptor register-------\n");

	printk("--------layer1 Descriptor register-------\n");
	printk("LayerSize:          %lx\n",reg_read(dev, DC_LAY1_DES));
	printk("LayerCfg:           %lx\n",reg_read(dev, DC_LAY1_DES));
	printk("LayerBufferAddr:    %lx\n",reg_read(dev, DC_LAY1_DES));
	printk("LayerScale:         %lx\n",reg_read(dev, DC_LAY1_DES));
	printk("LayerRotation:      %lx\n",reg_read(dev, DC_LAY1_DES));
	printk("LayerScratch:       %lx\n",reg_read(dev, DC_LAY1_DES));
	printk("LayerPos:           %lx\n",reg_read(dev, DC_LAY1_DES));
	printk("LayerResizeCoef_X:  %lx\n",reg_read(dev, DC_LAY1_DES));
	printk("LayerResizeCoef_Y:  %lx\n",reg_read(dev, DC_LAY1_DES));
	printk("LayerStride:        %lx\n",reg_read(dev, DC_LAY1_DES));
	printk("--------layer1 Descriptor register-------\n");

	printk("--------layer2 Descriptor register-------\n");
	printk("LayerSize:          %lx\n",reg_read(dev, DC_LAY2_DES));
	printk("LayerCfg:           %lx\n",reg_read(dev, DC_LAY2_DES));
	printk("LayerBufferAddr:    %lx\n",reg_read(dev, DC_LAY2_DES));
	printk("LayerScale:         %lx\n",reg_read(dev, DC_LAY2_DES));
	printk("LayerRotation:      %lx\n",reg_read(dev, DC_LAY2_DES));
	printk("LayerScratch:       %lx\n",reg_read(dev, DC_LAY2_DES));
	printk("LayerPos:           %lx\n",reg_read(dev, DC_LAY2_DES));
	printk("LayerResizeCoef_X:  %lx\n",reg_read(dev, DC_LAY2_DES));
	printk("LayerResizeCoef_Y:  %lx\n",reg_read(dev, DC_LAY2_DES));
	printk("LayerStride:        %lx\n",reg_read(dev, DC_LAY2_DES));
//	printk("Layer_UV_addr:	    %lx\n",reg_read(dev, DC_LAY2_DES));
//	printk("Layer_UV_stride:    %lx\n",reg_read(dev, DC_LAY2_DES));
	printk("--------layer2 Descriptor register-------\n");

	printk("--------layer3 Descriptor register-------\n");
	printk("LayerSize:          %lx\n",reg_read(dev, DC_LAY3_DES));
	printk("LayerCfg:           %lx\n",reg_read(dev, DC_LAY3_DES));
	printk("LayerBufferAddr:    %lx\n",reg_read(dev, DC_LAY3_DES));
	printk("LayerScale:         %lx\n",reg_read(dev, DC_LAY3_DES));
	printk("LayerRotation:      %lx\n",reg_read(dev, DC_LAY3_DES));
	printk("LayerScratch:       %lx\n",reg_read(dev, DC_LAY3_DES));
	printk("LayerPos:           %lx\n",reg_read(dev, DC_LAY3_DES));
	printk("LayerResizeCoef_X:  %lx\n",reg_read(dev, DC_LAY3_DES));
	printk("LayerResizeCoef_Y:  %lx\n",reg_read(dev, DC_LAY3_DES));
	printk("LayerStride:        %lx\n",reg_read(dev, DC_LAY3_DES));
//	printk("Layer_UV_addr:	    %lx\n",reg_read(dev, DC_LAY3_DES));
//	printk("Layer_UV_stride:    %lx\n",reg_read(dev, DC_LAY3_DES));
	printk("--------layer3 Descriptor register-------\n");
}

static void dump_srd_desc_reg(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	uint32_t ctrl;
	  ctrl = reg_read(dev, DC_CTRL);
	  ctrl |= (1 << 2);
	  reg_write(dev, DC_CTRL, ctrl);
	printk("====================rdma Descriptor register======================\n");
	printk("RdmaNextCfgAddr:    %lx\n",reg_read(dev, DC_RDMA_DES));
	printk("FrameBufferAddr:    %lx\n",reg_read(dev, DC_RDMA_DES));
	printk("Stride:             %lx\n",reg_read(dev, DC_RDMA_DES));
	printk("ChainCfg:           %lx\n",reg_read(dev, DC_RDMA_DES));
	printk("InterruptControl:   %lx\n",reg_read(dev, DC_RDMA_DES));
	printk("==================rdma Descriptor register end======================\n");
}

static void dump_frm_desc(struct ingenic_crtc *crtc,
		struct ingenicfb_framedesc *framedesc,
		int index)
{
	printk("-------User Frame Descriptor index[%d]-----\n", index);
	printk("FramedescAddr:	    0x%x\n",(uint32_t)framedesc);
	printk("FrameNextCfgAddr:   0x%x\n",framedesc->FrameNextCfgAddr);
	printk("FrameSize:          0x%x\n",framedesc->FrameSize.d32);
	printk("FrameCtrl:          0x%x\n",framedesc->FrameCtrl.d32);
	printk("Layer0CfgAddr:      0x%x\n",framedesc->Layer0CfgAddr);
	printk("Layer1CfgAddr:      0x%x\n",framedesc->Layer1CfgAddr);
	printk("LayerCfgEn:	    0x%x\n",framedesc->LayCfgEn.d32);
	printk("InterruptControl:   0x%x\n",framedesc->InterruptControl.d32);
	printk("-------User Frame Descriptor index[%d]-----\n", index);
}

static void dump_layer_desc(struct ingenic_crtc *crtc,
		struct ingenicfb_layerdesc *layerdesc,
		int row, int col)
{
	printk("------User layer Descriptor index[%d][%d]------\n", row, col);
	printk("LayerdescAddr:	    0x%x\n",(uint32_t)layerdesc);
	printk("LayerSize:          0x%x\n",layerdesc->LayerSize.d32);
	printk("LayerCfg:           0x%x\n",layerdesc->LayerCfg.d32);
	printk("LayerBufferAddr:    0x%x\n",layerdesc->LayerBufferAddr);
	printk("LayerScale:         0x%x\n",layerdesc->LayerScale.d32);
	printk("LayerResizeCoef_X:  0x%x\n",layerdesc->layerresizecoef_x);
	printk("LayerResizeCoef_Y:  0x%x\n",layerdesc->layerresizecoef_y);
	printk("LayerScale:         0x%x\n",layerdesc->LayerScale.d32);
	printk("LayerPos:           0x%x\n",layerdesc->LayerPos.d32);
	printk("LayerStride:        0x%x\n",layerdesc->LayerStride);
	printk("------User layer Descriptor index[%d][%d]------\n", row, col);
}

static void dump_srd_desc(struct ingenic_crtc *crtc,
		struct ingenicfb_sreadesc *framedesc,
		int index)
{
	printk("-------User Simple read Descriptor index[%d]-----\n", index);
	printk("SreadescAddr:	   0x%x\n",(uint32_t)framedesc);
	printk("RdmaNextCfgAddr:   0x%x\n",framedesc->RdmaNextCfgAddr);
	printk("FrameBufferAddr:   0x%x\n",framedesc->FrameBufferAddr);
	printk("Stride:		   0x%x\n",framedesc->Stride);
	printk("ChainCfg:	   0x%x\n",framedesc->ChainCfg.d32);
	printk("InterruptControl:  0x%x\n",framedesc->InterruptControl.d32);
	printk("-------User Simple read Descriptor index[%d]-----\n", index);
}


static void dump_cmp_registers(struct ingenic_crtc *crtc)
{
	dump_dc_reg(crtc);
	dump_tft_reg(crtc);
	dump_slcd_reg(crtc);
	dump_frm_desc_reg(crtc);
	dump_layer_desc_reg(crtc);
}

static void dump_srd_registers(struct ingenic_crtc *crtc)
{
	dump_dc_reg(crtc);
	dump_tft_reg(crtc);
	dump_slcd_reg(crtc);
	dump_srd_desc_reg(crtc);
}

static void dump_cmp_desc(struct ingenic_crtc *crtc)
{
	struct ingenic_cmp_priv *priv = crtc->crtc_priv;
	int i, j;
	for(i = 0; i < 2; i++) {
		dump_frm_desc(crtc, priv->framedesc[i], i);
		for(j = 0; j < DPU_SUPPORT_LAYER_NUM; j++) {
			dump_layer_desc(crtc, priv->layerdesc[i][j], i, j);
		}
	}
}

static void dump_srd_descs(struct ingenic_crtc *crtc)
{
	struct ingenic_srd_priv *priv = crtc->crtc_priv;
	int i;
	for(i = 0; i < 2; i++) {
		dump_srd_desc(crtc, priv->sreadesc[i], i);
	}
}

void dump_cmp_plane(struct ingenic_drm_plane * plane)
{
	printk("nv_en:		0x%x\n",plane->nv_en);
	printk("lay_en:		0x%x\n",plane->lay_en);
	printk("scale_en:	0x%x\n",plane->scale_en);
	printk("lay_z_order:	0x%x\n",plane->zorder);
	printk("src_x:		0x%x\n",plane->src_x);
	printk("src_y:		0x%x\n",plane->src_y);
	printk("src_w:		0x%x\n",plane->src_w);
	printk("src_h:		0x%x\n",plane->src_h);
	printk("disp_pos_x:	0x%x\n",plane->disp_pos_x);
	printk("disp_pos_y:	0x%x\n",plane->disp_pos_y);
	printk("scale_w:	0x%x\n",plane->scale_w);
	printk("scale_h:	0x%x\n",plane->scale_h);
	printk("g_alpha_en:	0x%x\n",plane->g_alpha_en);
	printk("g_alpha_val:	0x%x\n",plane->g_alpha_val);
	printk("color:		0x%x\n",plane->color);
	printk("format:		0x%x\n",plane->format);
	printk("stride:		0x%x\n",plane->stride);
	printk("uvstride:	0x%x\n",plane->uvstride);
	printk("addr_offset[0]:	0x%x\n",plane->addr_offset[0]);
	printk("addr_offset[1]:	0x%x\n",plane->addr_offset[1]);
}

void dpu_dump_cmp_all(struct ingenic_crtc *crtc)
{
	dump_cmp_registers(crtc);
	dump_cmp_desc(crtc);
}

void dpu_dump_srd_all(struct ingenic_crtc *crtc)
{
	dump_srd_registers(crtc);
	dump_srd_descs(crtc);
}

static const uint32_t cmp01_formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_XRGB1555,
	DRM_FORMAT_NV12,
	DRM_FORMAT_NV21,
	DRM_FORMAT_YUV422
};

static const uint32_t cmp23_formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_XRGB1555,
};

static const uint32_t srd_formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_XRGB1555,
};

void ingenic_cmp_plane_get_formats(uint32_t index,
		const uint32_t **formats, int *size)
{
	if(index == 0 || index == 1) {
		*formats = cmp01_formats;
		*size = ARRAY_SIZE(cmp01_formats);
	} else {
		*formats = cmp23_formats;
		*size = ARRAY_SIZE(cmp23_formats);
	}
}

void ingenic_srd_plane_get_formats(const uint32_t **formats, int *size)
{
	*formats = srd_formats;
	*size = ARRAY_SIZE(srd_formats);
}

void ingenicfb_clk_enable(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct ingenic_drm_private *priv = dev->dev_private;
	clk_prepare_enable(priv->disp_clk);
	clk_prepare_enable(priv->clk);
}

void ingenicfb_clk_disable(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct ingenic_drm_private *priv = dev->dev_private;
	clk_disable_unprepare(priv->clk);
	clk_disable_unprepare(priv->disp_clk);
}

void ingenic_crtc_update_clk(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct drm_display_mode *mode = &crtc->state->adjusted_mode;
	struct ingenic_drm_private *priv = dev->dev_private;
	unsigned long rate, prate;
	struct clk *clk;
	int ret;

	rate = mode->clock * 1000;
	clk = clk_get_parent(priv->disp_clk);
	prate = clk_get_rate(clk);
	if(prate % rate)
		rate = prate / (prate / rate) + 1;

	ret = clk_set_rate(priv->disp_clk, rate);
	if (ret) {
		dev_err(dev->dev, "failed to set display clock rate to: %d\n",
				crtc->mode.clock);
	}
}

static int slcd_pixel_refresh_times(struct ingenic_disport *disport,
			struct drm_display_mode *mode)
{
	struct drm_device *dev = disport_to_drm_dev(disport);
	struct lcd_panel *lcd_panel =
		(struct lcd_panel *)mode->private;
	struct smart_config *smart_config;

	if(!lcd_panel || !lcd_panel->smart_config) {
		dev_info(dev->dev, "slcd use default cfg\n");
		return 1;
	}

	smart_config = lcd_panel->smart_config;

	switch(smart_config->smart_type){
	case SMART_LCD_TYPE_8080:
	case SMART_LCD_TYPE_6800:
		break;
	case SMART_LCD_TYPE_SPI_3:
		return 9;
	case SMART_LCD_TYPE_SPI_4:
		return 8;
	default:
		break;
	}

	switch(smart_config->pix_fmt) {
	case SMART_LCD_FORMAT_888:
		if(smart_config->dwidth == SMART_LCD_DWIDTH_8_BIT)
			return 3;
		if(smart_config->dwidth == SMART_LCD_DWIDTH_24_BIT)
			return 1;
	case SMART_LCD_FORMAT_565:
		if(smart_config->dwidth == SMART_LCD_DWIDTH_8_BIT)
			return 2;
		if(smart_config->dwidth == SMART_LCD_DWIDTH_16_BIT)
			return 1;
	default:
		break;
	}

	return 1;
}

int refresh_pixclock_auto_adapt(struct ingenic_disport *ctx,
			struct drm_display_mode *mode)
{
	unsigned long rate;

	if(mode->vrefresh){
		rate = mode->vrefresh * mode->htotal * mode->vtotal;
		if(ctx->lcd_type == LCD_TYPE_SLCD) {
			rate *= slcd_pixel_refresh_times(ctx, mode);
		}
		mode->clock = round_up(rate, 1000) / 1000;
	}else if(mode->clock){
		rate = mode->clock * 1000;
		mode->vrefresh = rate / mode->htotal / mode->vtotal;
		if(ctx->lcd_type == LCD_TYPE_SLCD)
			mode->vrefresh /= slcd_pixel_refresh_times(ctx, mode);
	}else{
		return -EINVAL;
	}

	return 0;
}

static void slcd_cfg_init(struct ingenic_disport *disport,
		struct drm_display_mode *mode) {
	struct drm_device *dev = disport_to_drm_dev(disport);
	struct lcd_panel *lcd_panel =
		(struct lcd_panel *)mode->private;
	struct smart_config *smart_config;
	uint32_t slcd_cfg;

	if(!lcd_panel || !lcd_panel->smart_config) {
		dev_info(dev->dev, "slcd use default cfg\n");
		return;
	}

	smart_config = lcd_panel->smart_config;
	slcd_cfg = reg_read(dev, DC_SLCD_CFG);
	slcd_cfg &= ~DC_RDY_SWITCH;
	slcd_cfg &= ~DC_CS_EN;

	if(smart_config->te_switch) {
		slcd_cfg |= DC_TE_SWITCH;

		if(smart_config->te_dp) {
			slcd_cfg |= DC_TE_DP;
		} else {
			slcd_cfg &= ~DC_TE_DP;
		}
		if(smart_config->te_md) {
			slcd_cfg |= DC_TE_MD;
		} else {
			slcd_cfg &= ~DC_TE_MD;
		}
		if(smart_config->te_anti_jit) {
			slcd_cfg |= DC_TE_ANTI_JIT;
		} else {
			slcd_cfg &= ~DC_TE_ANTI_JIT;
		}
	} else {
		slcd_cfg &= ~DC_TE_SWITCH;
	}

	if(smart_config->te_mipi_switch) {
		slcd_cfg |= DC_TE_MIPI_SWITCH;
	} else {
		slcd_cfg &= ~DC_TE_MIPI_SWITCH;
	}

	if(smart_config->dc_md) {
		slcd_cfg |= DC_DC_MD;
	} else {
		slcd_cfg &= ~DC_DC_MD;
	}

	if(smart_config->wr_md) {
		slcd_cfg |= DC_WR_DP;
	} else {
		slcd_cfg &= ~DC_WR_DP;
	}

	slcd_cfg &= ~DC_DBI_TYPE_MASK;
	switch(smart_config->smart_type){
	case SMART_LCD_TYPE_8080:
		slcd_cfg |= DC_DBI_TYPE_B_8080;
		break;
	case SMART_LCD_TYPE_6800:
		slcd_cfg |= DC_DBI_TYPE_A_6800;
		break;
	case SMART_LCD_TYPE_SPI_3:
		slcd_cfg |= DC_DBI_TYPE_C_SPI_3;
		break;
	case SMART_LCD_TYPE_SPI_4:
		slcd_cfg |= DC_DBI_TYPE_C_SPI_4;
		break;
	default:
		printk("err!\n");
		break;
	}

	slcd_cfg &= ~DC_DATA_FMT_MASK;
	switch(smart_config->pix_fmt) {
	case SMART_LCD_FORMAT_888:
		slcd_cfg |= DC_DATA_FMT_888;
		break;
	case SMART_LCD_FORMAT_666:
		slcd_cfg |= DC_DATA_FMT_666;
		break;
	case SMART_LCD_FORMAT_565:
		slcd_cfg |= DC_DATA_FMT_565;
		break;
	default:
		printk("err!\n");
		break;
	}

	slcd_cfg &= ~DC_DWIDTH_MASK;
	switch(smart_config->dwidth) {
	case SMART_LCD_DWIDTH_8_BIT:
		slcd_cfg |= DC_DWIDTH_8BITS;
		break;
	case SMART_LCD_DWIDTH_9_BIT:
		slcd_cfg |= DC_DWIDTH_9BITS;
		break;
	case SMART_LCD_DWIDTH_16_BIT:
		slcd_cfg |= DC_DWIDTH_16BITS;
		break;
	case SMART_LCD_DWIDTH_18_BIT:
		slcd_cfg |= DC_DWIDTH_18BITS;
		break;
	case SMART_LCD_DWIDTH_24_BIT:
		slcd_cfg |= DC_DWIDTH_24BITS;
		break;
	default:
		printk("err!\n");
		break;
	}

	slcd_cfg &= ~DC_CWIDTH_MASK;
	switch(smart_config->cwidth) {
	case SMART_LCD_CWIDTH_8_BIT:
		slcd_cfg |= DC_CWIDTH_8BITS;
		break;
	case SMART_LCD_CWIDTH_9_BIT:
		slcd_cfg |= DC_CWIDTH_9BITS;
		break;
	case SMART_LCD_CWIDTH_16_BIT:
		slcd_cfg |= DC_CWIDTH_16BITS;
		break;
	case SMART_LCD_CWIDTH_18_BIT:
		slcd_cfg |= DC_CWIDTH_18BITS;
		break;
	case SMART_LCD_CWIDTH_24_BIT:
		slcd_cfg |= DC_CWIDTH_24BITS;
		break;
	default:
		printk("err!\n");
		break;
	}

	reg_write(dev, DC_SLCD_CFG, slcd_cfg);

	return;
}

static int slcd_timing_init(struct ingenic_disport *disport , struct drm_display_mode *mode)
{
	struct drm_device *dev = disport_to_drm_dev(disport);
	uint32_t width = mode->hdisplay;
	uint32_t height = mode->vdisplay;
	uint32_t dhtime = 0;
	uint32_t dltime = 0;
	uint32_t chtime = 0;
	uint32_t cltime = 0;
	uint32_t tah = 0;
	uint32_t tas = 0;
	uint32_t slowtime = 0;

	/*frm_size*/
	reg_write(dev, DC_SLCD_FRM_SIZE,
		  ((width << DC_SLCD_FRM_H_SIZE_LBIT) |
		   (height << DC_SLCD_FRM_V_SIZE_LBIT)));

	/* wr duty */
	reg_write(dev, DC_SLCD_WR_DUTY,
		  ((dhtime << DC_DSTIME_LBIT) |
		   (dltime << DC_DDTIME_LBIT) |
		   (chtime << DC_CSTIME_LBIT) |
		   (cltime << DC_CDTIME_LBIT)));

	/* slcd timing */
	reg_write(dev, DC_SLCD_TIMING,
		  ((tah << DC_TAH_LBIT) |
		  (tas << DC_TAS_LBIT)));

	/* slow time */
	reg_write(dev, DC_SLCD_SLOW_TIME, slowtime);

	return 0;
}

static void slcd_send_mcu_cmd(struct drm_device *dev, unsigned long cmd)
{
	int count = 10000;
	uint32_t slcd_cfg;

	while ((reg_read(dev, DC_SLCD_ST) & DC_SLCD_ST_BUSY) && count--) {
		udelay(10);
	}
	if (count < 0) {
		dev_err(dev->dev, "SLCDC wait busy state wrong");
	}

	slcd_cfg = reg_read(dev, DC_SLCD_CFG);
	reg_write(dev, DC_SLCD_REG_IF,  DC_SLCD_REG_IF_FLAG_CMD | (cmd & ~ DC_SLCD_REG_IF_FLAG_MASK));
}

static void slcd_send_mcu_data(struct drm_device *dev, unsigned long data)
{
	int count = 10000;
	uint32_t slcd_cfg;

	while ((reg_read(dev, DC_SLCD_ST) & DC_SLCD_ST_BUSY) && count--) {
		udelay(10);
	}
	if (count < 0) {
		dev_err(dev->dev, "SLCDC wait busy state wrong");
	}

	slcd_cfg = reg_read(dev, DC_SLCD_CFG);

	reg_write(dev, DC_SLCD_CFG, (slcd_cfg | DC_FMT_EN));
	reg_write(dev, DC_SLCD_REG_IF,  DC_SLCD_REG_IF_FLAG_DATA | (data & ~ DC_SLCD_REG_IF_FLAG_MASK));
}

static void slcd_send_mcu_prm(struct drm_device *dev, unsigned long data)
{
	int count = 10000;
	uint32_t slcd_cfg;

	while ((reg_read(dev, DC_SLCD_ST) & DC_SLCD_ST_BUSY) && count--) {
		udelay(10);
	}
	if (count < 0) {
		dev_err(dev->dev, "SLCDC wait busy state wrong");
	}

	slcd_cfg = reg_read(dev, DC_SLCD_CFG);
	reg_write(dev, DC_SLCD_REG_IF,  DC_SLCD_REG_IF_FLAG_PRM | (data & ~ DC_SLCD_REG_IF_FLAG_MASK));
}

static void slcd_mcu_init(struct ingenic_disport *disport , struct drm_display_mode *mode)
{
	struct drm_device *dev = disport_to_drm_dev(disport);
	struct lcd_panel *lcd_panel =
		(struct lcd_panel *)mode->private;
	struct smart_config *smart_config;
	struct smart_lcd_data_table *data_table;
	uint32_t length_data_table;
	uint32_t i;

	if(!lcd_panel || !lcd_panel->smart_config) {
		return;
	}
	smart_config = lcd_panel->smart_config;
	data_table = smart_config->data_table;
	length_data_table = smart_config->length_data_table;

	if(length_data_table && data_table) {
		for(i = 0; i < length_data_table; i++) {
			switch (data_table[i].type) {
			case SMART_CONFIG_DATA:
				slcd_send_mcu_data(dev, data_table[i].value);
				break;
			case SMART_CONFIG_PRM:
				slcd_send_mcu_prm(dev, data_table[i].value);
				break;
			case SMART_CONFIG_CMD:
				slcd_send_mcu_cmd(dev, data_table[i].value);
				break;
			case SMART_CONFIG_UDELAY:
				udelay(data_table[i].value);
				break;
			default:
				printk("Unknow SLCD data type\n");
				break;
			}
		}
	}
}

static void slcd_set_par(struct ingenic_disport *disport ,
		struct drm_display_mode *mode)
{
	slcd_cfg_init(disport, mode);
	slcd_timing_init(disport, mode);
	slcd_mcu_init(disport, mode);
}

static void wait_slcd_busy(struct ingenic_disport *disport )
{
	struct drm_device *dev = disport_to_drm_dev(disport);
	int count = 100000;
	while ((reg_read(dev, DC_SLCD_ST) & DC_SLCD_ST_BUSY)
			&& count--) {
		udelay(10);
	}
	if (count < 0) {
		dev_err(dev->dev,"SLCDC wait busy state wrong");
	}
}

static void slcd_mode_set(struct ingenic_disport *disport ,
				struct drm_display_mode *mode)
{
	struct drm_device *dev = disport_to_drm_dev(disport);
	uint32_t disp_com;

	disp_com = reg_read(dev, DC_DISP_COM);
	disp_com &= ~DC_DP_IF_SEL_MASK;
	if(disport->lcd_type == LCD_TYPE_SLCD)
		reg_write(dev, DC_DISP_COM, disp_com | DC_DISP_COM_SLCD);
	else
		reg_write(dev, DC_DISP_COM, disp_com | DC_DISP_COM_MIPI_SLCD);
	slcd_set_par(disport, mode);
}

static void slcd_enable(struct ingenic_disport *disport )
{
	struct drm_device *dev = disport_to_drm_dev(disport);

	slcd_send_mcu_cmd(dev, 0x2c);
	wait_slcd_busy(disport);
}

static void tft_set_par(struct ingenic_disport *disport ,
		struct drm_display_mode *mode)
{
	struct drm_device *dev = disport_to_drm_dev(disport);
	struct lcd_panel *lcd_panel = (struct lcd_panel *)mode->private;
	struct tft_config *tft_config = lcd_panel->tft_config;
	uint32_t tft_cfg = 0;
	uint32_t lcd_cgu;
	uint32_t hps;
	uint32_t hpe;
	uint32_t vps;
	uint32_t vpe;
	uint32_t hds;
	uint32_t hde;
	uint32_t vds;
	uint32_t vde;

	hps = mode->hsync_end - mode->hsync_start;
	hpe = mode->htotal;
	vps = mode->vsync_end - mode->vsync_start;
	vpe = mode->vtotal;

	hds = mode->htotal - mode->hsync_start;
	hde = hds + mode->hdisplay;
	vds = mode->vtotal - mode->vsync_start;
	vde = vds + mode->vdisplay;

	reg_write(dev, DC_TFT_HSYNC,
		  (hps << DC_HPS_LBIT) |
		  (hpe << DC_HPE_LBIT));
	reg_write(dev, DC_TFT_VSYNC,
		  (vps << DC_VPS_LBIT) |
		  (vpe << DC_VPE_LBIT));
	reg_write(dev, DC_TFT_HDE,
		  (hds << DC_HDS_LBIT) |
		  (hde << DC_HDE_LBIT));
	reg_write(dev, DC_TFT_VDE,
		  (vds << DC_VDS_LBIT) |
		  (vde << DC_VDE_LBIT));

	if(!tft_config) {
		dev_info(dev->dev, "tft lcd use default cfg\n");
		return;
	}

	lcd_cgu = *(volatile uint32_t *)(0xb0000064);
	if(tft_config->pix_clk_inv) {
		lcd_cgu |= (0x1 << 26);
	} else {
		lcd_cgu &= ~(0x1 << 26);
	}
	*(volatile uint32_t *)(0xb0000064) = lcd_cgu;

	tft_cfg = reg_read(dev, DC_TFT_CFG);
	if(tft_config->de_dl) {
		tft_cfg |= DC_DE_DL;
	} else {
		tft_cfg &= ~DC_DE_DL;
	}

	if(tft_config->sync_dl) {
		tft_cfg |= DC_SYNC_DL;
	} else {
		tft_cfg &= ~DC_SYNC_DL;
	}

	tft_cfg &= ~DC_COLOR_EVEN_MASK;
	switch(tft_config->color_even) {
	case TFT_LCD_COLOR_EVEN_RGB:
		tft_cfg |= DC_EVEN_RGB;
		break;
	case TFT_LCD_COLOR_EVEN_RBG:
		tft_cfg |= DC_EVEN_RBG;
		break;
	case TFT_LCD_COLOR_EVEN_BGR:
		tft_cfg |= DC_EVEN_BGR;
		break;
	case TFT_LCD_COLOR_EVEN_BRG:
		tft_cfg |= DC_EVEN_BRG;
		break;
	case TFT_LCD_COLOR_EVEN_GBR:
		tft_cfg |= DC_EVEN_GBR;
		break;
	case TFT_LCD_COLOR_EVEN_GRB:
		tft_cfg |= DC_EVEN_GRB;
		break;
	default:
		printk("err!\n");
		break;
	}

	tft_cfg &= ~DC_COLOR_ODD_MASK;
	switch(tft_config->color_odd) {
	case TFT_LCD_COLOR_ODD_RGB:
		tft_cfg |= DC_ODD_RGB;
		break;
	case TFT_LCD_COLOR_ODD_RBG:
		tft_cfg |= DC_ODD_RBG;
		break;
	case TFT_LCD_COLOR_ODD_BGR:
		tft_cfg |= DC_ODD_BGR;
		break;
	case TFT_LCD_COLOR_ODD_BRG:
		tft_cfg |= DC_ODD_BRG;
		break;
	case TFT_LCD_COLOR_ODD_GBR:
		tft_cfg |= DC_ODD_GBR;
		break;
	case TFT_LCD_COLOR_ODD_GRB:
		tft_cfg |= DC_ODD_GRB;
		break;
	default:
		printk("err!\n");
		break;
	}

	tft_cfg &= ~DC_MODE_MASK;
	switch(tft_config->mode){
	case TFT_LCD_MODE_PARALLEL_888:
		tft_cfg |= DC_MODE_PARALLEL_888;
		break;
	case TFT_LCD_MODE_PARALLEL_666:
		tft_cfg |= DC_MODE_PARALLEL_666;
		break;
	case TFT_LCD_MODE_PARALLEL_565:
		tft_cfg |= DC_MODE_PARALLEL_565;
		break;
	case TFT_LCD_MODE_SERIAL_RGB:
		tft_cfg |= DC_MODE_SERIAL_8BIT_RGB;
		break;
	case TFT_LCD_MODE_SERIAL_RGBD:
		tft_cfg |= DC_MODE_SERIAL_8BIT_RGBD;
		break;
	default:
		printk("err!\n");
		break;
	}
	reg_write(dev, DC_TFT_CFG, tft_cfg);
}

static void tft_enable(struct ingenic_disport *disport )
{
}

static void tft_mode_set(struct ingenic_disport *disport ,
				struct drm_display_mode *mode)
{
	struct drm_device *dev = disport_to_drm_dev(disport);
	uint32_t disp_com;

	disp_com = reg_read(dev, DC_DISP_COM);
	disp_com &= ~DC_DP_IF_SEL_MASK;
	reg_write(dev, DC_DISP_COM, disp_com | DC_DISP_COM_TFT);
	tft_set_par(disport, mode);
}

struct dpu_dp_ops slcd_dp_ops = {
	.enable = slcd_enable,
	.mode_set = slcd_mode_set,
};

struct dpu_dp_ops tft_dp_ops = {
	.enable = tft_enable,
	.mode_set = tft_mode_set,
};

static void cmp_update_plane(struct ingenic_crtc *crtc)
{
	struct drm_display_mode *mode = &crtc->base.state->adjusted_mode;
	struct ingenic_cmp_priv *priv = crtc->crtc_priv;
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	struct ingenicfb_framedesc *framedesc;
	struct ingenicfb_layerdesc **layerdesc;
	struct ingenic_drm_plane *plane = priv->plane;
	int f_index, j;

	f_index = crtc->frame_next;
	framedesc = priv->framedesc[f_index];
	layerdesc = priv->layerdesc[f_index];

	for(j =0; j < DPU_SUPPORT_LAYER_NUM; j++) {
		if(!plane[j].lay_en)
			continue;
//		dump_cmp_plane(&priv->plane[j]);
		layerdesc[j]->LayerSize.b.width = plane[j].src_w;
		layerdesc[j]->LayerSize.b.height = plane[j].src_h;
		layerdesc[j]->LayerPos.b.x_pos = plane[j].disp_pos_x;
		layerdesc[j]->LayerPos.b.y_pos = plane[j].disp_pos_y;

		layerdesc[j]->LayerCfg.d32 = 0;
		layerdesc[j]->LayerCfg.b.g_alpha_en = plane[j].g_alpha_en;
		layerdesc[j]->LayerCfg.b.g_alpha = plane[j].g_alpha_val;
		layerdesc[j]->LayerCfg.b.color = plane[j].color;
		layerdesc[j]->LayerCfg.b.domain_multi = 1;
		layerdesc[j]->LayerCfg.b.format = plane[j].format;
		layerdesc[j]->LayerCfg.b.sharpl = 0;

		layerdesc[j]->LayerScale.b.target_width = plane[j].scale_w;
		layerdesc[j]->LayerScale.b.target_height = plane[j].scale_h;
		layerdesc[j]->LayerStride = plane[j].stride;
		layerdesc[j]->UVStride = plane[j].uvstride;
		layerdesc[j]->LayerBufferAddr = plane[j].addr_offset[0];
		layerdesc[j]->UVBufferAddr = plane[j].addr_offset[1];
		layerdesc[j]->layerresizecoef_x = (512 * plane[j].src_w) / plane[j].scale_w;
		layerdesc[j]->layerresizecoef_y = (512 * plane[j].src_h) / plane[j].scale_h;
	}

	framedesc->FrameNextCfgAddr = priv->framedesc_phys[f_index];
	framedesc->FrameSize.b.width = mode->hdisplay;
	framedesc->FrameSize.b.height = mode->vdisplay;
	framedesc->FrameCtrl.d32 = 0;
	framedesc->FrameCtrl.b.stop = 0;
	framedesc->FrameCtrl.b.wb_en = 0;
	framedesc->FrameCtrl.b.direct_en = 1;
	framedesc->FrameCtrl.b.change_2_rdma = 0;
	framedesc->FrameCtrl.b.wb_dither_en = 0;
	framedesc->FrameCtrl.b.wb_dither_auto = 0;
	framedesc->FrameCtrl.b.wb_dither_auto = 0;
	framedesc->FrameCtrl.b.wb_dither_b_dw = 0;
	framedesc->FrameCtrl.b.wb_dither_g_dw = 0;
	framedesc->FrameCtrl.b.wb_dither_r_dw = 0;
	framedesc->FrameCtrl.b.wb_format = DC_WB_FORMAT_888;
	framedesc->WritebackAddr = 0;
	framedesc->WritebackStride = mode->hdisplay;

	framedesc->Layer0CfgAddr = priv->layerdesc_phys[f_index][0];
	framedesc->Layer1CfgAddr = priv->layerdesc_phys[f_index][1];
	framedesc->Layer2CfgAddr = priv->layerdesc_phys[f_index][2];
	framedesc->Layer3CfgAddr = priv->layerdesc_phys[f_index][3];

	framedesc->LayCfgEn.b.lay0_en = plane[0].lay_en;
	framedesc->LayCfgEn.b.lay1_en = plane[1].lay_en;
	framedesc->LayCfgEn.b.lay2_en = plane[2].lay_en;
	framedesc->LayCfgEn.b.lay3_en = plane[3].lay_en;

	framedesc->LayCfgEn.b.lay0_scl_en = plane[0].scale_en;
	framedesc->LayCfgEn.b.lay1_scl_en = plane[1].scale_en;
	framedesc->LayCfgEn.b.lay2_scl_en = plane[2].scale_en;
	framedesc->LayCfgEn.b.lay3_scl_en = plane[3].scale_en;

	framedesc->LayCfgEn.b.lay0_z_order = plane[0].zorder;
	framedesc->LayCfgEn.b.lay1_z_order = plane[1].zorder;
	framedesc->LayCfgEn.b.lay2_z_order = plane[2].zorder;
	framedesc->LayCfgEn.b.lay3_z_order = plane[3].zorder;
	framedesc->InterruptControl.d32 = DC_EOC_MSK;

	reg_write(dev, DC_FRM_CFG_ADDR, priv->framedesc_phys[f_index]);

	crtc->frame_current = f_index;
	crtc->frame_next = f_index ? 0 : 1;
}

static int wait_dc_state(struct ingenic_crtc *crtc, uint32_t state, uint32_t flag)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	unsigned long timeout = 20000;
	while(((!(reg_read(dev, DC_ST) & state)) == flag) && timeout) {
		timeout--;
		udelay(10);
	}
	if(timeout <= 0) {
		printk("LCD wait state timeout! state = %d, DC_ST = 0x%x\n", state, DC_ST);
		return -1;
	}
	return 0;
}

static void composer_start(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);

	reg_write(dev, DC_FRM_CFG_CTRL, DC_FRM_START);
}

static void composer_stop(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);

	reg_write(dev, DC_CTRL, DC_QCK_STP_CMP);
	wait_dc_state(crtc, DC_WORKING, 0);
}


static void dpu_common_mode_set(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	struct ingenic_cmp_priv *cmp_priv = crtc->crtc_priv;
	unsigned com_cfg = 0;
	uint32_t disp_com;

	disp_com = reg_read(dev, DC_DISP_COM);
	if(cmp_priv->dither_enable) {
		disp_com |= DC_DP_DITHER_EN;
		disp_com &= ~DC_DP_DITHER_DW_MASK;
		disp_com |= cmp_priv->dither.dither_red
			     << DC_DP_DITHER_DW_RED_LBIT;
		disp_com |= cmp_priv->dither.dither_green
			    << DC_DP_DITHER_DW_GREEN_LBIT;
		disp_com |= cmp_priv->dither.dither_blue
			    << DC_DP_DITHER_DW_BLUE_LBIT;
	} else {
		disp_com &= ~DC_DP_DITHER_EN;
	}
	reg_write(dev, DC_DISP_COM, disp_com);

	com_cfg = reg_read(dev, DC_COM_CONFIG);
	com_cfg &= ~DC_OUT_SEL;
	/* set burst length 32*/
	com_cfg &= ~DC_BURST_LEN_BDMA_MASK;
	com_cfg |= DC_BURST_LEN_BDMA_32;
	com_cfg &= ~DC_BURST_LEN_WDMA_MASK;
	com_cfg |= DC_BURST_LEN_WDMA_32;
	com_cfg &= ~DC_BURST_LEN_RDMA_MASK;
	com_cfg |= DC_BURST_LEN_RDMA_32;
	reg_write(dev, DC_COM_CONFIG, com_cfg);

	reg_write(dev, DC_CLR_ST, 0x01FFFFFE);

}

static void cmp_mode_set(struct ingenic_crtc *crtc)
{

}

static void cmp_enable(struct ingenic_crtc *crtc)
{
	if(crtc->enable)
		return;
	DRM_DEBUG("Composition Display enable\n");
	ingenicfb_clk_enable(&crtc->base);
	ingenic_crtc_update_clk(&crtc->base);
	dpu_common_mode_set(crtc);
	crtc->enable = true;
}

static void cmp_disable(struct ingenic_crtc *crtc)
{
	if(!crtc->enable)
		return;
	composer_stop(crtc);
	ingenicfb_clk_disable(&crtc->base);
	crtc->enable = false;
}

static void cmp_commit(struct ingenic_crtc *crtc)
{
	if(crtc->enable) {
		cmp_update_plane(crtc);
		composer_start(crtc);
//		dpu_dump_all(crtc);
	}
}

static void cmp_enable_vblank(struct ingenic_crtc *crtc, bool enable)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	u32 tmp;

	tmp = reg_read(dev, DC_INTC);
	if (enable) {
		reg_write(dev, DC_CLR_ST, DC_CLR_CMP_END);
		reg_write(dev, DC_INTC, tmp | DC_EOC_MSK);
	} else {
		reg_write(dev, DC_INTC, tmp & ~DC_EOC_MSK);
	}
}

static irqreturn_t cmp_irq_handler(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	uint32_t irq_flag;

	irq_flag = reg_read(dev, DC_INT_FLAG);
	if(likely(irq_flag & DC_CMP_END)) {
		unsigned long flags;
		spin_lock_irqsave(&dev->event_lock, flags);
		if(crtc->event) {
			drm_crtc_send_vblank_event(&crtc->base, crtc->event);
			crtc->event = NULL;
		}
		spin_unlock_irqrestore(&dev->event_lock, flags);
		drm_handle_vblank(dev, crtc->pipe);
		reg_write(dev, DC_CLR_ST, DC_CLR_CMP_END);
		return IRQ_HANDLED;
	}

	if(unlikely(irq_flag & DC_TFT_UNDR)) {
		reg_write(dev, DC_CLR_ST, DC_CLR_TFT_UNDR);
		crtc->tft_under_cnt++;
		printk("\nTFT_UNDR_num = %d\n\n", crtc->tft_under_cnt);
		return IRQ_HANDLED;
	}

	if(likely(irq_flag & DC_WDMA_OVER)) {
		reg_write(dev, DC_CLR_ST, DC_CLR_WDMA_OVER);
		crtc->wover_cnt++;
		printk("\nWRITE_OVER_num = %d\n\n",crtc->wover_cnt);
		return IRQ_HANDLED;
	}

	return IRQ_HANDLED;
}

struct ingenic_dpu_ops dpu_ops = {
	.enable		= cmp_enable,
	.disable	= cmp_disable,
	.enable_vblank	= cmp_enable_vblank,
	.mode_set	= cmp_mode_set,
	.commit		= cmp_commit,
	.irq_handler	= cmp_irq_handler,
};

static void srd_start(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);

	reg_write(dev, DC_RDMA_CHAIN_CTRL, DC_RDMA_START);
}

static void srd_stop(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);

	reg_write(dev, DC_CTRL, DC_QCK_STP_RDMA);
	wait_dc_state(crtc, DC_WORKING, 0);
}


static void srd_enable(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	unsigned com_cfg;
	if(crtc->enable)
		return;
	DRM_DEBUG("Simple read Display enable.\n");
	ingenicfb_clk_enable(&crtc->base);
	ingenic_crtc_update_clk(&crtc->base);

	com_cfg = reg_read(dev, DC_COM_CONFIG);
	com_cfg |= DC_OUT_SEL;
	com_cfg &= ~DC_BURST_LEN_RDMA_MASK;
	com_cfg |= DC_BURST_LEN_RDMA_32;
	reg_write(dev, DC_COM_CONFIG, com_cfg);

	reg_write(dev, DC_CLR_ST, 0x01FFFFFE);
	crtc->enable = true;
}

static void srd_disable(struct ingenic_crtc *crtc)
{
	if(!crtc->enable)
		return;
	srd_stop(crtc);
	ingenicfb_clk_disable(&crtc->base);
	crtc->enable = false;
}

static void srd_mode_set(struct ingenic_crtc *crtc)
{

}

static void srd_enable_vblank(struct ingenic_crtc *crtc, bool enable)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	u32 tmp;

	tmp = reg_read(dev, DC_INTC);
	if (enable) {
		reg_write(dev, DC_CLR_ST, DC_CLR_SRD_END);
		reg_write(dev, DC_INTC, tmp | DC_EOS_MSK);
	} else {
		reg_write(dev, DC_INTC, tmp & ~DC_EOS_MSK);
	}
}

static void srd_update_plane(struct ingenic_crtc *crtc)
{
	struct ingenic_srd_priv *priv = crtc->crtc_priv;
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	struct ingenic_drm_srd_plane *plane = &priv->plane;
	struct ingenicfb_sreadesc *sreadesc;
	int f_index;

	f_index = crtc->frame_next;
	sreadesc = priv->sreadesc[f_index];

	sreadesc->RdmaNextCfgAddr = priv->sreadesc_phys[f_index];
	sreadesc->FrameBufferAddr = plane->addr_offset;
	sreadesc->Stride = plane->stride;
	sreadesc->ChainCfg.d32 = 0;
	sreadesc->ChainCfg.b.format = plane->format;
	sreadesc->ChainCfg.b.color = plane->color;
	sreadesc->ChainCfg.b.change_2_cmp = 0;
	sreadesc->ChainCfg.b.chain_end = 0;
	sreadesc->InterruptControl.d32 = DC_EOS_MSK;

	crtc->frame_current = f_index;
	crtc->frame_next = f_index ? 0 : 1;
	reg_write(dev, DC_RDMA_CHAIN_ADDR, priv->sreadesc_phys[f_index]);
}

static void srd_commit(struct ingenic_crtc *crtc)
{
	if(crtc->enable) {
		srd_update_plane(crtc);
		srd_start(crtc);
//		dump_srd_desc_reg(crtc);
	}
}

static irqreturn_t srd_irq_handler(struct ingenic_crtc *crtc)
{
	struct drm_device *dev = crtc_to_drm_dev(crtc);
	uint32_t irq_flag;

	irq_flag = reg_read(dev, DC_INT_FLAG);

	if(likely(irq_flag & DC_SRD_END)) {
		unsigned long flags;
		spin_lock_irqsave(&dev->event_lock, flags);
		if(crtc->event) {
			drm_crtc_send_vblank_event(&crtc->base, crtc->event);
			crtc->event = NULL;
		}
		spin_unlock_irqrestore(&dev->event_lock, flags);
		drm_handle_vblank(dev, crtc->pipe);
		reg_write(dev, DC_CLR_ST, DC_CLR_SRD_END);
		return IRQ_HANDLED;
	}

	return IRQ_HANDLED;
}

struct ingenic_dpu_ops srd_dpu_ops = {
	.enable		= srd_enable,
	.disable	= srd_disable,
	.enable_vblank	= srd_enable_vblank,
	.mode_set	= srd_mode_set,
	.commit		= srd_commit,
	.irq_handler	= srd_irq_handler,
};

int cmp_register_crtc(struct ingenic_crtc *crtc)
{
	crtc->dpu_ops = &dpu_ops;
	crtc->data_channel = DPU_CMP_OUTPUT_CHANNEL;
	return 0;
}

int srd_register_crtc(struct ingenic_crtc *crtc)
{
	crtc->dpu_ops = &srd_dpu_ops;
	crtc->data_channel = DPU_SIMPLE_READ_CHANNEL;
	return 0;
}

int dpu_register_disport(struct ingenic_disport *disport)
{
	switch(disport->lcd_type) {
	case LCD_TYPE_SLCD:
	case LCD_TYPE_MIPI_SLCD:
		disport->dp_ops = &slcd_dp_ops;
		break;
	case LCD_TYPE_TFT:
	case LCD_TYPE_MIPI_TFT:
		disport->dp_ops = &tft_dp_ops;
		break;
	default:
		break;
	}
	return 0;
}
