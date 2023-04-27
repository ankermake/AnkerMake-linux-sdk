
#include <common.h>
#include <soc/base.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <utils/clock.h>

#include "dpu_hal.h"


#define DPU_ADDR(reg)  ((volatile unsigned long *)CKSEG1ADDR(DPU_IOBASE + reg))

void dpu_write(unsigned int reg, unsigned int val)
{
    *DPU_ADDR(reg) = val;
}

unsigned int dpu_read(unsigned int reg)
{
    return *DPU_ADDR(reg);
}

void dpu_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(DPU_ADDR(reg), start, end, val);
}

unsigned int dpu_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(DPU_ADDR(reg), start, end);
}

void dpu_start_composer(void)
{
    dpu_write(FRM_CFG_CTRL, bit_field_val(FRM_CFG_START, 1));
}

void dpu_genernal_stop_display(void)
{
    dpu_write(CTRL, bit_field_val(GEN_STP_CMP, 1));
}

void dpu_config_srdma_ch(void)
{
    dpu_set_bit(COM_CFG, CH_SEL, 1);
}

void dpu_config_composer_ch(void)
{
    dpu_set_bit(COM_CFG, CH_SEL, 0);
}

void dpu_quick_stop_display(void)
{
    dpu_write(CTRL, bit_field_val(QCK_STP_CMP, 1));
}

void dpu_quick_stop_srdma(void)
{
    dpu_write(CTRL, bit_field_val(QCK_STP_SRD,1));
}

void dpu_start_simple_dma(void)
{
    dpu_write(SRD_CHAIN_CTRL, bit_field_val(SRD_CHAIN_START, 1));
}

static inline int bytes_per_pixel(enum fb_fmt fmt)
{
    if (fmt == fb_fmt_RGB888 || fmt == fb_fmt_ARGB8888)
        return 4;
    return 2;
}

void dpu_enable_layer(struct framedesc *desc, int id, int enable)
{
    int off = bit_field_start(f_layer0En) + id;
    set_bit_field(&desc->LayerCfgScaleEn, off, off, enable);
}

void dpu_enable_layer_scaling(struct framedesc *desc, int id, int enable)
{
    int off = bit_field_start(f_lay0ScaleEn) + id;
    set_bit_field(&desc->LayerCfgScaleEn, off, off, enable);
}

void dpu_set_layer_order(struct framedesc *desc, int id, int order)
{
    int i;
    int temp_off;
    int save_order;
    int enable_off;
    int enable;

    int off = bit_field_start(f_layer0order) + id * 2;
    if (order == lcdc_layer_top)
        order = lcdc_layer_3;
    if (order == lcdc_layer_bottom)
        order = lcdc_layer_0;
    order = order - lcdc_layer_0;

    save_order = get_bit_field(&desc->LayerCfgScaleEn, off, off + 1);

    if (save_order == order)
        return;

    for (i = 0; i < 4; i++) {
        temp_off = bit_field_start(f_layer0order) + i * 2;
        enable_off = bit_field_start(f_layer0En) + i;
        enable = get_bit_field(&desc->LayerCfgScaleEn, enable_off, enable_off);

        if (get_bit_field(&desc->LayerCfgScaleEn, temp_off, temp_off + 1) == order) {
            if (!enable)
                set_bit_field(&desc->LayerCfgScaleEn, temp_off, temp_off + 1, save_order);
            else {
                printk(KERN_ERR "can not set layer_order,layer%d order = %d\n", id, save_order);
                return;
            }

            break;
        }
    }

    set_bit_field(&desc->LayerCfgScaleEn, off, off + 1, order);
}

void dpu_init_layer_desc(struct layerdesc *layer, struct lcdc_layer *cfg)
{
    int format;
    switch (cfg->fb_fmt) {
    case fb_fmt_RGB555:
        format = 0; break;
    case fb_fmt_RGB565:
        format = 2; break;
    case fb_fmt_RGB888:
        format = 4; break;
    case fb_fmt_ARGB8888:
        format = 5; break;
    case fb_fmt_NV12:
        format = 8; break;
    case fb_fmt_NV21:
        format = 9; break;
    case fb_fmt_yuv422:
        format = 10;break;
    default:
        panic("format err: %d\n", cfg->fb_fmt); break;
    }

    unsigned int addr_y, addr_uv;
    unsigned int stride_y, stride_uv;
    if (cfg->fb_fmt == fb_fmt_NV12 || cfg->fb_fmt == fb_fmt_NV21) {
        addr_y = virt_to_phys(cfg->y.mem);
        addr_uv = virt_to_phys(cfg->uv.mem);
        stride_y = cfg->y.stride;
        stride_uv = cfg->uv.stride;
    } else {
        addr_y = virt_to_phys(cfg->rgb.mem);
        addr_uv = 0;
        stride_y = cfg->rgb.stride / bytes_per_pixel(cfg->fb_fmt);
        stride_uv = 0;
    }

    layer->LayerSize = 0;
    set_bit_field(&layer->LayerSize, l_Height, cfg->yres);
    set_bit_field(&layer->LayerSize, l_Width, cfg->xres);

    layer->LayerCfg = 0;
    set_bit_field(&layer->LayerCfg, l_SHARPL, 1);
    set_bit_field(&layer->LayerCfg, l_Format, format);
    set_bit_field(&layer->LayerCfg, l_PREMULT, 1);
    set_bit_field(&layer->LayerCfg, l_GAlpha_en, cfg->alpha.enable);
    set_bit_field(&layer->LayerCfg, l_Color, 0);
    set_bit_field(&layer->LayerCfg, l_GAlpha, cfg->alpha.value);

    layer->LayerPos = 0;
    set_bit_field(&layer->LayerPos, l_YPos, cfg->ypos);
    set_bit_field(&layer->LayerPos, l_XPos, cfg->xpos);

    unsigned int target_xres = cfg->scaling.xres ? cfg->scaling.xres : cfg->xres;
    unsigned int target_yres = cfg->scaling.yres ? cfg->scaling.yres : cfg->yres;
    layer->Layer_Resize_Coef_X = cfg->xres * 512 / target_xres;
    layer->Layer_Resize_Coef_Y = cfg->yres * 512 / target_yres;

    if (cfg->scaling.enable) {
        layer->LayerTargetSize = 0;
        set_bit_field(&layer->LayerTargetSize, l_TargetHeight, target_yres);
        set_bit_field(&layer->LayerTargetSize, l_TargetWidth, target_xres);
    } else {
        layer->LayerTargetSize = 0;
    }

    layer->LayerBufferAddr = addr_y;
    layer->BufferAddr_UV = addr_uv;
    layer->LayerStride = stride_y;
    layer->stride_UV = stride_uv;
    layer->Reserved1 = 0;
    layer->Reserved2 = 0;
}

EXPORT_SYMBOL_GPL(dpu_write);
EXPORT_SYMBOL_GPL(dpu_read);
EXPORT_SYMBOL_GPL(dpu_set_bit);
EXPORT_SYMBOL_GPL(dpu_get_bit);

EXPORT_SYMBOL_GPL(dpu_start_composer);
EXPORT_SYMBOL_GPL(dpu_start_simple_dma);
EXPORT_SYMBOL_GPL(dpu_config_srdma_ch);
EXPORT_SYMBOL_GPL(dpu_config_composer_ch);

EXPORT_SYMBOL_GPL(dpu_genernal_stop_display);
EXPORT_SYMBOL_GPL(dpu_quick_stop_display);
EXPORT_SYMBOL_GPL(dpu_quick_stop_srdma);

EXPORT_SYMBOL_GPL(dpu_enable_layer);
EXPORT_SYMBOL_GPL(dpu_enable_layer_scaling);
EXPORT_SYMBOL_GPL(dpu_set_layer_order);
EXPORT_SYMBOL_GPL(dpu_init_layer_desc);
