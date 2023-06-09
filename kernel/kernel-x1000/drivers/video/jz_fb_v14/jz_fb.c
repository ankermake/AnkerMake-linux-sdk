/*
 * kernel/drivers/video/jz_fb_v14/jz_fb.c
 *
 * Copyright (c) 2012 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Core file for Ingenic Display Controller driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/suspend.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/gpio.h>
#include <asm/cacheflush.h>
#include <mach/platform.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <soc/gpio.h>
#include "jz_fb.h"
#include "dpu_reg.h"

#include "fb_rotate.c"

static int uboot_inited;
static int showFPS = 0;
static struct jzfb *jzfb;

static const struct fb_fix_screeninfo jzfb_fix  = {
	.id = "jzfb",
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR,
	.xpanstep = 0,
	.ypanstep = 1,
	.ywrapstep = 0,
	.accel = FB_ACCEL_NONE,
};

struct jzfb_colormode {
	uint32_t mode;
	uint32_t color;
	uint32_t bits_per_pixel;
	uint32_t nonstd;
	struct fb_bitfield red;
	struct fb_bitfield green;
	struct fb_bitfield blue;
	struct fb_bitfield transp;
};

static struct jzfb_colormode jzfb_colormodes[] = {
	{
		.mode = LAYER_CFG_FORMAT_RGB888,
		.bits_per_pixel = 32,
		.nonstd = 0,
#ifdef CONFIG_ANDROID
		.color = LAYER_CFG_COLOR_BGR,
		.red	= { .length = 8, .offset = 0, .msb_right = 0 },
		.green	= { .length = 8, .offset = 8, .msb_right = 0 },
		.blue	= { .length = 8, .offset = 16, .msb_right = 0 },
#else
		.color = LAYER_CFG_COLOR_RGB,
		.red	= { .length = 8, .offset = 16, .msb_right = 0 },
		.green	= { .length = 8, .offset = 8, .msb_right = 0 },
		.blue	= { .length = 8, .offset = 0, .msb_right = 0 },
#endif
		.transp	= { .length = 0, .offset = 0, .msb_right = 0 },
	}, {
		.mode = LAYER_CFG_FORMAT_ARGB8888,
		.bits_per_pixel = 32,
		.nonstd = 0,
#ifdef CONFIG_ANDROID
		.color = LAYER_CFG_COLOR_BGR,
		.red	= { .length = 8, .offset = 0, .msb_right = 0 },
		.green	= { .length = 8, .offset = 8, .msb_right = 0 },
		.blue	= { .length = 8, .offset = 16, .msb_right = 0 },
#else
		.color = LAYER_CFG_COLOR_RGB,
		.red	= { .length = 8, .offset = 16, .msb_right = 0 },
		.green	= { .length = 8, .offset = 8, .msb_right = 0 },
		.blue	= { .length = 8, .offset = 0, .msb_right = 0 },
#endif
		.transp	= { .length = 8, .offset = 24, .msb_right = 0 },
	}, {
		.mode = LAYER_CFG_FORMAT_RGB555,
		.color = LAYER_CFG_COLOR_RGB,
		.bits_per_pixel = 16,
		.nonstd = 0,
		.red	= { .length = 5, .offset = 10, .msb_right = 0 },
		.green	= { .length = 5, .offset = 5, .msb_right = 0 },
		.blue	= { .length = 5, .offset = 0, .msb_right = 0 },
		.transp	= { .length = 0, .offset = 0, .msb_right = 0 },
	}, {
		.mode = LAYER_CFG_FORMAT_ARGB1555,
		.color = LAYER_CFG_COLOR_RGB,
		.bits_per_pixel = 16,
		.nonstd = 0,
		.red	= { .length = 5, .offset = 10, .msb_right = 0 },
		.green	= { .length = 5, .offset = 5, .msb_right = 0 },
		.blue	= { .length = 5, .offset = 0, .msb_right = 0 },
		.transp	= { .length = 1, .offset = 15, .msb_right = 0 },
	}, {
		.mode = LAYER_CFG_FORMAT_RGB565,
		.color = LAYER_CFG_COLOR_RGB,
		.bits_per_pixel = 16,
		.nonstd = 0,
		.red	= { .length = 5, .offset = 11, .msb_right = 0 },
		.green	= { .length = 6, .offset = 5, .msb_right = 0 },
		.blue	= { .length = 5, .offset = 0, .msb_right = 0 },
		.transp	= { .length = 0, .offset = 0, .msb_right = 0 },
	}, {
		.mode = LAYER_CFG_FORMAT_YUV422,
		.bits_per_pixel = 16,
		.nonstd = LAYER_CFG_FORMAT_YUV422,
	},
};

static void dump_dc_reg(void)
{
	printk("-----------------dc_reg------------------\n");
	printk("DC_FRM_CFG_ADDR:    %lx\n",reg_read(jzfb, DC_FRM_CFG_ADDR));
	printk("DC_FRM_CFG_CTRL:    %lx\n",reg_read(jzfb, DC_FRM_CFG_CTRL));
	printk("DC_CTRL:            %lx\n",reg_read(jzfb, DC_CTRL));
	printk("DC_LAYER0_CSC_MULT_YRV:    %lx\n",reg_read(jzfb, DC_LAYER0_CSC_MULT_YRV));
	printk("DC_LAYER0_CSC_MULT_GUGV:   %lx\n",reg_read(jzfb, DC_LAYER0_CSC_MULT_GUGV));
	printk("DC_LAYER0_CSC_MULT_BU:     %lx\n",reg_read(jzfb, DC_LAYER0_CSC_MULT_BU));
	printk("DC_LAYER0_CSC_SUB_YUV:     %lx\n",reg_read(jzfb, DC_LAYER0_CSC_SUB_YUV));
	printk("DC_LAYER1_CSC_MULT_YRV:    %lx\n",reg_read(jzfb, DC_LAYER1_CSC_MULT_YRV));
	printk("DC_LAYER1_CSC_MULT_GUGV:   %lx\n",reg_read(jzfb, DC_LAYER1_CSC_MULT_GUGV));
	printk("DC_LAYER1_CSC_MULT_BU:     %lx\n",reg_read(jzfb, DC_LAYER1_CSC_MULT_BU));
	printk("DC_LAYER1_CSC_SUB_YUV:     %lx\n",reg_read(jzfb, DC_LAYER1_CSC_SUB_YUV));
	printk("DC_ST:              %lx\n",reg_read(jzfb, DC_ST));
	printk("DC_INTC:            %lx\n",reg_read(jzfb, DC_INTC));
	printk("DC_INT_FLAG:	    %lx\n",reg_read(jzfb, DC_INT_FLAG));
	printk("DC_COM_CONFIG:      %lx\n",reg_read(jzfb, DC_COM_CONFIG));
	printk("DC_PCFG_RD_CTRL:    %lx\n",reg_read(jzfb, DC_PCFG_RD_CTRL));
	printk("DC_OFIFO_PCFG:	    %lx\n",reg_read(jzfb, DC_OFIFO_PCFG));
	printk("DC_DISP_COM:        %lx\n",reg_read(jzfb, DC_DISP_COM));
	printk("-----------------dc_reg------------------\n");
}

static void dump_tft_reg(void)
{
	printk("----------------tft_reg------------------\n");
	printk("TFT_TIMING_HSYNC:   %lx\n",reg_read(jzfb, DC_TFT_HSYNC));
	printk("TFT_TIMING_VSYNC:   %lx\n",reg_read(jzfb, DC_TFT_VSYNC));
	printk("TFT_TIMING_HDE:     %lx\n",reg_read(jzfb, DC_TFT_HDE));
	printk("TFT_TIMING_VDE:     %lx\n",reg_read(jzfb, DC_TFT_VDE));
	printk("TFT_TRAN_CFG:       %lx\n",reg_read(jzfb, DC_TFT_CFG));
	printk("TFT_ST:             %lx\n",reg_read(jzfb, DC_TFT_ST));
	printk("----------------tft_reg------------------\n");
}

static void dump_slcd_reg(void)
{
	printk("---------------slcd_reg------------------\n");
	printk("SLCD_CFG:           %lx\n",reg_read(jzfb, DC_SLCD_CFG));
	printk("SLCD_WR_DUTY:       %lx\n",reg_read(jzfb, DC_SLCD_WR_DUTY));
	printk("SLCD_TIMING:        %lx\n",reg_read(jzfb, DC_SLCD_TIMING));
	printk("SLCD_FRM_SIZE:      %lx\n",reg_read(jzfb, DC_SLCD_FRM_SIZE));
	printk("SLCD_SLOW_TIME:     %lx\n",reg_read(jzfb, DC_SLCD_SLOW_TIME));
	printk("SLCD_CMD:           %lx\n",reg_read(jzfb, DC_SLCD_CMD));
	printk("SLCD_ST:            %lx\n",reg_read(jzfb, DC_SLCD_ST));
	printk("---------------slcd_reg------------------\n");
}

static void dump_frm_desc_reg(void)
{
	unsigned int ctrl;
	ctrl = reg_read(jzfb, DC_CTRL);
	ctrl |= DC_DES_CNT_RST;
	reg_write(jzfb, DC_CTRL, ctrl);

	printk("--------Frame Descriptor register--------\n");
	printk("FrameNextCfgAddr:   %lx\n",reg_read(jzfb, DC_FRM_DES));
	printk("FrameSize:          %lx\n",reg_read(jzfb, DC_FRM_DES));
	printk("FrameCtrl:          %lx\n",reg_read(jzfb, DC_FRM_DES));
	printk("WritebackAddr:      %lx\n",reg_read(jzfb, DC_FRM_DES));
	printk("WritebackStride:    %lx\n",reg_read(jzfb, DC_FRM_DES));
	printk("Layer0CfgAddr:      %lx\n",reg_read(jzfb, DC_FRM_DES));
	printk("Layer1CfgAddr:      %lx\n",reg_read(jzfb, DC_FRM_DES));
	printk("Layer2CfgAddr:      %lx\n",reg_read(jzfb, DC_FRM_DES));
	printk("Layer3CfgAddr:      %lx\n",reg_read(jzfb, DC_FRM_DES));
	printk("LayCfgEn:	    %lx\n",reg_read(jzfb, DC_FRM_DES));
	printk("InterruptControl:   %lx\n",reg_read(jzfb, DC_FRM_DES));
	printk("--------Frame Descriptor register--------\n");
}

static void dump_layer_desc_reg(void)
{
	unsigned int ctrl;
	ctrl = reg_read(jzfb, DC_CTRL);
	ctrl |= DC_DES_CNT_RST;
	reg_write(jzfb, DC_CTRL, ctrl);

	printk("--------layer0 Descriptor register-------\n");
	printk("LayerSize:          %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("LayerCfg:           %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("LayerBufferAddr:    %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("LayerScale:         %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("LayerRotation:      %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("LayerScratch:       %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("LayerPos:           %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("LayerResizeCoef_X:  %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("LayerResizeCoef_Y:  %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("LayerStride:        %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("BufferAddr_UV:      %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("stride_UV:	    %lx\n",reg_read(jzfb, DC_LAY0_DES));
	printk("--------layer0 Descriptor register-------\n");

	printk("--------layer1 Descriptor register-------\n");
	printk("LayerSize:          %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("LayerCfg:           %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("LayerBufferAddr:    %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("LayerScale:         %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("LayerRotation:      %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("LayerScratch:       %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("LayerPos:           %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("LayerResizeCoef_X:  %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("LayerResizeCoef_Y:  %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("LayerStride:        %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("BufferAddr_UV:      %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("stride_UV:	    %lx\n",reg_read(jzfb, DC_LAY1_DES));
	printk("--------layer1 Descriptor register-------\n");
}

static void dump_frm_desc(struct jzfb_framedesc *framedesc, int index)
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

static void dump_layer_desc(struct jzfb_layerdesc *layerdesc, int row, int col)
{
	printk("------User layer Descriptor index[%d][%d]------\n", row, col);
	printk("LayerdescAddr:	    0x%x\n",(uint32_t)layerdesc);
	printk("LayerSize:          0x%x\n",layerdesc->LayerSize.d32);
	printk("LayerCfg:           0x%x\n",layerdesc->LayerCfg.d32);
	printk("LayerBufferAddr:    0x%x\n",layerdesc->LayerBufferAddr);
	printk("LayerScale:         0x%x\n",layerdesc->LayerScale.d32);
	printk("LayerPos:           0x%x\n",layerdesc->LayerPos.d32);
	printk("LayerStride:        0x%x\n",layerdesc->LayerStride);
	printk("------User layer Descriptor index[%d][%d]------\n", row, col);
}

void dump_lay_cfg(struct jzfb_lay_cfg * lay_cfg, int index)
{
	printk("------User disp set index[%d]------\n", index);
	printk("lay_en:		   0x%x\n",lay_cfg->lay_en);
	printk("lay_z_order:	   0x%x\n",lay_cfg->lay_z_order);
	printk("pic_witdh:	   0x%x\n",lay_cfg->pic_width);
	printk("pic_heght:	   0x%x\n",lay_cfg->pic_height);
	printk("disp_pos_x:	   0x%x\n",lay_cfg->disp_pos_x);
	printk("disp_pos_y:	   0x%x\n",lay_cfg->disp_pos_y);
	printk("g_alpha_en:	   0x%x\n",lay_cfg->g_alpha_en);
	printk("g_alpha_val:	   0x%x\n",lay_cfg->g_alpha_val);
	printk("color:		   0x%x\n",lay_cfg->color);
	printk("format:		   0x%x\n",lay_cfg->format);
	printk("stride:		   0x%x\n",lay_cfg->stride);
	printk("------User disp set index[%d]------\n", index);
}

static void dump_lcdc_registers(void)
{
	dump_dc_reg();
	dump_tft_reg();
	dump_slcd_reg();
	dump_frm_desc_reg();
	dump_layer_desc_reg();
}

static void dump_desc(struct jzfb *jzfb)
{
	int i, j;
	for(i = 0; i < MAX_DESC_NUM; i++) {
		for(j = 0; j < MAX_LAYER_NUM; j++) {
			dump_layer_desc(jzfb->layerdesc[i][j], i, j);
		}
		dump_frm_desc(jzfb->framedesc[i], i);
	}
}

static ssize_t
dump_lcd(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct jzfb *jzfb = dev_get_drvdata(dev);
	printk("\nDisp_end_num = %d\n\n", jzfb->irq_cnt);
	printk("\nTFT_UNDR_num = %d\n\n", jzfb->tft_undr_cnt);
	printk("\nFrm_start_num = %d\n\n", jzfb->frm_start);
	printk(" Pand display count=%d\n",jzfb->pan_display_count);
	printk("timestamp.wp = %d , timestamp.rp = %d\n\n", jzfb->timestamp.wp, jzfb->timestamp.rp);
	dump_lcdc_registers();
	dump_desc(jzfb);
	return 0;
}

static void dump_all(struct jzfb *jzfb)
{
	printk("\ndisp_end_num = %d\n\n", jzfb->irq_cnt);
	printk("\nTFT_UNDR_num = %d\n\n", jzfb->tft_undr_cnt);
	printk("\nFrm_start_num = %d\n\n", jzfb->frm_start);
	dump_lcdc_registers();
	dump_desc(jzfb);
}

void jzfb_clk_enable(struct jzfb *jzfb)
{
	if(jzfb->is_clk_en){
		return;
	}
	clk_prepare_enable(jzfb->ahb1);
	clk_prepare_enable(jzfb->pclk);
	clk_prepare_enable(jzfb->clk);
	jzfb->is_clk_en = 1;
}

void jzfb_clk_disable(struct jzfb *jzfb)
{
	if(!jzfb->is_clk_en){
		return;
	}
	jzfb->is_clk_en = 0;
	clk_disable_unprepare(jzfb->clk);
	clk_disable_unprepare(jzfb->pclk);
	clk_disable_unprepare(jzfb->ahb1);
}

static void
jzfb_videomode_to_var(struct jzfb *jzfb, struct fb_var_screeninfo *var,
		const struct fb_videomode *mode, int lcd_type)
{
	var->xres = mode->xres;
	var->yres = mode->yres;
	var->xres_virtual = mode->xres * MAX_LAYER_NUM;
	if (jzfb->fb_copy_type != FB_COPY_TYPE_NONE)
		var->yres_virtual = mode->yres;
	else
		var->yres_virtual = mode->yres * MAX_DESC_NUM;
	var->xoffset = 0;
	var->yoffset = 0;
	var->left_margin = mode->left_margin;
	var->right_margin = mode->right_margin;
	var->upper_margin = mode->upper_margin;
	var->lower_margin = mode->lower_margin;
	var->hsync_len = mode->hsync_len;
	var->vsync_len = mode->vsync_len;
	var->sync = mode->sync;
	var->vmode = mode->vmode & FB_VMODE_MASK;
	var->pixclock = mode->pixclock;
}

static struct fb_videomode *jzfb_get_mode(struct fb_var_screeninfo *var,
		struct fb_info *info)
{
	size_t i;
	struct jzfb *jzfb = info->par;
	struct fb_videomode *mode = jzfb->pdata->modes;

	for (i = 0; i < jzfb->pdata->num_modes; ++i, ++mode) {
		if (mode->xres == var->xres && mode->yres == var->yres
				&& mode->vmode == var->vmode
				&& mode->right_margin == var->right_margin) {
			if (jzfb->pdata->lcd_type != LCD_TYPE_SLCD) {
				if (mode->pixclock == var->pixclock)
					return mode;
			} else {
				return mode;
			}
		}
	}

	return NULL;
}

static int jzfb_check_frm_cfg(struct fb_info *info, struct jzfb_frm_cfg *frm_cfg)
{
	struct fb_var_screeninfo *var = &info->var;
	struct jzfb_lay_cfg *lay_cfg;
	struct fb_videomode *mode;

	mode = jzfb_get_mode(var, info);
	if (mode == NULL) {
		dev_err(info->dev, "%s get video mode failed\n", __func__);
		return -EINVAL;
	}

	lay_cfg = frm_cfg->lay_cfg;

	if(!lay_cfg[0].lay_en && !lay_cfg[1].lay_en) {
		dev_err(info->dev,"%s frame cfg value is err!\n", __func__);
			return -EINVAL;
	}


	if(lay_cfg[0].lay_en) {
		if((lay_cfg[0].lay_z_order == lay_cfg[1].lay_z_order) ||
		   (lay_cfg[0].pic_width > mode->xres) ||
		   (lay_cfg[0].pic_width == 0) ||
		   (lay_cfg[0].pic_height > mode->yres) ||
		   (lay_cfg[0].pic_height == 0) ||
		   (lay_cfg[0].disp_pos_x > mode->xres) ||
		   (lay_cfg[0].disp_pos_y > mode->yres) ||
		   (lay_cfg[0].color > LAYER_CFG_COLOR_BGR) ||
		   (lay_cfg[0].stride > 4096)) {
			dev_err(info->dev,"%s layer[0] cfg value is err!\n",__func__);
			return -EINVAL;
		}

		switch (lay_cfg[0].format) {
		case LAYER_CFG_FORMAT_RGB555:
		case LAYER_CFG_FORMAT_ARGB1555:
		case LAYER_CFG_FORMAT_RGB565:
		case LAYER_CFG_FORMAT_RGB888:
		case LAYER_CFG_FORMAT_ARGB8888:
		case LAYER_CFG_FORMAT_NV12:
		case LAYER_CFG_FORMAT_NV21:
			break;
		default:
			dev_err(info->dev,"%s layer[0] cfg value is err!\n",__func__);
			return -EINVAL;
		}
	}

	if(lay_cfg[1].lay_en) {
		if((lay_cfg[1].lay_z_order == lay_cfg[0].lay_z_order) ||
		   (lay_cfg[1].pic_width > mode->xres) ||
		   (lay_cfg[1].pic_width == 0) ||
		   (lay_cfg[1].pic_height > mode->yres) ||
		   (lay_cfg[1].pic_height == 0) ||
		   (lay_cfg[1].disp_pos_x > mode->xres) ||
		   (lay_cfg[1].disp_pos_y > mode->yres) ||
		   (lay_cfg[1].color > LAYER_CFG_COLOR_BGR) ||
		   (lay_cfg[1].stride > 4096)) {
			printk("%s,%d\n",__func__,lay_cfg[1].stride);
			dev_err(info->dev,"%s layer[1] cfg value is err!\n",__func__);
			return -EINVAL;
		}
		switch (lay_cfg[1].format) {
		case LAYER_CFG_FORMAT_RGB555:
		case LAYER_CFG_FORMAT_ARGB1555:
		case LAYER_CFG_FORMAT_RGB565:
		case LAYER_CFG_FORMAT_RGB888:
		case LAYER_CFG_FORMAT_ARGB8888:
		case LAYER_CFG_FORMAT_NV12:
		case LAYER_CFG_FORMAT_NV21:
			break;
		default:
			dev_err(info->dev,"%s layer[1] cfg value is err!\n",__func__);
			return -EINVAL;
		}
	}

	return 0;
}

static void jzfb_colormode_to_var(struct fb_var_screeninfo *var,
		struct jzfb_colormode *color)
{
	var->bits_per_pixel = color->bits_per_pixel;
	var->nonstd = color->nonstd;
	var->red = color->red;
	var->green = color->green;
	var->blue = color->blue;
	var->transp = color->transp;
}

static bool cmp_var_to_colormode(struct fb_var_screeninfo *var,
		struct jzfb_colormode *color)
{
	bool cmp_component(struct fb_bitfield *f1, struct fb_bitfield *f2)
	{
		return f1->length == f2->length &&
			f1->offset == f2->offset &&
			f1->msb_right == f2->msb_right;
	}

	if (var->bits_per_pixel == 0 ||
			var->red.length == 0 ||
			var->blue.length == 0 ||
			var->green.length == 0)
		return 0;

	return var->bits_per_pixel == color->bits_per_pixel &&
		cmp_component(&var->red, &color->red) &&
		cmp_component(&var->green, &color->green) &&
		cmp_component(&var->blue, &color->blue) &&
		cmp_component(&var->transp, &color->transp);
}

static int jzfb_check_colormode(struct fb_var_screeninfo *var, uint32_t *mode)
{
	int i;

	if (var->nonstd) {
		for (i = 0; i < ARRAY_SIZE(jzfb_colormodes); ++i) {
			struct jzfb_colormode *m = &jzfb_colormodes[i];
			if (var->nonstd == m->nonstd) {
				jzfb_colormode_to_var(var, m);
				*mode = m->mode;
				return 0;
			}
		}

		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(jzfb_colormodes); ++i) {
		struct jzfb_colormode *m = &jzfb_colormodes[i];
		if (cmp_var_to_colormode(var, m)) {
			jzfb_colormode_to_var(var, m);
			*mode = m->mode;
			return 0;
		}
	}

	return -EINVAL;
}

static int jzfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct fb_videomode *mode;
	uint32_t colormode;
	int ret;

	mode = jzfb_get_mode(var, info);
	if (mode == NULL) {
		dev_err(info->dev, "%s get video mode failed\n", __func__);
		return -EINVAL;
	}

	jzfb_videomode_to_var(jzfb, var, mode, jzfb->pdata->lcd_type);

	ret = jzfb_check_colormode(var, &colormode);
	if(ret) {
		dev_err(info->dev,"Check colormode failed!\n");
		return  ret;
	}

	return 0;
}

static void slcd_send_mcu_command(struct jzfb *jzfb, unsigned long cmd)
{
	int count = 10000;
	uint32_t slcd_cfg;

	while ((reg_read(jzfb, DC_SLCD_ST) & DC_SLCD_ST_BUSY) && count--) {
		udelay(10);
	}
	if (count < 0) {
		dev_err(jzfb->dev, "SLCDC wait busy state wrong");
	}

	slcd_cfg = reg_read(jzfb, DC_SLCD_CFG);
	reg_write(jzfb, DC_SLCD_CFG, (slcd_cfg & ~DC_FMT_EN));
	reg_write(jzfb, DC_SLCD_CMD, DC_SLCD_CMD_FLAG_CMD | (cmd & ~DC_SLCD_CMD_FLAG_MASK));
}

static void slcd_send_mcu_data(struct jzfb *jzfb, unsigned long data)
{
	int count = 10000;
	uint32_t slcd_cfg;

	while ((reg_read(jzfb, DC_SLCD_ST) & DC_SLCD_ST_BUSY) && count--) {
		udelay(10);
	}
	if (count < 0) {
		dev_err(jzfb->dev, "SLCDC wait busy state wrong");
	}

	slcd_cfg = reg_read(jzfb, DC_SLCD_CFG);
	reg_write(jzfb, DC_SLCD_CFG, (slcd_cfg | DC_FMT_EN));
	reg_write(jzfb, DC_SLCD_CMD, DC_SLCD_CMD_FLAG_DATA | (data & ~DC_SLCD_CMD_FLAG_MASK));
}

static void slcd_send_mcu_prm(struct jzfb *jzfb, unsigned long data)
{
	int count = 10000;
	uint32_t slcd_cfg;

	while ((reg_read(jzfb, DC_SLCD_ST) & DC_SLCD_ST_BUSY) && count--) {
		udelay(10);
	}
	if (count < 0) {
		dev_err(jzfb->dev, "SLCDC wait busy state wrong");
	}

	slcd_cfg = reg_read(jzfb, DC_SLCD_CFG);
	reg_write(jzfb, DC_SLCD_CFG, (slcd_cfg & ~DC_FMT_EN));
	reg_write(jzfb, DC_SLCD_CMD, DC_SLCD_CMD_FLAG_PRM | (data & ~DC_SLCD_CMD_FLAG_MASK));
}

static void wait_slcd_busy(void)
{
	int count = 100000;
	while ((reg_read(jzfb, DC_SLCD_ST) & DC_SLCD_ST_BUSY)
			&& count--) {
		udelay(10);
	}
	if (count < 0) {
		dev_err(jzfb->dev,"SLCDC wait busy state wrong");
	}
}

static int wait_dc_state(uint32_t state, uint32_t flag)
{
	unsigned long timeout = 20000;
	while(((!(reg_read(jzfb, DC_ST) & state)) == flag) && timeout) {
		timeout--;
		udelay(10);
	}
	if(timeout <= 0) {
		printk("LCD wait state timeout! state = %d, DC_ST = 0x%x\n", state, DC_ST);
		return -1;
	}
	return 0;
}

static void jzfb_slcd_mcu_init(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct jzfb_platform_data *pdata = jzfb->pdata;
	struct jzfb_smart_config *smart_config;
	struct smart_lcd_data_table *data_table;
	uint32_t length_data_table;
	uint32_t i;

	smart_config = pdata->smart_config;
	data_table = smart_config->data_table;
	length_data_table = smart_config->length_data_table;
	if (pdata->lcd_type != LCD_TYPE_SLCD)
		return;

	if(length_data_table && data_table) {
		for(i = 0; i < length_data_table; i++) {
			switch (data_table[i].type) {
			case SMART_CONFIG_DATA:
				slcd_send_mcu_data(jzfb, data_table[i].value);
				break;
			case SMART_CONFIG_PRM:
				slcd_send_mcu_prm(jzfb, data_table[i].value);
				break;
			case SMART_CONFIG_CMD:
				slcd_send_mcu_command(jzfb, data_table[i].value);
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

static void jzfb_cmp_start(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;

	if(!(reg_read(jzfb, DC_ST) & DC_FRM_WORKING)) {
		reg_write(jzfb, DC_FRM_CFG_CTRL, DC_FRM_START);
	} else {
		dev_err(jzfb->dev, "Composer has enabled.\n");
	}
}

static void jzfb_tft_start(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;

	if(!(reg_read(jzfb, DC_TFT_ST) & DC_TFT_WORKING)) {
		reg_write(jzfb, DC_CTRL, DC_TFT_START);
	} else {
		dev_err(jzfb->dev, "TFT has enabled.\n");
	}
}

static void jzfb_slcd_start(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;

	if(!(reg_read(jzfb, DC_SLCD_ST) & DC_SLCD_ST_BUSY)) {
		reg_write(jzfb, DC_CTRL, DC_SLCD_START);
	} else {
		dev_err(jzfb->dev, "Slcd has enabled.\n");
	}
}

static void jzfb_gen_stp_cmp(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;

	reg_write(jzfb, DC_CTRL, DC_GEN_STP_CMP);
}

static void jzfb_qck_stp_cmp(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;

	reg_write(jzfb, DC_CTRL, DC_QCK_STP_CMP);
}

static void tft_timing_init(struct fb_videomode *modes) {
	uint32_t hps, hpe, vps, vpe;
	uint32_t hds, hde, vds, vde;
	uint32_t xres, yres;

	/* buffer xy changed, timing not change */
	if (jzfb->change_xy) {
		xres = modes->yres;
		yres = modes->xres;
	} else {
		xres = modes->xres;
		yres = modes->yres;
	}

	hps = modes->hsync_len;
	hpe = hps + modes->left_margin + xres + modes->right_margin;
	vps = modes->vsync_len;
	vpe = vps + modes->upper_margin + yres + modes->lower_margin;

	hds = modes->hsync_len + modes->left_margin;
	hde = hds + xres;
	vds = modes->vsync_len + modes->upper_margin;
	vde = vds + yres;

	reg_write(jzfb, DC_TFT_HSYNC,
		  (hps << DC_HPS_LBIT) |
		  (hpe << DC_HPE_LBIT));
	reg_write(jzfb, DC_TFT_VSYNC,
		  (vps << DC_VPS_LBIT) |
		  (vpe << DC_VPE_LBIT));
	reg_write(jzfb, DC_TFT_HDE,
		  (hds << DC_HDS_LBIT) |
		  (hde << DC_HDE_LBIT));
	reg_write(jzfb, DC_TFT_VDE,
		  (vds << DC_VDS_LBIT) |
		  (vde << DC_VDE_LBIT));
}

void tft_cfg_init(struct jzfb_tft_config *tft_config) {
	uint32_t tft_cfg;

	tft_cfg = reg_read(jzfb, DC_TFT_CFG);
	if(tft_config->pix_clk_inv) {
		tft_cfg |= DC_PIX_CLK_INV;
	} else {
		tft_cfg &= ~DC_PIX_CLK_INV;
	}

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
	switch(tft_config->mode) {
	case TFT_LCD_MODE_PARALLEL_24B:
		tft_cfg |= DC_MODE_PARALLEL_24BIT;
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
	reg_write(jzfb, DC_TFT_CFG, tft_cfg);
}

static int jzfb_tft_set_par(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct jzfb_platform_data *pdata = jzfb->pdata;
	struct lcd_callback_ops *callback_ops;
	struct lcdc_gpio_struct *gpio_assign;
	struct fb_videomode *mode;

	callback_ops = pdata->lcd_callback_ops;
	gpio_assign = pdata->gpio_assign;
	mode = jzfb_get_mode(&info->var, info);
	if (mode == NULL) {
		dev_err(info->dev, "%s get video mode failed\n", __func__);
		return -EINVAL;
	}

	tft_timing_init(mode);
	tft_cfg_init(pdata->tft_config);
	if(callback_ops && callback_ops->lcd_power_on_begin)
		callback_ops->lcd_power_on_begin(gpio_assign);

	return 0;
}

static void slcd_cfg_init(struct jzfb_smart_config *smart_config) {
	uint32_t slcd_cfg;

	slcd_cfg = reg_read(jzfb, DC_SLCD_CFG);

	if(smart_config->frm_md)
		slcd_cfg |= DC_FRM_MD;
	else
		slcd_cfg &= ~DC_FRM_MD;

	if(smart_config->rdy_switch) {
		slcd_cfg |= DC_RDY_SWITCH;

		if(smart_config->rdy_dp) {
			slcd_cfg |= DC_RDY_DP;
		} else {
			slcd_cfg &= ~DC_RDY_DP;
		}
		if(smart_config->rdy_anti_jit) {
			slcd_cfg |= DC_RDY_ANTI_JIT;
		} else {
			slcd_cfg &= ~DC_RDY_ANTI_JIT;
		}
	} else {
		slcd_cfg &= ~DC_RDY_SWITCH;
	}

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

	if(smart_config->cs_en) {
		slcd_cfg |= DC_CS_EN;

		if(smart_config->cs_dp) {
			slcd_cfg |= DC_CS_DP;
		} else {
			slcd_cfg &= ~DC_CS_DP;
		}
	} else {
		slcd_cfg &= ~DC_CS_EN;
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

	reg_write(jzfb, DC_SLCD_CFG, slcd_cfg);

	return;
}

static int slcd_timing_init(struct jzfb_platform_data *jzfb_pdata)
{
	uint32_t width = jzfb_pdata->width;
	uint32_t height = jzfb_pdata->height;
	uint32_t dhtime = 0;
	uint32_t dltime = 0;
	uint32_t chtime = 0;
	uint32_t cltime = 0;
	uint32_t tah = 0;
	uint32_t tas = 0;
	uint32_t slowtime = 0;

	/*frm_size*/
	reg_write(jzfb, DC_SLCD_FRM_SIZE,
		  ((width << DC_SLCD_FRM_H_SIZE_LBIT) |
		   (height << DC_SLCD_FRM_V_SIZE_LBIT)));

	/* wr duty */
	reg_write(jzfb, DC_SLCD_WR_DUTY,
		  ((dhtime << DC_DSTIME_LBIT) |
		   (dltime << DC_DDTIME_LBIT) |
		   (chtime << DC_CSTIME_LBIT) |
		   (cltime << DC_CDTIME_LBIT)));

	/* slcd timing */
	reg_write(jzfb, DC_SLCD_TIMING,
		  ((tah << DC_TAH_LBIT) |
		  (tas << DC_TAS_LBIT)));

	/* slow time */
	reg_write(jzfb, DC_SLCD_SLOW_TIME, slowtime);

	return 0;
}

static int jzfb_slcd_set_par(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct jzfb_platform_data *pdata = jzfb->pdata;
	struct lcd_callback_ops *callback_ops;
	struct lcdc_gpio_struct *gpio_assign;

	callback_ops = pdata->lcd_callback_ops;
	gpio_assign = pdata->gpio_assign;

	slcd_cfg_init(pdata->smart_config);
	slcd_timing_init(pdata);

	if(callback_ops && callback_ops->lcd_power_on_begin)
		    callback_ops->lcd_power_on_begin(gpio_assign);

	jzfb_slcd_mcu_init(info);

	return 0;
}

static int csc_mode_set(struct fb_info *info, jzfb_layer_csc_t mode) {
	uint32_t MultYRv, MultGuGv, MultBu, SubYUV;

	switch(mode.csc_mode) {
		case CSC_MODE_0:
			MultYRv = DC_CSC_MULT_Y_MD0 | DC_CSC_MULT_RV_MD0;
			MultGuGv = DC_CSC_MULT_GU_MD0 | DC_CSC_MULT_GV_MD0;
			MultBu = DC_CSC_MULT_BU_MD0;
			SubYUV = DC_CSC_SUB_Y_MD0 | DC_CSC_SUB_UV_MD0;
			break;
		case CSC_MODE_1:
			MultYRv = DC_CSC_MULT_Y_MD1 | DC_CSC_MULT_RV_MD1;
			MultGuGv = DC_CSC_MULT_GU_MD1 | DC_CSC_MULT_GV_MD1;
			MultBu = DC_CSC_MULT_BU_MD1;
			SubYUV = DC_CSC_SUB_Y_MD1 | DC_CSC_SUB_UV_MD1;
			break;
		case CSC_MODE_2:
			MultYRv = DC_CSC_MULT_Y_MD2 | DC_CSC_MULT_RV_MD2;
			MultGuGv = DC_CSC_MULT_GU_MD2 | DC_CSC_MULT_GV_MD2;
			MultBu = DC_CSC_MULT_BU_MD2;
			SubYUV = DC_CSC_SUB_Y_MD2 | DC_CSC_SUB_UV_MD2;
			break;
		case CSC_MODE_3:
			MultYRv = DC_CSC_MULT_Y_MD3 | DC_CSC_MULT_RV_MD3;
			MultGuGv = DC_CSC_MULT_GU_MD3 | DC_CSC_MULT_GV_MD3;
			MultBu = DC_CSC_MULT_BU_MD3;
			SubYUV = DC_CSC_SUB_Y_MD3 | DC_CSC_SUB_UV_MD3;
			break;
		default:
			dev_err(info->dev, "%s: layer(%d) set csc mode failed\n", __func__, mode.layer);
			return -EINVAL;
	}

	switch(mode.layer) {
		case 0:
			reg_write(info->par, DC_LAYER0_CSC_MULT_YRV, MultYRv);
			reg_write(info->par, DC_LAYER0_CSC_MULT_GUGV, MultGuGv);
			reg_write(info->par, DC_LAYER0_CSC_MULT_BU, MultBu);
			reg_write(info->par, DC_LAYER0_CSC_SUB_YUV, SubYUV);
			break;
		case 1:
			reg_write(info->par, DC_LAYER1_CSC_MULT_YRV, MultYRv);
			reg_write(info->par, DC_LAYER1_CSC_MULT_GUGV, MultGuGv);
			reg_write(info->par, DC_LAYER1_CSC_MULT_BU, MultBu);
			reg_write(info->par, DC_LAYER1_CSC_SUB_YUV, SubYUV);
			break;
		default:
			dev_err(info->dev, "%s: layer(%d) don't exit\n", __func__, mode.layer);
			return -EINVAL;
	}

	return 0;
}

static void jzfb_map_kernel_mem(struct jzfb *jzfb, int buffer_size)
{
	struct vm_struct	*kvma;
	struct vm_area_struct	vma;
	pgprot_t		pgprot;
	unsigned long		vaddr;
    unsigned int size = buffer_size;
    unsigned long fb_mem_phys = jzfb->vidmem_phys[0][0];

	kvma = get_vm_area(size, VM_IOREMAP);

	vma.vm_mm = &init_mm;

	vaddr = (unsigned long)kvma->addr;
	vma.vm_start = vaddr;
	vma.vm_end = vaddr + size;

	pgprot = PAGE_KERNEL;
    pgprot_val(pgprot) &= ~_CACHE_MASK;
    pgprot_val(pgprot) |= 0;

	if (io_remap_pfn_range(&vma, vaddr,
			   fb_mem_phys >> PAGE_SHIFT,
			   size, pgprot) < 0) {
		printk("kernel mmap for FB memory failed\n");
		jzfb->fb_mem_mapped = NULL;
		return;
	}

    jzfb->fb_mem_mapped = (void *)vaddr;
}

static int jzfb_alloc_devmem(struct jzfb *jzfb)
{
	struct fb_videomode *mode;
	struct fb_info *fb = jzfb->fb;
	uint32_t buff_size;
	void *page;
	uint8_t *addr;
	dma_addr_t addr_phy;
	int i, j;

	mode = jzfb->pdata->modes;
	if (!mode) {
		dev_err(jzfb->dev, "Checkout video mode fail\n");
		return -EINVAL;
	}

	buff_size = sizeof(struct jzfb_framedesc);
	buff_size = ALIGN(buff_size, DESC_ALIGN);
	addr = dma_alloc_coherent(jzfb->dev, buff_size * MAX_DESC_NUM,
				  &addr_phy, GFP_KERNEL);
	if(addr == NULL) {
		return -ENOMEM;
	}
	for(i = 0; i < MAX_DESC_NUM; i++) {
		jzfb->framedesc[i] =
			(struct jzfb_framedesc *)(addr + i * buff_size);
		jzfb->framedesc_phys[i] = addr_phy + i * buff_size;
	}

	buff_size = sizeof(struct jzfb_layerdesc);
	buff_size = ALIGN(buff_size, DESC_ALIGN);
	addr = dma_alloc_coherent(jzfb->dev, buff_size * MAX_DESC_NUM * MAX_LAYER_NUM,
				  &addr_phy, GFP_KERNEL);
	if(addr == NULL) {
		return -ENOMEM;
	}
	for(j = 0; j < MAX_DESC_NUM; j++) {
		for(i = 0; i < MAX_LAYER_NUM; i++) {
			jzfb->layerdesc[j][i] = (struct jzfb_layerdesc *)
				(addr + i * buff_size + j * buff_size * MAX_LAYER_NUM);
			jzfb->layerdesc_phys[j][i] =
				addr_phy + i * buff_size + j * buff_size * MAX_LAYER_NUM;
		}
	}

	buff_size = mode->xres * mode->yres;
	jzfb->frm_size = buff_size * fb->var.bits_per_pixel >> 3;
	buff_size *= MAX_BITS_PER_PIX >> 3;

	buff_size = buff_size *  MAX_DESC_NUM * MAX_LAYER_NUM;
	jzfb->vidmem_size = PAGE_ALIGN(buff_size);

	jzfb->vidmem[0][0] = dma_alloc_coherent(jzfb->dev, jzfb->vidmem_size,
					  &jzfb->vidmem_phys[0][0], GFP_KERNEL);
	if(jzfb->vidmem[0][0] == NULL) {
		return -ENOMEM;
	}
	for (page = jzfb->vidmem[0][0];
	     page < jzfb->vidmem[0][0] + PAGE_ALIGN(jzfb->vidmem_size);
	     page += PAGE_SIZE) {
		SetPageReserved(virt_to_page(page));
	}

	if (jzfb->fb_copy_type != FB_COPY_TYPE_NONE) {
		jzfb->user_vidmem_size = jzfb->frm_size;
		jzfb->user_vidmem = kmalloc(jzfb->user_vidmem_size, GFP_KERNEL);
		jzfb->user_vidmem_phys = virt_to_phys(jzfb->user_vidmem);
		jzfb_map_kernel_mem(jzfb, buff_size * MAX_DESC_NUM);
	} else {
		jzfb->user_vidmem_size = jzfb->vidmem_size;
		jzfb->user_vidmem = jzfb->vidmem[0][0];
		jzfb->user_vidmem_phys = jzfb->vidmem_phys[0][0];
	}

	return 0;
}

#define DPU_WAIT_IRQ_TIME 2000
static int jzfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct jzfb *jzfb = info->par;
	int ret;
	jzfb_layer_csc_t csc_mode;
	int value;
	int tmp;
	int i;

	switch (cmd) {
		case JZFB_CMP_START:
			jzfb_cmp_start(info);
			break;
		case JZFB_TFT_START:
			jzfb_tft_start(info);
			break;
		case JZFB_SLCD_START:
			slcd_send_mcu_command(jzfb, jzfb->pdata->smart_config->write_gram_cmd);
			wait_slcd_busy();
			jzfb_slcd_start(info);
			break;
		case JZFB_GEN_STP_CMP:
			jzfb_gen_stp_cmp(info);
			wait_dc_state(DC_WORKING, 0);
			break;
		case JZFB_QCK_STP_CMP:
			jzfb_qck_stp_cmp(info);
			wait_dc_state(DC_WORKING, 0);
			break;
		case JZFB_DUMP_LCDC_REG:
			dump_all(info->par);
			break;
		case JZFB_SET_VSYNCINT:
			if (unlikely(copy_from_user(&value, argp, sizeof(int))))
				return -EFAULT;
			tmp = reg_read(jzfb, DC_INTC);
			if (value) {
				if(jzfb->pdata->lcd_type == LCD_TYPE_SLCD &&
						!jzfb->slcd_continua) {
					reg_write(jzfb, DC_CLR_ST, DC_CLR_DISP_END);
					reg_write(jzfb, DC_INTC, tmp | DC_EOD_MSK);
				} else {
					reg_write(jzfb, DC_CLR_ST, DC_CLR_FRM_START);
					reg_write(jzfb, DC_INTC, tmp | DC_SOF_MSK);
				}
			} else {
				if(jzfb->pdata->lcd_type == LCD_TYPE_SLCD &&
						!jzfb->slcd_continua) {
					reg_write(jzfb, DC_INTC, tmp & ~DC_EOD_MSK);
				} else {
					reg_write(jzfb, DC_INTC, tmp & ~DC_SOF_MSK);
				}
			}
			break;
		case FBIO_WAITFORVSYNC:
			unlock_fb_info(info);
			ret = wait_event_interruptible_timeout(jzfb->vsync_wq,
					jzfb->timestamp.wp != jzfb->timestamp.rp,
					msecs_to_jiffies(DPU_WAIT_IRQ_TIME));
			lock_fb_info(info);
			if(ret == 0) {
				dev_err(info->dev, "DPU wait vsync timeout!\n");
				return -EFAULT;
			}

			ret = copy_to_user(argp, jzfb->timestamp.value + jzfb->timestamp.rp,
					sizeof(u64));
			jzfb->timestamp.rp = (jzfb->timestamp.rp + 1) % TIMESTAMP_CAP;

			if (unlikely(ret))
				return -EFAULT;
			break;
		case JZFB_PUT_FRM_CFG:
			ret = jzfb_check_frm_cfg(info, (struct jzfb_frm_cfg *)argp);
			if(ret) {
				return ret;
			}
			copy_from_user(&jzfb->current_frm_mode.frm_cfg,
				       (void *)argp,
				       sizeof(struct jzfb_frm_cfg));

			for (i = 0; i < ARRAY_SIZE(jzfb_colormodes); ++i) {
				struct jzfb_colormode *m = &jzfb_colormodes[i];
				if (m->mode == jzfb->current_frm_mode.frm_cfg.lay_cfg[0].format) {
					jzfb_colormode_to_var(&info->var, m);
					break;
				}
			}

			for(i = 0; i < MAX_DESC_NUM; i++) {
				jzfb->current_frm_mode.update_st[i] = FRAME_CFG_NO_UPDATE;
			}
			break;
		case JZFB_GET_FRM_CFG:
			copy_to_user((void *)argp,
				      &jzfb->current_frm_mode.frm_cfg,
				      sizeof(struct jzfb_frm_cfg));
			break;
		case JZFB_SET_CSC_MODE:
			if (unlikely(copy_from_user(&csc_mode, argp, sizeof(jzfb_layer_csc_t))))
				return -EFAULT;
			ret = csc_mode_set(info, csc_mode);
			if(ret)
				return -EFAULT;
			break;
	       case JZFB_GET_PHY_FRAMEBUF_BASE:
		copy_to_user((void *)argp,
			     &jzfb->fb->fix.smem_start,
			     sizeof(unsigned int));
		        break;
		default:
			printk("Command:%x Error!\n",cmd);
			break;
	}
	return 0;
}

static int jzfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct jzfb *jzfb = info->par;
	unsigned long start;
	unsigned long off;
	u32 len;

	off = vma->vm_pgoff << PAGE_SHIFT;

	/* frame buffer memory */
	start = jzfb->fb->fix.smem_start;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + jzfb->fb->fix.smem_len);
	start &= PAGE_MASK;

	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;

	vma->vm_pgoff = off >> PAGE_SHIFT;
	vma->vm_flags |= VM_IO;

	pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
	/* Write-Acceleration */
	//pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_WA;

	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
				vma->vm_end - vma->vm_start, vma->vm_page_prot))
	{
		return -EAGAIN;
	}

	return 0;
}

static void jzfb_set_vsync_value(struct jzfb *jzfb)
{
	jzfb->vsync_skip_map = (jzfb->vsync_skip_map >> 1 |
				jzfb->vsync_skip_map << 9) & 0x3ff;
	if(likely(jzfb->vsync_skip_map & 0x1)) {
		jzfb->timestamp.value[jzfb->timestamp.wp] =
			ktime_to_ns(ktime_get());
		jzfb->timestamp.wp = (jzfb->timestamp.wp + 1) % TIMESTAMP_CAP;
		wake_up_interruptible(&jzfb->vsync_wq);
	}
}

static int jzfb_update_frm_msg(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct jzfb_frmdesc_msg *desc_msg_done;
	struct jzfb_frmdesc_msg *desc_msg_reday;

	if(list_empty(&jzfb->desc_run_list)) {
		printk(KERN_DEBUG "%s %d dpu irq run list empty\n", __func__, __LINE__);
		return 0;
	}
	desc_msg_done = list_first_entry(&jzfb->desc_run_list,
			struct jzfb_frmdesc_msg, list);
	if(desc_msg_done->state != DESC_ST_RUNING) {
		dev_err(info->dev, "%s Desc state is err!!\n", __func__);
	}
	list_del(&desc_msg_done->list);
	desc_msg_done->state = DESC_ST_AVAILABLE;
#if 0
	// when close frm end.
	while(!list_empty(&jzfb->desc_run_list)){
		struct jzfb_frmdesc_msg *next;
		next = list_entry(desc_msg_done->list.next,
			struct jzfb_frmdesc_msg, list);

		list_del(&desc_msg_done->list);
		desc_msg_done->state = DESC_ST_AVAILABLE;
		if(desc_msg_done->addr_virt->FrameCtrl.b.stop == 0)
			desc_msg_done = next;
		else
			break;
	}
#endif
	jzfb->vsync_skip_map = (jzfb->vsync_skip_map >> 1 |
				jzfb->vsync_skip_map << 9) & 0x3ff;
	if(likely(jzfb->vsync_skip_map & 0x1)) {
		jzfb->timestamp.value[jzfb->timestamp.wp] =
			ktime_to_ns(ktime_get());
		jzfb->timestamp.wp = (jzfb->timestamp.wp + 1) % TIMESTAMP_CAP;
		wake_up_interruptible(&jzfb->vsync_wq);
	}
	// restart  slcd
	if(!(list_empty(&jzfb->desc_run_list))) {
		desc_msg_reday = list_first_entry(&jzfb->desc_run_list,
				struct jzfb_frmdesc_msg, list);
		reg_write(jzfb, DC_FRM_CFG_ADDR, desc_msg_reday->addr_phy);
		jzfb_cmp_start(info);
		jzfb_slcd_start(info);
	}
	complete(&jzfb->frm_msg_complete);
	return 0;
}

static irqreturn_t jzfb_irq_handler(int irq, void *data)
{
	unsigned int irq_flag;
	struct jzfb *jzfb = (struct jzfb *)data;

	spin_lock(&jzfb->irq_lock);
	irq_flag = reg_read(jzfb, DC_INT_FLAG);
	if(likely(irq_flag & DC_ST_FRM_START)) {
		reg_write(jzfb, DC_CLR_ST, DC_CLR_FRM_START);
		jzfb->frm_start++;

		jzfb->next_frm_is_done = 1;

		if((jzfb->pdata->lcd_type == LCD_TYPE_SLCD && jzfb->slcd_continua) ||
				jzfb->pdata->lcd_type == LCD_TYPE_TFT) {
			jzfb_set_vsync_value(jzfb);
		}
		spin_unlock(&jzfb->irq_lock);
		return IRQ_HANDLED;
	}

	if(likely(irq_flag & DC_DISP_END)) {
		reg_write(jzfb, DC_CLR_ST, DC_CLR_DISP_END);
		jzfb->irq_cnt++;
		if(jzfb->pdata->lcd_type == LCD_TYPE_SLCD && !jzfb->slcd_continua) {
			jzfb_update_frm_msg(jzfb->fb);
		}

		spin_unlock(&jzfb->irq_lock);
		return IRQ_HANDLED;
	}

	if(unlikely(irq_flag & DC_STOP_DISP_ACK)) {
		reg_write(jzfb, DC_CLR_ST, DC_CLR_STOP_DISP_ACK);
		wake_up_interruptible(&jzfb->gen_stop_wq);
		spin_unlock(&jzfb->irq_lock);
		return IRQ_HANDLED;
	}

	if(unlikely(irq_flag & DC_TFT_UNDR)) {
		reg_write(jzfb, DC_CLR_ST, DC_CLR_TFT_UNDR);
		jzfb->tft_undr_cnt++;
		printk("\nTFT_UNDR_num = %d\n\n", jzfb->tft_undr_cnt);
		spin_unlock(&jzfb->irq_lock);
		return IRQ_HANDLED;
	}

	dev_err(jzfb->dev, "DPU irq nothing do, please check!!!\n");
	spin_unlock(&jzfb->irq_lock);
	return IRQ_HANDLED;
}

static inline uint32_t convert_color_to_hw(unsigned val, struct fb_bitfield *bf)
{
	return (((val << bf->length) + 0x7FFF - val) >> 16) << bf->offset;
}

static int jzfb_setcolreg(unsigned regno, unsigned red, unsigned green,
		unsigned blue, unsigned transp, struct fb_info *fb)
{
	if (regno >= 16)
		return -EINVAL;

	((uint32_t *)(fb->pseudo_palette))[regno] =
		convert_color_to_hw(red, &fb->var.red) |
		convert_color_to_hw(green, &fb->var.green) |
		convert_color_to_hw(blue, &fb->var.blue) |
		convert_color_to_hw(transp, &fb->var.transp);

	return 0;
}

static void jzfb_display_v_color_bar(struct fb_info *info)
{
	int i, j;
	int w, h;
	int bpp;
	unsigned short *p16;
	unsigned int *p32;
	struct jzfb *jzfb = info->par;
	struct fb_videomode *mode = jzfb->pdata->modes;

	if (!mode) {
		dev_err(jzfb->dev, "%s, video mode is NULL\n", __func__);
		return;
	}
	if (!jzfb->vidmem_phys[jzfb->current_frm_desc][0]) {
		dev_err(jzfb->dev, "Not allocate frame buffer yet\n");
		return;
	}
	if (!jzfb->vidmem[jzfb->current_frm_desc][0])
		jzfb->vidmem[jzfb->current_frm_desc][0] =
			(void *)phys_to_virt(jzfb->vidmem_phys[jzfb->current_frm_desc][0]);
	p16 = (unsigned short *)jzfb->vidmem[jzfb->current_frm_desc][0];
	p32 = (unsigned int *)jzfb->vidmem[jzfb->current_frm_desc][0];
	w = mode->xres;
	h = mode->yres;
	bpp = info->var.bits_per_pixel;

	dev_info(info->dev,
			"LCD V COLOR BAR w,h,bpp(%d,%d,%d) jzfb->vidmem[0]=%p\n", w, h,
			bpp, jzfb->vidmem[jzfb->current_frm_desc][0]);

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			short c16;
			int c32 = 0;
			switch ((j / 10) % 4) {
				case 0:
					c16 = 0xF800;
					c32 = 0xFFFF0000;
					break;
				case 1:
					c16 = 0x07C0;
					c32 = 0xFF00FF00;
					break;
				case 2:
					c16 = 0x001F;
					c32 = 0xFF0000FF;
					break;
				default:
					c16 = 0xFFFF;
					c32 = 0xFFFFFFFF;
					break;
			}
			switch (bpp) {
				case 18:
				case 24:
				case 32:
					*p32++ = c32;
					break;
				default:
					*p16++ = c16;
			}
		}
		if (w % PIXEL_ALIGN) {
			switch (bpp) {
				case 18:
				case 24:
				case 32:
					p32 += (ALIGN(mode->xres, PIXEL_ALIGN) - w);
					break;
				default:
					p16 += (ALIGN(mode->xres, PIXEL_ALIGN) - w);
					break;
			}
		}
	}
}

static void jzfb_display_h_color_bar(struct fb_info *info)
{
	int i, j;
	int w, h;
	int bpp;
	unsigned short *p16;
	unsigned int *p32;
	struct jzfb *jzfb = info->par;
	struct fb_videomode *mode = jzfb->pdata->modes;

	if (!mode) {
		dev_err(jzfb->dev, "%s, video mode is NULL\n", __func__);
		return;
	}
	if (!jzfb->vidmem_phys[jzfb->current_frm_desc][0]) {
		dev_err(jzfb->dev, "Not allocate frame buffer yet\n");
		return;
	}
	if (!jzfb->vidmem[jzfb->current_frm_desc][0])
		jzfb->vidmem[jzfb->current_frm_desc][0] =
			(void *)phys_to_virt(jzfb->vidmem_phys[jzfb->current_frm_desc][0]);
	p16 = (unsigned short *)jzfb->vidmem[jzfb->current_frm_desc][0];
	p32 = (unsigned int *)jzfb->vidmem[jzfb->current_frm_desc][0];
	w = mode->xres;
	h = mode->yres;
	bpp = info->var.bits_per_pixel;

	dev_info(info->dev,
			"LCD H COLOR BAR w,h,bpp(%d,%d,%d), jzfb->vidmem[0]=%p\n", w, h,
			bpp, jzfb->vidmem[jzfb->current_frm_desc][0]);

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			short c16;
			int c32;
			switch ((i / 10) % 4) {
				case 0:
					c16 = 0xF800;
					c32 = 0x00FF0000;
					break;
				case 1:
					c16 = 0x07C0;
					c32 = 0x0000FF00;
					break;
				case 2:
					c16 = 0x001F;
					c32 = 0x000000FF;
					break;
				default:
					c16 = 0xFFFF;
					c32 = 0xFFFFFFFF;
					break;
			}
			switch (bpp) {
				case 18:
				case 24:
				case 32:
					*p32++ = c32;
					break;
				default:
					*p16++ = c16;
			}
		}
		if (w % PIXEL_ALIGN) {
			switch (bpp) {
				case 18:
				case 24:
				case 32:
					p32 += (ALIGN(mode->xres, PIXEL_ALIGN) - w);
					break;
				default:
					p16 += (ALIGN(mode->xres, PIXEL_ALIGN) - w);
					break;
			}
		}
	}
}

int lcd_display_inited_by_uboot( void )
{
	if (*(unsigned int*)(0xb3050000 + DC_ST) & DC_WORKING)
		uboot_inited = 1;
	else
		uboot_inited = 0;
	return uboot_inited;
}

static int slcd_pixel_refresh_times(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct jzfb_platform_data *pdata = jzfb->pdata;
	struct jzfb_smart_config *smart_config = pdata->smart_config;
	unsigned int cycle = 0;

	switch (smart_config->smart_type) {
	case SMART_LCD_TYPE_8080:
	case SMART_LCD_TYPE_6800:
		break;
	case SMART_LCD_TYPE_SPI_3:
		return 9 * 2;
	case SMART_LCD_TYPE_SPI_4:
		return 8 * 2;
	default:
		printk("%s %d err!\n",__func__,__LINE__);
		break;
	}

	switch (smart_config->dwidth) {
	case SMART_LCD_DWIDTH_8_BIT:
		if (smart_config->pix_fmt == SMART_LCD_FORMAT_565)
			cycle = 2;
		if (smart_config->pix_fmt == SMART_LCD_FORMAT_888)
			cycle = 3;
		break;
	case SMART_LCD_DWIDTH_9_BIT:
		if (smart_config->pix_fmt == SMART_LCD_FORMAT_666)
			cycle = 2;
		break;
	case SMART_LCD_DWIDTH_16_BIT:
		if (smart_config->pix_fmt == SMART_LCD_FORMAT_565)
			cycle = 1;
		break;
	case SMART_LCD_DWIDTH_18_BIT:
		if (smart_config->pix_fmt == SMART_LCD_FORMAT_666)
			cycle = 1;
		break;
	case SMART_LCD_DWIDTH_24_BIT:
		if (smart_config->pix_fmt == SMART_LCD_FORMAT_888)
			cycle = 1;
		break;
	default:
		printk("%s %d err!\n",__func__,__LINE__);
		break;
	}

	return cycle * 2 + 1;
}

static int refresh_pixclock_auto_adapt(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct jzfb_platform_data *pdata = jzfb->pdata;
	struct fb_var_screeninfo *var = &info->var;
	struct fb_videomode *mode;
	uint16_t hds, vds;
	uint16_t hde, vde;
	uint16_t ht, vt;
	unsigned long rate;

	mode = pdata->modes;
	if (mode == NULL) {
		dev_err(jzfb->dev, "%s get video mode failed\n", __func__);
		return -EINVAL;
	}

	hds = mode->hsync_len + mode->left_margin;
	hde = hds + mode->xres;
	ht = hde + mode->right_margin;

	vds = mode->vsync_len + mode->upper_margin;
	vde = vds + mode->yres;
	vt = vde + mode->lower_margin;

	if (mode->refresh) {
		rate = mode->refresh * vt * ht;
		if(jzfb->pdata->lcd_type == LCD_TYPE_SLCD)
			rate *= slcd_pixel_refresh_times(info);
		mode->pixclock = KHZ2PICOS(rate / 1000);
		var->pixclock = mode->pixclock;
	} else if (mode->pixclock) {
		rate = PICOS2KHZ(mode->pixclock) * 1000;
		mode->refresh = rate / vt / ht;
	} else {
		dev_err(jzfb->dev,"%s error:lcd important config info is absenced\n",__func__);
		return -EINVAL;
	}

	return 0;
}

static void jzfb_enable(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct jzfb_platform_data *pdata = jzfb->pdata;

	mutex_lock(&jzfb->lock);
	if (jzfb->is_lcd_en) {
		mutex_unlock(&jzfb->lock);
		return;
	}

	if(pdata->lcd_type == LCD_TYPE_SLCD) {
		slcd_send_mcu_command(jzfb, pdata->smart_config->write_gram_cmd);
		wait_slcd_busy();
		jzfb_cmp_start(info);
		jzfb_slcd_start(info);
	} else {
		jzfb_cmp_start(info);
		jzfb_tft_start(info);
	}

	jzfb->is_lcd_en = 1;
	mutex_unlock(&jzfb->lock);
	return;
}

static void jzfb_disable(struct fb_info *info, stop_mode_t stop_md)
{
	mutex_lock(&jzfb->lock);
	if (!jzfb->is_lcd_en) {
		mutex_unlock(&jzfb->lock);
		return;
	}

	if(stop_md == QCK_STOP) {
		reg_write(jzfb, DC_CTRL, DC_QCK_STP_CMP);
		wait_dc_state(DC_WORKING, 0);
	} else {
		reg_write(jzfb, DC_CTRL, DC_GEN_STP_CMP);
		wait_event_interruptible(jzfb->gen_stop_wq,
				!(reg_read(jzfb, DC_ST) & DC_WORKING));
	}

	jzfb->is_lcd_en = 0;
	mutex_unlock(&jzfb->lock);
}

static int jzfb_desc_init(struct fb_info *info, int frm_num)
{
	struct jzfb *jzfb = info->par;
	struct fb_videomode *mode;
	struct jzfb_frm_mode *frm_mode;
	struct jzfb_frm_cfg *frm_cfg;
	struct jzfb_lay_cfg *lay_cfg;
	struct fb_var_screeninfo *var = &info->var;
	struct jzfb_framedesc **framedesc;
	int frm_num_mi, frm_num_ma;
	int frm_size;
	int i, j;
	int ret = 0;

	mode = jzfb_get_mode(var, info);
	if (mode == NULL) {
		dev_err(info->dev, "%s get video mode failed\n", __func__);
		return -EINVAL;
	}

	framedesc = jzfb->framedesc;
	frm_mode = &jzfb->current_frm_mode;
	frm_cfg = &frm_mode->frm_cfg;
	lay_cfg = frm_cfg->lay_cfg;

	ret = jzfb_check_frm_cfg(info, frm_cfg);
	if(ret) {
		dev_err(info->dev, "%s configure framedesc[%d] error!\n", __func__, frm_num);
		return ret;
	}

	if(frm_num == FRAME_CFG_ALL_UPDATE) {
		frm_num_mi = 0;
		frm_num_ma = MAX_DESC_NUM;
	} else {
		if(frm_num < 0 || frm_num > MAX_DESC_NUM) {
			dev_err(info->dev, "framedesc num err!\n");
			return -EINVAL;
		}
		frm_num_mi = frm_num;
		frm_num_ma = frm_num + 1;
	}

	for(i = frm_num_mi; i < frm_num_ma; i++) {
		framedesc[i]->FrameNextCfgAddr = jzfb->framedesc_phys[i];
		framedesc[i]->FrameSize.b.width = mode->xres;
		framedesc[i]->FrameSize.b.height = mode->yres;
		framedesc[i]->FrameCtrl.d32 = FRAME_CTRL_DEFAULT_SET;
		framedesc[i]->WritebackStride = mode->xres;
		framedesc[i]->Layer0CfgAddr = jzfb->layerdesc_phys[i][0];
		framedesc[i]->Layer1CfgAddr = jzfb->layerdesc_phys[i][1];
		framedesc[i]->LayCfgEn.b.lay0_en = lay_cfg[0].lay_en;
		framedesc[i]->LayCfgEn.b.lay1_en = lay_cfg[1].lay_en;
		framedesc[i]->LayCfgEn.b.lay0_z_order = lay_cfg[0].lay_z_order;
		framedesc[i]->LayCfgEn.b.lay1_z_order = lay_cfg[1].lay_z_order;
		if(jzfb->pdata->lcd_type == LCD_TYPE_SLCD && !jzfb->slcd_continua) {
			framedesc[i]->FrameCtrl.b.stop = 1;
			framedesc[i]->InterruptControl.d32 = DC_EOD_MSK;
		} else {
			framedesc[i]->FrameCtrl.b.stop = 0;
			framedesc[i]->InterruptControl.d32 = DC_SOF_MSK;
		}
	}

	frm_size = mode->xres * mode->yres;
	jzfb->frm_size = frm_size * info->var.bits_per_pixel >> 3;
	info->screen_size = jzfb->frm_size;

	for(i = frm_num_mi; i < frm_num_ma; i++) {
		for(j =0; j < MAX_LAYER_NUM; j++) {
			if(!lay_cfg[j].lay_en)
				continue;
			jzfb->layerdesc[i][j]->LayerSize.b.width = lay_cfg[j].pic_width;
			jzfb->layerdesc[i][j]->LayerSize.b.height = lay_cfg[j].pic_height;
			jzfb->layerdesc[i][j]->LayerPos.b.x_pos = lay_cfg[j].disp_pos_x;
			jzfb->layerdesc[i][j]->LayerPos.b.y_pos = lay_cfg[j].disp_pos_y;
			jzfb->layerdesc[i][j]->LayerCfg.b.g_alpha_en = lay_cfg[j].g_alpha_en;
			jzfb->layerdesc[i][j]->LayerCfg.b.g_alpha = lay_cfg[j].g_alpha_val;
			jzfb->layerdesc[i][j]->LayerCfg.b.color = lay_cfg[j].color;
			jzfb->layerdesc[i][j]->LayerCfg.b.domain_multi = lay_cfg[j].domain_multi;
			jzfb->layerdesc[i][j]->LayerCfg.b.format = lay_cfg[j].format;
			jzfb->layerdesc[i][j]->LayerStride = lay_cfg[j].stride;
			jzfb->vidmem_phys[i][j] = jzfb->vidmem_phys[0][0] +
						   i * jzfb->frm_size +
						   j * jzfb->frm_size * MAX_DESC_NUM;
			jzfb->vidmem[i][j] = jzfb->vidmem[0][0] +
						   i * jzfb->frm_size +
						   j * jzfb->frm_size * MAX_DESC_NUM;
			jzfb->layerdesc[i][j]->LayerBufferAddr = jzfb->vidmem_phys[i][j];
			jzfb->layerdesc[i][j]->BufferAddr_UV = jzfb->vidmem_phys[i][j] + lay_cfg[j].pic_width * lay_cfg[j].pic_height;
			jzfb->layerdesc[i][j]->stride_UV = lay_cfg[j].stride;
		}
	}

	for(i = frm_num_mi; i < frm_num_ma; i++) {
		frm_mode->update_st[i] = FRAME_CFG_UPDATE;
	}

	return 0;
}

static int jzfb_update_frm_mode(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct fb_videomode *mode;
	struct jzfb_frm_mode *frm_mode;
	struct jzfb_frm_cfg *frm_cfg;
	struct jzfb_lay_cfg *lay_cfg;
	struct fb_var_screeninfo *var = &info->var;
	int i;

	mode = jzfb_get_mode(var, info);
	if (mode == NULL) {
		dev_err(info->dev, "%s get video mode failed\n", __func__);
		return -EINVAL;
	}

	frm_mode = &jzfb->current_frm_mode;
	frm_cfg = &frm_mode->frm_cfg;
	lay_cfg = frm_cfg->lay_cfg;

	/*Only set layer0 work*/
	lay_cfg[0].lay_en = 1;
	lay_cfg[0].lay_z_order = 1;
	lay_cfg[1].lay_en = 0;
	lay_cfg[1].lay_z_order = 0;

	for(i = 0; i < MAX_LAYER_NUM; i++) {
		lay_cfg[i].pic_width = mode->xres;
		lay_cfg[i].pic_height = mode->yres;
		lay_cfg[i].disp_pos_x = 0;
		lay_cfg[i].disp_pos_y = 0;
		lay_cfg[i].g_alpha_en = 0;
		lay_cfg[i].g_alpha_val = 0xff;
		lay_cfg[i].color = jzfb_colormodes[0].color;
		lay_cfg[i].format = jzfb_colormodes[0].mode;
		lay_cfg[i].domain_multi = 1;
		lay_cfg[i].stride = mode->xres;
	}

	jzfb->current_frm_desc = 0;

	return 0;
}

static void disp_common_init(struct jzfb_platform_data *jzfb_pdata) {
	uint32_t disp_com;

	disp_com = reg_read(jzfb, DC_DISP_COM);
	disp_com &= ~DC_DP_IF_SEL;
	if(jzfb_pdata->lcd_type == LCD_TYPE_SLCD) {
		disp_com |= DC_DISP_COM_SLCD;
	} else {
		disp_com |= DC_DISP_COM_TFT;
	}
	if(jzfb_pdata->dither_enable) {
		disp_com |= DC_DP_DITHER_EN;
		disp_com &= ~DC_DP_DITHER_DW_MASK;
		disp_com |= jzfb_pdata->dither.dither_red
			     << DC_DP_DITHER_DW_RED_LBIT;
		disp_com |= jzfb_pdata->dither.dither_green
			    << DC_DP_DITHER_DW_GREEN_LBIT;
		disp_com |= jzfb_pdata->dither.dither_blue
			    << DC_DP_DITHER_DW_BLUE_LBIT;
	} else {
		disp_com &= ~DC_DP_DITHER_EN;
	}
	reg_write(jzfb, DC_DISP_COM, disp_com);
}

static void common_cfg_init(void)
{
	unsigned com_cfg = 0;

	com_cfg = reg_read(jzfb, DC_COM_CONFIG);
	/*Keep COM_CONFIG reg first bit 0 */
	com_cfg &= ~DC_OUT_SEL;

	/* set burst length 32*/
	com_cfg &= ~DC_BURST_LEN_BDMA_MASK;
	com_cfg |= DC_BURST_LEN_BDMA_32;

	reg_write(jzfb, DC_COM_CONFIG, com_cfg);
}

static int jzfb_set_fix_par(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct jzfb_platform_data *pdata = jzfb->pdata;
	unsigned int intc;
	unsigned int is_lcd_en;

	mutex_lock(&jzfb->lock);
	is_lcd_en = jzfb->is_lcd_en;
	mutex_unlock(&jzfb->lock);

	jzfb_disable(info, GEN_STOP);

	disp_common_init(pdata);

	common_cfg_init();

	reg_write(jzfb, DC_CLR_ST, 0x80FFFFFE);

	if (pdata->lcd_type == LCD_TYPE_SLCD) {
		jzfb_slcd_set_par(info);
	} else {
		jzfb_tft_set_par(info);
	}
	/*       disp end      gen_stop    tft_under    frm_start    frm_end  */
//	intc = DC_EOD_MSK | DC_SDA_MSK | DC_UOT_MSK | DC_SOF_MSK | DC_EOF_MSK;
	intc = DC_EOD_MSK | DC_SDA_MSK | DC_UOT_MSK | DC_SOF_MSK;
	if (pdata->lcd_type == LCD_TYPE_SLCD && !jzfb->slcd_continua) {
		intc = DC_EOD_MSK | DC_SDA_MSK | DC_UOT_MSK;
	} else {
		intc = DC_SDA_MSK | DC_UOT_MSK | DC_SOF_MSK;
	}
	reg_write(jzfb, DC_INTC, intc);

	if(is_lcd_en) {
		jzfb_enable(info);
	}

	return 0;
}

static int jzfb_set_par(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct fb_var_screeninfo *var = &info->var;
	struct fb_videomode *mode;
	struct jzfb_frmdesc_msg *desc_msg;
	unsigned long flags;
	uint32_t colormode;
	int ret;

	mode = jzfb_get_mode(var, info);
	if (mode == NULL) {
		dev_err(info->dev, "%s get video mode failed\n", __func__);
		return -EINVAL;
	}
	info->mode = mode;

	ret = jzfb_check_colormode(var, &colormode);
	if(ret) {
		dev_err(info->dev,"Check colormode failed!\n");
		return  ret;
	}

	jzfb->current_frm_mode.frm_cfg.lay_cfg[0].format = colormode;

	ret = jzfb_desc_init(info, FRAME_CFG_ALL_UPDATE);
	if(ret) {
		dev_err(jzfb->dev, "Desc init err!\n");
		return ret;
	}

	spin_lock_irqsave(&jzfb->irq_lock, flags);
	if(jzfb->pdata->lcd_type == LCD_TYPE_SLCD && !jzfb->slcd_continua) {
		if(!list_empty(&jzfb->desc_run_list)) {
			desc_msg = list_first_entry(&jzfb->desc_run_list,
					struct jzfb_frmdesc_msg, list);
		} else {
			desc_msg = &jzfb->frmdesc_msg[jzfb->current_frm_desc];
			desc_msg->addr_virt->FrameCtrl.b.stop = 1;
			desc_msg->state = DESC_ST_RUNING;
			list_add_tail(&desc_msg->list, &jzfb->desc_run_list);
		}
		reg_write(jzfb, DC_FRM_CFG_ADDR, desc_msg->addr_phy);
	} else {
		reg_write(jzfb, DC_FRM_CFG_ADDR, jzfb->framedesc_phys[jzfb->current_frm_desc]);
	}
	spin_unlock_irqrestore(&jzfb->irq_lock, flags);

	return 0;
}

#ifdef CONFIG_FB_JZ_DEBUG
static int test_pattern(struct jzfb *jzfb)
{
	int ret;

	jzfb_disable(jzfb->fb, QCK_STOP);
	jzfb_display_h_color_bar(jzfb->fb);
	jzfb->current_frm_desc = 0;
	jzfb_set_fix_par(jzfb->fb);
	ret = jzfb_set_par(jzfb->fb);
	if(ret) {
		dev_err(jzfb->dev, "Set par failed!\n");
		return ret;
	}
	jzfb_enable(jzfb->fb);

	return 0;
}
#endif

#ifdef CONFIG_FB_JZ_LOGO

static void jzfb_copy_fb_memory_from_user(struct jzfb *jzfb, struct fb_var_screeninfo *var, int user_frm, int next_frm);

static int test_show_logo(struct jzfb *jzfb)
{
	int ret;
	jzfb_disable(jzfb->fb, QCK_STOP);

	if (fb_prepare_logo(jzfb->fb, FB_ROTATE_UR))
		fb_show_logo(jzfb->fb, FB_ROTATE_UR);

	if (jzfb->fb_copy_type != FB_COPY_TYPE_NONE) {
		memcpy(jzfb->user_vidmem, jzfb->fb_mem_mapped, jzfb->frm_size);
		jzfb_copy_fb_memory_from_user(jzfb, &jzfb->fb->var, 0, 0);
	}

	jzfb->current_frm_desc = 0;
	jzfb_set_fix_par(jzfb->fb);
	ret = jzfb_set_par(jzfb->fb);
	if(ret) {
		dev_err(jzfb->dev, "Set par failed!\n");
		return ret;
	}

	jzfb_enable(jzfb->fb);
	return 0;
}
#endif

static inline int timeval_sub_to_us(struct timeval lhs,
						struct timeval rhs)
{
	int sec, usec;
	sec = lhs.tv_sec - rhs.tv_sec;
	usec = lhs.tv_usec - rhs.tv_usec;

	return (sec*1000000 + usec);
}

static inline int time_us2ms(int us)
{
	return (us/1000);
}

static void calculate_frame_rate(void)
{
	static struct timeval time_now, time_last;
	unsigned int interval_in_us;
	unsigned int interval_in_ms;
	static unsigned int fpsCount = 0;

	switch(showFPS){
	case 1:
		fpsCount++;
		do_gettimeofday(&time_now);
		interval_in_us = timeval_sub_to_us(time_now, time_last);
		if ( interval_in_us > (USEC_PER_SEC) ) { /* 1 second = 1000000 us. */
			printk(" Pan display FPS: %d\n",fpsCount);
			fpsCount = 0;
			time_last = time_now;
		}
		break;
	case 2:
		do_gettimeofday(&time_now);
		interval_in_us = timeval_sub_to_us(time_now, time_last);
		interval_in_ms = time_us2ms(interval_in_us);
		printk(" Pan display interval ms: %d\n",interval_in_ms);
		time_last = time_now;
		break;
	default:
		if (showFPS > 3) {
			int d, f;
			fpsCount++;
			do_gettimeofday(&time_now);
			interval_in_us = timeval_sub_to_us(time_now, time_last);
			if (interval_in_us > USEC_PER_SEC * showFPS ) { /* 1 second = 1000000 us. */
				d = fpsCount / showFPS;
				f = (fpsCount * 10) / showFPS - d * 10;
				printk(" Pan display FPS: %d.%01d\n", d, f);
				fpsCount = 0;
				time_last = time_now;
			}
		}
		break;
	}
}

static int jzfb_set_desc_msg(struct fb_info *info, int num)
{
	struct jzfb *jzfb = info->par;
	struct jzfb_frmdesc_msg *desc_msg_curr;
	struct jzfb_frmdesc_msg *desc_msg;
	int i;

	desc_msg_curr = &jzfb->frmdesc_msg[num];
	if((desc_msg_curr->state != DESC_ST_AVAILABLE)
			&& (desc_msg_curr->state != DESC_ST_FREE)) {
		printk(KERN_DEBUG "%s Desc state is err!\n"
			"Now there are %d bufs, hoping to use polling to improve efficiency\n"
			"Current framenum %d\n"
			, __func__, MAX_DESC_NUM,num);
		//need irq finish
		return -1;
	}


	/* Whether the controller in the work */

	desc_msg_curr->state = DESC_ST_RUNING;
	list_add_tail(&desc_msg_curr->list, &jzfb->desc_run_list);
	desc_msg_curr->addr_virt->FrameCtrl.b.stop = 1;

	//check runlist tail
	desc_msg = list_entry(jzfb->desc_run_list.prev,
		struct jzfb_frmdesc_msg, list);

	if((reg_read(jzfb, DC_FRM_DES) != desc_msg->addr_phy) && (desc_msg != desc_msg_curr))
	{
		desc_msg->addr_virt->FrameNextCfgAddr = desc_msg_curr->addr_phy;
		desc_msg->addr_virt->FrameCtrl.b.stop = 0;
	}
	if(!(reg_read(jzfb, DC_ST) & DC_WORKING))
	{
		reg_write(jzfb, DC_FRM_CFG_ADDR, desc_msg_curr->addr_phy);
		jzfb_cmp_start(info);
		jzfb_slcd_start(info);
	}
	for(i = 0; i < MAX_DESC_NUM; i++) {
		if(jzfb->frmdesc_msg[i].state == DESC_ST_FREE) {
			jzfb->frmdesc_msg[i].state = DESC_ST_AVAILABLE;
			jzfb->vsync_skip_map = (jzfb->vsync_skip_map >> 1 |
						jzfb->vsync_skip_map << 9) & 0x3ff;
			if(likely(jzfb->vsync_skip_map & 0x1)) {
				jzfb->timestamp.value[jzfb->timestamp.wp] =
					ktime_to_ns(ktime_get());
				jzfb->timestamp.wp = (jzfb->timestamp.wp + 1) % TIMESTAMP_CAP;
				wake_up_interruptible(&jzfb->vsync_wq);
			}
		}
	}

	return 0;
}

static void jzfb_copy_fb_memory_from_user(
	struct jzfb *jzfb, struct fb_var_screeninfo *var, int user_frm, int fb_frm)
{
    unsigned int *mem, *fb_mem;
    unsigned int xres, yres, src_stride, dst_stride;
	int bytes_per_pixel = var->bits_per_pixel >> 3;

    mem = jzfb->user_vidmem + jzfb->frm_size * user_frm;

    fb_mem = jzfb->fb_mem_mapped + jzfb->frm_size * fb_frm;

	xres = var->xres;
	yres = var->yres;

    src_stride = xres * bytes_per_pixel;
	if (jzfb->change_xy)
		dst_stride = yres * bytes_per_pixel;
	else
		dst_stride = xres * bytes_per_pixel;

	if (bytes_per_pixel == 2)
		xres /= 2;

    switch (jzfb->fb_copy_type) {
    case FB_COPY_TYPE_ROTATE_0:
        memcpy(fb_mem, mem, jzfb->frm_size);
        break;
    case FB_COPY_TYPE_ROTATE_180:
        rotate_180(fb_mem, mem, xres, yres, src_stride, dst_stride);
        break;
    case FB_COPY_TYPE_ROTATE_270:
        rotate_270(fb_mem, mem, xres, yres, src_stride, dst_stride);
        break;
    case FB_COPY_TYPE_ROTATE_90:
        rotate_90(fb_mem, mem, xres, yres, src_stride, dst_stride);
        break;
    default:
        return;
    }
}


#include <linux/hrtimer.h>

static void m_sleep_ms(int ms)
{
	__set_current_state(TASK_UNINTERRUPTIBLE);

	ktime_t t = ktime_set(0, ms * 1000 * 1000);
	schedule_hrtimeout(&t, HRTIMER_MODE_REL);
}

#include <soc/cache.h>

static int jzfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	unsigned long flags;
	int next_frm;
	int ret = 0;

	if (var->xoffset - info->var.xoffset) {
		dev_err(info->dev, "No support for X panning for now\n");
		return -EINVAL;
	}

	jzfb->pan_display_count++;
	if(showFPS){
		calculate_frame_rate();
	}

	if (jzfb->fb_copy_type != FB_COPY_TYPE_NONE) {
		jzfb->next_frm = !jzfb->next_frm;
		next_frm = jzfb->next_frm;
		if(next_frm >= MAX_DESC_NUM)
			next_frm = 0;
	} else {
		next_frm = var->yoffset / var->yres;
	}

	if(jzfb->current_frm_mode.update_st[next_frm] == FRAME_CFG_NO_UPDATE) {
		if((ret = jzfb_desc_init(info, next_frm))) {
			dev_err(info->dev, "%s: desc init err!\n", __func__);
			dump_lcdc_registers();
			return ret;
		}
	}

	if(jzfb->pdata->lcd_type == LCD_TYPE_SLCD && !jzfb->slcd_continua) {
		if (jzfb->fb_copy_type != FB_COPY_TYPE_NONE) {
			while (reg_read(jzfb, DC_ST) & DC_WORKING) {
				msleep(1);
			}
			fast_iob();
			jzfb_copy_fb_memory_from_user(jzfb, var, 0, next_frm);
			fast_iob();
		}
		spin_lock_irqsave(&jzfb->irq_lock, flags);
		init_completion(&jzfb->frm_msg_complete);
		ret = jzfb_set_desc_msg(info, next_frm);
		spin_unlock_irqrestore(&jzfb->irq_lock, flags);
		if(ret) {
			int retrycount = 0;
			ret = 0;
			while(!ret){
				ret = wait_for_completion_timeout(&jzfb->frm_msg_complete,msecs_to_jiffies(60));
				if(ret < 0)
					return -1;

				retrycount++;
				if(retrycount > 2){
					dev_err(info->dev, "%s: wait frm msg complete err!\n", __func__);
					return -1;
				}
				if(ret >= 0){
					spin_lock_irqsave(&jzfb->irq_lock, flags);
					init_completion(&jzfb->frm_msg_complete);
					ret = jzfb_set_desc_msg(info, next_frm);
					spin_unlock_irqrestore(&jzfb->irq_lock, flags);
					if(ret == 0)
						break;
					else // continue try
						ret = 0;
				}
			}
		}
	} else {
		if (jzfb->fb_copy_type != FB_COPY_TYPE_NONE) {
			fast_iob();
			jzfb_copy_fb_memory_from_user(jzfb, var, 0, next_frm);
			fast_iob();

			while (!jzfb->next_frm_is_done)
				m_sleep_ms(1);

			jzfb->next_frm_is_done = 0;

			jzfb->layerdesc[next_frm][0]->LayerBufferAddr =
				jzfb->vidmem_phys[0][0] + jzfb->frm_size * next_frm;

			reg_write(jzfb, DC_FRM_CFG_ADDR, jzfb->framedesc_phys[next_frm]);
		} else {
			reg_write(jzfb, DC_FRM_CFG_ADDR, jzfb->framedesc_phys[next_frm]);
		}
	}

	if(!jzfb->is_lcd_en)
		jzfb_enable(info);
	jzfb->current_frm_desc = next_frm;

	return 0;
}

static void jzfb_do_resume(struct jzfb *jzfb)
{
	int ret;

	mutex_lock(&jzfb->suspend_lock);
	if(jzfb->is_suspend) {
		jzfb_clk_enable(jzfb);
		jzfb_set_fix_par(jzfb->fb);
		ret = jzfb_set_par(jzfb->fb);
		if(ret) {
			dev_err(jzfb->dev, "Set par failed!\n");
		}
		jzfb_enable(jzfb->fb);
		jzfb->is_suspend = 0;
	}
	mutex_unlock(&jzfb->suspend_lock);
}

static void jzfb_do_suspend(struct jzfb *jzfb)
{
	struct jzfb_platform_data *pdata = jzfb->pdata;
	struct lcd_callback_ops *callback_ops;
	struct lcdc_gpio_struct *gpio_assign;

	callback_ops = pdata->lcd_callback_ops;
	gpio_assign = pdata->gpio_assign;

	mutex_lock(&jzfb->suspend_lock);
	if (!jzfb->is_suspend){
		jzfb_disable(jzfb->fb, QCK_STOP);
		jzfb_clk_disable(jzfb);
		if(callback_ops && callback_ops->lcd_power_off_begin)
			callback_ops->lcd_power_off_begin(gpio_assign);

		jzfb->is_suspend = 1;
	}
	mutex_unlock(&jzfb->suspend_lock);
}

static int jzfb_blank(int blank_mode, struct fb_info *info)
{
	struct jzfb *jzfb = info->par;

	if (blank_mode == FB_BLANK_POWERDOWN) {
		jzfb_do_suspend(jzfb);
	} else {
		jzfb_do_resume(jzfb);
	}

	return 0;
}

static int jzfb_open(struct fb_info *info, int user)
{
	struct jzfb *jzfb = info->par;
	int ret;
	int i;

	if (!jzfb->is_lcd_en && jzfb->vidmem_phys) {
		for(i = 0; i < MAX_DESC_NUM; i++) {
			jzfb->frmdesc_msg[i].state = DESC_ST_FREE;
		}
		INIT_LIST_HEAD(&jzfb->desc_run_list);
		jzfb->timestamp.rp = 0;
		jzfb->timestamp.wp = 0;
		jzfb_set_fix_par(info);
		ret = jzfb_set_par(info);
		if(ret) {
			dev_err(info->dev, "Set par failed!\n");
			return ret;
		}
		memset(jzfb->vidmem[jzfb->current_frm_desc][0], 0, jzfb->frm_size);
		jzfb_enable(info);
	}

	dev_dbg(info->dev, "####open count : %d\n", ++jzfb->open_cnt);

	return 0;
}

static int jzfb_release(struct fb_info *info, int user)
{
	struct jzfb *jzfb = info->par;
	int i;

	dev_dbg(info->dev, "####close count : %d\n", jzfb->open_cnt--);
	if(!jzfb->open_cnt) {
		for(i = 0; i < MAX_DESC_NUM; i++) {
			jzfb->frmdesc_msg[i].state = DESC_ST_FREE;
		}
		INIT_LIST_HEAD(&jzfb->desc_run_list);
		jzfb->timestamp.rp = 0;
		jzfb->timestamp.wp = 0;
	}
	return 0;
}

static ssize_t jzfb_write(struct fb_info *info, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	struct jzfb *jzfb = info->par;
	u8 *buffer, *src;
	u8 __iomem *dst;
	int c, cnt = 0, err = 0;
	unsigned long total_size;
	unsigned long p = *ppos;
	unsigned long flags;
	int next_frm = 0;
	int ret;

	total_size = info->screen_size;

	if (total_size == 0)
		total_size = info->fix.smem_len;

	if (p > total_size)
		return -EFBIG;

	if (count > total_size) {
		err = -EFBIG;
		count = total_size;
	}

	if (count + p > total_size) {
		if (!err)
			err = -ENOSPC;

		count = total_size - p;
	}

	buffer = kmalloc((count > PAGE_SIZE) ? PAGE_SIZE : count,
			 GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	dst = (u8 __iomem *) (info->screen_base + p);

	while (count) {
		c = (count > PAGE_SIZE) ? PAGE_SIZE : count;
		src = buffer;

		if (copy_from_user(src, buf, c)) {
			err = -EFAULT;
			break;
		}

		fb_memcpy_tofb(dst, src, c);
		dst += c;
		src += c;
		*ppos += c;
		buf += c;
		cnt += c;
		count -= c;
	}

	kfree(buffer);

	if(jzfb->pdata->lcd_type == LCD_TYPE_SLCD && !jzfb->slcd_continua) {
		spin_lock_irqsave(&jzfb->irq_lock, flags);
		ret = jzfb_set_desc_msg(info, next_frm);
		spin_unlock_irqrestore(&jzfb->irq_lock, flags);
		if(ret) {
            int retrycount = 0;
            ret = 0;
            while(!ret){
                ret = wait_for_completion_timeout(&jzfb->frm_msg_complete,msecs_to_jiffies(60));
                if(ret < 0)
                    return -1;

                retrycount++;
                if(retrycount > 2){
                    dev_err(info->dev, "%s: wait frm msg complete err!\n", __func__);
                    return -1;
                }
                if(ret >= 0){
                    spin_lock_irqsave(&jzfb->irq_lock, flags);
                    init_completion(&jzfb->frm_msg_complete);
                    ret = jzfb_set_desc_msg(info, next_frm);
                    spin_unlock_irqrestore(&jzfb->irq_lock, flags);
                    if(ret == 0)
                        break;
                    else // continue try
                        ret = 0;
                }
            }
        }
	} else {
		reg_write(jzfb, DC_FRM_CFG_ADDR, jzfb->framedesc_phys[next_frm]);
	}

	if(!jzfb->is_lcd_en)
		jzfb_enable(info);
	jzfb->current_frm_desc = next_frm;

	return (cnt) ? cnt : err;
}

static struct fb_ops jzfb_ops = {
	.owner = THIS_MODULE,
	.fb_open = jzfb_open,
	.fb_release = jzfb_release,
	.fb_write = jzfb_write,
	.fb_check_var = jzfb_check_var,
	.fb_set_par = jzfb_set_par,
	.fb_setcolreg = jzfb_setcolreg,
	.fb_blank = jzfb_blank,
	.fb_pan_display = jzfb_pan_display,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_ioctl = jzfb_ioctl,
	.fb_mmap = jzfb_mmap,
};

	static ssize_t
dump_h_color_bar(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct jzfb *jzfb = dev_get_drvdata(dev);
	jzfb_display_h_color_bar(jzfb->fb);
	if (jzfb->pdata->lcd_type == LCD_TYPE_SLCD && !jzfb->slcd_continua) {
		jzfb_cmp_start(jzfb->fb);
		jzfb_slcd_start(jzfb->fb);
	}
	return 0;
}

	static ssize_t
dump_v_color_bar(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct jzfb *jzfb = dev_get_drvdata(dev);
	jzfb_display_v_color_bar(jzfb->fb);
	if (jzfb->pdata->lcd_type == LCD_TYPE_SLCD && !jzfb->slcd_continua) {
		jzfb_cmp_start(jzfb->fb);
		jzfb_slcd_start(jzfb->fb);
	}
	return 0;
}

	static ssize_t
vsync_skip_r(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct jzfb *jzfb = dev_get_drvdata(dev);
	mutex_lock(&jzfb->lock);
	snprintf(buf, 3, "%d\n", jzfb->vsync_skip_ratio);
	printk("vsync_skip_map = 0x%08x\n", jzfb->vsync_skip_map);
	mutex_unlock(&jzfb->lock);
	return 3;		/* sizeof ("%d\n") */
}

static int vsync_skip_set(struct jzfb *jzfb, int vsync_skip)
{
	unsigned int map_wide10 = 0;
	int rate, i, p, n;
	int fake_float_1k;

	if (vsync_skip < 0 || vsync_skip > 9)
		return -EINVAL;

	rate = vsync_skip + 1;
	fake_float_1k = 10000 / rate;	/* 10.0 / rate */

	p = 1;
	n = (fake_float_1k * p + 500) / 1000;	/* +0.5 to int */

	for (i = 1; i <= 10; i++) {
		map_wide10 = map_wide10 << 1;
		if (i == n) {
			map_wide10++;
			p++;
			n = (fake_float_1k * p + 500) / 1000;
		}
	}
	mutex_lock(&jzfb->lock);
	jzfb->vsync_skip_map = map_wide10;
	jzfb->vsync_skip_ratio = rate - 1;	/* 0 ~ 9 */
	mutex_unlock(&jzfb->lock);

	printk("vsync_skip_ratio = %d\n", jzfb->vsync_skip_ratio);
	printk("vsync_skip_map = 0x%08x\n", jzfb->vsync_skip_map);

	return 0;
}

	static ssize_t
vsync_skip_w(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct jzfb *jzfb = dev_get_drvdata(dev);

	if ((count != 1) && (count != 2))
		return -EINVAL;
	if ((*buf < '0') && (*buf > '9'))
		return -EINVAL;

	vsync_skip_set(jzfb, *buf - '0');

	return count;
}

static ssize_t fps_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk("\n-----you can choice print way:\n");
	printk("Example: echo NUM > show_fps\n");
	printk("NUM = 0: close fps statistics\n");
	printk("NUM = 1: print recently fps\n");
	printk("NUM = 2: print interval between last and this pan_display\n");
	printk("NUM = 3: print pan_display count\n\n");
	return 0;
}

static ssize_t fps_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
	int num = 0;
	num = simple_strtoul(buf, NULL, 0);
	if(num < 0){
		printk("\n--please 'cat show_fps' to view using the method\n\n");
		return n;
	}
	showFPS = num;
	if(showFPS == 3)
		printk(KERN_DEBUG " Pand display count=%d\n",jzfb->pan_display_count);
	return n;
}


	static ssize_t
debug_clr_st(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct jzfb *jzfb = dev_get_drvdata(dev);
	reg_write(jzfb, DC_CLR_ST, 0xffffffff);
	return 0;
}

static ssize_t test_suspend(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
	int num = 0;
	num = simple_strtoul(buf, NULL, 0);
	printk("0 --> resume | 1 --> suspend\nnow input %d\n", num);
	if (num == 0) {
		jzfb_do_resume(jzfb);
	} else {
		jzfb_do_suspend(jzfb);
	}
	return n;
}

/**********************lcd_debug***************************/
static DEVICE_ATTR(dump_lcd, S_IRUGO|S_IWUSR, dump_lcd, NULL);
static DEVICE_ATTR(dump_h_color_bar, S_IRUGO|S_IWUSR, dump_h_color_bar, NULL);
static DEVICE_ATTR(dump_v_color_bar, S_IRUGO|S_IWUSR, dump_v_color_bar, NULL);
static DEVICE_ATTR(vsync_skip, S_IRUGO|S_IWUSR, vsync_skip_r, vsync_skip_w);
static DEVICE_ATTR(show_fps, S_IRUGO|S_IWUSR, fps_show, fps_store);
static DEVICE_ATTR(debug_clr_st, S_IRUGO|S_IWUSR, debug_clr_st, NULL);
static DEVICE_ATTR(test_suspend, S_IRUGO|S_IWUSR, NULL, test_suspend);

static struct attribute *lcd_debug_attrs[] = {
	&dev_attr_dump_lcd.attr,
	&dev_attr_dump_h_color_bar.attr,
	&dev_attr_dump_v_color_bar.attr,
	&dev_attr_vsync_skip.attr,
	&dev_attr_show_fps.attr,
	&dev_attr_debug_clr_st.attr,
	&dev_attr_test_suspend.attr,
	NULL,
};

const char lcd_group_name[] = "debug";
static struct attribute_group lcd_debug_attr_group = {
	.name	= lcd_group_name,
	.attrs	= lcd_debug_attrs,
};

static void jzfb_free_devmem(struct jzfb *jzfb)
{
	size_t buff_size;

	dma_free_coherent(jzfb->dev,
			  jzfb->vidmem_size,
			  jzfb->vidmem[0][0],
			  jzfb->vidmem_phys[0][0]);

	buff_size = sizeof(struct jzfb_layerdesc);
	buff_size = ALIGN(buff_size, DESC_ALIGN);
	dma_free_coherent(jzfb->dev,
			  buff_size * MAX_DESC_NUM * MAX_LAYER_NUM,
			  jzfb->layerdesc[0],
			  jzfb->layerdesc_phys[0][0]);

	buff_size = sizeof(struct jzfb_framedesc);
	buff_size = ALIGN(buff_size, DESC_ALIGN);
	dma_free_coherent(jzfb->dev,
			  buff_size * MAX_DESC_NUM,
			  jzfb->framedesc[0],
			  jzfb->framedesc_phys[0]);
}

static int jzfb_copy_logo(struct fb_info *info)
{
	unsigned long src_addr = 0;	/* u-boot logo buffer address */
	unsigned long dst_addr = 0;	/* kernel frame buffer address */
	struct jzfb *jzfb = info->par;
	unsigned long size;
	unsigned int ctrl;
	unsigned read_times;
	lay_cfg_en_t lay_cfg_en;

	/* Sure the uboot SLCD using the continuous mode, Close irq */
	if (!(reg_read(jzfb, DC_ST) & DC_WORKING)) {
		dev_err(jzfb->dev, "uboot is not display logo!\n");
		return -ENOEXEC;
	}

	/*jzfb->is_lcd_en = 1;*/

	/* Reading Desc from regisger need reset */
	ctrl = reg_read(jzfb, DC_CTRL);
	ctrl |= DC_DES_CNT_RST;
	reg_write(jzfb, DC_CTRL, ctrl);

	/* For geting LayCfgEn need read  DC_FRM_DES 10 times */
	read_times = 10 - 1;
	while(read_times--) {
		reg_read(jzfb, DC_FRM_DES);
	}
	lay_cfg_en.d32 = reg_read(jzfb, DC_FRM_DES);
	if(!lay_cfg_en.b.lay0_en) {
		dev_err(jzfb->dev, "Uboot initialization is not using layer0!\n");
		return -ENOEXEC;
	}

	/* For geting LayerBufferAddr need read  DC_LAY0_DES 3 times */
	read_times = 3 - 1;
	/* get buffer physical address */
	while(read_times--) {
		reg_read(jzfb, DC_LAY0_DES);
	}
	src_addr = (unsigned long)reg_read(jzfb, DC_LAY0_DES);

	if (src_addr) {
		size = info->fix.line_length * info->var.yres;
		src_addr = (unsigned long)phys_to_virt(src_addr);
		dst_addr = (unsigned long)jzfb->vidmem[0][0];
		memcpy((void *)dst_addr, (void *)src_addr, size);
	}

	return 0;
}

static int jzfb_probe(struct platform_device *pdev)
{
	struct jzfb_platform_data *pdata = pdev->dev.platform_data;
	struct fb_videomode *video_mode;
	struct resource *mem;
	struct fb_info *fb;
	unsigned long rate;
	int ret = 0;
	int i;

	if (!pdata) {
		dev_err(&pdev->dev, "Missing platform data\n");
		return -ENXIO;
	}

	fb = framebuffer_alloc(sizeof(struct jzfb), &pdev->dev);
	if (!fb) {
		dev_err(&pdev->dev, "Failed to allocate framebuffer device\n");
		return -ENOMEM;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "Failed to get register memory resource\n");
		ret = -ENXIO;
		goto err_framebuffer_release;
	}

	mem = request_mem_region(mem->start, resource_size(mem), pdev->name);
	if (!mem) {
		dev_err(&pdev->dev,
				"Failed to request register memory region\n");
		ret = -EBUSY;
		goto err_framebuffer_release;
	}

	jzfb = fb->par;
	jzfb->fb = fb;
	jzfb->dev = &pdev->dev;
	jzfb->pdata = pdata;
	jzfb->mem = mem;
	if(pdata->smart_config)
		jzfb->slcd_continua = pdata->smart_config->frm_md;

	spin_lock_init(&jzfb->irq_lock);
	mutex_init(&jzfb->lock);
	mutex_init(&jzfb->suspend_lock);
	sprintf(jzfb->clk_name, "lcd");
	sprintf(jzfb->pclk_name, "cgu_lpc");
	sprintf(jzfb->ahb1_name, "ahb1");

	jzfb->clk = clk_get(&pdev->dev, jzfb->clk_name);
	jzfb->pclk = clk_get(&pdev->dev, jzfb->pclk_name);
	jzfb->ahb1 = clk_get(&pdev->dev, jzfb->ahb1_name);

	if (IS_ERR(jzfb->clk) ||
			IS_ERR(jzfb->pclk) ||
			IS_ERR(jzfb->ahb1)) {
		ret = PTR_ERR(jzfb->clk);
		dev_err(&pdev->dev, "Failed to get lcdc clock: %d\n", ret);
		goto err_release_mem_region;
	}

	jzfb->base = ioremap(mem->start, resource_size(mem));
	if (!jzfb->base) {
		dev_err(&pdev->dev,
				"Failed to ioremap register memory region\n");
		ret = -EBUSY;
		goto err_put_clk;
	}

	ret = refresh_pixclock_auto_adapt(fb);
	if(ret){
		goto err_iounmap;
	}

	video_mode = jzfb->pdata->modes;
	if (!video_mode) {
		ret = -ENXIO;
		goto err_iounmap;
	}

	if (pdata->lcd_type == LCD_TYPE_SLCD && !jzfb->slcd_continua)
		jzfb->fb_copy_type = pdata->smart_config->fb_copy_type;
	else
		jzfb->fb_copy_type = pdata->tft_config->fb_copy_type;
	jzfb->change_xy = jzfb->fb_copy_type == FB_COPY_TYPE_ROTATE_90 ||
	                  jzfb->fb_copy_type == FB_COPY_TYPE_ROTATE_270;

	if (jzfb->change_xy) {
		int i, t;
		for (i = 0; i < pdata->num_modes; i++) {
			t = pdata->modes[i].yres;
			pdata->modes[i].yres = pdata->modes[i].xres;
			pdata->modes[i].xres = t;
		}
	}

	fb_videomode_to_modelist(pdata->modes, pdata->num_modes, &fb->modelist);

	jzfb_videomode_to_var(jzfb, &fb->var, video_mode, jzfb->pdata->lcd_type);
	fb->fbops = &jzfb_ops;
	fb->flags = FBINFO_DEFAULT;
	fb->var.width = pdata->width;
	fb->var.height = pdata->height;

	jzfb_colormode_to_var(&fb->var, &jzfb_colormodes[0]);

	ret = jzfb_check_var(&fb->var, fb);
	if (ret) {
		goto err_iounmap;
	}

	/*
	 * #BUG: if uboot pixclock is different from kernel. this may cause problem.
	 * */
	rate = PICOS2KHZ(fb->var.pixclock) * 1000;
	clk_set_rate(jzfb->pclk, rate);
	if(rate != clk_get_rate(jzfb->pclk)) {
		dev_err(&pdev->dev, "failed to set pixclock!, rate:%ld, clk_rate = %ld\n", rate, clk_get_rate(jzfb->pclk));
	}

	clk_prepare_enable(jzfb->clk);
	clk_prepare_enable(jzfb->pclk);
	clk_prepare_enable(jzfb->ahb1);
	jzfb->is_clk_en = 1;

	ret = jzfb_alloc_devmem(jzfb);
	if (ret) {
		dev_err(&pdev->dev, "Failed to allocate video memory\n");
		goto err_iounmap;
	}

	fb->fix = jzfb_fix;
	fb->fix.line_length = (fb->var.bits_per_pixel * fb->var.xres) >> 3;
	fb->fix.mmio_start = mem->start;
	fb->fix.mmio_len = resource_size(mem);
	fb->fix.smem_start = jzfb->user_vidmem_phys;
	fb->fix.smem_len = jzfb->user_vidmem_size;
	fb->screen_size = jzfb->frm_size;
	fb->screen_base = jzfb->vidmem[0][0];
	fb->pseudo_palette = jzfb->pseudo_palette;

	vsync_skip_set(jzfb, CONFIG_FB_VSYNC_SKIP);
	init_waitqueue_head(&jzfb->vsync_wq);
	init_waitqueue_head(&jzfb->gen_stop_wq);
	init_completion(&jzfb->frm_msg_complete);
	jzfb->open_cnt = 0;
	jzfb->is_lcd_en = 0;
	jzfb->timestamp.rp = 0;
	jzfb->timestamp.wp = 0;

	INIT_LIST_HEAD(&jzfb->desc_run_list);
	for(i = 0; i < MAX_DESC_NUM; i++) {
		jzfb->frmdesc_msg[i].state = DESC_ST_FREE;
		jzfb->frmdesc_msg[i].addr_virt = jzfb->framedesc[i];
		jzfb->frmdesc_msg[i].addr_phy = jzfb->framedesc_phys[i];
		jzfb->frmdesc_msg[i].index = i;
	}

	jzfb_update_frm_mode(jzfb->fb);

	jzfb->irq = platform_get_irq(pdev, 0);
	sprintf(jzfb->irq_name, "lcdc%d", pdev->id);
	if (request_irq(jzfb->irq, jzfb_irq_handler, IRQF_DISABLED,
				jzfb->irq_name, jzfb)) {
		dev_err(&pdev->dev, "request irq failed\n");
		ret = -EINVAL;
		goto err_free_devmem;
	}

	platform_set_drvdata(pdev, jzfb);

	ret = sysfs_create_group(&jzfb->dev->kobj, &lcd_debug_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "device create sysfs group failed\n");

		ret = -EINVAL;
		goto err_free_irq;
	}

	ret = register_framebuffer(fb);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register framebuffer: %d\n",
				ret);
		goto err_free_file;
	}

	if (jzfb->vidmem_phys) {
		if (!jzfb_copy_logo(jzfb->fb)) {
			jzfb->is_lcd_en = 1;
			ret = jzfb_desc_init(fb, 0);
			if(ret) {
				dev_err(jzfb->dev, "Desc init err!\n");
				goto err_free_file;
			}
			reg_write(jzfb, DC_FRM_CFG_ADDR, jzfb->framedesc_phys[jzfb->current_frm_desc]);
		}

#if defined(CONFIG_FB_JZ_LOGO)
		test_show_logo(jzfb);

		if(CONFIG_FB_JZ_V14_DELAY_BACKLIGHT > 0) {
			udelay(CONFIG_FB_JZ_V14_DELAY_BACKLIGHT);
		}

		fb_blank(jzfb->fb, FB_BLANK_UNBLANK);
#elif defined(CONFIG_FB_JZ_DEBUG)
		test_pattern(jzfb);

		if(CONFIG_FB_JZ_V14_DELAY_BACKLIGHT > 0) {
			udelay(CONFIG_FB_JZ_V14_DELAY_BACKLIGHT);
		}

		fb_blank(jzfb->fb, FB_BLANK_UNBLANK);
#endif

	}else{
		jzfb_clk_disable(jzfb);
		ret = -ENOMEM;
		goto err_free_file;
	}

	return 0;

err_free_file:
	sysfs_remove_group(&jzfb->dev->kobj, &lcd_debug_attr_group);
err_free_irq:
	free_irq(jzfb->irq, jzfb);
err_free_devmem:
	jzfb_free_devmem(jzfb);
err_iounmap:
	iounmap(jzfb->base);
err_put_clk:
	if (jzfb->clk)
		clk_put(jzfb->clk);
	if (jzfb->pclk)
		clk_put(jzfb->pclk);
	if (jzfb->ahb1)
		clk_put(jzfb->ahb1);
err_release_mem_region:
	release_mem_region(mem->start, resource_size(mem));
err_framebuffer_release:
	framebuffer_release(fb);
	return ret;
}

static int jzfb_remove(struct platform_device *pdev)
{
	struct jzfb *jzfb = platform_get_drvdata(pdev);

	jzfb_free_devmem(jzfb);
	platform_set_drvdata(pdev, NULL);

	clk_put(jzfb->pclk);
	clk_put(jzfb->clk);
	clk_put(jzfb->ahb1);

	sysfs_remove_group(&jzfb->dev->kobj, &lcd_debug_attr_group);

	iounmap(jzfb->base);
	release_mem_region(jzfb->mem->start, resource_size(jzfb->mem));

	framebuffer_release(jzfb->fb);

	return 0;
}

static void jzfb_shutdown(struct platform_device *pdev)
{
	struct jzfb *jzfb = platform_get_drvdata(pdev);
	int is_fb_blank;
	mutex_lock(&jzfb->suspend_lock);
	is_fb_blank = (jzfb->is_suspend != 1);
	jzfb->is_suspend = 1;
	mutex_unlock(&jzfb->suspend_lock);
	if (is_fb_blank)
		fb_blank(jzfb->fb, FB_BLANK_POWERDOWN);
};

#ifdef CONFIG_PM

static int jzfb_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jzfb *jzfb = platform_get_drvdata(pdev);

	fb_blank(jzfb->fb, FB_BLANK_POWERDOWN);

	return 0;
}

static int jzfb_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jzfb *jzfb = platform_get_drvdata(pdev);

	fb_blank(jzfb->fb, FB_BLANK_UNBLANK);

	return 0;
}

static const struct dev_pm_ops jzfb_pm_ops = {
	.suspend = jzfb_suspend,
	.resume = jzfb_resume,
};
#endif
static struct platform_driver jzfb_driver = {
	.probe = jzfb_probe,
	.remove = jzfb_remove,
	.shutdown = jzfb_shutdown,
	.driver = {
		.name = "jz-fb",
#ifdef CONFIG_PM
		.pm = &jzfb_pm_ops,
#endif
	},
};

static int __init jzfb_init(void)
{
	platform_driver_register(&jzfb_driver);
	return 0;
}

static void __exit jzfb_cleanup(void)
{
	platform_driver_unregister(&jzfb_driver);
}

device_initcall_sync(jzfb_init);
module_exit(jzfb_cleanup);

MODULE_DESCRIPTION("JZ LCD Controller driver");
MODULE_AUTHOR("Sean Tang <ctang@ingenic.cn>");
MODULE_LICENSE("GPL");
