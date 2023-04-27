

#include <common.h>
#include <soc/base.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <utils/clock.h>

#include "lcdc_data.h"
#include "lcd_gpio.c"
#include "../fb_layer_mixer/dpu_hal.h"

#define CPM_ADDR(reg)  ((volatile unsigned long *)CKSEG1ADDR(CPM_IOBASE + reg))

#define LPCDR       0x64
#define LCD_IO_INV  26, 26

void cpm_write(unsigned int reg, unsigned int val)
{
    *CPM_ADDR(reg) = val;
}

unsigned int cpm_read(unsigned int reg)
{
    return *CPM_ADDR(reg);
}

static unsigned int spi_send_delay = 0;

struct srdmadesc {
    unsigned long RdmaNextCfgAddr;
    unsigned long FrameBufferAddr;
    unsigned long stride;
    unsigned long FrameCtrl;
    unsigned long InterruptControl;
};

static inline int slcd_wait_busy_us(unsigned int count_udelay)
{
    int busy;
    uint64_t old = local_clock_us();

    busy = dpu_get_bit(SLCD_ST, BUSY);
    while (busy && local_clock_us() - old < count_udelay) {
        usleep_range(500, 500);
        busy = dpu_get_bit(SLCD_ST, BUSY);
    }

    return busy;
}

static inline int slcd_wait_busy(unsigned int count)
{
    int busy;

    busy = dpu_get_bit(SLCD_ST, BUSY);
    while (count-- && busy) {
        busy = dpu_get_bit(SLCD_ST, BUSY);
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
    dpu_write(SLCD_REG_IF, val);

    if (spi_send_delay != 0)
        udelay(spi_send_delay);
}

static void slcd_send_data(unsigned int data)
{
    unsigned long val = 0;

    if (slcd_wait_busy(10 * 1000))
        panic("lcdc busy\n");

    set_bit_field(&val, FLAG, 1);
    set_bit_field(&val, CONTENT, data);
    dpu_write(SLCD_REG_IF, val);

    if (spi_send_delay != 0)
        udelay(spi_send_delay);
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

static void init_tft_mipi(struct lcdc_data *pdata)
{
    int hps = pdata->hsync_len;
    int hpe = hps + pdata->left_margin + pdata->xres + pdata->right_margin;
    int vps = pdata->vsync_len;
    int vpe = vps + pdata->upper_margin + pdata->yres + pdata->lower_margin;
    int hds = pdata->hsync_len + pdata->left_margin;
    int hde = hds + pdata->xres;
    int vds = pdata->vsync_len + pdata->upper_margin;
    int vde = vds + pdata->yres;

    dpu_write(TFT_HSYNC, bit_field_val(HPS,hps) | bit_field_val(HPE,hpe));
    dpu_write(TFT_VSYNC, bit_field_val(VPS,vps) | bit_field_val(VPE,vpe));
    dpu_write(TFT_HDE, bit_field_val(HDS,hds) | bit_field_val(HDE,hde));
    dpu_write(TFT_VDE, bit_field_val(VDS,vds) | bit_field_val(VDE,vde));

    unsigned long tft_tran_cfg = 0;
    set_bit_field(&tft_tran_cfg, PIX_CLK_INV, 0);
    set_bit_field(&tft_tran_cfg, DE_DL, 0);
    set_bit_field(&tft_tran_cfg, SYNC_DL, 0);
    set_bit_field(&tft_tran_cfg, COLOR_EVEN, 0);
    set_bit_field(&tft_tran_cfg, COLOR_ODD, 0);
    set_bit_field(&tft_tran_cfg, MODE, pdata->lcd_mode);

    switch (pdata->out_format) {
        case OUT_FORMAT_RGB888:
            set_bit_field(&tft_tran_cfg, MODE, 0);
            break;
        case OUT_FORMAT_RGB666:
            set_bit_field(&tft_tran_cfg, MODE, 1);
            break;
        case OUT_FORMAT_RGB565:
            set_bit_field(&tft_tran_cfg, MODE, 2);
            break;
        default:
            break;
    }

    dpu_write(TFT_CFG, tft_tran_cfg);
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

    dpu_write(TFT_HSYNC, bit_field_val(HPS,hps) | bit_field_val(HPE,hpe));
    dpu_write(TFT_VSYNC, bit_field_val(VPS,vps) | bit_field_val(VPE,vpe));
    dpu_write(TFT_HDE, bit_field_val(HDS,hds) | bit_field_val(HDE,hde));
    dpu_write(TFT_VDE, bit_field_val(VDS,vds) | bit_field_val(VDE,vde));

    unsigned long tft_tran_cfg = 0;
    set_bit_field(&tft_tran_cfg, PIX_CLK_INV, pdata->tft.pix_clk_polarity == AT_FALLING_EDGE);
    set_bit_field(&tft_tran_cfg, DE_DL, pdata->tft.de_active_level == AT_LOW_LEVEL);
    set_bit_field(&tft_tran_cfg, SYNC_DL, pdata->tft.hsync_vsync_active_level == AT_LOW_LEVEL);
    set_bit_field(&tft_tran_cfg, COLOR_EVEN, pdata->tft.even_line_order);
    set_bit_field(&tft_tran_cfg, COLOR_ODD, pdata->tft.odd_line_order);
    set_bit_field(&tft_tran_cfg, MODE, pdata->lcd_mode);

    dpu_write(TFT_CFG, tft_tran_cfg);

    unsigned long lpcdr = cpm_read(LPCDR);
    set_bit_field(&lpcdr, LCD_IO_INV, pdata->tft.pix_clk_inv == INVERT_ENABLE);
    cpm_write(LPCDR, lpcdr);
}

static void inline init_slcd_mipi(struct lcdc_data *pdata)
{
    unsigned long slcd_cfg = dpu_read(SLCD_CFG);
    set_bit_field(&slcd_cfg, TE_SWITCH, pdata->mipi.slcd_te_pin_mode == TE_LCDC_TRIGGER);
    set_bit_field(&slcd_cfg, TE_DP, pdata->mipi.slcd_te_data_transfered_edge == AT_RISING_EDGE);
    set_bit_field(&slcd_cfg, DWIDTH, 4);
    set_bit_field(&slcd_cfg, CWIDTH, 0);
    dpu_write(SLCD_CFG, slcd_cfg);

    dpu_write(SLCD_WR_DUTY, 0);
    dpu_write(SLCD_TIMING, 0);

    unsigned long slcd_frm_size = 0;
    set_bit_field(&slcd_frm_size, V_SIZE, pdata->yres);
    set_bit_field(&slcd_frm_size, H_SIZE, pdata->xres);
    dpu_write(SLCD_FRM_SIZE, slcd_frm_size);

    dpu_write(SLCD_SLOW_TIME, 0);
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
    dpu_write(SLCD_CFG, slcd_cfg);

    dpu_write(SLCD_WR_DUTY, 0);
    dpu_write(SLCD_TIMING, 0);

    unsigned long slcd_frm_size = 0;
    set_bit_field(&slcd_frm_size, V_SIZE, pdata->yres);
    set_bit_field(&slcd_frm_size, H_SIZE, pdata->xres);
    dpu_write(SLCD_FRM_SIZE, slcd_frm_size);

    dpu_write(SLCD_SLOW_TIME, 0);
}

static inline int is_tft(struct lcdc_data *pdata)
{
    return pdata->lcd_mode <= TFT_8BITS_DUMMY_SERIAL;
}

static inline int is_slcd(struct lcdc_data *pdata)
{
    return pdata->lcd_mode >= SLCD_6800 && pdata->lcd_mode != SLCD_MIPI;
}

static inline int is_slcd_mipi(struct lcdc_data *pdata)
{
    return pdata->lcd_mode == SLCD_MIPI;
}

static inline int is_tft_mipi(struct lcdc_data *pdata)
{
    return pdata->lcd_mode == TFT_MIPI;
}

static inline int is_video_mode(struct lcdc_data *pdata)
{
    return pdata->lcd_mode <= TFT_MIPI;
}

static void init_lcdc(struct lcdc_data *pdata, struct framedesc *desc)
{
    unsigned long intc = dpu_read(INTC);
    set_bit_field(&intc, EOD_MSK, 1);
    set_bit_field(&intc, EOS_MSK, 1);
    // set_bit_field(&intc, EOC_MSK, 1);
    set_bit_field(&intc, SCA_MSK, 1);
    set_bit_field(&intc, UOT_MSK, 1);
    set_bit_field(&intc, SSA_MSK, 1);
    dpu_write(INTC, intc);

    dpu_write(CLR_ST, dpu_read(INT_FLAG));

    unsigned long com_cfg = dpu_read(COM_CFG);
    set_bit_field(&com_cfg, BURST_LEN_BDMA, 3);
    dpu_write(COM_CFG, com_cfg);

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

    unsigned long disp_com = dpu_read(DISP_COM);
    set_bit_field(&disp_com, DP_DITHER_EN, dither_en);
    set_bit_field(&disp_com, DP_DITHER_DW, dither_dw);

    if (is_slcd_mipi(pdata))
        set_bit_field(&disp_com, DP_IF_SEL, 3);
    else if (is_slcd(pdata))
        set_bit_field(&disp_com, DP_IF_SEL, 2);
    else
        set_bit_field(&disp_com, DP_IF_SEL, 1);

    dpu_write(DISP_COM, disp_com);

    if (is_tft_mipi(pdata))
        init_tft_mipi(pdata);

    if (is_slcd_mipi(pdata))
        init_slcd_mipi(pdata);

    if (is_tft(pdata))
        init_tft(pdata);

    if (is_slcd(pdata))
        init_slcd(pdata);
}

static void init_frame_desc(
    struct lcdc_data *pdata,
    struct framedesc *desc,
    struct layerdesc *layer0,
    struct layerdesc *layer1,
    struct layerdesc *layer2,
    struct layerdesc *layer3)
{
    desc->FrameCfgAddr = virt_to_phys(desc);

    set_bit_field(&desc->FrameSize, f_Width, pdata->xres);
    set_bit_field(&desc->FrameSize, f_Height, pdata->yres);

    desc->FrameCtrl = 0;
    set_bit_field(&desc->FrameCtrl, f_DirectEn, 1);
    set_bit_field(&desc->FrameCtrl, f_WriteBack, 0);
    set_bit_field(&desc->FrameCtrl, f_stop, !is_video_mode(pdata));

    desc->WritebackBufferAddr = 0;
    desc->WritebackStride = pdata->xres;

    desc->Layer0CfgAddr = virt_to_phys(layer0);
    desc->Layer1CfgAddr = virt_to_phys(layer1);
    desc->Layer2CfgAddr = virt_to_phys(layer2);
    desc->Layer3CfgAddr = virt_to_phys(layer3);

    desc->LayerCfgScaleEn = 0;
    set_bit_field(&desc->LayerCfgScaleEn, f_layer0order, 0);
    set_bit_field(&desc->LayerCfgScaleEn, f_layer1order, 1);
    set_bit_field(&desc->LayerCfgScaleEn, f_layer2order, 2);
    set_bit_field(&desc->LayerCfgScaleEn, f_layer3order, 3);

    desc->InterruptControl = 0;
    set_bit_field(&desc->InterruptControl, f_EOD_MSK, 1);
    set_bit_field(&desc->InterruptControl, f_SOC_MSK, 1);
    set_bit_field(&desc->InterruptControl, f_EOC_MSK, 1);
}

static void lcdc_config_srdma_desc(struct srdmadesc *desc, struct srdma_cfg *cfg)
{
    int format;
    switch (cfg->fb_fmt) {
    case fb_fmt_RGB555:
        format = 0; break;
    case fb_fmt_RGB565:
        format = 2; break;
    case fb_fmt_ARGB8888:
    case fb_fmt_RGB888:
        format = 4; break;
    default:
        panic("format err:%d\n", cfg->fb_fmt); break;
    }

    desc->RdmaNextCfgAddr = virt_to_phys(desc);

    desc->FrameCtrl = 0;
    set_bit_field(&desc->FrameCtrl, s_Format, format);
    set_bit_field(&desc->FrameCtrl, s_Color, 0);
    set_bit_field(&desc->FrameCtrl, s_CHAIN_END, !cfg->is_video);
    set_bit_field(&desc->FrameCtrl, s_Change2Comp, 1);

    desc->InterruptControl = 0;
    set_bit_field(&desc->InterruptControl, s_EOS_MSK, 1);
    // set_bit_field(&desc->InterruptControl, s_EOD_MSK, 1);

    desc->stride = cfg->stride;

    desc->FrameBufferAddr = virt_to_phys(cfg->fb_mem);
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

static int slcd_spi_pixclock_cycle(struct lcdc_data *pdata)
{
    assert(pdata->lcd_mode >= SLCD_SPI_3LINE);

    int cycle = 0;
    int width = pdata->slcd.mcu_data_width;
    int pix_fmt = pdata->out_format;
    if (width == MCU_WIDTH_8BITS) {
        if (pix_fmt == OUT_FORMAT_RGB565)
            cycle = 16;
        if (pix_fmt == OUT_FORMAT_RGB888)
            cycle = 24;
    }
    if (width == MCU_WIDTH_9BITS) {
        if (pix_fmt == OUT_FORMAT_RGB666)
            cycle = 24;
    }
    if (width == MCU_WIDTH_16BITS) {
            cycle = 16;
    }

    assert(cycle);

    return cycle * 2;
}

static void auto_calculate_pixel_clock(struct lcdc_data *pdata)
{
    int hps = pdata->hsync_len;
    int hpe = hps + pdata->left_margin + pdata->xres + pdata->right_margin;
    int vps = pdata->vsync_len;
    int vpe = vps + pdata->upper_margin + pdata->yres + pdata->lower_margin;

    if (!pdata->refresh)
        pdata->refresh = 40;

    if (is_tft(pdata) || is_tft_mipi(pdata)) {
        if (!pdata->pixclock)
            pdata->pixclock = hpe * vpe * pdata->refresh;
    }

    if (is_slcd(pdata)) {
        if (!pdata->pixclock) {
            pdata->pixclock = pdata->xres * pdata->yres * pdata->refresh;
            if (pdata->lcd_mode >= SLCD_SPI_3LINE)
                pdata->pixclock *= slcd_spi_pixclock_cycle(pdata);
            else
                pdata->pixclock *= slcd_pixclock_cycle(pdata);
        }

        if (!pdata->slcd.pixclock_when_init)
            pdata->slcd.pixclock_when_init = pdata->xres * pdata->yres * 3;
    }

    if (is_slcd_mipi(pdata)) {
        if (!pdata->pixclock) {
            pdata->pixclock = pdata->xres * pdata->yres * pdata->refresh * 4;
        }
    }
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
            ret = tft_init_gpio(8, 8, 8);
        break;
    case TFT_18BITS:
        if (pdata->out_format == OUT_FORMAT_RGB666)
            ret = tft_18bit_init_gpio();
        else
            printk(KERN_ERR "lcdc: tft 18bits only support out_format_rgb666!\n");
        break;
    case TFT_16BITS:
        if (pdata->out_format == OUT_FORMAT_RGB565)
            ret = tft_16bit_init_gpio();
        else
            printk(KERN_ERR "lcdc: tft 16bits only support out_format_rgb565!\n");
        break;
    case TFT_8BITS_SERIAL:
    case TFT_8BITS_DUMMY_SERIAL:
        ret = tft_serial_init_gpio();
        break;
    case TFT_MIPI:
        ret = 0;
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
    case SLCD_MIPI:
        if (pdata->mipi.slcd_te_pin_mode == TE_LCDC_TRIGGER)
            ret = mipi_slcd_init_te();
        else
            ret = 0;
        break;
    case SLCD_SPI_3LINE:
    case SLCD_SPI_4LINE:
        ret = slcd_init_gpio_data0(pdata->slcd.enable_rdy_pin, pdata->slcd.te_pin_mode == TE_LCDC_TRIGGER, 0);
        break;

    default:
        printk("This mode is not currently implemented: %d\n", pdata->lcd_mode);
        break;
    }

    return ret;
}

static inline void lcdc_dump_regs(void)
{
    printk("FRM_CFG_ADDR %08x\n", dpu_read(FRM_CFG_ADDR));
    printk("FRM_CFG_CTRL %08x\n", dpu_read(FRM_CFG_CTRL));
    printk("CTRL %08x\n", dpu_read(CTRL));
    printk("ST %08x\n", dpu_read(ST));
    printk("CLR_ST %08x\n", dpu_read(CLR_ST));
    printk("INTC %08x\n", dpu_read(INTC));
    printk("INT_FLAG %08x\n", dpu_read(INT_FLAG));
    printk("COM_CFG %08x\n", dpu_read(COM_CFG));
    printk("PCFG_RD_CTRL %08x\n", dpu_read(PCFG_RD_CTRL));
    printk("PCFG_OFIFO %08x\n", dpu_read(PCFG_OFIFO));

    printk("FRM_DES %08x\n", dpu_read(FRM_DES));
    printk("FRM_DES %08x\n", dpu_read(FRM_DES));
    printk("FRM_DES %08x\n", dpu_read(FRM_DES));
    printk("FRM_DES %08x\n", dpu_read(FRM_DES));
    printk("FRM_DES %08x\n", dpu_read(FRM_DES));
    printk("FRM_DES %08x\n", dpu_read(FRM_DES));
    printk("FRM_DES %08x\n", dpu_read(FRM_DES));
    printk("FRM_DES %08x\n", dpu_read(FRM_DES));
    printk("FRM_DES %08x\n", dpu_read(FRM_DES));
    printk("FRM_DES %08x\n", dpu_read(FRM_DES));
    printk("FRM_DES %08x\n", dpu_read(FRM_DES));

    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));
    printk("LAY0_DES_READ %08x\n", dpu_read(LAY0_DES_READ));

    printk("LAY1_DES_READ %08x\n", dpu_read(LAY1_DES_READ));
    printk("FRM_CHAIN_SITE %08x\n", dpu_read(FRM_CHAIN_SITE));
    printk("LAY0_Y_SITE %08x\n", dpu_read(LAY0_Y_SITE));
    printk("LAY0_UV_SITE %08x\n", dpu_read(LAY0_UV_SITE));
    printk("LAY1_Y_SITE %08x\n", dpu_read(LAY1_Y_SITE));
    printk("LAY1_UV_SITE %08x\n", dpu_read(LAY1_UV_SITE));
    printk("LAY0_CSC_MULT_YRV %08x\n", dpu_read(LAY0_CSC_MULT_YRV));
    printk("LAY0_CSC_MULT_GUGV %08x\n", dpu_read(LAY0_CSC_MULT_GUGV));
    printk("LAY0_CSC_MULT_BU %08x\n", dpu_read(LAY0_CSC_MULT_BU));
    printk("LAY0_CSC_SUB_YUV %08x\n", dpu_read(LAY0_CSC_SUB_YUV));
    printk("LAY1_CSC_MULT_YRV %08x\n", dpu_read(LAY1_CSC_MULT_YRV));
    printk("LAY1_CSC_MULT_GUGV %08x\n", dpu_read(LAY1_CSC_MULT_GUGV));
    printk("LAY1_CSC_MULT_BU %08x\n", dpu_read(LAY1_CSC_MULT_BU));
    printk("LAY1_CSC_SUB_YUV %08x\n", dpu_read(LAY1_CSC_SUB_YUV));
    printk("DISP_COM %08x\n", dpu_read(DISP_COM));
    printk("TFT_HSYNC %08x\n", dpu_read(TFT_HSYNC));
    printk("TFT_VSYNC %08x\n", dpu_read(TFT_VSYNC));
    printk("TFT_HDE %08x\n", dpu_read(TFT_HDE));
    printk("TFT_VDE %08x\n", dpu_read(TFT_VDE));
    printk("TFT_CFG %08x\n", dpu_read(TFT_CFG));
    printk("TFT_ST %08x\n", dpu_read(TFT_ST));
    printk("SLCD_CFG %08x\n", dpu_read(SLCD_CFG));
    printk("SLCD_WR_DUTY %08x\n", dpu_read(SLCD_WR_DUTY));
    printk("SLCD_TIMING %08x\n", dpu_read(SLCD_TIMING));
    printk("SLCD_FRM_SIZE %08x\n", dpu_read(SLCD_FRM_SIZE));
    printk("SLCD_SLOW_TIME %08x\n", dpu_read(SLCD_SLOW_TIME));
    printk("SLCD_REG_IF %08x\n", dpu_read(SLCD_REG_IF));
    printk("SLCD_ST %08x\n", dpu_read(SLCD_ST));
    printk("SLCD_REG_CTRL %08x\n", dpu_read(SLCD_REG_CTRL));

}
