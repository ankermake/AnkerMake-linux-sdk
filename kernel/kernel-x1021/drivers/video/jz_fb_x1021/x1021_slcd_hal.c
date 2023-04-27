#include <hal_x1021/x1021_slcd_hal.h>
#include "x1021_slcd_regs.h"
#include "ingenic_bit_field.h"
#include "ingenic_common.h"

#define LCDC_REG_BASE 0xB3050000

#define LCDC_ADDR(reg) ((volatile unsigned int *)(LCDC_REG_BASE + (reg)))

static inline void jzfb_write_reg(unsigned int reg, unsigned int value)
{
    *LCDC_ADDR(reg) = value;
}

static inline unsigned int jzfb_read_reg(unsigned int reg)
{
    return *LCDC_ADDR(reg);
}

static inline void jzfb_set_bit_field(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(LCDC_ADDR(reg), start, end, val);
}

static inline unsigned int jzfb_get_bit_field(unsigned int reg, int start, int end)
{
    return get_bit_field(LCDC_ADDR(reg), start, end);
}

void jzfb_hal_init(struct jzfb_config_data *config)
{
    assert(config->fb_format != SRC_FORMAT_reserve);
    assert_range(config->fb_format, SRC_FORMAT_555, SRC_FORMAT_NV21);
    assert_range(config->out_format, OUT_FORMAT_565, OUT_FORMAT_888);
    assert_range(config->out_order, ORDER_RGB, ORDER_BGR);
    assert_range(config->mcu_data_width, MCU_WIDTH_8BITS, MCU_WIDTH_24BITS);
    assert_range(config->mcu_cmd_width, MCU_WIDTH_8BITS, MCU_WIDTH_24BITS);
    assert_bool(config->refresh_mode);
    assert_bool(config->wr_data_sample_edge);
    assert_bool(config->dc_pin);
    assert_bool(config->te_data_transfered_level);
    assert_bool(config->rdy_cmd_send_level);

    unsigned int glb_cfg = jzfb_read_reg(GLB_CFG);
    unsigned int disp_com = jzfb_read_reg(DISP_COM);
    unsigned int slcd_cfg = jzfb_read_reg(SLCD_CFG);
    unsigned int slcd_wr_duty = jzfb_read_reg(SLCD_WR_DUTY);
    unsigned int slcd_timing = jzfb_read_reg(SLCD_TIMING);

    set_bit_field(&glb_cfg, GLB_CFG_COLOR, config->out_order);
    set_bit_field(&glb_cfg, GLB_CFG_FORMAT, config->fb_format);
    set_bit_field(&glb_cfg, GLB_CFG_DMA_SEL, config->fb_format >= SRC_FORMAT_NV12);

    // enable dma clk
    set_bit_field(&glb_cfg, GLB_CFG_CLKGATE_CLS, 1);

    if (config->fb_format == SRC_FORMAT_888) {
        if (config->out_format == OUT_FORMAT_565) {
            set_bit_field(&disp_com, DISP_COM_DITHER_DW, 0x26);
            set_bit_field(&disp_com, DISP_COM_DITHER_EN, 1);
            set_bit_field(&disp_com, DISP_COM_DISP_CG_CLS, 0);
        } else if (config->out_format == OUT_FORMAT_666) {
            set_bit_field(&disp_com, DISP_COM_DITHER_DW, 0x15);
            set_bit_field(&disp_com, DISP_COM_DITHER_EN, 1);
            set_bit_field(&disp_com, DISP_COM_DISP_CG_CLS, 0);
        } else {
            set_bit_field(&disp_com, DISP_COM_DITHER_EN, 0);
            set_bit_field(&disp_com, DISP_COM_DISP_CG_CLS, 1);
        }
    } else {
        set_bit_field(&disp_com, DISP_COM_DITHER_EN, 0);
        set_bit_field(&disp_com, DISP_COM_DISP_CG_CLS, 1);
    }

    set_bit_field(&slcd_cfg, SLCD_CFG_FRM_MD, config->refresh_mode);
    set_bit_field(&slcd_cfg, SLCD_CFG_DATA_FMT, config->out_format);
    set_bit_field(&slcd_cfg, SLCD_CFG_TE_SWITCH, config->te_pin == TE_LCDC_TRIGGER);
    set_bit_field(&slcd_cfg, SLCD_CFG_TE_DP, !config->te_data_transfered_level);
    set_bit_field(&slcd_cfg, SLCD_CFG_RDY_SWITCH, config->enable_rdy_pin);
    set_bit_field(&slcd_cfg, SLCD_CFG_RDY_DP, !config->rdy_cmd_send_level);
    set_bit_field(&slcd_cfg, SLCD_CFG_DC_MD, config->dc_pin);
    set_bit_field(&slcd_cfg, SLCD_CFG_WR_MD, config->wr_data_sample_edge);
    set_bit_field(&slcd_cfg, SLCD_CFG_DWIDTH, config->mcu_data_width);
    set_bit_field(&slcd_cfg, SLCD_CFG_CWIDTH, config->mcu_cmd_width);
    set_bit_field(&slcd_cfg, SLCD_CFG_DBI_TYPE, 2); // bus type 1: 6800 2: 8080
    set_bit_field(&slcd_cfg, SLCD_CFG_RDY_ANTI_JIT, 1); // rdy pin sample cycle 0: 1 cycle 1: 3 cycle
    set_bit_field(&slcd_cfg, SLCD_CFG_TE_ANTI_JIT, 1); // te pin sample cycle 0: 1 cycle 1: 3 cycle
    set_bit_field(&slcd_cfg, SLCD_CFG_TE_MD, 0); // active edge of TE: 0: front edge 1: back edge

    set_bit_field(&slcd_wr_duty, SLCD_WR_DUTY_DSTIME, 0);
    set_bit_field(&slcd_wr_duty, SLCD_WR_DUTY_DDTIME, 0);
    set_bit_field(&slcd_wr_duty, SLCD_WR_DUTY_CSTIME, 0);
    set_bit_field(&slcd_wr_duty, SLCD_WR_DUTY_CDTIME, 0);

    set_bit_field(&slcd_timing, SLCD_TIMING_TCH, 0);
    set_bit_field(&slcd_timing, SLCD_TIMING_TCS, 0);
    set_bit_field(&slcd_timing, SLCD_TIMING_TAH, 0);
    set_bit_field(&slcd_timing, SLCD_TIMING_TAS, 0);

    jzfb_write_reg(GLB_CFG, glb_cfg);
    jzfb_write_reg(DISP_COM, disp_com);
    jzfb_write_reg(SLCD_CFG, slcd_cfg);
    jzfb_write_reg(SLCD_WR_DUTY, slcd_wr_duty);
    jzfb_write_reg(SLCD_TIMING, slcd_timing);

    jzfb_set_bit_field(SLCD_FRM_SIZE, SLCD_FRM_SIZE_H_SIZE, config->xres);
    jzfb_set_bit_field(SLCD_FRM_SIZE, SLCD_FRM_SIZE_V_SIZE, config->yres);

    jzfb_set_bit_field(SLCD_SLOW_TIME, SLCD_SLOW_TIME_SLOW_TIME, 0);
}

int jzfb_slcd_wait_busy(unsigned int count_udelay)
{
    int busy;

    busy = jzfb_get_bit_field(SLCD_ST, SLCD_ST_BUSY);
    while (count_udelay-- && busy) {
        udelay(1);
        busy = jzfb_get_bit_field(SLCD_ST, SLCD_ST_BUSY);
    }

    return busy;
}

void jzfb_slcd_send_cmd(unsigned int cmd)
{
    unsigned int val = 0;

    jzfb_slcd_wait_busy(10 * 1000);

    set_bit_field(&val, SLCD_REG_IF_FLAG, 2); // mark as command
    set_bit_field(&val, SLCD_REG_IF_CONTENT, cmd & 0x00ffffff);
    jzfb_write_reg(SLCD_REG_IF, val);
}

void jzfb_slcd_send_param(unsigned int data)
{
    unsigned int val = 0;

    jzfb_slcd_wait_busy(10 * 1000);

    set_bit_field(&val, SLCD_REG_IF_FLAG, 1); // mark as parameter
    set_bit_field(&val, SLCD_REG_IF_CONTENT, data & 0x00ffffff);
    jzfb_write_reg(SLCD_REG_IF, val);
}

void jzfb_slcd_send_data(unsigned int data)
{
    unsigned int val = 0;

    jzfb_slcd_wait_busy(10 * 1000);

    set_bit_field(&val, SLCD_REG_IF_FLAG, 0); // mark as data
    set_bit_field(&val, SLCD_REG_IF_CONTENT, data & 0x00ffffff);
    jzfb_write_reg(SLCD_REG_IF, val);
}

void jzfb_disable_slcd_fmt_convert(void)
{
    jzfb_set_bit_field(SLCD_CFG, SLCD_CFG_FMT_EN, 0);
}

void jzfb_enable_slcd_fmt_convert(void)
{
    jzfb_set_bit_field(SLCD_CFG, SLCD_CFG_FMT_EN, 1);
}

unsigned int jzfb_line_stride_pixels(struct jzfb_config_data *config, unsigned int xres)
{
    assert(config->fb_format != SRC_FORMAT_NV12);
    assert(config->fb_format != SRC_FORMAT_NV21);

    unsigned int align = config->fb_format == SRC_FORMAT_888 ? 4 : 2;
    unsigned int stride = xres + align - 1;

    return stride - stride % align;
}

unsigned int jzfb_bytes_per_pixel(struct jzfb_config_data *config)
{
    assert(config->fb_format != SRC_FORMAT_NV12);
    assert(config->fb_format != SRC_FORMAT_NV21);

    return config->fb_format == SRC_FORMAT_888 ? 4 : 2;
}

void jzfb_set_frame_desc(unsigned int desc_addr)
{
    jzfb_write_reg(CHAIN_ADDR, desc_addr);
    jzfb_set_bit_field(CTRL, CTRL_START, 1);
}

void jzfb_scld_start_dma(void)
{
    jzfb_set_bit_field(CTRL, CTRL_SLCD_START, 1);
}

void jzfb_general_stop(void)
{
    jzfb_set_bit_field(CTRL, CTRL_GEN_STOP, 1);
}

void jzfb_quick_stop(void)
{
    jzfb_set_bit_field(CTRL, CTRL_QCK_STOP, 1);
}

void jzfb_disable_all_interrupt(void)
{
    unsigned int val = jzfb_read_reg(INTC);
    set_bit_field(&val, INTC_EOD_MSK, 0);
    set_bit_field(&val, INTC_EOF_MSK, 0);
    set_bit_field(&val, INTC_GSA_MASK, 0);
    set_bit_field(&val, INTC_QSA_MASK, 0);

    jzfb_write_reg(INTC, val);
}

void jzfb_enable_interrupt(enum jzfb_interrupt_type irq_type)
{
    jzfb_set_bit_field(INTC, irq_type, irq_type, 1);
}

void jzfb_disable_interrupt(enum jzfb_interrupt_type irq_type)
{
    jzfb_set_bit_field(INTC, irq_type, irq_type, 0);
}

unsigned int jzfb_get_interrupts(void)
{
    return jzfb_read_reg(INT_FLAG);
}

void jzfb_clear_interrupt(enum jzfb_interrupt_type irq_type)
{
    jzfb_set_bit_field(CSR, irq_type, irq_type, 1);
}

int jzfb_check_interrupt(unsigned int flags, enum jzfb_interrupt_type irq_type)
{
    return get_bit_field(&flags, irq_type, irq_type);
}

unsigned int jzfb_get_current_frame_desc(void)
{
    return jzfb_read_reg(CHAIN_SITE);
}

unsigned int jzfb_user_read_reg(unsigned int reg)
{
    return jzfb_read_reg(reg);
}

void jzfb_user_write_reg(unsigned int reg, unsigned int value)
{
    return jzfb_write_reg(reg, value);
}

void jzfb_user_set_bit_field(unsigned int reg, int start, int end, unsigned int val)
{
    jzfb_set_bit_field(reg, start, end, val);
}

unsigned int jzfb_user_get_bit_field(unsigned int reg, int start, int end)
{
    return jzfb_get_bit_field(reg, start, end);
}