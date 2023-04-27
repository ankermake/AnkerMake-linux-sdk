

#include <common.h>
#include <soc/base.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <utils/clock.h>

#include "lcdc_layer.h"
#include "lcdc_data.h"
#include "lcdc_regs.h"
#include "lcd_gpio.c"

struct framedesc {
    unsigned long FrameCfgAddr;
    unsigned long FrameSize;
    unsigned long FrameCtrl;
    unsigned long Reserved0;
    unsigned long Reserved1;
    unsigned long Layer0CfgAddr;
    unsigned long Layer1CfgAddr;
    unsigned long Reserved2;
    unsigned long Reserved3;
    unsigned long LayerCfgScaleEn;
    unsigned long InterruptControl;
};

struct layerdesc {
    unsigned long LayerSize;
    unsigned long LayerCfg;
    unsigned long LayerBufferAddr;
    unsigned long Reserved0;
    unsigned long Reserved1;
    unsigned long Reserved2;
    unsigned long LayerPos;
    unsigned long Reserved3;
    unsigned long Reserved4;
    unsigned long LayerStride;
    unsigned long BufferAddr_UV;
    unsigned long stride_UV;
};

struct lcdc_frame {
    struct framedesc *framedesc;
    struct layerdesc *layer0;
    struct layerdesc *layer1;
};

#define LCD_ADDR(reg)  ((volatile unsigned long *)CKSEG1ADDR(LCDC_IOBASE + reg))

static inline void lcd_write(unsigned int reg, unsigned int val)
{
    *LCD_ADDR(reg) = val;
}

static inline unsigned int lcd_read(unsigned int reg)
{
    return *LCD_ADDR(reg);
}

static inline void lcd_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(LCD_ADDR(reg), start, end, val);
}

static inline unsigned int lcd_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(LCD_ADDR(reg), start, end);
}

static inline void start_composer(void)
{
    lcd_write(FRM_CFG_CTRL, bit_field_val(FRM_CFG_START, 1));
}

static inline void start_tft(void)
{
    lcd_write(CTRL, bit_field_val(TFT_START, 1));
}

static inline void start_slcd(void)
{
    lcd_write(CTRL, bit_field_val(SLCD_START, 1));
}

static inline void genernal_stop_display(void)
{
    lcd_write(CTRL, bit_field_val(GEN_STP_DISP, 1));
}

static inline void quick_stop_display(void)
{
    lcd_write(CTRL, bit_field_val(QCK_STP_DISP, 1));
}


static inline int slcd_wait_busy_us(unsigned int count_udelay)
{
    int busy;
    uint64_t old = local_clock_us();

    busy = lcd_get_bit(SLCD_ST, BUSY);
    while (busy && local_clock_us() - old < count_udelay) {
        usleep_range(500, 500);
        busy = lcd_get_bit(SLCD_ST, BUSY);
    }

    return busy;
}

static inline int slcd_wait_busy(unsigned int count)
{
    int busy;

    busy = lcd_get_bit(SLCD_ST, BUSY);
    while (count-- && busy) {
        busy = lcd_get_bit(SLCD_ST, BUSY);
    }

    return busy;
}

static void slcd_send_cmd(unsigned int cmd)
{
    unsigned long val = 0;

    if (slcd_wait_busy(10 * 1000))
        panic("lcdc busy\n");

    set_bit_field(&val, FLAG, 2);
    set_bit_field(&val, CONTENT, cmd);
    lcd_write(SLCD_REG_IF, val);
}

static void slcd_send_data(unsigned int data)
{
    unsigned long val = 0;

    if (slcd_wait_busy(10 * 1000))
        panic("lcdc busy\n");

    set_bit_field(&val, FLAG, 1);
    set_bit_field(&val, CONTENT, data);
    lcd_write(SLCD_REG_IF, val);
}

static inline int bytes_per_pixel(enum fb_fmt fmt)
{
    if (fmt == fb_fmt_RGB888 || fmt == fb_fmt_ARGB8888)
        return 4;
    return 2;
}

static inline int bits_per_pixel(enum fb_fmt fmt)
{
    if (fmt == fb_fmt_RGB888)
        return 32;
    if (fmt == fb_fmt_ARGB8888)
        return 32;
    return 16;
}

static inline int bytes_per_line(int xres, enum fb_fmt fmt)
{
    int len = xres * bytes_per_pixel(fmt);
    return ALIGN(len, 8);
}

static inline int bytes_per_frame(int line_len, int yres)
{
    unsigned int frame_size = line_len * yres;

    return frame_size;
}

static void init_tft(struct lcdc_data *pdata)
{
    int hps = pdata->hsync_len;
    int hpe = hps + pdata->left_margin + pdata->xres + pdata->right_margin;
    int vps = pdata->vsync_len;
    int vpe = vps + pdata->upper_margin + pdata->yres + pdata->lower_margin;
    int hds = pdata->hsync_len + pdata->left_margin;
    int hde = hds + pdata->xres;
    int vds = pdata->vsync_len + pdata->upper_margin;
    int vde = vds + pdata->yres;

    lcd_write(TFT_HSYNC, bit_field_val(HPS,hps) | bit_field_val(HPE,hpe));
    lcd_write(TFT_VSYNC, bit_field_val(VPS,vps) | bit_field_val(VPE,vpe));
    lcd_write(TFT_HDE, bit_field_val(HDS,hds) | bit_field_val(HDE,hde));
    lcd_write(TFT_VDE, bit_field_val(VDS,vds) | bit_field_val(VDE,vde));

    unsigned long tft_tran_cfg = 0;
    set_bit_field(&tft_tran_cfg, PIX_CLK_INV, pdata->tft.pix_clk_polarity == AT_FALLING_EDGE);
    set_bit_field(&tft_tran_cfg, DE_DL, pdata->tft.de_active_level == AT_LOW_LEVEL);
    set_bit_field(&tft_tran_cfg, SYNC_DL, pdata->tft.hsync_vsync_active_level == AT_LOW_LEVEL);
    set_bit_field(&tft_tran_cfg, COLOR_EVEN, pdata->tft.even_line_order);
    set_bit_field(&tft_tran_cfg, COLOR_ODD, pdata->tft.odd_line_order);
    set_bit_field(&tft_tran_cfg, MODE, pdata->lcd_mode);
    lcd_write(TFT_CFG, tft_tran_cfg);
}

static void init_slcd(struct lcdc_data *pdata)
{
    int dbi_type = 2;
    if (pdata->lcd_mode == SLCD_6800)
        dbi_type = 1;
    if (pdata->lcd_mode == SLCD_8080)
        dbi_type = 2;
    if (pdata->lcd_mode == SLCD_SPI_3LINE)
        dbi_type = 4;
    if (pdata->lcd_mode == SLCD_SPI_4LINE)
        dbi_type = 5;

    int pix_fmt = pdata->out_format;
    if (pdata->out_format == OUT_FORMAT_RGB444)
        pix_fmt = 1;
    if (pdata->out_format == OUT_FORMAT_RGB555)
        panic("slcd outformat can't be 555\n");

    unsigned long slcd_cfg = 0;
    set_bit_field(&slcd_cfg, FRM_MD, 0);
    set_bit_field(&slcd_cfg, RDY_ANTI_JIT, 0);
    set_bit_field(&slcd_cfg, FMT_EN, 0);
    set_bit_field(&slcd_cfg, DBI_TYPE, dbi_type);
    set_bit_field(&slcd_cfg, PIX_FMT, pix_fmt);
    set_bit_field(&slcd_cfg, TE_ANTI_JIT, 1);
    set_bit_field(&slcd_cfg, TE_MD, 0);
    set_bit_field(&slcd_cfg, TE_SWITCH, pdata->slcd.te_pin_mode == TE_LCDC_TRIGGER);
    set_bit_field(&slcd_cfg, RDY_SWITCH, pdata->slcd.enable_rdy_pin);
    set_bit_field(&slcd_cfg, CS_EN, 0);
    set_bit_field(&slcd_cfg, CS_DP, 0);
    set_bit_field(&slcd_cfg, RDY_DP, pdata->slcd.rdy_cmd_send_level == AT_HIGH_LEVEL);
    set_bit_field(&slcd_cfg, DC_MD, pdata->slcd.dc_pin == CMD_HIGH_DATA_LOW);
    set_bit_field(&slcd_cfg, WR_MD, pdata->slcd.wr_data_sample_edge == AT_RISING_EDGE);
    set_bit_field(&slcd_cfg, TE_DP, pdata->slcd.te_data_transfered_edge == AT_RISING_EDGE);
    set_bit_field(&slcd_cfg, DWIDTH, pdata->slcd.mcu_data_width);
    set_bit_field(&slcd_cfg, CWIDTH, pdata->slcd.mcu_cmd_width);
    lcd_write(SLCD_CFG, slcd_cfg);

    lcd_write(SLCD_WR_DUTY, 0);
    lcd_write(SLCD_TIMING, 0);

    unsigned long slcd_frm_size = 0;
    set_bit_field(&slcd_frm_size, V_SIZE, pdata->yres);
    set_bit_field(&slcd_frm_size, H_SIZE, pdata->xres);
    lcd_write(SLCD_FRM_SIZE, slcd_frm_size);

    lcd_write(SLCD_SLOW_TIME, 0);
}

static inline int is_tft(struct lcdc_data *pdata)
{
    return pdata->lcd_mode <= TFT_8BITS_DUMMY_SERIAL;
}

static void init_lcdc(struct lcdc_data *pdata, struct framedesc *desc)
{
    unsigned long intc = 0;
    set_bit_field(&intc, EOD_MSK, 1);
    set_bit_field(&intc, SDA_MSK, 1);
    set_bit_field(&intc, UOT_MSK, 1);
    lcd_write(INTC, intc);

    lcd_write(CLR_ST, lcd_read(INT_FLAG));

    unsigned long com_cfg = lcd_read(COM_CFG);
    set_bit_field(&com_cfg, BURST_LEN_BDMA, 3);
    lcd_write(COM_CFG, com_cfg);

    int dither_en = 0;
    int dither_dw = 0;
    if (pdata->fb_fmt == fb_fmt_RGB888 || pdata->fb_fmt == fb_fmt_ARGB8888) {
        if (pdata->out_format != OUT_FORMAT_RGB888)
            dither_en = 1;
        if (pdata->out_format == OUT_FORMAT_RGB444)
            dither_dw = 0b111111;
        if (pdata->out_format == OUT_FORMAT_RGB555)
            dither_dw = 0b101010;
        if (pdata->out_format == OUT_FORMAT_RGB565)
            dither_dw = 0b100110;
        if (pdata->out_format == OUT_FORMAT_RGB666)
            dither_dw = 0b010101;
    }

    unsigned long disp_com = lcd_read(DISP_COM);
    set_bit_field(&disp_com, DP_DITHER_EN, dither_en);
    set_bit_field(&disp_com, DP_DITHER_DW, dither_dw);
    set_bit_field(&disp_com, DP_IF_SEL, is_tft(pdata) ? 1 : 2);
    lcd_write(DISP_COM, disp_com);

    if (is_tft(pdata))
        init_tft(pdata);
    else
        init_slcd(pdata);

    lcd_write(FRM_CFG_ADDR, virt_to_phys(desc));
}

static void init_frame_desc(
    struct lcdc_data *pdata,
    struct framedesc *desc, struct layerdesc *layer0, struct layerdesc *layer1)
{
    desc->FrameCfgAddr = virt_to_phys(desc);

    set_bit_field(&desc->FrameSize, f_Width, pdata->xres);
    set_bit_field(&desc->FrameSize, f_Height, pdata->yres);

    desc->FrameCtrl = is_tft(pdata) ? 4 : 5;

    desc->Reserved1 = pdata->xres;

    desc->Layer0CfgAddr = virt_to_phys(layer0);
    desc->Layer1CfgAddr = virt_to_phys(layer1);

    desc->LayerCfgScaleEn = 0;
    set_bit_field(&desc->LayerCfgScaleEn, f_layer0order, 0);
    set_bit_field(&desc->LayerCfgScaleEn, f_layer1order, 1);

    desc->InterruptControl = 0;
    set_bit_field(&desc->InterruptControl, f_EOD_MSK, 1);
    set_bit_field(&desc->InterruptControl, f_SOF_MSK, 1);
    set_bit_field(&desc->InterruptControl, f_EOF_MSK, 1);
}

static void init_layer_desc(struct layerdesc *layer, struct lcdc_layer *cfg)
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

    set_bit_field(&layer->LayerSize, l_Height, cfg->yres);
    set_bit_field(&layer->LayerSize, l_Width, cfg->xres);

    set_bit_field(&layer->LayerCfg, l_Format, format);
    set_bit_field(&layer->LayerCfg, l_PREMULT, 0);
    set_bit_field(&layer->LayerCfg, l_GAlpha_en, cfg->alpha.enable);
    set_bit_field(&layer->LayerCfg, l_Color, 0);
    set_bit_field(&layer->LayerCfg, l_GAlpha, cfg->alpha.value);

    set_bit_field(&layer->LayerPos, l_YPos, cfg->ypos);
    set_bit_field(&layer->LayerPos, l_XPos, cfg->xpos);

    layer->LayerBufferAddr = addr_y;
    layer->BufferAddr_UV = addr_uv;
    layer->LayerStride = stride_y;
    layer->stride_UV = stride_uv;
    layer->Reserved0 = 0;
    layer->Reserved1 = 0;
    layer->Reserved2 = 0;
    layer->Reserved3 = 0;
    layer->Reserved4 = 0;
}

static void enable_layer(struct framedesc *desc, int id, int enable)
{
    if (id)
        set_bit_field(&desc->LayerCfgScaleEn, f_layer1En, enable);
    else
        set_bit_field(&desc->LayerCfgScaleEn, f_layer0En, enable);
}

static void set_layer_order(struct framedesc *desc, int id, int order)
{
    if (id)
        set_bit_field(&desc->LayerCfgScaleEn, f_layer0order, order);
    else
        set_bit_field(&desc->LayerCfgScaleEn, f_layer1order, order);
}

static int slcd_pixclock_cycle(struct lcdc_data *pdata)
{
    assert(pdata->lcd_mode == SLCD_6800 || pdata->lcd_mode == SLCD_8080);

    int cycle = 0;
    int width = pdata->slcd.mcu_data_width;
    int pix_fmt = pdata->out_format;
    if (width == MCU_WIDTH_8BITS) {
        if (pix_fmt == OUT_FORMAT_RGB565)
            cycle = 2;
        if (pix_fmt == OUT_FORMAT_RGB888)
            cycle = 3;
    }
    if (width == MCU_WIDTH_9BITS) {
        cycle = 2;
    }
    if (width == MCU_WIDTH_16BITS) {
        cycle = 1;
    }

    assert(cycle);

    return cycle * 2 + 1;
}

static void auto_calculate_pixel_clock(struct lcdc_data *pdata)
{
    int hps = pdata->hsync_len;
    int hpe = hps + pdata->left_margin + pdata->xres + pdata->right_margin;
    int vps = pdata->vsync_len;
    int vpe = vps + pdata->upper_margin + pdata->yres + pdata->lower_margin;

    if (!pdata->refresh)
        pdata->refresh = 40;

    if (is_tft(pdata)) {
        if (!pdata->pixclock)
            pdata->pixclock = hpe * vpe * pdata->refresh;
    } else {
        if (!pdata->pixclock) {
            pdata->pixclock = pdata->xres * pdata->yres * pdata->refresh;
            pdata->pixclock *= slcd_pixclock_cycle(pdata);
        }

        if (!pdata->slcd.pixclock_when_init)
            pdata->slcd.pixclock_when_init = pdata->xres * pdata->yres * 3;
    }

    pdata->refresh = pdata->pixclock / (hpe * vpe);
}

static int init_gpio(struct lcdc_data *pdata)
{
    int ret = -EINVAL;

    switch (pdata->lcd_mode) {
    case TFT_24BITS:
        if (pdata->out_format == OUT_FORMAT_RGB444)
            ret = tft_init_gpio(4, 4, 4);
        if (pdata->out_format == OUT_FORMAT_RGB555)
            ret = tft_init_gpio(5, 5, 5);
        if (pdata->out_format == OUT_FORMAT_RGB565)
            ret = tft_init_gpio(5, 6, 5);
        if (pdata->out_format == OUT_FORMAT_RGB666)
            ret = tft_init_gpio(6, 6, 6);
        if (pdata->out_format == OUT_FORMAT_RGB888)
            printk(KERN_ERR "tft out format can't be rgb888\n");
        break;

    case TFT_8BITS_SERIAL:
    case TFT_8BITS_DUMMY_SERIAL:
        ret = tft_serial_init_gpio();
        break;

    case SLCD_6800:
    case SLCD_8080: {
        int use_cs = 0;
        int use_rdy = pdata->slcd.enable_rdy_pin;
        int use_te = pdata->slcd.te_pin_mode == TE_LCDC_TRIGGER;
        int width = max(pdata->slcd.mcu_cmd_width, pdata->slcd.mcu_data_width);
        if (width == MCU_WIDTH_8BITS)
            ret = slcd_init_gpio_data8(use_rdy, use_te, use_cs);
        if (width == MCU_WIDTH_9BITS)
            ret = slcd_init_gpio_data9(use_rdy, use_te, use_cs);
        if (width == MCU_WIDTH_16BITS)
            ret = slcd_init_gpio_data16(use_rdy, use_te, use_cs);
        break;
    }

    default:
        printk(KERN_ERR "This mode is not currently implemented: %d\n", pdata->lcd_mode);
        break;
    }

    return ret;
}

static inline void lcdc_dump_regs(void)
{
    printk("FRM_CFG_ADDR %08x\n", lcd_read(FRM_CFG_ADDR));
    printk("FRM_CFG_CTRL %08x\n", lcd_read(FRM_CFG_CTRL));
    printk("CTRL %08x\n", lcd_read(CTRL));
    printk("ST %08x\n", lcd_read(ST));
    printk("CLR_ST %08x\n", lcd_read(CLR_ST));
    printk("INTC %08x\n", lcd_read(INTC));
    printk("INT_FLAG %08x\n", lcd_read(INT_FLAG));
    printk("COM_CFG %08x\n", lcd_read(COM_CFG));
    printk("PCFG_RD_CTRL %08x\n", lcd_read(PCFG_RD_CTRL));
    printk("PCFG_OFIFO %08x\n", lcd_read(PCFG_OFIFO));

    printk("FRM_DES %08x\n", lcd_read(FRM_DES));
    printk("FRM_DES %08x\n", lcd_read(FRM_DES));
    printk("FRM_DES %08x\n", lcd_read(FRM_DES));
    printk("FRM_DES %08x\n", lcd_read(FRM_DES));
    printk("FRM_DES %08x\n", lcd_read(FRM_DES));
    printk("FRM_DES %08x\n", lcd_read(FRM_DES));
    printk("FRM_DES %08x\n", lcd_read(FRM_DES));
    printk("FRM_DES %08x\n", lcd_read(FRM_DES));
    printk("FRM_DES %08x\n", lcd_read(FRM_DES));
    printk("FRM_DES %08x\n", lcd_read(FRM_DES));
    printk("FRM_DES %08x\n", lcd_read(FRM_DES));

    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", lcd_read(LAY0_DES_READ));

    printk("LAY1_DES_READ %08x\n", lcd_read(LAY1_DES_READ));
    printk("FRM_CHAIN_SITE %08x\n", lcd_read(FRM_CHAIN_SITE));
    printk("LAY0_RGB_SITE %08x\n", lcd_read(LAY0_RGB_SITE));
    printk("LAY1_RGB_SITE %08x\n", lcd_read(LAY1_RGB_SITE));
    printk("LAY0_Y_SITE %08x\n", lcd_read(LAY0_Y_SITE));
    printk("LAY0_UV_SITE %08x\n", lcd_read(LAY0_UV_SITE));
    printk("LAY1_Y_SITE %08x\n", lcd_read(LAY1_Y_SITE));
    printk("LAY1_UV_SITE %08x\n", lcd_read(LAY1_UV_SITE));
    printk("LAY0_CSC_MULT_YRV %08x\n", lcd_read(LAY0_CSC_MULT_YRV));
    printk("LAY0_CSC_MULT_GUGV %08x\n", lcd_read(LAY0_CSC_MULT_GUGV));
    printk("LAY0_CSC_MULT_BU %08x\n", lcd_read(LAY0_CSC_MULT_BU));
    printk("LAY0_CSC_SUB_YUV %08x\n", lcd_read(LAY0_CSC_SUB_YUV));
    printk("LAY1_CSC_MULT_YRV %08x\n", lcd_read(LAY1_CSC_MULT_YRV));
    printk("LAY1_CSC_MULT_GUGV %08x\n", lcd_read(LAY1_CSC_MULT_GUGV));
    printk("LAY1_CSC_MULT_BU %08x\n", lcd_read(LAY1_CSC_MULT_BU));
    printk("LAY1_CSC_SUB_YUV %08x\n", lcd_read(LAY1_CSC_SUB_YUV));
    printk("DISP_COM %08x\n", lcd_read(DISP_COM));
    printk("TFT_HSYNC %08x\n", lcd_read(TFT_HSYNC));
    printk("TFT_VSYNC %08x\n", lcd_read(TFT_VSYNC));
    printk("TFT_HDE %08x\n", lcd_read(TFT_HDE));
    printk("TFT_VDE %08x\n", lcd_read(TFT_VDE));
    printk("TFT_CFG %08x\n", lcd_read(TFT_CFG));
    printk("TFT_ST %08x\n", lcd_read(TFT_ST));
    printk("SLCD_CFG %08x\n", lcd_read(SLCD_CFG));
    printk("SLCD_WR_DUTY %08x\n", lcd_read(SLCD_WR_DUTY));
    printk("SLCD_TIMING %08x\n", lcd_read(SLCD_TIMING));
    printk("SLCD_FRM_SIZE %08x\n", lcd_read(SLCD_FRM_SIZE));
    printk("SLCD_SLOW_TIME %08x\n", lcd_read(SLCD_SLOW_TIME));
    printk("SLCD_REG_IF %08x\n", lcd_read(SLCD_REG_IF));
    printk("SLCD_ST %08x\n", lcd_read(SLCD_ST));
    printk("SLCD_REG_CTRL %08x\n", lcd_read(SLCD_REG_CTRL));

}
