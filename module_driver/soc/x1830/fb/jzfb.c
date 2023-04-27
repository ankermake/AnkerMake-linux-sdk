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
#include <linux/ioctl.h>
#include <soc/base.h>
#include <soc/irq.h>

#include "common.h"

#include "lcdc.c"

#define CMD_set_cfg       _IOWR('i', 80, struct lcdc_layer)
#define CMD_enable_cfg    _IO('i', 81)
#define CMD_disable_cfg   _IO('i', 82)

struct fb_mem_info {
    void *fb_mem;
    enum fb_fmt fb_fmt;
    unsigned int xres;
    unsigned int yres;
    unsigned int bytes_per_line;
    unsigned int bytes_per_frame;
    unsigned int frame_count;
};

struct layer_data {
    struct fb_info *fb;
    enum lcdc_layer_order layer_order;
    int is_enable;
    int is_power_on;
    int frame_count;
    struct fb_mem_info user;
    struct fb_videomode videomode;

    int is_user_setting;
    struct lcdc_layer cfg;
};

#define State_clear 0
#define State_display_start   1
#define State_display_end     2

static struct {
    struct clk *clk;
    struct clk *pclk;
    struct clk *clk_ahb1;
    int irq;
    struct mutex lock;
    spinlock_t spinlock;

    struct lcdc_frame frames[3];
    struct lcdc_data *pdata;
    void *fb_mem;
    void *fb_mem1;
    unsigned int fb_mem_size;
    unsigned int fb_mem1_size;
    int is_inited;
    int is_enabled;
    int is_wait_stop;
    int is_open;
    int display_state;
    int layer0_alpha;
    int pan_display_sync;
    int frame_index;

    wait_queue_head_t wait_queue;

    struct layer_data layer[2];
} jzfb;

module_param_named(pan_display_sync, jzfb.pan_display_sync, int, 0644);
module_param_named(layer0_alpha, jzfb.layer0_alpha, int, 0644);
module_param_named(layer0_frames, jzfb.layer[0].frame_count, int, 0644);
module_param_named(layer1_enable, jzfb.layer[1].is_enable, int, 0644);
module_param_named(layer1_frames, jzfb.layer[1].frame_count, int, 0644);

static inline void dump_frame(void)
{

    int i;
    for (i = 0; i < 3; i++) {
        struct lcdc_frame *frame = &jzfb.frames[i];
        printk("frame%d\n", i);
        printk("FrameCfgAddr %08lx\n", frame->framedesc->FrameCfgAddr);
        printk("FrameSize %08lx\n", frame->framedesc->FrameSize);
        printk("FrameCtrl %08lx\n", frame->framedesc->FrameCtrl);
        printk("Reserved0 %08lx\n", frame->framedesc->Reserved0);
        printk("Reserved1 %08lx\n", frame->framedesc->Reserved1);
        printk("Layer0CfgAddr %08lx\n", frame->framedesc->Layer0CfgAddr);
        printk("Layer1CfgAddr %08lx\n", frame->framedesc->Layer1CfgAddr);
        printk("Reserved2 %08lx\n", frame->framedesc->Reserved2);
        printk("Reserved3 %08lx\n", frame->framedesc->Reserved3);
        printk("LayerCfgScaleEn %08lx\n", frame->framedesc->LayerCfgScaleEn);
        printk("InterruptControl %08lx\n", frame->framedesc->InterruptControl);

        printk("LayerSize %08lx\n", frame->layer0->LayerSize);
        printk("LayerCfg %08lx\n", frame->layer0->LayerCfg);
        printk("LayerBufferAddr %08lx\n", frame->layer0->LayerBufferAddr);
        printk("Reserved0 %08lx\n", frame->layer0->Reserved0);
        printk("Reserved1 %08lx\n", frame->layer0->Reserved1);
        printk("Reserved2 %08lx\n", frame->layer0->Reserved2);
        printk("LayerPos %08lx\n", frame->layer0->LayerPos);
        printk("Reserved3 %08lx\n", frame->layer0->Reserved3);
        printk("Reserved4 %08lx\n", frame->layer0->Reserved4);
        printk("LayerStride %08lx\n", frame->layer0->LayerStride);
        printk("BufferAddr_UV %08lx\n", frame->layer0->BufferAddr_UV);
        printk("stride_UV %08lx\n", frame->layer0->stride_UV);

        printk("LayerSize %08lx\n", frame->layer1->LayerSize);
        printk("LayerCfg %08lx\n", frame->layer1->LayerCfg);
        printk("LayerBufferAddr %08lx\n", frame->layer1->LayerBufferAddr);
        printk("Reserved0 %08lx\n", frame->layer1->Reserved0);
        printk("Reserved1 %08lx\n", frame->layer1->Reserved1);
        printk("Reserved2 %08lx\n", frame->layer1->Reserved2);
        printk("LayerPos %08lx\n", frame->layer1->LayerPos);
        printk("Reserved3 %08lx\n", frame->layer1->Reserved3);
        printk("Reserved4 %08lx\n", frame->layer1->Reserved4);
        printk("LayerStride %08lx\n", frame->layer1->LayerStride);
        printk("BufferAddr_UV %08lx\n", frame->layer1->BufferAddr_UV);
        printk("stride_UV %08lx\n", frame->layer1->stride_UV);
        printk("\n");
    }
}

static void init_video_mode(struct fb_mem_info *info, struct fb_videomode *mode)
{
    struct lcdc_data *pdata = jzfb.pdata;

    mode->name = pdata->name;
    mode->refresh = pdata->refresh;
    mode->xres = info->xres;
    mode->yres = info->yres;
    mode->pixclock = pdata->pixclock;
    mode->left_margin = pdata->left_margin;
    mode->right_margin = pdata->right_margin;
    mode->upper_margin = pdata->upper_margin;
    mode->lower_margin = pdata->lower_margin;
    mode->hsync_len = pdata->hsync_len;
    mode->vsync_len = pdata->vsync_len;
    mode->sync = 0;
    mode->vmode = 0;
    mode->flag = 0;
}

static void init_fix_info(struct fb_mem_info *info, struct fb_fix_screeninfo *fix)
{
    strcpy(fix->id, "jzfb");
    fix->type = FB_TYPE_PACKED_PIXELS;
    fix->visual = FB_VISUAL_TRUECOLOR;
    fix->xpanstep = 0;
    fix->ypanstep = 1;
    fix->ywrapstep = 0;
    fix->accel = FB_ACCEL_NONE;
    fix->line_length = info->bytes_per_line;
    fix->smem_start = virt_to_phys(info->fb_mem);
    fix->smem_len = info->frame_count * info->bytes_per_frame;
    fix->mmio_start = 0;
    fix->mmio_len = 0;
}

static inline void set_fb_bitfield(struct fb_bitfield *field, int length, int offset, int msb_right)
{
    field->length = length;
    field->offset = offset;
    field->msb_right = msb_right;
}

static void init_var_info(struct fb_mem_info *info, struct fb_var_screeninfo *var)
{
    struct lcdc_data *pdata = jzfb.pdata;

    var->xres = info->xres;
    var->yres = info->yres;
    var->xres_virtual = info->xres;
    var->yres_virtual = info->yres * info->frame_count;
    var->xoffset = 0;
    var->yoffset = 0;
    var->height = pdata->height;
    var->width = pdata->width;
    var->pixclock = pdata->pixclock;
    var->left_margin = pdata->left_margin;
    var->right_margin = pdata->right_margin;
    var->upper_margin = pdata->upper_margin;
    var->lower_margin = pdata->lower_margin;
    var->hsync_len = pdata->hsync_len;
    var->vsync_len = pdata->vsync_len;
    var->sync = 0;
    var->vmode = 0;

    var->bits_per_pixel = bits_per_pixel(pdata->fb_fmt);
    switch (pdata->fb_fmt) {
    case fb_fmt_RGB555:
        set_fb_bitfield(&var->red, 5, 10, 0);
        set_fb_bitfield(&var->green, 5, 5, 0);
        set_fb_bitfield(&var->blue, 5, 0, 0);
        set_fb_bitfield(&var->transp, 0, 0, 0);
        break;
    case fb_fmt_RGB565:
        set_fb_bitfield(&var->red, 5, 11, 0);
        set_fb_bitfield(&var->green, 6, 5, 0);
        set_fb_bitfield(&var->blue, 5, 0, 0);
        set_fb_bitfield(&var->transp, 0, 0, 0);
        break;
    case fb_fmt_RGB888:
        set_fb_bitfield(&var->red, 8, 16, 0);
        set_fb_bitfield(&var->green, 8, 8, 0);
        set_fb_bitfield(&var->blue, 8, 0, 0);
        set_fb_bitfield(&var->transp, 0, 0, 0);
        break;
    case fb_fmt_ARGB8888:
        set_fb_bitfield(&var->red, 8, 16, 0);
        set_fb_bitfield(&var->green, 8, 8, 0);
        set_fb_bitfield(&var->blue, 8, 0, 0);
        set_fb_bitfield(&var->transp, 8, 24, 0);
        break;
    default:
        assert(0);
    }
}

static inline void *m_dma_alloc_coherent(int size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(NULL, size, &dma_handle, GFP_KERNEL);
    assert(mem);

    return (void *)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(void *mem, int size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    dma_free_coherent(NULL, size, (void *)CKSEG1ADDR(mem), dma_handle);
}

static inline void m_cache_sync(void *mem, int size)
{
    dma_cache_sync(NULL, mem, size, DMA_TO_DEVICE);
}

static void init_frame_layer(void)
{
    struct lcdc_layer cfg;
    struct fb_mem_info *info = &jzfb.layer[0].user;

    memset(&cfg, 0, sizeof(cfg));
    cfg.xpos = 0;
    cfg.ypos = 0;
    cfg.fb_fmt = info->fb_fmt;
    cfg.xres = info->xres;
    cfg.yres= info->yres;
    cfg.alpha.value = 0xff;
    cfg.alpha.enable = 1;
    cfg.rgb.stride = info->bytes_per_line;

    int i;
    for (i = 0; i < 3; i++) {
        cfg.rgb.mem = info->fb_mem + info->bytes_per_frame * i;
        init_layer_desc(jzfb.frames[i].layer0, &cfg);
        init_layer_desc(jzfb.frames[i].layer1, &cfg);
        init_frame_desc(jzfb.pdata, jzfb.frames[i].framedesc, jzfb.frames[i].layer0, jzfb.frames[i].layer1);
        enable_layer(jzfb.frames[i].framedesc, 0, 1);
        m_cache_sync(jzfb.frames[i].layer0, sizeof(*jzfb.frames[i].layer0));
        m_cache_sync(jzfb.frames[i].layer1, sizeof(*jzfb.frames[i].layer1));
        m_cache_sync(jzfb.frames[i].framedesc, sizeof(*jzfb.frames[i].framedesc));
    }
}

static void init_layer_data(void)
{
    struct lcdc_data *pdata = jzfb.pdata;
    struct fb_mem_info *info;
    int line_len = bytes_per_line(pdata->xres, pdata->fb_fmt);
    unsigned int frame_size = bytes_per_frame(line_len, pdata->yres);

    info = &jzfb.layer[0].user;
    jzfb.layer[0].layer_order = lcdc_layer_top;
    info->fb_fmt = pdata->fb_fmt;
    info->fb_mem = jzfb.fb_mem;
    info->xres = pdata->xres;
    info->yres = pdata->yres;
    info->bytes_per_line = line_len;
    info->bytes_per_frame = frame_size;
    info->frame_count = jzfb.layer[0].frame_count;

    info = &jzfb.layer[1].user;
    jzfb.layer[1].layer_order = lcdc_layer_bottom;
    info->fb_fmt = pdata->fb_fmt;
    info->fb_mem = jzfb.fb_mem1;
    info->xres = pdata->xres;
    info->yres = pdata->yres;
    info->bytes_per_line = line_len;
    info->bytes_per_frame = frame_size;
    info->frame_count = jzfb.layer[1].frame_count;
}

static int alloc_mem(void)
{
    struct lcdc_data *pdata = jzfb.pdata;
    unsigned int frame_size = 0;
    unsigned int size;
    void *mem = NULL;
    int line_len = bytes_per_line(pdata->xres, pdata->fb_fmt);
    frame_size = bytes_per_frame(line_len, pdata->yres);

    if (jzfb.layer[0].frame_count) {
        size = frame_size * jzfb.layer[0].frame_count;
        size = ALIGN(size, PAGE_SIZE);
        mem = m_dma_alloc_coherent(size);
        if (mem == NULL) {
            printk(KERN_ERR "jzfb: failed to alloc fb mem\n");
            return -ENOMEM;
        }

        jzfb.fb_mem = mem;
        jzfb.fb_mem_size = size;
    }

    if (jzfb.layer[1].frame_count) {

        size = frame_size * jzfb.layer[1].frame_count;
        size = ALIGN(size, PAGE_SIZE);
        mem = m_dma_alloc_coherent(size);
        if (mem == NULL) {
            m_dma_free_coherent(jzfb.fb_mem, jzfb.fb_mem_size);
            printk(KERN_ERR "jzfb: failed to alloc fb mem1\n");
            return -ENOMEM;
        }

        jzfb.fb_mem1 = mem;
        jzfb.fb_mem1_size = size;
    }

    int i;
    for (i = 0; i < 3; i++) {
        jzfb.frames[i].framedesc = m_dma_alloc_coherent(sizeof(*jzfb.frames[i].framedesc));
        jzfb.frames[i].layer0 = m_dma_alloc_coherent(sizeof(*jzfb.frames[i].layer0));
        jzfb.frames[i].layer1 = m_dma_alloc_coherent(sizeof(*jzfb.frames[i].layer0));
    }

    return 0;
}

static int free_mem(void)
{
    m_dma_free_coherent(jzfb.fb_mem, jzfb.fb_mem_size);
    m_dma_free_coherent(jzfb.fb_mem1, jzfb.fb_mem1_size);

    int i;
    for (i = 0; i < 3; i++) {
        m_dma_free_coherent(jzfb.frames[i].framedesc, sizeof(*jzfb.frames[i].framedesc));
        m_dma_free_coherent(jzfb.frames[i].layer0, sizeof(*jzfb.frames[i].layer0));
        m_dma_free_coherent(jzfb.frames[i].layer1, sizeof(*jzfb.frames[i].layer0));
    }

    return 0;
}

static void process_slcd_data_table(struct smart_lcd_data_table *table, unsigned int length)
{
    int i = 0;

    for (; i < length; i++) {
        switch (table[i].type) {
        case SMART_CONFIG_CMD:
            slcd_send_cmd(table[i].value);
            break;
        case SMART_CONFIG_DATA:
            slcd_send_data(table[i].value);
            break;
        case SMART_CONFIG_UDELAY:
            if (table[i].value >= 500)
                usleep_range(table[i].value, table[i].value);
            else
                udelay(table[i].value);
            break;
        default:
            panic("why this type: %d\n", table[i].type);
            break;
        }
    }

    if (slcd_wait_busy(10 * 1000))
        panic("lcdc busy\n");
}

static void enable_fb(void)
{
    if (jzfb.is_enabled++ != 0)
        return;

    unsigned int rate = jzfb.pdata->pixclock;
    if (!is_tft(jzfb.pdata)) {
        if (jzfb.pdata->slcd.pixclock_when_init)
            rate = jzfb.pdata->slcd.pixclock_when_init;
    }

    clk_enable(jzfb.clk);
    clk_set_rate(jzfb.pclk, rate);
    clk_enable(jzfb.pclk);

    init_lcdc(jzfb.pdata, jzfb.frames[0].framedesc);

    jzfb.pdata->power_on(NULL);

    process_slcd_data_table(
        jzfb.pdata->slcd_data_table, jzfb.pdata->slcd_data_table_length);

    if (rate != jzfb.pdata->pixclock)
        clk_set_rate(jzfb.pclk, jzfb.pdata->pixclock);

    if (!is_tft(jzfb.pdata)) {
        slcd_send_cmd(jzfb.pdata->slcd.cmd_of_start_frame);
        lcd_set_bit(SLCD_CFG, FMT_EN, 1);
        jzfb.display_state = State_clear;
    } else {
        jzfb.display_state = State_clear;
    }

}

static void disable_fb(void)
{
    if (--jzfb.is_enabled != 0)
        return;

    jzfb.is_wait_stop = 1;
    genernal_stop_display();

    wait_event(jzfb.wait_queue, jzfb.is_wait_stop);

    jzfb.pdata->power_off(NULL);

    clk_disable(jzfb.clk);
    clk_disable(jzfb.pclk);
}

void lcdc_pan_display(unsigned int frame_index)
{
    assert(frame_index < 3);
    unsigned long flags;

    mutex_lock(&jzfb.lock);

    if (!jzfb.is_enabled)
        goto unlock;

    struct lcdc_frame *frame = &jzfb.frames[frame_index];

    /* 刷新非cacheline对齐的写穿透cache/内存 */
    fast_iob();

    if (jzfb.display_state != State_clear) {
        /* 旧的一帧已经刷新完成,则不需要等待 */
        if (is_tft(jzfb.pdata) && jzfb.frame_index == -1)
            goto start_display;

        wait_event_timeout(jzfb.wait_queue,
            jzfb.display_state == State_display_end, msecs_to_jiffies(300));
        if (jzfb.display_state != State_display_end) {
            printk(KERN_EMERG "jzfb: lcd pan display wait timeout\n");
            goto unlock;
        }
    }

start_display:
    if (!is_tft(jzfb.pdata)) {
        jzfb.frame_index = frame_index;
        jzfb.display_state = State_display_start;
        lcd_write(FRM_CFG_ADDR, virt_to_phys(frame->framedesc));
        start_composer();
        start_slcd();
    } else {
        int need_start = jzfb.display_state == State_clear;

        spin_lock_irqsave(&jzfb.spinlock, flags);
        jzfb.frame_index = frame_index;
        jzfb.display_state = State_display_start;
        lcd_write(FRM_CFG_ADDR, virt_to_phys(frame->framedesc));
        lcd_write(FRM_CFG_ADDR, virt_to_phys(frame->framedesc));
        spin_unlock_irqrestore(&jzfb.spinlock, flags);

        if (need_start) {
            start_composer();
            start_tft();
        }

        wait_event_timeout(jzfb.wait_queue,
        jzfb.display_state == State_display_end, msecs_to_jiffies(300));
        if (jzfb.display_state != State_display_end)
            printk(KERN_EMERG "jzfb: lcd pan display wait timeout\n");
    }

    if (jzfb.pan_display_sync) {
        wait_event_timeout(jzfb.wait_queue,
            jzfb.display_state == State_display_end, msecs_to_jiffies(300));
        if (jzfb.display_state != State_display_end)
            printk(KERN_EMERG "jzfb: lcd pan display wait timeout\n");
    }

unlock:
    mutex_unlock(&jzfb.lock);
}

void lcdc_config_layer(unsigned int frame_index,
     unsigned int layer_id, struct lcdc_layer *cfg)
{
    assert(layer_id < 2);
    assert(frame_index < 3);
    struct lcdc_frame *frame = &jzfb.frames[frame_index];
    struct layerdesc *layer = layer_id ? frame->layer1 : frame->layer0;

    init_layer_desc(layer, cfg);
    enable_layer(frame->framedesc, layer_id, !!cfg->layer_enable);
    set_layer_order(frame->framedesc, layer_id, cfg->layer_order);

    m_cache_sync(layer, sizeof(*layer));
    m_cache_sync(frame->framedesc, sizeof(*frame->framedesc));
}

void lcdc_set_layer_order(unsigned int frame_index,
     unsigned int layer_id, enum lcdc_layer_order layer_order)
{
    assert(layer_id < 2);
    assert(frame_index < 3);
    struct lcdc_frame *frame = &jzfb.frames[frame_index];

    set_layer_order(frame->framedesc, layer_id, layer_order);

    m_cache_sync(frame->framedesc, sizeof(*frame->framedesc));
}

void lcdc_enable_layer(unsigned int frame_index,
     unsigned int layer_id, int enable)
{
    assert(layer_id < 2);
    assert(frame_index < 3);
    struct lcdc_frame *frame = &jzfb.frames[frame_index];

    enable_layer(frame->framedesc, layer_id, !!enable);

    m_cache_sync(frame->framedesc, sizeof(*frame->framedesc));
}

static irqreturn_t jzfb_irq_handler(int irq, void *data)
{
    unsigned long flags = lcd_read(INT_FLAG);

    if (get_bit_field(&flags, DISP_END)) {
        lcd_write(CLR_ST, bit_field_val(CLR_DISP_END, 1));
        jzfb.display_state = State_display_end;
        jzfb.frame_index = -1;
        wake_up_all(&jzfb.wait_queue);
        return IRQ_HANDLED;
    }

    if (get_bit_field(&flags, STOP_DISP_ACK)) {
        lcd_write(CLR_ST, bit_field_val(CLR_STP_DISP_ACK, 1));
        jzfb.is_wait_stop = 0;
        wake_up_all(&jzfb.wait_queue);
        return IRQ_HANDLED;
    }

    if (get_bit_field(&flags, TFT_UNDR)) {
        printk(KERN_ERR "err: lcd underrun\n");
        lcd_write(CLR_ST, bit_field_val(CLR_TFT_UNDR, 1));
        return IRQ_HANDLED;
    }

    printk(KERN_ERR "lcd: why this irq: %lx\n", flags);

    lcd_write(CLR_ST, flags);

    return IRQ_HANDLED;
}


static int jzfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
    struct layer_data *layer = info->par;
    unsigned long start;
    unsigned long off;
    u32 len;

    if (layer->fb->fix.smem_len == 0)
        return -ENOMEM;

    off = vma->vm_pgoff << PAGE_SHIFT;

    len = layer->fb->fix.smem_len;
    len = ALIGN(len, PAGE_SIZE);
    if ((vma->vm_end - vma->vm_start + off) > len)
        return -EINVAL;

    start = layer->fb->fix.smem_start;
    start &= PAGE_MASK;
    off += start;

    vma->vm_pgoff = off >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;

    /* 0: cachable,write through (cache + cacheline对齐写穿)
     * 1: uncachable,write Acceleration (uncache + 硬件写加速)
     * 2: uncachable
     * 3: cachable
     * pan_display 模式需要调用pan_display 接口,在那里去fast_iob()
     * 把非cacheline 对齐的内存同步一下
     */
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= 0;

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start, vma->vm_page_prot))
    {
        return -EAGAIN;
    }

    return 0;
}

static int jzfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    struct layer_data *layer = info->par;

    if (var->xres != layer->user.xres ||
        var->yres != layer->user.yres)
        return -EINVAL;

    init_var_info(&layer->user, var);

    return 0;
}

static int jzfb_set_par(struct fb_info *info)
{
    return jzfb_check_var(&info->var, info);
}

static int jzfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    struct layer_data *layer = info->par;

    switch (cmd) {
    case CMD_set_cfg: {
        struct lcdc_layer *cfg = (void *)arg;
        if (!cfg)
            return -EINVAL;
        if (cfg->layer_order > lcdc_layer_bottom) {
            printk(KERN_ERR "jzfb: invalid order: %d\n", cfg->layer_order);
            return -EINVAL;
        }
        if (cfg->alpha.value >= 256) {
            printk(KERN_ERR "jzfb: invalid alpha: %x\n", cfg->alpha.value);
            return -EINVAL;
        }
        if (cfg->fb_fmt > fb_fmt_NV21) {
            printk(KERN_ERR "jzfb: invalid fmt: %d\n", cfg->fb_fmt);
            return -EINVAL;
        }
        if (cfg->fb_fmt < fb_fmt_NV12) {
            if (cfg->rgb.mem >= (void *)(512*1024*1024)) {
                printk(KERN_ERR "jzfb: must be phys address\n");
                return -EINVAL;
            }
        } else {
            if (cfg->y.mem >= (void *)(512*1024*1024)) {
                printk(KERN_ERR "jzfb: must be phys address\n");
                return -EINVAL;
            }
            if (cfg->uv.mem >= (void *)(512*1024*1024)) {
                printk(KERN_ERR "jzfb: must be phys address\n");
                return -EINVAL;
            }
        }
        if (cfg->xres + cfg->xpos > jzfb.pdata->xres) {
            printk(KERN_ERR "jzfb: invalid xres: %d %d\n", cfg->xres, cfg->xpos);
            return -EINVAL;
        }
        if (cfg->yres + cfg->ypos > jzfb.pdata->yres) {
            printk(KERN_ERR "jzfb: invalid yres: %d %d\n", cfg->yres, cfg->ypos);
            return -EINVAL;
        }

        mutex_lock(&jzfb.lock);
        layer->cfg = *cfg;
        if (cfg->fb_fmt < fb_fmt_NV12) {
            layer->cfg.rgb.mem = (void *)CKSEG0ADDR(cfg->rgb.mem);
        } else {
            layer->cfg.y.mem = (void *)CKSEG0ADDR(cfg->y.mem);
            layer->cfg.uv.mem = (void *)CKSEG0ADDR(cfg->uv.mem);
        }
        mutex_unlock(&jzfb.lock);

        return 0;
    }

    case CMD_enable_cfg:
        mutex_lock(&jzfb.lock);
        layer->is_user_setting = 1;
        mutex_unlock(&jzfb.lock);
        return 0;

    case CMD_disable_cfg:
        mutex_lock(&jzfb.lock);
        layer->is_user_setting = 0;
        mutex_unlock(&jzfb.lock);
        return 0;

    default:
        printk(KERN_ERR "jzfb: not support this cmd: %x\n", cmd);
        return -EINVAL;
    }

    return 0;
}

static int jzfb_open(struct fb_info *info, int user)
{
    mutex_lock(&jzfb.lock);
    jzfb.is_open++;
    mutex_unlock(&jzfb.lock);
    return 0;
}

static int jzfb_release(struct fb_info *info, int user)
{
    struct layer_data *layer = info->par;

    mutex_lock(&jzfb.lock);
    jzfb.is_open--;
    layer->is_user_setting = 0;
    mutex_unlock(&jzfb.lock);
    return 0;
}

static int jzfb_blank(int blank_mode, struct fb_info *info)
{
    struct layer_data *layer = info->par;

    mutex_lock(&jzfb.lock);

    if (blank_mode == FB_BLANK_UNBLANK) {
        if (!layer->is_power_on) {
            enable_fb();
            layer->is_power_on = 1;
        }
    } else {
        if (layer->is_power_on) {
            disable_fb();
            layer->is_power_on = 0;
        }
    }

    mutex_unlock(&jzfb.lock);

    if (blank_mode != FB_BLANK_UNBLANK)
        lcdc_layer_server_disable(layer - jzfb.layer, 1);

    return 0;
}

static int jzfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    struct layer_data *layer = info->par;
    struct fb_mem_info *user = &layer->user;
    int next_frm;
    int ret = -EINVAL;

    mutex_lock(&jzfb.lock);

    if (!layer->user.frame_count && !layer->is_user_setting)
        goto err;

    if (var->xoffset - info->var.xoffset) {
        printk(KERN_ERR "jzfb: No support for X panning for now\n");
        goto err;
    }

    next_frm = var->yoffset / var->yres;
    if (next_frm >= user->frame_count) {
        printk(KERN_ERR "jzfb: yoffset is out of framebuffer: %d\n", var->yoffset);
        goto err;
    }

    if (!layer->is_power_on) {
        printk(KERN_ERR "jzfb: layer%d is not enabled\n", layer - jzfb.layer);
        ret = -EBUSY;
        goto err;
    }

    if (layer->is_user_setting && next_frm) {
        printk(KERN_ERR "jzfb: can't use yoffset, if use cfg\n");
        goto err;
    }

    struct lcdc_layer layer_cfg;
    if (!layer->is_user_setting) {
        layer_cfg.fb_fmt = user->fb_fmt;
        layer_cfg.xres = user->xres;
        layer_cfg.yres = user->yres;
        layer_cfg.xpos = 0;
        layer_cfg.ypos = 0;

        layer_cfg.layer_order = layer->layer_order;
        layer_cfg.layer_enable = 1;

        layer_cfg.rgb.mem =
            user->fb_mem + next_frm * user->bytes_per_frame;
        layer_cfg.rgb.stride = user->bytes_per_line;
        layer_cfg.alpha.enable = 1;
        layer_cfg.alpha.value = layer->layer_order == lcdc_layer_bottom ? 0xff : jzfb.layer0_alpha;
    } else {
        layer_cfg = layer->cfg;
        if (layer_cfg.layer_order == lcdc_layer_bottom)
            layer_cfg.alpha.value = 0xff;
    }

    mutex_unlock(&jzfb.lock);

    lcdc_layer_server_update(layer - jzfb.layer, &layer_cfg, jzfb.pan_display_sync);

    return 0;
err:
    mutex_unlock(&jzfb.lock);
    return ret;
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

static void init_one_fb(struct layer_data *layer)
{
    struct fb_info *fb;
    struct fb_mem_info *info;
    int ret;

    if (!layer->is_enable)
        return;

    fb = framebuffer_alloc(0, NULL);
    assert(fb);

    fb->par = layer;
    layer->fb = fb;
    info = &layer->user;
    init_video_mode(info, &layer->videomode);
    fb_videomode_to_modelist(&layer->videomode, 1, &fb->modelist);
    init_fix_info(info, &fb->fix);
    init_var_info(info, &fb->var);
    fb->fbops = &jzfb_ops;
    fb->flags = FBINFO_DEFAULT;
    fb->screen_base = info->fb_mem;
    fb->screen_size = info->bytes_per_frame;
    ret = register_framebuffer(fb);
    assert(!ret);
}

static void release_one_fb(struct layer_data *layer)
{
    if (!layer->is_enable)
        return;

    int ret = unregister_framebuffer(layer->fb);
    assert(!ret);

    framebuffer_release(layer->fb);
}

static void init_fb(void)
{
    init_one_fb(&jzfb.layer[0]);
    init_one_fb(&jzfb.layer[1]);
}

static void release_fb(void)
{
    release_one_fb(&jzfb.layer[0]);
    release_one_fb(&jzfb.layer[1]);
}

static int check_scld_fmt(struct lcdc_data *pdata)
{
    int pix_fmt = pdata->out_format;

    if (pdata->lcd_mode >= SLCD_SPI_3LINE)
        return -1;

    int width = pdata->slcd.mcu_data_width;
    if (width == MCU_WIDTH_8BITS) {
        if (pix_fmt != OUT_FORMAT_RGB565 && pix_fmt != OUT_FORMAT_RGB888)
            return -1;
    }
    if (width == MCU_WIDTH_9BITS) {
        if (pix_fmt != OUT_FORMAT_RGB666)
            return -1;
    }
    if (width == MCU_WIDTH_16BITS) {
        if (pix_fmt != OUT_FORMAT_RGB666)
            return -1;
    }

    return 0;
}

static int check_tft_fmt(struct lcdc_data *pdata)
{
    if (pdata->out_format == OUT_FORMAT_RGB888)
        return -1;
    return 0;
}

#define error_if(_cond) \
    do { \
        if (_cond) { \
            printk(KERN_ERR "jzfb: failed to check: %s\n", #_cond); \
            ret = -1; \
            goto unlock; \
        } \
    } while (0)

int jzfb_register_lcd(struct lcdc_data *pdata)
{
    int ret;

    mutex_lock(&jzfb.lock);

    error_if(jzfb.pdata != NULL);
    error_if(pdata == NULL);
    error_if(pdata->name == NULL);
    error_if(pdata->power_on == NULL);
    error_if(pdata->power_off == NULL);
    error_if(pdata->xres < 32 || pdata->xres >= 2048);
    error_if(pdata->yres < 32 || pdata->yres >= 2048);
    error_if(pdata->fb_fmt >= fb_fmt_NV12);

    if (!is_tft(pdata))
        error_if(check_scld_fmt(pdata));
    else
        error_if(check_tft_fmt(pdata));

    ret = init_gpio(pdata);
    if (ret)
        goto unlock;

    jzfb.pdata = pdata;
    ret = alloc_mem();
    if (ret) {
        jzfb.pdata = NULL;
        jzfb_release_pins();
        goto unlock;
    }

    auto_calculate_pixel_clock(pdata);

    init_layer_data();

    init_frame_layer();

    init_fb();

    lcdc_layer_server_init();

unlock:
    mutex_unlock(&jzfb.lock);

    return ret;
}

void jzfb_unregister_lcd(struct lcdc_data *pdata)
{
    mutex_lock(&jzfb.lock);

    assert(pdata == jzfb.pdata);

    assert(!jzfb.is_open);

    lcdc_layer_server_exit();

    if (jzfb.is_enabled) {
        jzfb.is_enabled = 1;
        disable_fb();
    }

    free_mem();

    release_fb();

    jzfb_release_pins();

    jzfb.pdata = NULL;

    mutex_unlock(&jzfb.lock);
}

static int jzfb_module_init(void)
{
    int ret;

    /* layer0 也就是fb0 一定会有
     */
    jzfb.layer[0].is_enable = 1;

    if (jzfb.layer[0].frame_count < 0 || jzfb.layer[0].frame_count > 3) {
        printk(KERN_ERR "jzfb: frame_count invalid: layer%d\n", 0);
        return -EINVAL;
    }

    if (jzfb.layer[1].frame_count < 0 || jzfb.layer[1].frame_count > 3) {
        printk(KERN_ERR "jzfb: frame_count invalid: layer%d\n", 1);
        return -EINVAL;
    }

    if (!jzfb.layer[1].is_enable)
        jzfb.layer[1].frame_count = 0;

    if ((unsigned int)jzfb.layer0_alpha >= 256)
        jzfb.layer0_alpha = 0xff;

    mutex_init(&jzfb.lock);
    spin_lock_init(&jzfb.spinlock);
    init_waitqueue_head(&jzfb.wait_queue);
    jzfb.irq = IRQ_LCDC;

    jzfb.clk = clk_get(NULL, "lcd");
    assert(!IS_ERR(jzfb.clk));

    jzfb.pclk = clk_get(NULL, "cgu_lpc");
    assert(!IS_ERR(jzfb.pclk));

    jzfb.clk_ahb1 = clk_get(NULL, "ahb1");
    assert(!IS_ERR(jzfb.clk_ahb1));

    clk_enable(jzfb.clk_ahb1);

    ret = request_irq(jzfb.irq, jzfb_irq_handler, 0, "jzfb", NULL);
    assert(!ret);

    return 0;
}

static void jzfb_module_exit(void)
{
    assert(!jzfb.pdata);

    disable_irq(jzfb.irq);

    clk_put(jzfb.clk);
    clk_put(jzfb.pclk);
    clk_put(jzfb.clk_ahb1);

    free_irq(jzfb.irq, NULL);
}

module_init(jzfb_module_init);

module_exit(jzfb_module_exit);

EXPORT_SYMBOL(jzfb_register_lcd);
EXPORT_SYMBOL(jzfb_unregister_lcd);

MODULE_DESCRIPTION("Ingenic Soc FB driver");
MODULE_LICENSE("GPL");
