#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>

#include "ingenic_common.h"
#include "x1021_fb_linux.h"
#include "x1021_slcd_regs.h"
#include "x1021_slcd_gpio_init.c"
#include "fb_rotate.c"

#define debug_info(x...) \
    do { \
        if (jzfb->enable_debug) \
            printk(x); \
    } while (0)

static void jzfb_print_lcdc_regs(struct jzfb *jzfb)
{
    printk("offset   name          value    \n");
    printk("--------------------------------\n");
    printk("0x%04x CHAIN_ADDR     0x%08x\n", CHAIN_ADDR,     jzfb_user_read_reg(CHAIN_ADDR));
    printk("0x%04x GLB_CFG        0x%08x\n", GLB_CFG,        jzfb_user_read_reg(GLB_CFG));
    printk("0x%04x CSC_MULT_YRV   0x%08x\n", CSC_MULT_YRV,   jzfb_user_read_reg(CSC_MULT_YRV));
    printk("0x%04x CSC_MULT_GUGV  0x%08x\n", CSC_MULT_GUGV,  jzfb_user_read_reg(CSC_MULT_GUGV));
    printk("0x%04x CSC_MULT_BU    0x%08x\n", CSC_MULT_BU,    jzfb_user_read_reg(CSC_MULT_BU));
    printk("0x%04x CSC_SUB_YUV    0x%08x\n", CSC_SUB_YUV,    jzfb_user_read_reg(CSC_SUB_YUV));
    printk("0x%04x CTRL           0x%08x\n", CTRL,           jzfb_user_read_reg(CTRL));
    printk("0x%04x ST             0x%08x\n", ST,             jzfb_user_read_reg(ST));
    printk("0x%04x CSR            0x%08x\n", CSR,            jzfb_user_read_reg(CSR));
    printk("0x%04x INTC           0x%08x\n", INTC,           jzfb_user_read_reg(INTC));
    printk("0x%04x PCFG           0x%08x\n", PCFG,           jzfb_user_read_reg(PCFG));
    printk("0x%04x INT_FLAG       0x%08x\n", INT_FLAG,       jzfb_user_read_reg(INT_FLAG));
    printk("0x%04x RGB_DMA_SITE   0x%08x\n", RGB_DMA_SITE,   jzfb_user_read_reg(RGB_DMA_SITE));
    printk("0x%04x Y_DMA_SITE     0x%08x\n", Y_DMA_SITE,     jzfb_user_read_reg(Y_DMA_SITE));
    printk("0x%04x CHAIN_SITE     0x%08x\n", CHAIN_SITE,     jzfb_user_read_reg(CHAIN_SITE));
    printk("0x%04x UV_DMA_SITE    0x%08x\n", UV_DMA_SITE,    jzfb_user_read_reg(UV_DMA_SITE));
    printk("0x%04x DES_READ       0x%08x\n", DES_READ,       jzfb_user_read_reg(DES_READ));
    printk("0x%04x DISP_COM       0x%08x\n", DISP_COM,       jzfb_user_read_reg(DISP_COM));
    printk("0x%04x SLCD_CFG       0x%08x\n", SLCD_CFG,       jzfb_user_read_reg(SLCD_CFG));
    printk("0x%04x SLCD_WR_DUTY   0x%08x\n", SLCD_WR_DUTY,   jzfb_user_read_reg(SLCD_WR_DUTY));
    printk("0x%04x SLCD_TIMING    0x%08x\n", SLCD_TIMING,    jzfb_user_read_reg(SLCD_TIMING));
    printk("0x%04x SLCD_FRM_SIZE  0x%08x\n", SLCD_FRM_SIZE,  jzfb_user_read_reg(SLCD_FRM_SIZE));
    printk("0x%04x SLCD_SLOW_TIME 0x%08x\n", SLCD_SLOW_TIME, jzfb_user_read_reg(SLCD_SLOW_TIME));
    printk("0x%04x SLCD_REG_IF    0x%08x\n", SLCD_REG_IF,    jzfb_user_read_reg(SLCD_REG_IF));
    printk("0x%04x SLCD_ST        0x%08x\n", SLCD_ST,        jzfb_user_read_reg(SLCD_ST));
    printk("--------------------------\n");
}

static void jzfb_print_members(struct jzfb *jzfb)
{
    printk("jzfb->bytes_per_pixel            %u\n", jzfb->bytes_per_pixel);
    printk("jzfb->line_stride                %u\n", jzfb->line_stride);
    printk("jzfb->frame_size                 %u\n", jzfb->frame_size);
    printk("jzfb->frame_nums                 %u\n", jzfb->frame_nums);
    printk("jzfb->fb_mem                     %p\n", jzfb->fb_mem);
    printk("jzfb->frame_desc                 %p\n", jzfb->frame_desc);
    printk("jzfb->frame_desc.next_desc_addr  %08x\n", jzfb->frame_desc->next_desc_addr);
    printk("jzfb->frame_desc.buffer_addr_rgb %08x\n", jzfb->frame_desc->buffer_addr_rgb);
    printk("jzfb->frame_desc.stride_rgb      %u\n", jzfb->frame_desc->stride_rgb);
    printk("jzfb->frame_desc.chain_end       %x\n", jzfb->frame_desc->chain_end);
    printk("jzfb->frame_desc.eof_mask        %x\n", jzfb->frame_desc->eof_mask);
    printk("jzfb->frame_desc.buffer_addr_uv  %08x\n", jzfb->frame_desc->buffer_addr_uv);
    printk("jzfb->frame_desc.stride_uv       %u\n", jzfb->frame_desc->stride_uv);
    printk("jzfb->user_fb_mem                %p\n", jzfb->user_fb_mem);
    printk("jzfb->user_fb_mem_phys           %08x\n", (unsigned int)jzfb->user_fb_mem_phys);
    printk("jzfb->user_line_stride           %d\n", jzfb->user_line_stride);
    printk("jzfb->user_bytes_per_pixel       %d\n", jzfb->user_bytes_per_pixel); 
    printk("jzfb->user_frame_size            %d\n", jzfb->user_frame_size);
    printk("jzfb->user_frame_nums            %d\n", jzfb->user_frame_nums); 
    printk("jzfb->user_xres                  %d\n", jzfb->user_xres);
    printk("jzfb->user_yres                  %d\n", jzfb->user_yres);
    printk("jzfb->user_width                 %d\n", jzfb->user_width);
    printk("jzfb->user_height                %d\n", jzfb->user_height);
}

static void jzfb_init_user_info(struct jzfb *jzfb)
{
    struct jzfb_lcd_pdata *pdata = jzfb->pdata;

    if (!jzfb->pan_display_en ||
        jzfb->fb_copy_type == FB_COPY_TYPE_NONE) {
        jzfb->user_fb_mem = jzfb->fb_mem;
        jzfb->user_bytes_per_pixel = jzfb->bytes_per_pixel;
        jzfb->user_line_stride = jzfb->line_stride;
        jzfb->user_frame_nums = jzfb->frame_nums;
        jzfb->user_frame_size = jzfb->frame_size;
        jzfb->user_xres = pdata->config.xres;
        jzfb->user_yres = pdata->config.yres;
        jzfb->user_width = pdata->width;
        jzfb->user_height = pdata->height;
        return;
    }

    if (jzfb->fb_copy_type == FB_COPY_TYPE_ROTATE_90 || 
        jzfb->fb_copy_type == FB_COPY_TYPE_ROTATE_270) {
        jzfb->user_xres = pdata->config.yres;
        jzfb->user_yres = pdata->config.xres;
        jzfb->user_width = pdata->height;
        jzfb->user_height = pdata->width;
    } else {
        jzfb->user_xres = pdata->config.xres;
        jzfb->user_yres = pdata->config.yres;
        jzfb->user_width = pdata->width;
        jzfb->user_height = pdata->height;
    }

    jzfb->user_bytes_per_pixel = jzfb->bytes_per_pixel;
    jzfb->user_line_stride = jzfb_line_stride_pixels(&pdata->config, jzfb->user_xres);
    jzfb->user_frame_size = jzfb->user_line_stride * jzfb->user_yres * jzfb->user_bytes_per_pixel;
    jzfb->user_frame_size = ALIGN(jzfb->user_frame_size, PAGE_SIZE);
    jzfb->user_frame_nums = 1;

    jzfb->user_fb_mem = kmalloc(jzfb->user_frame_size * jzfb->user_frame_nums, GFP_KERNEL);
    jzfb->user_fb_mem_phys = virt_to_phys(jzfb->user_fb_mem);
    assert(jzfb->user_fb_mem);
}

static void jzfb_init_video_mode(struct jzfb *jzfb, struct fb_videomode *mode)
{
    struct jzfb_config_data *config = &jzfb->pdata->config;

    mode->name = config->name;
    mode->refresh = config->refresh;
    mode->xres = jzfb->user_xres;
    mode->yres = jzfb->user_yres;
    mode->pixclock = config->pixclock;
    mode->left_margin = config->left_margin;
    mode->right_margin = config->right_margin;
    mode->upper_margin = config->upper_margin;
    mode->lower_margin = config->lower_margin;
    mode->hsync_len = config->hsync_len;
    mode->vsync_len = config->vsync_len;
    mode->sync = 0;
    mode->vmode = 0;
    mode->flag = 0;
}

static void jzfb_init_fix_info(struct jzfb *jzfb, struct fb_fix_screeninfo *fix)
{
    strcpy(fix->id, "jzfb");
    fix->type = FB_TYPE_PACKED_PIXELS;
    fix->visual = FB_VISUAL_TRUECOLOR;
    fix->xpanstep = 0;
    fix->ypanstep = 1;
    fix->ywrapstep = 0;
    fix->accel = FB_ACCEL_NONE;
    fix->line_length = jzfb->user_bytes_per_pixel * jzfb->user_line_stride;
    fix->smem_start = virt_to_phys(jzfb->user_fb_mem);
    fix->smem_len = jzfb->user_frame_size * jzfb->user_frame_nums;
    fix->mmio_start = 0;
    fix->mmio_len = 0;
}

static inline void set_fb_bitfield(struct fb_bitfield *field, int length, int offset, int msb_right)
{
    field->length = length;
    field->offset = offset;
    field->msb_right = msb_right;
}

static void jzfb_init_var_info(struct jzfb *jzfb, struct fb_var_screeninfo *var)
{
    struct jzfb_config_data *config = &jzfb->pdata->config;

    var->xres = jzfb->user_xres;
    var->yres = jzfb->user_yres;
    var->xres_virtual = jzfb->user_xres;
    var->yres_virtual = jzfb->user_yres * jzfb->user_frame_nums;
    var->xoffset = 0;
    var->yoffset = 0;
    var->height = jzfb->user_height;
    var->width = jzfb->user_width;
    var->pixclock = config->pixclock;
    var->left_margin = config->left_margin;
    var->right_margin = config->right_margin;
    var->upper_margin = config->upper_margin;
    var->lower_margin = config->lower_margin;
    var->hsync_len = config->hsync_len;
    var->vsync_len = config->vsync_len;
    var->sync = 0;
    var->vmode = 0;

    var->bits_per_pixel = jzfb->user_bytes_per_pixel * 8;
    switch (config->fb_format) {
    case SRC_FORMAT_555:
        set_fb_bitfield(&var->red, 5, 10, 0);
        set_fb_bitfield(&var->green, 5, 5, 0);
        set_fb_bitfield(&var->blue, 5, 0, 0);
        set_fb_bitfield(&var->transp, 0, 0, 0);
        break;
    case SRC_FORMAT_565:
        set_fb_bitfield(&var->red, 5, 11, 0);
        set_fb_bitfield(&var->green, 6, 5, 0);
        set_fb_bitfield(&var->blue, 5, 0, 0);
        set_fb_bitfield(&var->transp, 0, 0, 0);
        break;
    case SRC_FORMAT_888:
        set_fb_bitfield(&var->red, 8, 16, 0);
        set_fb_bitfield(&var->green, 8, 8, 0);
        set_fb_bitfield(&var->blue, 8, 0, 0);
        set_fb_bitfield(&var->transp, 0, 0, 0);
        break;
    default:
        assert(0);
    }
}

static struct jzfb_user_data *jzfb_find_user_data(struct jzfb *jzfb, pid_t tgid)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(jzfb->user_data); i++) {
        if (jzfb->user_data[i].tgid == tgid)
            return &jzfb->user_data[i];
    }

    return NULL;
}

static void jzfb_set_user_data(struct jzfb *jzfb, pid_t tgid, void *map_addr)
{
    struct jzfb_user_data *data = jzfb_find_user_data(jzfb, tgid);

    assert(data);
    data->map_addr = data->count == 1 ? map_addr : 0;
}

static int jzfb_add_user_data(struct jzfb *jzfb, pid_t tgid)
{
    struct jzfb_user_data *data = jzfb_find_user_data(jzfb, tgid);

    if (data) {
        data->map_addr = 0;
        data->count++;
        return 0;
    }

    data = jzfb_find_user_data(jzfb, 0);
    if (data) {
        data->tgid = tgid;
        data->count = 1;
        data->map_addr = 0;
        return 0;
    }

    return -ENOMEM;
}

static void jzfb_del_user_data(struct jzfb *jzfb, pid_t tgid)
{
    struct jzfb_user_data *data = jzfb_find_user_data(jzfb, tgid);

    if (data && data->count-- == 1) {
        data->tgid = 0;
        data->map_addr = 0;
    }
}

static void init_frame_end_desc(
    struct jzfb_frame_desc *desc, void *fb_mem, unsigned int line_stride)
{
    desc->next_desc_addr = virt_to_phys(desc);
    desc->buffer_addr_rgb = virt_to_phys(fb_mem);
    desc->buffer_addr_uv =  virt_to_phys(fb_mem);
    desc->stride_rgb = line_stride;
    desc->stride_uv = line_stride;
    desc->chain_end = 1; // end of desc chain
    desc->eof_mask =  0; // no frame end interrupt
}

static void init_frame_desc(
    struct jzfb_frame_desc *desc, void *fb_mem, unsigned int line_stride, int enable_frame_end_irq,
    struct jzfb_frame_desc *next_desc)
{
    desc->next_desc_addr = virt_to_phys(next_desc);
    desc->buffer_addr_rgb = virt_to_phys(fb_mem);
    desc->buffer_addr_uv =  virt_to_phys(fb_mem);
    desc->stride_rgb = line_stride;
    desc->stride_uv = line_stride;
    desc->chain_end = 0; // not end of desc chain
    desc->eof_mask = enable_frame_end_irq ? 2 : 0; // frame end interrupt
}

static void jzfb_do_start_frame(struct jzfb *jzfb)
{
    jzfb_set_frame_desc(jzfb->frame_desc_phys);
    jzfb_scld_start_dma();
}

static void jzfb_start_frame(struct jzfb *jzfb)
{
    jzfb->frame_sending = 1;

    if (!jzfb->pan_display_en || !jzfb->te_irq) {
        jzfb_do_start_frame(jzfb);
        return;
    }

    if (gpio_get_value(jzfb->te_gpio) == jzfb->te_level) {
        jzfb_do_start_frame(jzfb);
    } else {
        jzfb->te_en = 1;
        enable_irq(jzfb->te_irq);
    }
}

static void jzfb_stop_frame(struct jzfb *jzfb)
{
    jzfb->stop_en = 1;
    jzfb_general_stop();
    wait_event_interruptible(jzfb->wait_queue, jzfb->stop_en == 0);
}

static int jzfb_wait_frame_end(struct jzfb *jzfb)
{
    if (jzfb->pan_display_en) {
        wait_event_interruptible(jzfb->wait_queue, jzfb->frame_sending == 0);
        return jzfb->frame_sending;
    } else {
        int frame_count = jzfb->frame_count;
        wait_event_interruptible(jzfb->wait_queue, jzfb->frame_count != frame_count);
        return 0;
    }
}

static inline int jzfb_wait_busy_msleep(struct jzfb *jzfb, unsigned int msecs)
{
    u64 start = 0;
    const int timeout = msecs * 1000 * 1000;

    while (jzfb_slcd_wait_busy(0)) {
        if (start == 0)
            start = local_clock();
        if (jzfb->wait_frame_end_when_pan_display)
            debug_info("wait scld busy !\n");
        msleep(1);
        if (local_clock() - start >= timeout) { 
            debug_info(KERN_ERR "wait slcd busy time out !\n");
            return -1;
        }
    }

    return 0;
}

static void jzfb_process_slcd_data_table(struct smart_lcd_data_table *table, unsigned int length)
{
    int i = 0;

    for (; i < length; i++) {
        switch (table[i].type) {
        case SMART_CONFIG_CMD:
            jzfb_slcd_send_cmd(table[i].value);
            break;
        case SMART_CONFIG_DATA:
        case SMART_CONFIG_PRM:
            jzfb_slcd_send_param(table[i].value);
            break;
        case SMART_CONFIG_UDELAY:
            if (table[i].value >= 10000) // 一般来讲10 ms 就可以用msleep()
                msleep((table[i].value / 10000) * 10);
            udelay(table[i].value % 10000);
            break;
        default:
            assert(0);
            break;
        }
    }

    jzfb_slcd_wait_busy(10 * 1000);
}

static void jzfb_flush_color(struct jzfb *jzfb, void *mem, struct jzfb_frame_desc *frame_desc, unsigned int color)
{
    struct jzfb_lcd_pdata *pdata = jzfb->pdata;
    unsigned int *ptr = mem;
    unsigned int i, j;

    for (i = 0; i < pdata->config.yres; i++) {
        for (j = 0; j < pdata->config.xres; j++) {
            *ptr++ = color;
        }
        ptr += jzfb->line_stride - pdata->config.xres;
    }

    dma_cache_sync(jzfb->dev, mem, jzfb->frame_size, DMA_TO_DEVICE);

    jzfb_stop_frame(jzfb);
    jzfb_start_frame(jzfb);
    jzfb_wait_frame_end(jzfb);
}

static inline void jzfb_color_test(struct jzfb *jzfb)
{
    int count = 50;

    while (count--) {
        jzfb_flush_color(jzfb, jzfb->fb_mem, jzfb->frame_desc, 0xff0000);

        if (count % 2)
            mdelay(1000);

        jzfb_flush_color(jzfb, jzfb->fb_mem, jzfb->frame_desc, 0x0000ff);

        if (!(count % 2))
            mdelay(1000);
    }
}

static void jzfb_init_gpio(struct jzfb *jzfb)
{
    if (jzfb->gpio_en)
        return;
    jzfb->gpio_en = 1;

    if (jzfb->pdata->init_slcd_gpio)
        jzfb->pdata->init_slcd_gpio(jzfb);
}

static void jzfb_enable_clk(struct jzfb *jzfb)
{
    if (jzfb->clk_en)
        return;
    jzfb->clk_en = 1;

    clk_enable(jzfb->clk_lcd);
    clk_enable(jzfb->clk_ahb1);
    clk_enable(jzfb->clk_lpc);
}

static void jzfb_disable_clk(struct jzfb *jzfb)
{
    if (!jzfb->clk_en)
        return;
    jzfb->clk_en = 0;

    clk_disable(jzfb->clk_lcd);
    clk_disable(jzfb->clk_ahb1);
    clk_disable(jzfb->clk_lpc);
}

static void jzfb_enable_lcd_controller(struct jzfb *jzfb)
{
    if (jzfb->lcdc_en)
        return;
    jzfb->lcdc_en = 1;

    jzfb_hal_init(&jzfb->pdata->config);
    jzfb_disable_all_interrupt();
    jzfb_enable_interrupt(JZFB_irq_general_stop);
    jzfb_enable_interrupt(JZFB_irq_frame_end);
}

static void jzfb_disable_lcd_controller(struct jzfb *jzfb)
{
    if (!jzfb->lcdc_en)
        return;
    jzfb->lcdc_en = 0;

    jzfb_stop_frame(jzfb);
}

static void jzfb_set_clock_rate(struct jzfb *jzfb, unsigned int pix_rate)
{
    unsigned int rate = pix_rate * 3;

    if (jzfb->clk_en) {
        clk_disable(jzfb->clk_lpc);
        clk_set_rate(jzfb->clk_lpc, rate);
        clk_enable(jzfb->clk_lpc);
    } else {
        clk_set_rate(jzfb->clk_lpc, rate);
    }
}

static void jzfb_enable_lcd(struct jzfb *jzfb)
{
    struct jzfb_lcd_pdata *pdata = jzfb->pdata;

    if (jzfb->lcd_en)
        return;
    jzfb->lcd_en = 1;

    jzfb_disable_slcd_fmt_convert();
    jzfb_set_clock_rate(jzfb, jzfb->pdata->config.pixclock_when_init);

    if (pdata->power_on)
        pdata->power_on(jzfb);

    jzfb_process_slcd_data_table(pdata->slcd_data_table, pdata->slcd_data_table_length);

    jzfb_set_clock_rate(jzfb, pdata->config.pixclock);

    jzfb_slcd_send_cmd(pdata->cmd_of_start_frame);

    jzfb_enable_slcd_fmt_convert();
}

static void jzfb_disable_lcd(struct jzfb *jzfb)
{
    struct jzfb_lcd_pdata *pdata = jzfb->pdata;

    if (!jzfb->lcd_en)
        return;
    jzfb->lcd_en = 0;

    jzfb_disable_slcd_fmt_convert();
    if (pdata->power_off)
        pdata->power_off(jzfb);
}

static void jzfb_init(struct jzfb *jzfb)
{
    if (jzfb->jzfb_en)
        return;
    jzfb->jzfb_en = 1;

    jzfb_init_gpio(jzfb);
    jzfb_enable_clk(jzfb);
    jzfb_enable_lcd_controller(jzfb);
    jzfb_enable_lcd(jzfb);

    if (!jzfb->pan_display_en)
        jzfb_start_frame(jzfb);

    if (jzfb->enable_debug) {
        jzfb_print_lcdc_regs(jzfb);
        jzfb_print_members(jzfb);
    }
}

static void jzfb_deinit(struct jzfb *jzfb)
{
    if (!jzfb->jzfb_en)
        return;
    jzfb->jzfb_en = 0;

    jzfb_disable_lcd_controller(jzfb);
    jzfb_disable_lcd(jzfb);
    jzfb_disable_clk(jzfb);
}

static irqreturn_t jzfb_irq_handler(int irq, void *data)
{
    struct jzfb *jzfb = (struct jzfb *)data;
    unsigned int flags = jzfb_get_interrupts();

    if (jzfb_check_interrupt(flags, JZFB_irq_frame_end)) {
        jzfb_clear_interrupt(JZFB_irq_frame_end);
        jzfb->frame_sending = 0;
        jzfb->frame_count += 1;
        wake_up_all(&jzfb->wait_queue);
        debug_info("frame end irq %d\n", gpio_get_value(jzfb->te_gpio));
        return IRQ_HANDLED;
    }

    if (jzfb_check_interrupt(flags, JZFB_irq_dma_end)) {
        jzfb_clear_interrupt(JZFB_irq_dma_end);
        jzfb->frame_sending = 0;
        jzfb->frame_count += 1;
        wake_up_all(&jzfb->wait_queue);
        debug_info("dma end irq\n");
        return IRQ_HANDLED;
    }

    if (jzfb_check_interrupt(flags, JZFB_irq_quick_stop)) {
        jzfb_clear_interrupt(JZFB_irq_quick_stop);
        jzfb->stop_en = 0;
        wake_up_all(&jzfb->wait_queue);
        debug_info("quick stop irq\n");
        return IRQ_HANDLED;
    }

    if (jzfb_check_interrupt(flags, JZFB_irq_general_stop)) {
        jzfb_clear_interrupt(JZFB_irq_general_stop);
        jzfb->stop_en = 0;
        wake_up_all(&jzfb->wait_queue);
        debug_info("general stop irq\n");
        return IRQ_HANDLED;
    }

    return IRQ_HANDLED;
}

static irqreturn_t jzfb_te_irq_handler(int irq, void *data)
{
    struct jzfb *jzfb = (struct jzfb *)data;

    if (jzfb->te_en) {
        debug_info("te irq %d\n", gpio_get_value(jzfb->te_gpio));
        jzfb_do_start_frame(jzfb);
    }

    disable_irq_nosync(irq);

    return IRQ_HANDLED;
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

    /* 0: cachable,write through (cache + cacheline对齐写穿)
     * 1: uncachable,write Acceleration (uncache + 硬件写加速)
     * 2: uncachable
     * 3: cachable
     * pan_display 模式需要调用pan_display 接口,在那里去fast_iob()
     * 把非cacheline 对齐的内存同步一下
     * continuos 模式不会调用pan_display 接口,所以要用 "1"
     */
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    if (jzfb->pan_display_en)
        pgprot_val(vma->vm_page_prot) |= 0;
    else
        pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_WA;

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start, vma->vm_page_prot))
    {
        return -EAGAIN;
    }

    mutex_lock(&jzfb->mutex);
    jzfb_set_user_data(jzfb, current->tgid, (void *)vma->vm_start);
    mutex_unlock(&jzfb->mutex);

    return 0;
}

static int jzfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    struct jzfb *jzfb = info->par;

    if (var->xres != jzfb->user_xres ||
        var->yres != jzfb->user_yres)
        return -EINVAL;

    jzfb_init_var_info(jzfb, var);

    return 0;
}

static int jzfb_set_par(struct fb_info *info)
{
    return jzfb_check_var(&info->var, info);
}

static int jzfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int jzfb_open(struct fb_info *info, int user)
{
    struct jzfb *jzfb = info->par;
    int ret = 0;

    mutex_lock(&jzfb->mutex);

    ret = jzfb_add_user_data(jzfb, current->tgid);
    if (ret)
        goto unlock;

    if (!jzfb->pan_display_en)
        jzfb_init(jzfb);

unlock:
    mutex_unlock(&jzfb->mutex);

    return ret;
}

static int jzfb_release(struct fb_info *info, int user)
{
    struct jzfb *jzfb = info->par;

    mutex_lock(&jzfb->mutex);
    jzfb_del_user_data(jzfb, current->tgid);
    mutex_unlock(&jzfb->mutex);

    return 0;
}

static int jzfb_blank(int blank_mode, struct fb_info *info)
{
    struct jzfb *jzfb = info->par;

    mutex_lock(&jzfb->mutex);

    if (blank_mode == FB_BLANK_UNBLANK) {
        jzfb_init(jzfb);
        if (!jzfb->pan_display_en || jzfb->pdata->refresh_lcd_when_resume)
            jzfb_start_frame(jzfb);
        jzfb->is_suspend = 0;
    } else {
        jzfb_deinit(jzfb);
        jzfb->is_suspend = 1;
    }

    mutex_unlock(&jzfb->mutex);

    return 0;
}

static void jzfb_map_kernel_mem(struct jzfb *jzfb)
{
	struct vm_struct	*kvma;
	struct vm_area_struct	vma;
	pgprot_t		pgprot;
	unsigned long		vaddr;
    unsigned int size = jzfb->frame_size * jzfb->frame_nums;    
    unsigned long fb_mem_phys = virt_to_phys(jzfb->fb_mem);

	kvma = get_vm_area(size, VM_IOREMAP);
    assert(kvma);

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
		assert(0);
	}

    jzfb->fb_mem_mapped = (void *)vaddr;
}

static void jzfb_copy_fb_memory_from_user(struct jzfb *jzfb, int next_frm)
{
    unsigned int *mem, *fb_mem;
    unsigned int xres, yres, src_stride, dst_stride;

    // struct jzfb_user_data *data = jzfb_find_user_data(jzfb, current->tgid);
    // mem = data->map_addr + next_frm * jzfb->user_frame_size;
    mem = jzfb->user_fb_mem + next_frm * jzfb->user_frame_size;
    dst_stride = jzfb->line_stride * jzfb->user_bytes_per_pixel;
    src_stride = jzfb->user_line_stride * jzfb->user_bytes_per_pixel;

    fb_mem = jzfb->fb_mem_mapped;

    switch (jzfb->fb_copy_type) {
    case FB_COPY_TYPE_ROTATE_0:
        memcpy(fb_mem, mem, jzfb->frame_size);
        break;
    case FB_COPY_TYPE_ROTATE_180:
        xres = jzfb->user_xres;
        yres = jzfb->user_yres;
        if (jzfb->user_bytes_per_pixel == 2)
            xres /= 2;
        rotate_180(fb_mem, mem, xres, yres, src_stride, dst_stride);
        break;
    case FB_COPY_TYPE_ROTATE_270:
        xres = jzfb->user_xres;
        yres = jzfb->user_yres;
        if (jzfb->user_bytes_per_pixel == 2)
            xres /= 2;
        rotate_270(fb_mem, mem, xres, yres, src_stride, dst_stride);
        break;
    case FB_COPY_TYPE_ROTATE_90:
        xres = jzfb->user_xres;
        yres = jzfb->user_yres;
        if (jzfb->user_bytes_per_pixel == 2)
            xres /= 2;
        rotate_90(fb_mem, mem, xres, yres, src_stride, dst_stride);
        break;
    default:
        return;
    }
}

static int jzfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    struct jzfb *jzfb = info->par;
    int next_frm;

    if (var->xoffset - info->var.xoffset) {
        dev_err(info->dev, "No support for X panning for now\n");
        return -EINVAL;
    }

    next_frm = var->yoffset / var->yres;
    if (next_frm >= jzfb->user_frame_nums) {
        dev_err(info->dev, "yoffset is out of framebuffer: %d\n", var->yoffset);
        return -EINVAL;
    }

    mutex_lock(&jzfb->mutex);

    if (jzfb->is_suspend)
        goto unlock;

    jzfb_init(jzfb);

    if (!jzfb->pan_display_en) {
        jzfb_wait_frame_end(jzfb);
        goto unlock;
    }

#if 1
    if (jzfb_wait_frame_end(jzfb))
        goto unlock;
#else
    if (jzfb_wait_busy_msleep(jzfb, 200))
        goto unlock;
#endif

    // u64 start = local_clock();
    if (jzfb->fb_copy_type != FB_COPY_TYPE_NONE)
        jzfb_copy_fb_memory_from_user(jzfb, next_frm);
    // printk("%s %lld\n", __FUNCTION__, local_clock() - start);

    /* 刷新非cacheline对齐的写穿透cache/内存 */
    fast_iob();

    /*
     * 不出意外控制器有bug
     * 会将上一帧的最开始的500个左右的像素缓冲
     * 必须要调用一次general stop,把这个缓冲干掉
     */
    jzfb_stop_frame(jzfb);

    jzfb_start_frame(jzfb);

    if (jzfb->wait_frame_end_when_pan_display)
        jzfb_wait_frame_end(jzfb);

unlock:
    mutex_unlock(&jzfb->mutex);

    return 0;
}

static struct fb_ops jzfb_ops = {
    .owner = THIS_MODULE,
    .fb_open = jzfb_open,
    .fb_release = jzfb_release,
    .fb_check_var = jzfb_check_var,
    .fb_set_par = jzfb_set_par,
    .fb_blank = jzfb_blank,
    .fb_pan_display = jzfb_pan_display,
    .fb_fillrect = cfb_fillrect,
    .fb_copyarea = cfb_copyarea,
    .fb_imageblit = cfb_imageblit,
    .fb_ioctl = jzfb_ioctl,
    .fb_mmap = jzfb_mmap,
};

static ssize_t
printk_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    static int count = 0 ;
    printk("count : %d\n", count++);
    return n;
}

static ssize_t
frame_count_r(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jzfb *jzfb = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", jzfb->frame_count);
}

static ssize_t
frame_count_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    struct jzfb *jzfb = dev_get_drvdata(dev);
    int num = simple_strtoul(buf, NULL, 0);

    jzfb->frame_count = num;

    return n;
}

static ssize_t
enable_debug_r(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jzfb *jzfb = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", jzfb->enable_debug);
}

static ssize_t
enable_debug_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    struct jzfb *jzfb = dev_get_drvdata(dev);
    int num = simple_strtoul(buf, NULL, 0);

    jzfb->enable_debug = num;

    return n;
}

static ssize_t
test_suspend(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    struct jzfb *jzfb = dev_get_drvdata(dev);
    int num = simple_strtoul(buf, NULL, 0);

    printk("calling %s\n", num ? "jzfb_deinit()" : "jzfb_init()");

    if (num == 0)
        jzfb_init(jzfb);
    else
        jzfb_deinit(jzfb);

    return n;
}

static ssize_t
print_lcdc_regs_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    struct jzfb *jzfb = dev_get_drvdata(dev);

    jzfb_print_lcdc_regs(jzfb);
    jzfb_print_members(jzfb);

    return n;
}

static DEVICE_ATTR(frame_count, S_IRUGO|S_IWUSR, frame_count_r, frame_count_w);
static DEVICE_ATTR(printk, S_IRUGO|S_IWUSR, NULL, printk_w);
static DEVICE_ATTR(enable_debug, S_IRUGO|S_IWUSR, enable_debug_r, enable_debug_w);
static DEVICE_ATTR(test_suspend, S_IRUGO|S_IWUSR, NULL, test_suspend);
static DEVICE_ATTR(print_lcdc_regs, S_IRUGO|S_IWUSR, NULL, print_lcdc_regs_w);

static struct attribute *lcd_debug_attrs[] = {
    &dev_attr_printk.attr,
    &dev_attr_frame_count.attr,
    &dev_attr_enable_debug.attr,
    &dev_attr_test_suspend.attr,
    &dev_attr_print_lcdc_regs.attr,
    NULL,
};

static struct attribute_group jzfb_debug_attr_group = {
    .name	= "debug",
    .attrs	= lcd_debug_attrs,
};

static int jzfb_probe(struct platform_device *pdev)
{
    struct jzfb_lcd_pdata *pdata;
    struct fb_info *fb;
    struct jzfb *jzfb;
    int ret = 0;

    pdata = pdev->dev.platform_data;
    assert(pdata);

    assert(pdata->config.xres);
    assert(pdata->config.yres);
    assert(pdata->config.pixclock);
    assert(pdata->config.pixclock_when_init);
    assert_range(pdata->fb_copy_type, FB_COPY_TYPE_NONE, FB_COPY_TYPE_ROTATE_270);
    if (pdata->fb_copy_type > FB_COPY_TYPE_ROTATE_0)
        assert(!(pdata->config.xres % 2));
    if (pdata->config.refresh_mode == REFRESH_CONTINUOUS)
        assert(pdata->fb_copy_type == FB_COPY_TYPE_NONE);

    fb = framebuffer_alloc(sizeof(struct jzfb), &pdev->dev);
    assert(fb);

    jzfb = fb->par;
    jzfb->fb = fb;
    jzfb->dev = &pdev->dev;
    jzfb->pdata = pdata;
    platform_set_drvdata(pdev, jzfb);

    jzfb->clk_ahb1 = clk_get(NULL, "ahb1");
    jzfb->clk_lcd = clk_get(NULL, "lcd");
    jzfb->clk_lpc = clk_get(NULL, "cgu_lpc");
    assert(!IS_ERR(jzfb->clk_ahb1));
    assert(!IS_ERR(jzfb->clk_lcd));
    assert(!IS_ERR(jzfb->clk_lpc));

    spin_lock_init(&jzfb->spinlock);
    mutex_init(&jzfb->mutex);
    init_waitqueue_head(&jzfb->wait_queue);

    jzfb->jzfb_en = 0;
    jzfb->lcdc_en = 0;
    jzfb->clk_en = 0;
    jzfb->frame_sending = 0;
    jzfb->frame_count = 0;
    jzfb->enable_debug = 0;
    jzfb->is_suspend = 0;
    jzfb->te_en = 0;
    jzfb->te_frame_is_sending = 0;
    jzfb->te_irq = 0;
    jzfb->lcd_en = !pdata->init_lcd_when_start;
    jzfb->fb_copy_type = pdata->fb_copy_type;
    jzfb->pan_display_en = pdata->config.refresh_mode == REFRESH_PAN_DISPLAY;
    jzfb->wait_frame_end_when_pan_display = pdata->wait_frame_end_when_pan_display;
    jzfb->te_gpio = pdata->override_te_gpio > 0 ? pdata->override_te_gpio : GPIO_SLCD_TE;

    memset(jzfb->user_data, 0, sizeof(jzfb->user_data));

    jzfb->bytes_per_pixel = jzfb_bytes_per_pixel(&pdata->config);
    jzfb->line_stride = jzfb_line_stride_pixels(&pdata->config, pdata->config.xres);
    jzfb->frame_size = jzfb->line_stride * pdata->config.yres * jzfb->bytes_per_pixel;
    jzfb->frame_size = ALIGN(jzfb->frame_size, PAGE_SIZE);
    jzfb->frame_nums = 1;

    dma_addr_t phy_addr;

    jzfb->fb_mem = kmalloc(jzfb->frame_size * jzfb->frame_nums, GFP_KERNEL);
    // jzfb->fb_mem = dma_alloc_coherent(jzfb->dev, jzfb->frame_size * jzfb->frame_nums, &phy_addr, GFP_KERNEL);

    jzfb->frame_desc = dma_alloc_coherent(jzfb->dev, sizeof(*jzfb->frame_desc) * 2, &phy_addr, GFP_KERNEL);
    jzfb->frame_end_desc = jzfb->frame_desc + 1;
    jzfb->frame_desc_phys = phy_addr;

    assert(jzfb->fb_mem);
    assert(jzfb->frame_desc);

    init_frame_end_desc(jzfb->frame_end_desc, jzfb->fb_mem, jzfb->line_stride);
    init_frame_desc(jzfb->frame_desc, jzfb->fb_mem, jzfb->line_stride, 1, jzfb->frame_desc);

    if (jzfb->pan_display_en && pdata->config.te_pin == TE_LCDC_TRIGGER)
        init_frame_desc(jzfb->frame_desc, jzfb->fb_mem, jzfb->line_stride, 1, jzfb->frame_end_desc);

    jzfb_map_kernel_mem(jzfb);

    jzfb_init_user_info(jzfb);

    jzfb_init_video_mode(jzfb, &jzfb->videomode);
    jzfb_init_fix_info(jzfb, &fb->fix);
    jzfb_init_var_info(jzfb, &fb->var);
    fb_videomode_to_modelist(&jzfb->videomode, 1, &fb->modelist);
    fb->fbops = &jzfb_ops;
    fb->flags = FBINFO_DEFAULT;
    fb->screen_base = jzfb->user_fb_mem;
    fb->screen_size = jzfb->user_frame_size;

    ret = register_framebuffer(fb);
    assert(!ret);

    jzfb->irq = platform_get_irq(pdev, 0);
    ret = request_irq(jzfb->irq, jzfb_irq_handler, IRQF_DISABLED, "lcdc", jzfb);
    assert(!ret);

    ret = sysfs_create_group(&jzfb->dev->kobj, &jzfb_debug_attr_group);
    assert(!ret);

    if (jzfb->pan_display_en && pdata->config.te_pin == TE_GPIO_IRQ_TRIGGER) {
        ret = gpio_request(jzfb->te_gpio, "slcd_te");
        jzfb->te_irq = gpio_to_irq(jzfb->te_gpio);
        jzfb->te_level = pdata->config.te_data_transfered_level;
        assert(!ret);

        int irq_flags = IRQF_DISABLED;
        if (jzfb->te_level == AT_LOW_LEVEL)
            irq_flags |= IRQ_TYPE_LEVEL_LOW; /* IRQ_TYPE_EDGE_FALLING */
        else
            irq_flags |= IRQ_TYPE_LEVEL_HIGH; /* IRQ_TYPE_EDGE_RISING */
        gpio_direction_input(jzfb->te_gpio);

        ret = request_irq(jzfb->te_irq, jzfb_te_irq_handler, irq_flags, "slcd_te", jzfb);
        assert(!ret);
    }

    return 0;
}

static int jzfb_remove(struct platform_device *pdev)
{
   	struct jzfb *jzfb = platform_get_drvdata(pdev);

    jzfb_disable_lcd_controller(jzfb);

    disable_irq(jzfb->te_irq);
    free_irq(jzfb->te_irq, jzfb);

    disable_irq(jzfb->irq);
    free_irq(jzfb->irq, jzfb);

    kfree(jzfb->fb_mem);
    // dma_free_coherent(
    //    jzfb->dev, jzfb->frame_nums * jzfb->frame_size, jzfb->fb_mem, virt_to_phys(jzfb->fb_mem));

    dma_free_coherent(
        jzfb->dev, sizeof(*jzfb->frame_desc) * 2, jzfb->frame_desc, jzfb->frame_desc_phys);

    if (jzfb->fb_mem != jzfb->user_fb_mem)
        kfree(jzfb->user_fb_mem);

	platform_set_drvdata(pdev, NULL);

	clk_put(jzfb->clk_lpc);
	clk_put(jzfb->clk_lcd);
	clk_put(jzfb->clk_ahb1);

	sysfs_remove_group(&jzfb->dev->kobj, &jzfb_debug_attr_group);

	framebuffer_release(jzfb->fb);

    return 0;
}

static void jzfb_shutdown(struct platform_device *pdev)
{

};

#ifdef CONFIG_PM
static int jzfb_suspend(struct device *dev)
{
    struct jzfb *jzfb = dev_get_drvdata(dev);

    fb_blank(jzfb->fb, FB_BLANK_POWERDOWN);

    return 0;
}

static int jzfb_resume(struct device *dev)
{
    struct jzfb *jzfb = dev_get_drvdata(dev);

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

module_platform_driver(jzfb_driver);