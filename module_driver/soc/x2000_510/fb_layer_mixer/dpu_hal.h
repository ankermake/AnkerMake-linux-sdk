#ifndef _DPU_HAL_H_
#define _DPU_HAL_H_

#include "../fb/lcdc_data.h"
#include "dpu_regs.h"
#include <bit_field.h>

struct framedesc {
    unsigned long FrameCfgAddr;
    unsigned long FrameSize;
    unsigned long FrameCtrl;
    unsigned long WritebackBufferAddr;
    unsigned long WritebackStride;
    unsigned long Layer0CfgAddr;
    unsigned long Layer1CfgAddr;
    unsigned long Layer2CfgAddr;
    unsigned long Layer3CfgAddr;
    unsigned long LayerCfgScaleEn;
    unsigned long InterruptControl;
};

struct layerdesc {
    unsigned long LayerSize;
    unsigned long LayerCfg;
    unsigned long LayerBufferAddr;
    unsigned long LayerTargetSize;
    unsigned long Reserved1;
    unsigned long Reserved2;
    unsigned long LayerPos;
    unsigned long Layer_Resize_Coef_X;
    unsigned long Layer_Resize_Coef_Y;
    unsigned long LayerStride;
    unsigned long BufferAddr_UV;
    unsigned long stride_UV;
};

void dpu_write(unsigned int reg, unsigned int val);
unsigned int dpu_read(unsigned int reg);
void dpu_set_bit(unsigned int reg, int start, int end, unsigned int val);
unsigned int dpu_get_bit(unsigned int reg, int start, int end);

void dpu_start_composer(void);
void dpu_start_simple_dma(void);
void dpu_genernal_stop_display(void);
void dpu_quick_stop_display(void);
void dpu_quick_stop_srdma(void);

void dpu_enable_layer(struct framedesc *desc, int id, int enable);
void dpu_enable_layer_scaling(struct framedesc *desc, int id, int enable);
void dpu_set_layer_order(struct framedesc *desc, int id, int order);
void dpu_init_layer_desc(struct layerdesc *layer, struct lcdc_layer *cfg);

void dpu_config_srdma_ch(void);
void dpu_config_composer_ch(void);

#endif