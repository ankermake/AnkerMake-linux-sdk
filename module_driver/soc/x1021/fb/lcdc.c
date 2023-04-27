#include <common.h>
#include <soc/base.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <utils/clock.h>
#include "include/bit_field.h"

#include "lcdc_reg.h"
#include "lcdc_data.h"

#define X1021_LCDC_IOBASE   0xB3050000
#define LCDC_ADDR(reg) ((volatile unsigned long *)(X1021_LCDC_IOBASE + reg))

struct lcdc_frame_desc {
    unsigned int next_desc_addr;
    unsigned int buffer_addr_rgb;
    unsigned int stride_rgb;
    unsigned int chain_end;
    unsigned int eof_mask;
    unsigned int buffer_addr_uv;
    unsigned int stride_uv;
    unsigned int reserve;
};

enum lcdc_interrupt_type {
    lcdc_irq_frame_end = 1,
    lcdc_irq_dma_end = 2,
    lcdc_irq_general_stop = 5,
    lcdc_irq_quick_stop = 6,
};

static inline void lcdc_write(unsigned int reg, int val)
{
    *LCDC_ADDR(reg) = val;
}

static inline unsigned int lcdc_read(unsigned int reg)
{
    return *LCDC_ADDR(reg);
}

static inline void lcdc_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(LCDC_ADDR(reg), start, end, val);
}

static inline unsigned int lcdc_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(LCDC_ADDR(reg), start, end);
}

static void lcdc_disable_all_interrupt(void)
{
    unsigned long val = lcdc_read(INTC);
    set_bit_field(&val, INTC_EOD_MSK, 0);
    set_bit_field(&val, INTC_EOF_MSK, 0);
    set_bit_field(&val, INTC_GSA_MASK, 0);
    set_bit_field(&val, INTC_QSA_MASK, 0);

    lcdc_write(INTC, val);
}

static inline void lcdc_enable_interrupt(enum lcdc_interrupt_type irq_type)
{
    lcdc_set_bit(INTC, irq_type, irq_type, 1);
}

static inline void lcdc_disable_interrupt(enum lcdc_interrupt_type irq_type)
{
    lcdc_set_bit(INTC, irq_type, irq_type, 0);
}

static int lcdc_check_interrupt(unsigned long flags, enum lcdc_interrupt_type irq_type)
{
    return get_bit_field(&flags, irq_type, irq_type);
}

static void lcdc_clear_interrupt(enum lcdc_interrupt_type irq_type)
{
    lcdc_set_bit(CSR, irq_type, irq_type, 1);
}

static unsigned long lcdc_get_interrupts(void)
{
    return lcdc_read(INT_FLAG);
}

static inline int lcdc_get_busy(void)
{
    return lcdc_get_bit(SLCD_ST, SLCD_ST_BUSY);
}

// static int lcdc_wait_busy_us(unsigned int count_udelay)
// {
//     int busy;
//     uint64_t old = systick_get_time_us();

//     busy = lcdc_get_bit(SLCD_ST, SLCD_ST_BUSY);
//     while (busy && systick_get_time_us() - old < count_udelay) {
//         usleep(500);
//         busy = lcdc_get_bit(SLCD_ST, SLCD_ST_BUSY);
//     }

//     return busy;
// }

static int lcdc_wait_busy(unsigned int count)
{
    int busy;

    busy = lcdc_get_bit(SLCD_ST, SLCD_ST_BUSY);
    while (count-- && busy) {
        busy = lcdc_get_bit(SLCD_ST, SLCD_ST_BUSY);
    }

    return busy;
}

void lcdc_disable_slcd_fmt_convert(void)
{
    lcdc_set_bit(SLCD_CFG, SLCD_CFG_FMT_EN, 0);
}

void lcdc_enable_slcd_fmt_convert(void)
{
    lcdc_set_bit(SLCD_CFG, SLCD_CFG_FMT_EN, 1);
}

void lcdc_dump_regs(void)
{
    printk("offset   name          value    \n");
    printk("--------------------------------\n");
    printk("0x%04x CHAIN_ADDR     0x%08x\n", CHAIN_ADDR,     lcdc_read(CHAIN_ADDR));
    printk("0x%04x GLB_CFG        0x%08x\n", GLB_CFG,        lcdc_read(GLB_CFG));
    printk("0x%04x CSC_MULT_YRV   0x%08x\n", CSC_MULT_YRV,   lcdc_read(CSC_MULT_YRV));
    printk("0x%04x CSC_MULT_GUGV  0x%08x\n", CSC_MULT_GUGV,  lcdc_read(CSC_MULT_GUGV));
    printk("0x%04x CSC_MULT_BU    0x%08x\n", CSC_MULT_BU,    lcdc_read(CSC_MULT_BU));
    printk("0x%04x CSC_SUB_YUV    0x%08x\n", CSC_SUB_YUV,    lcdc_read(CSC_SUB_YUV));
    printk("0x%04x CTRL           0x%08x\n", CTRL,           lcdc_read(CTRL));
    printk("0x%04x ST             0x%08x\n", ST,             lcdc_read(ST));
    printk("0x%04x CSR            0x%08x\n", CSR,            lcdc_read(CSR));
    printk("0x%04x INTC           0x%08x\n", INTC,           lcdc_read(INTC));
    printk("0x%04x PCFG           0x%08x\n", PCFG,           lcdc_read(PCFG));
    printk("0x%04x INT_FLAG       0x%08x\n", INT_FLAG,       lcdc_read(INT_FLAG));
    printk("0x%04x RGB_DMA_SITE   0x%08x\n", RGB_DMA_SITE,   lcdc_read(RGB_DMA_SITE));
    printk("0x%04x Y_DMA_SITE     0x%08x\n", Y_DMA_SITE,     lcdc_read(Y_DMA_SITE));
    printk("0x%04x CHAIN_SITE     0x%08x\n", CHAIN_SITE,     lcdc_read(CHAIN_SITE));
    printk("0x%04x UV_DMA_SITE    0x%08x\n", UV_DMA_SITE,    lcdc_read(UV_DMA_SITE));
    printk("0x%04x DES_READ       0x%08x\n", DES_READ,       lcdc_read(DES_READ));
    printk("0x%04x DISP_COM       0x%08x\n", DISP_COM,       lcdc_read(DISP_COM));
    printk("0x%04x SLCD_CFG       0x%08x\n", SLCD_CFG,       lcdc_read(SLCD_CFG));
    printk("0x%04x SLCD_WR_DUTY   0x%08x\n", SLCD_WR_DUTY,   lcdc_read(SLCD_WR_DUTY));
    printk("0x%04x SLCD_TIMING    0x%08x\n", SLCD_TIMING,    lcdc_read(SLCD_TIMING));
    printk("0x%04x SLCD_FRM_SIZE  0x%08x\n", SLCD_FRM_SIZE,  lcdc_read(SLCD_FRM_SIZE));
    printk("0x%04x SLCD_SLOW_TIME 0x%08x\n", SLCD_SLOW_TIME, lcdc_read(SLCD_SLOW_TIME));
    printk("0x%04x SLCD_REG_IF    0x%08x\n", SLCD_REG_IF,    lcdc_read(SLCD_REG_IF));
    printk("0x%04x SLCD_ST        0x%08x\n", SLCD_ST,        lcdc_read(SLCD_ST));
    printk("--------------------------\n");
}

void lcdc_hal_init(struct lcdc_data *data)
{
    unsigned long glb_cfg = lcdc_read(GLB_CFG);
    unsigned long disp_com = lcdc_read(DISP_COM);
    unsigned long slcd_cfg = lcdc_read(SLCD_CFG);
    unsigned long slcd_wr_duty = lcdc_read(SLCD_WR_DUTY);
    unsigned long slcd_timing = lcdc_read(SLCD_TIMING);

    set_bit_field(&glb_cfg, GLB_CFG_COLOR, data->slcd.out_order);
    set_bit_field(&glb_cfg, GLB_CFG_FORMAT, data->fb_format);
    set_bit_field(&glb_cfg, GLB_CFG_DMA_SEL, data->fb_format >= fb_fmt_NV12);

    set_bit_field(&glb_cfg, GLB_CFG_CLKGATE_CLS, 1);

    if (data->fb_format == fb_fmt_RGB888) {
        if (data->out_format == OUT_FORMAT_565) {
            set_bit_field(&disp_com, DISP_COM_DITHER_DW, 0b100110);
            set_bit_field(&disp_com, DISP_COM_DITHER_EN, 1);
            set_bit_field(&disp_com, DISP_COM_DISP_CG_CLS, 0);
        } else if (data->out_format == OUT_FORMAT_666) {
            set_bit_field(&disp_com, DISP_COM_DITHER_DW, 0b010101);
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

    set_bit_field(&slcd_cfg, SLCD_CFG_FRM_MD, 0);
    set_bit_field(&slcd_cfg, SLCD_CFG_DATA_FMT, data->out_format);
    set_bit_field(&slcd_cfg, SLCD_CFG_TE_SWITCH, data->slcd.te_pin_mode == TE_LCDC_TRIGGER);
    set_bit_field(&slcd_cfg, SLCD_CFG_TE_DP, data->slcd.te_data_transfered_edge);
    set_bit_field(&slcd_cfg, SLCD_CFG_RDY_SWITCH, data->slcd.enable_rdy_pin);
    set_bit_field(&slcd_cfg, SLCD_CFG_RDY_DP, data->slcd.rdy_cmd_send_level);

    set_bit_field(&slcd_cfg, SLCD_CFG_DC_MD, data->slcd.dc_pin);
    set_bit_field(&slcd_cfg, SLCD_CFG_WR_MD, data->slcd.wr_data_sample_edge);
    set_bit_field(&slcd_cfg, SLCD_CFG_DWIDTH, data->slcd.mcu_data_width);
    set_bit_field(&slcd_cfg, SLCD_CFG_CWIDTH, data->slcd.mcu_cmd_width);
    set_bit_field(&slcd_cfg, SLCD_CFG_DBI_TYPE, data->lcd_mode); // bus type 1: 6800 2: 8080
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

    lcdc_write(GLB_CFG, glb_cfg);
    lcdc_write(DISP_COM, disp_com);
    lcdc_write(SLCD_CFG, slcd_cfg);
    lcdc_write(SLCD_WR_DUTY, slcd_wr_duty);
    lcdc_write(SLCD_TIMING, slcd_timing);

    lcdc_set_bit(SLCD_FRM_SIZE, SLCD_FRM_SIZE_H_SIZE, data->xres);
    lcdc_set_bit(SLCD_FRM_SIZE, SLCD_FRM_SIZE_V_SIZE, data->yres);

    lcdc_set_bit(SLCD_SLOW_TIME, SLCD_SLOW_TIME_SLOW_TIME, 0);
}

void jzfb_send_cmd(unsigned int cmd)
{
    unsigned long val = 0;

    // mutex_lock(&mutex);

    if (lcdc_wait_busy(10 * 1000))
        panic("lcdc busy\n");

    set_bit_field(&val, SLCD_REG_IF_FLAG, 2);
    set_bit_field(&val, SLCD_REG_IF_CONTENT, cmd & 0x00ffffff);
    lcdc_write(SLCD_REG_IF, val);

    // mutex_unlock(&mutex);
}

void jzfb_send_data(unsigned int data)
{
    unsigned long val = 0;

    // mutex_lock(&mutex);

    if (lcdc_wait_busy(10 * 1000))
        panic("lcdc busy\n");

    set_bit_field(&val, SLCD_REG_IF_FLAG, 1);
    set_bit_field(&val, SLCD_REG_IF_CONTENT, data & 0x00ffffff);
    lcdc_write(SLCD_REG_IF, val);

    // mutex_unlock(&mutex);
}

static void init_frame_end_desc(
    struct lcdc_frame_desc *desc, void *fb_mem, unsigned int line_stride)
{
    desc->next_desc_addr = virt_to_phys(desc);
    desc->buffer_addr_rgb = virt_to_phys(fb_mem);
    desc->buffer_addr_uv =  virt_to_phys(fb_mem);
    desc->stride_rgb = line_stride;
    desc->stride_uv = line_stride;
    desc->chain_end = 1; // end of desc chain
    desc->eof_mask =  0; // no frame end interrupt
}

static void init_frame_data_desc(
    struct lcdc_frame_desc *desc, void *fb_mem, unsigned int line_stride, int enable_frame_end_irq,
    struct lcdc_frame_desc *next_desc)
{
    desc->next_desc_addr = virt_to_phys(next_desc);
    desc->buffer_addr_uv =  virt_to_phys(fb_mem);
    desc->buffer_addr_rgb = virt_to_phys(fb_mem);
    desc->stride_rgb = line_stride;
    desc->stride_uv = line_stride;
    desc->chain_end = 0; //not end of desc chain
    desc->eof_mask = enable_frame_end_irq ? 2 : 0; //frame end interrupt
}

static void lcdc_general_stop(void)
{
    lcdc_set_bit(CTRL, CTRL_GEN_STOP, 1);
}

static void lcdc_set_frame_desc(unsigned int desc_addr)
{
    lcdc_write(CHAIN_ADDR, desc_addr);
    lcdc_set_bit(CTRL, CTRL_START, 1);
}

static void lcdc_start_dma(void)
{
    lcdc_set_bit(CTRL, CTRL_SLCD_START, 1);
}


