#include <common.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <utils/clock.h>
#include "include/bit_field.h"

#include "lcdc_reg.h"
#include "lcdc_data.h"

#define LCDC_IOBASE 0xB3050000
#define LCDC_ADDR(reg) ((volatile unsigned long *)(LCDC_IOBASE + reg))

struct lcdc_frame_desc {
    unsigned long next_desc_addr;
    unsigned long buffer_addr;
    unsigned long frame_id;
    unsigned long cmd;
    unsigned long offsize;
    unsigned long page_width;
    unsigned long cnum_pos;
    unsigned long dessize;
};

struct jzfb_lcd_msg {
    void *fb_mem;
    unsigned int xres;
    unsigned int yres;
    unsigned int bytes_per_line;
    unsigned int bytes_per_frame;
    unsigned int frame_count;
    unsigned int frame_alloc_size;
};

enum jzfb_interrupt_type {
    FRAME_START = 4,
    FRAME_END = 5,
    QUICK_STOP = 7,
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

static inline void lcdc_enable(void)
{
    lcdc_set_bit(LCDCTRL, LCDCTRL_ENA, 1);
}

static inline void lcdc_disable(void)
{
    lcdc_set_bit(LCDCTRL, LCDCTRL_ENA, 0);
}

static void lcdc_write_framedesc(unsigned int descaddr)
{
    lcdc_write(LCDDA0, descaddr);
}

static void lcdc_start_dma(void)
{
    lcdc_set_bit(MCTRL, MCTRL_DMASTART, 1);
}

static inline unsigned int get_burst_len(struct lcdc_data *data)
{
    return 4; // 64 word burst
}

static inline int get_data_times(struct lcdc_data *data)
{
    enum lcdc_mcu_data_width data_width = data->slcd.mcu_data_width;

    unsigned int times;
    if (data_width == MCU_WIDTH_9BITS)
        times = 2;
    else if (data_width == MCU_WIDTH_8BITS)
        times = (data->out_format <= OUT_FORMAT_565) ? 2 : 3;
    else
        times = 1;

    return times;
}

void lcdc_dump_regs(void)
{
    printk("LCDCFG: %08x\n", lcdc_read(LCDCFG));
    printk("LCDCTRL: %08x\n", lcdc_read(LCDCTRL));
    printk("LCDSTATE: %08x\n", lcdc_read(LCDSTATE));
    printk("LCDOSDC: %08x\n", lcdc_read(LCDOSDC));
    printk("LCDOSDCTRL: %08x\n", lcdc_read(LCDOSDCTRL));
    printk("LCDOSDS: %08x\n", lcdc_read(LCDOSDS));
    printk("LCDBGC0: %08x\n", lcdc_read(LCDBGC0));
    printk("LCDBGC1: %08x\n", lcdc_read(LCDBGC1));
    printk("LCDKEY0: %08x\n", lcdc_read(LCDKEY0));
    printk("LCDKEY1: %08x\n", lcdc_read(LCDKEY1));
    printk("LCDALPHA: %08x\n", lcdc_read(LCDALPHA));
    printk("LCDRGBC: %08x\n", lcdc_read(LCDRGBC));
    printk("LCDVAT: %08x\n", lcdc_read(LCDVAT));
    printk("LCDDAH: %08x\n", lcdc_read(LCDDAH));
    printk("LCDDAV: %08x\n", lcdc_read(LCDDAV));
    printk("LCDXYP0: %08x\n", lcdc_read(LCDXYP0));
    printk("LCDXYP1: %08x\n", lcdc_read(LCDXYP1));
    printk("LCDSIZE0: %08x\n", lcdc_read(LCDSIZE0));
    printk("LCDSIZE1: %08x\n", lcdc_read(LCDSIZE1));
    printk("LCDVSYNC: %08x\n", lcdc_read(LCDVSYNC));
    printk("LCDHSYNC: %08x\n", lcdc_read(LCDHSYNC));
    printk("LCDIID: %08x\n", lcdc_read(LCDIID));
    printk("LCDDA0: %08x\n", lcdc_read(LCDDA0));
    printk("LCDSA0: %08x\n", lcdc_read(LCDSA0));
    printk("LCDFID0: %08x\n", lcdc_read(LCDFID0));
    printk("LCDCMD0: %08x\n", lcdc_read(LCDCMD0));
    printk("LCDOFFS0: %08x\n", lcdc_read(LCDOFFS0));
    printk("LCDPW0: %08x\n", lcdc_read(LCDPW0));
    printk("LCDCNUM0: %08x\n", lcdc_read(LCDCNUM0));
    printk("LCDPOS0: %08x\n", lcdc_read(LCDPOS0));
    printk("LCDDESSIZE0: %08x\n", lcdc_read(LCDDESSIZE0));
    printk("LCDDA1: %08x\n", lcdc_read(LCDDA1));
    printk("LCDSA1: %08x\n", lcdc_read(LCDSA1));
    printk("LCDFID1: %08x\n", lcdc_read(LCDFID1));
    printk("LCDCMD1: %08x\n", lcdc_read(LCDCMD1));
    printk("LCDOFFS1: %08x\n", lcdc_read(LCDOFFS1));
    printk("LCDPW1: %08x\n", lcdc_read(LCDPW1));
    printk("LCDCNUM1: %08x\n", lcdc_read(LCDCNUM1));
    printk("LCDPOS1: %08x\n", lcdc_read(LCDPOS1));
    printk("LCDDESSIZE1: %08x\n", lcdc_read(LCDDESSIZE1));
    printk("LCDPCFG: %08x\n", lcdc_read(LCDPCFG));
    printk("MCFG: %08x\n", lcdc_read(MCFG));
    printk("MCFG_NEW: %08x\n", lcdc_read(MCFG_NEW));
    printk("MCTRL: %08x\n", lcdc_read(MCTRL));
    printk("MSTATE: %08x\n", lcdc_read(MSTATE));
    printk("MDATA: %08x\n", lcdc_read(MDATA));
    printk("WTIME: %08x\n", lcdc_read(WTIME));
    printk("TASH: %08x\n", lcdc_read(TASH));
    printk("SMWT: %08x\n", lcdc_read(SMWT));
}

void lcdc_hal_init(struct lcdc_data *data)
{
    unsigned long lcdctrl = lcdc_read(LCDCTRL);
    unsigned long mcfg_new = lcdc_read(MCFG_NEW);
    unsigned long mctrl = lcdc_read(MCTRL);
    unsigned long wtime = 0;
    unsigned long tash = 0;
    unsigned long lcdcfg = 0;
    unsigned long lcdrgbc = 0;

    set_bit_field(&mcfg_new, MCFG_NEW_DTIMES_NEW, get_data_times(data) - 1);
    set_bit_field(&mcfg_new, MCFG_NEW_DWIDTH_NEW, data->slcd.mcu_data_width);
    set_bit_field(&mcfg_new, MCFG_NEW_cmd_9bit, data->slcd.mcu_cmd_width == MCU_WIDTH_9BITS);
    set_bit_field(&mcfg_new, MCFG_NEW_CSPLY_NEW, !data->slcd.wr_data_sample_edge);
    set_bit_field(&mcfg_new, MCFG_NEW_RSPLY_NEW, data->slcd.dc_pin);
    set_bit_field(&mcfg_new, MCFG_NEW_DTYPE_NEW, data->slcd.data_trans_mode);
    set_bit_field(&mcfg_new, MCFG_NEW_CTYPE_NEW, data->slcd.cmd_trans_mode);
    set_bit_field(&mcfg_new, MCFG_NEW_FMT_CONV, 1); // 如果发送初始化数据的时候,这个位要清0

    set_bit_field(&mctrl, MCTRL_NOT_USE_TE, 1);
    set_bit_field(&mctrl, MCTRL_FAST_MODE, 0);

    set_bit_field(&mctrl, MCTRL_DMAMODE, 1);
    set_bit_field(&mctrl, MCTRL_DMASTART, 0);
    set_bit_field(&mctrl, MCTRL_DMATXEN, 1);
    set_bit_field(&mctrl, MCTRL_GATE_MASK, 1); // 如果用寄存器发送数据的时候,这个位要清0
    set_bit_field(&mctrl, MCTRL_NARROW_TE, 0);

    set_bit_field(&wtime, WTIME_CHTIME, 0);
    set_bit_field(&wtime, WTIME_CLTIME, 0);
    set_bit_field(&wtime, WTIME_DHTIME, 0);
    set_bit_field(&wtime, WTIME_DLTIME, 0);

    set_bit_field(&tash, TASH_TAH, 0);
    set_bit_field(&tash, TASH_TAS, 0);

    // 这个寄存器的设置不起作用,姑且写在这里
    set_bit_field(&lcdcfg, LCDCFG_LCDPIN, 1);
    set_bit_field(&lcdcfg, LCDCFG_NEWDES, 1);
    set_bit_field(&lcdcfg, LCDCFG_RECOVER, 1);
    set_bit_field(&lcdcfg, LCDCFG_MODE, 13);

    // 这个寄存器的设置不起作用,姑且也写在这里
    set_bit_field(&lcdrgbc, LCDC_RGBC_RGBFMT, 1);
    set_bit_field(&lcdrgbc, LCDC_RGBC_ODDRGB, LCDC_RGBC_ODD_RGB);
    set_bit_field(&lcdrgbc, LCDC_RGBC_EVENRGB, LCDC_RGBC_EVEN_RGB);

    set_bit_field(&lcdctrl, LCDCTRL_BST, get_burst_len(data)); // 尽量选择大的burst len
    set_bit_field(&lcdctrl, LCDCTRL_EOFM, 1);  // 开启帧结束中断
    set_bit_field(&lcdctrl, LCDCTRL_IFUM0, 1); // 开启under run 中断
    set_bit_field(&lcdctrl, LCDCTRL_SOFM, 0);
    set_bit_field(&lcdctrl, LCDCTRL_QDM, 0);
    set_bit_field(&lcdctrl, LCDCTRL_BEDN, 0);
    set_bit_field(&lcdctrl, LCDCTRL_PEDN, 0);
    set_bit_field(&lcdctrl, LCDCTRL_ENA, 0);

    static const unsigned char cmd_width[] = {1, 0, 0, 2, 3};
    lcdc_set_bit(MCFG, MCFG_CWIDTH, cmd_width[data->slcd.mcu_cmd_width]);

    lcdc_write(MCFG_NEW, mcfg_new);
    lcdc_write(MCTRL, mctrl);
    lcdc_write(WTIME, wtime);
    lcdc_write(TASH, tash);
    lcdc_write(SMWT, 0);
    lcdc_write(LCDHSYNC, 0);
    lcdc_write(LCDVSYNC, 0);
    lcdc_write(LCDDAH, data->xres);
    lcdc_write(LCDDAV, data->yres);
    lcdc_write(LCDVAT, data->xres << 16 | data->yres);
    lcdc_write(LCDCFG, lcdcfg);
    lcdc_write(LCDRGBC, lcdrgbc);
    lcdc_write(LCDPCFG, 0xC0000000 | (511 << 18) | (400 << 9) | (256 << 0));
    lcdc_write(LCDCTRL, lcdctrl);
}

static inline int lcdc_get_busy(void)
{
    return lcdc_get_bit(MSTATE, MSTATE_BUSY);
}

static int lcdc_wait_busy(unsigned int count)
{
    int busy;

    busy = lcdc_get_bit(MSTATE, MSTATE_BUSY);
    while (count-- && busy) {
        busy = lcdc_get_bit(MSTATE, MSTATE_BUSY);
    }

    return busy;
}


static void lcdc_enable_register_mode(void)
{
    if (lcdc_wait_busy(10 * 1000))
        panic("lcdc busy\n");

    unsigned long mctrl = lcdc_read(MCTRL);
    set_bit_field(&mctrl, MCTRL_DMATXEN, 0);
    set_bit_field(&mctrl, MCTRL_GATE_MASK, 0);
    lcdc_write(MCTRL, mctrl);

    unsigned long mcfg_new = lcdc_read(MCFG_NEW);
    set_bit_field(&mcfg_new, MCFG_NEW_DTIMES_NEW, 0);
    set_bit_field(&mcfg_new, MCFG_NEW_FMT_CONV, 0);
    lcdc_write(MCFG_NEW, mcfg_new);
}

static void lcdc_disable_register_mode(struct lcdc_data *data)
{
    if (lcdc_wait_busy(10 * 1000))
        panic("lcdc busy\n");

    unsigned long mctrl = lcdc_read(MCTRL);
    set_bit_field(&mctrl, MCTRL_DMATXEN, 1);
    set_bit_field(&mctrl, MCTRL_GATE_MASK, 0);
    lcdc_write(MCTRL, mctrl);

    unsigned long mcfg_new = lcdc_read(MCFG_NEW);
    set_bit_field(&mcfg_new, MCFG_NEW_DTIMES_NEW, get_data_times(data) - 1);
    set_bit_field(&mcfg_new, MCFG_NEW_FMT_CONV, 1);
    lcdc_write(MCFG_NEW, mcfg_new);
}

void lcdc_send_cmd(unsigned int cmd)
{
    unsigned long val = 0;

    // mutex_lock(&mutex);

    if (lcdc_wait_busy(10 * 1000))
        panic("lcdc busy\n");

    set_bit_field(&val, MDATA_PTR, 1);
    set_bit_field(&val, MDATA_DATA_CMD, cmd);
    lcdc_write(MDATA, val);

    // mutex_unlock(&mutex);
}

void lcdc_send_data(unsigned int data)
{
    unsigned long val = 0;

    // mutex_lock(&mutex);

    if (lcdc_wait_busy(10 * 1000))
        panic("lcdc busy\n");

    set_bit_field(&val, MDATA_PTR, 0);
    set_bit_field(&val, MDATA_DATA_CMD, data);
    lcdc_write(MDATA, val);

    // mutex_unlock(&mutex);
}

static void init_cmd_framedesc(struct lcdc_data *data, void *cmd_buf, struct lcdc_frame_desc *desc, struct lcdc_frame_desc *next_desc)
{
    enum lcdc_mcu_data_width mcu_cmd_width = data->slcd.mcu_cmd_width;

    if (mcu_cmd_width == MCU_WIDTH_8BITS) {
        unsigned char *cmd = cmd_buf;
        cmd[0] = cmd[1] = cmd[2] = cmd[3] = (unsigned char )data->cmd_of_start_frame;
    }
    if (mcu_cmd_width == MCU_WIDTH_9BITS) {
        unsigned short *cmd = cmd_buf;
        cmd[0] = cmd[1] = (unsigned short )data->cmd_of_start_frame;
    }
    if (mcu_cmd_width == MCU_WIDTH_16BITS) {
        unsigned short *cmd = cmd_buf;
        cmd[0] = cmd[1] = (unsigned short )data->cmd_of_start_frame;
    }
    if (mcu_cmd_width == MCU_WIDTH_24BITS) {
        unsigned int *cmd = cmd_buf;
        *cmd = data->cmd_of_start_frame;
    }

    desc->next_desc_addr = virt_to_phys(next_desc);
    desc->buffer_addr = virt_to_phys(cmd_buf);
    desc->frame_id = 0;
    desc->cmd = 0;
    set_bit_field(&desc->cmd, LCDCMD_CMD, 1);
    set_bit_field(&desc->cmd, LCDCMD_FRM_EN, 1);
    set_bit_field(&desc->cmd, LCDCMD_LEN, 1);
    desc->offsize = 0;
    desc->page_width = 0;
    desc->dessize = 0;

    if (mcu_cmd_width == MCU_WIDTH_8BITS)
        desc->cnum_pos = 4;
    if (mcu_cmd_width == MCU_WIDTH_9BITS)
        desc->cnum_pos = 2;
    if (mcu_cmd_width == MCU_WIDTH_16BITS)
        desc->cnum_pos = 2;
    if (mcu_cmd_width == MCU_WIDTH_24BITS)
        desc->cnum_pos = 1;
}

static void init_data_framedesc(struct lcdc_data *data,
                                struct jzfb_lcd_msg *lcd,
                                struct lcdc_frame_desc *desc,
                                struct lcdc_frame_desc *next_desc)
{
    desc->next_desc_addr = virt_to_phys(next_desc);
    desc->buffer_addr = virt_to_phys(lcd->fb_mem);
    desc->frame_id = 1;
    desc->cmd = 0;
    set_bit_field(&desc->cmd, LCDCMD_SOFINT, 0);
    set_bit_field(&desc->cmd, LCDCMD_EOFINT, 1);
    set_bit_field(&desc->cmd, LCDCMD_CMD, 0);
    set_bit_field(&desc->cmd, LCDCMD_FRM_EN, 1);
    set_bit_field(&desc->cmd, LCDCMD_LEN, lcd->bytes_per_line * lcd->yres / 4);
    desc->offsize = 0;
    desc->page_width = 0;
    desc->cnum_pos = 0;
    set_bit_field(&desc->cnum_pos, LCDCPOS_RGB0, data->fb_format == fb_fmt_RGB555);
    set_bit_field(&desc->cnum_pos, LCDCPOS_BPP0, data->fb_format == fb_fmt_RGB888 ? 5 : 4);
    set_bit_field(&desc->cnum_pos, LCDCPOS_YPOS0, 0);
    set_bit_field(&desc->cnum_pos, LCDCPOS_XPOS0, 0);
    desc->dessize = 0;
    set_bit_field(&desc->dessize, LCDDESSIZE_Width, lcd->xres);
    set_bit_field(&desc->dessize, LCDDESSIZE_Height, lcd->yres);
}

static void lcdc_set_te(struct lcdc_data *data)
{
    unsigned long mctrl = lcdc_read(MCTRL);

    if (data->slcd.te_pin_mode == TE_LCDC_TRIGGER) {
        set_bit_field(&mctrl, MCTRL_NOT_USE_TE, 0);
        set_bit_field(&mctrl, MCTRL_TE_INV, data->slcd.te_data_transfered_edge == AT_FALLING_EDGE);
        lcdc_write(MCTRL, mctrl);
    }
}

static int lcdc_check_interrupt(unsigned long *flage, enum jzfb_interrupt_type irq_type)
{
    *flage = lcdc_read(LCDSTATE);

    return get_bit_field(flage, irq_type, irq_type);
}

static void lcdc_clear_interrupt(enum jzfb_interrupt_type irq_type)
{
    lcdc_set_bit(LCDSTATE, irq_type, irq_type, 0);
}