#include <common.h>
#include <soc/base.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <utils/clock.h>
#include "include/bit_field.h"

#include "lcdc_regs.h"
#include "lcdc_data.h"
#include "lcd_gpio.c"


struct srdmadesc {
    unsigned long RdmaNextCfgAddr;
    unsigned long FrameBufferAddr;
    unsigned long stride;
    unsigned long FrameCtrl;
    unsigned long InterruptControl;
};


#define LCD_ADDR(reg)  ((volatile unsigned long *)CKSEG1ADDR(DPU_IOBASE + reg))

void lcd_write(unsigned int reg, unsigned int val)
{
    *LCD_ADDR(reg) = val;
}

unsigned int lcd_read(unsigned int reg)
{
    return *LCD_ADDR(reg);
}

void lcd_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(LCD_ADDR(reg), start, end, val);
}

unsigned int lcd_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(LCD_ADDR(reg), start, end);
}


void lcd_genernal_stop_display(void)
{
    lcd_write(CTRL, bit_field_val(GEN_STP_SRD, 1));
}

void lcd_quick_stop_display(void)
{
    lcd_write(CTRL, bit_field_val(QCK_STP_SRD, 1));
}

void lcd_start_simple_read(void)
{
    lcd_write(SRD_CHAIN_CTRL, bit_field_val(SRD_CHAIN_START, 1));
}

static inline int is_tft(struct lcdc_data *pdata)
{
    return pdata->lcd_mode <= TFT_8BITS_DUMMY_SERIAL;
}

static inline int is_slcd(struct lcdc_data *pdata)
{
    return pdata->lcd_mode >= SLCD_6800;
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

static int slcd_wait_busy_us(unsigned int count_udelay)
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

static int slcd_wait_busy(unsigned int count)
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
    set_bit_field(&tft_tran_cfg, VSYNC_DL, pdata->tft.vsync_active_level == AT_LOW_LEVEL);
    set_bit_field(&tft_tran_cfg, HSYNC_DL, pdata->tft.hsync_active_level == AT_LOW_LEVEL);
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


static void init_srdma_desc(struct srdmadesc *desc, struct srdma_cfg *cfg)
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

    desc->InterruptControl = 0;
    // set_bit_field(&desc->InterruptControl, s_EOS_MSK, 1);
    set_bit_field(&desc->InterruptControl, s_EOD_MSK, 1);

    desc->stride = cfg->stride;

    desc->FrameBufferAddr = virt_to_phys(cfg->fb_mem);

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

static void init_lcdc(struct lcdc_data *pdata)
{
    unsigned long intc = 0;
    set_bit_field(&intc, EOD_MSK, 1);
    set_bit_field(&intc, UOT_MSK, 1);
    set_bit_field(&intc, SSA_MSK, 1);
    // set_bit_field(&intc, EOS_MSK, 1);

    lcd_write(INTC, intc);

    lcd_write(CLR_ST, lcd_read(INT_FLAG));

    unsigned long com_cfg = lcd_read(COM_CFG);
    set_bit_field(&com_cfg, BURST_LEN_RDMA, 3);
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
    set_bit_field(&disp_com, SLCD_CLKGATE_EN, 1);
    set_bit_field(&disp_com, TFT_CLKGATE_EN, 1);

    if (is_slcd(pdata))
        set_bit_field(&disp_com, DP_IF_SEL, 2);
    else
        set_bit_field(&disp_com, DP_IF_SEL, 1);

    lcd_write(DISP_COM, disp_com);

    if (is_tft(pdata))
        init_tft(pdata);

    if (is_slcd(pdata))
        init_slcd(pdata);
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
    }

    if (is_slcd(pdata)) {
        if (!pdata->pixclock) {
            pdata->pixclock = pdata->xres * pdata->yres * pdata->refresh;
            pdata->pixclock *= slcd_pixclock_cycle(pdata);
        }

        if (!pdata->slcd.pixclock_when_init)
            pdata->slcd.pixclock_when_init = pdata->xres * pdata->yres * 3;
    }
}
