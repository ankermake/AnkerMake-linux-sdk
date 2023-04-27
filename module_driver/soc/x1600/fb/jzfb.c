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
#include <soc/base.h>

#include "drivers/rmem_manager/rmem_manager.h"

#include "common.h"

#include "lcdc_data.h"
#include "lcdc.c"


// void lcdc_dump_regs(void);

enum display_state {
    state_clear,
    state_display_start,
    state_display_end,
    state_stop,
};

struct fb_mem_info {
    void *fb_mem;
    enum fb_fmt fb_fmt;
    unsigned int xres;
    unsigned int yres;
    unsigned int bytes_per_line;
    unsigned int bytes_per_frame;
    unsigned int frame_count;
};

struct fbdev_data {
    struct fb_info *fb;
    int is_power_on;

    int frame_count;
    void *fb_mem;
    unsigned int fb_mem_size;
    struct fb_mem_info user;
    struct fb_videomode videomode;

    struct srdma_cfg srdma_cfg;

    int is_display;
};

struct {
    struct srdmadesc *frames[2];

    struct clk *clk;
    struct clk *pclk;
    int irq;
    struct mutex lock;
    struct mutex frames_lock;
    spinlock_t spinlock;

    wait_queue_head_t wait_queue;

    struct lcdc_data *pdata;

    int is_open;
    int is_inited;
    int is_enabled;
    int is_wait_stop;
    int frame_index;
    enum display_state display_state;

    int pan_display_sync;



    struct fbdev_data fbdev;
} jzfb;

static int lcd_is_inited = 0;
module_param(lcd_is_inited, int, 0644);

module_param_named(pan_display_sync, jzfb.pan_display_sync, int, 0644);
module_param_named(frame_num, jzfb.fbdev.frame_count, int, 0644);


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


static inline void *m_dma_alloc_coherent(int size, int align)
{
    void *mem = rmem_alloc_aligned(ALIGN(size, 64), align);
    assert(mem);

    return mem;
}

static inline void m_dma_free_coherent(void *mem, int size)
{
    rmem_free(mem, ALIGN(size, 64));
}

static inline void m_cache_sync(void *mem, int size)
{
    dma_cache_sync(NULL, mem, size, DMA_TO_DEVICE);
}

static irqreturn_t jzfb_irq_handler(int irq, void *data)
{
    unsigned long flags = lcd_read(INT_FLAG);

    if (get_bit_field(&flags, DISP_END)) {
        lcd_write(CLR_ST, bit_field_val(CLR_DISP_END, 1));
        jzfb.display_state = state_display_end;
        wake_up_all(&jzfb.wait_queue);
        return IRQ_HANDLED;
    }

    if (get_bit_field(&flags, STOP_SRD_ACK)) {
        lcd_write(CLR_ST, bit_field_val(CLR_STOP_SRD_ACK, 1));
        jzfb.display_state = state_stop;
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
            usleep_range(table[i].value, table[i].value);
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
    if (is_slcd(jzfb.pdata)) {
        if (jzfb.pdata->slcd.pixclock_when_init)
            rate = jzfb.pdata->slcd.pixclock_when_init;
    }

    clk_prepare_enable(jzfb.clk);
    clk_prepare_enable(jzfb.pclk);

    clk_set_rate(jzfb.pclk, rate);

    init_lcdc(jzfb.pdata);

    jzfb.display_state = state_clear;

    if (lcd_is_inited)
        return;

    jzfb.pdata->power_on(NULL);

    process_slcd_data_table(
        jzfb.pdata->slcd_data_table, jzfb.pdata->slcd_data_table_length);

    if (rate != jzfb.pdata->pixclock) {
        clk_set_rate(jzfb.pclk, jzfb.pdata->pixclock);
    }

    if (is_slcd(jzfb.pdata)) {
        slcd_send_cmd(jzfb.pdata->slcd.cmd_of_start_frame);
        lcd_set_bit(SLCD_CFG, FMT_EN, 1);
    }
}

static void disable_fb(void)
{
    if (--jzfb.is_enabled != 0)
        return;

    jzfb.is_wait_stop = 1;
    lcd_genernal_stop_display();

    while(jzfb.is_wait_stop)
        msleep(1);


    jzfb.pdata->power_off(NULL);

    clk_disable_unprepare(jzfb.clk);
    clk_disable_unprepare(jzfb.pclk);

}

#define error_if(_cond) \
    do { \
        if (_cond) { \
            panic("fb: failed to check: %s\n", #_cond); \
        } \
    } while (0)


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
        if (pix_fmt != OUT_FORMAT_RGB565)
            return -1;
    }

    return 0;
}

static void init_fbdev_data(struct fbdev_data *fbdev, int frame_count)
{
    int fb_mem_size;
    struct fb_mem_info *info = &fbdev->user;
    struct lcdc_data *pdata = jzfb.pdata;

    info->fb_fmt = pdata->fb_fmt;
    info->xres = pdata->xres;
    info->yres = pdata->yres;

    info->bytes_per_line = bytes_per_line(info->xres, info->fb_fmt);

    info->bytes_per_frame = bytes_per_frame(info->bytes_per_line, info->yres);
    fb_mem_size = info->bytes_per_frame * frame_count;
    fb_mem_size = ALIGN(fb_mem_size, PAGE_SIZE);
    info->frame_count = frame_count;

    fbdev->fb_mem = m_dma_alloc_coherent(fb_mem_size, PAGE_SIZE);
    fbdev->fb_mem_size = fb_mem_size;
    memset(info->fb_mem, 0, fb_mem_size);
    info->fb_mem = fbdev->fb_mem;

    struct srdma_cfg *srdma_cfg = &fbdev->srdma_cfg;
    srdma_cfg->is_video = is_tft(jzfb.pdata);
    srdma_cfg->fb_fmt = info->fb_fmt;
    srdma_cfg->fb_mem = info->fb_mem;
    srdma_cfg->stride = info->xres;
}


static void lcdc_init_fbdev(void)
{
    init_fbdev_data(&jzfb.fbdev, jzfb.fbdev.frame_count);
}


void lcdc_alloc_desc(void)
{
    int i;
    for (i = 0; i < 2;i++) {
        jzfb.frames[i] = m_dma_alloc_coherent(sizeof(struct srdmadesc), 64);
    }
}

void lcdc_srdma_init(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        init_srdma_desc(jzfb.frames[i], &jzfb.fbdev.srdma_cfg);
        m_cache_sync(jzfb.frames[i], sizeof(struct srdmadesc));
    }
}

static int free_mem(void)
{
    int i;

    struct fbdev_data *fbdev = &jzfb.fbdev;
    if (fbdev->frame_count)
        m_dma_free_coherent(fbdev->fb_mem, fbdev->fb_mem_size);

    for (i = 0; i < 2; i++)
        m_dma_free_coherent(jzfb.frames[i], sizeof(*jzfb.frames[i]));

    return 0;
}


static int lcdc_tft_pan_display(struct srdmadesc *frame, struct fbdev_data *fbdev)
{
    unsigned long flags;

    spin_lock_irqsave(&jzfb.spinlock, flags);
    jzfb.display_state = state_display_start;

    lcd_write(SRD_CHAIN_ADDR, virt_to_phys(frame));
    lcd_start_simple_read();

    spin_unlock_irqrestore(&jzfb.spinlock, flags);

    wait_event_timeout(jzfb.wait_queue,
    jzfb.display_state == state_display_end, msecs_to_jiffies(300));
    if (jzfb.display_state != state_display_end) {
        printk(KERN_EMERG "jzfb: tft pan display wait timeout %d\n", jzfb.display_state);
        return -1;
    }

    return 0;
}

static int lcdc_slcd_pan_display(struct srdmadesc *frame, struct fbdev_data *fbdev)
{
    /*等待旧的一帧刷新完成*/
    if (jzfb.display_state != state_clear) {
        wait_event_timeout(jzfb.wait_queue,
            jzfb.display_state == state_display_end, msecs_to_jiffies(300));
        if (jzfb.display_state != state_display_end) {
            printk(KERN_EMERG "jzfb: slcd pan display wait timeout\n");
            return -1;
        }
    }

    slcd_wait_busy_us(10*1000);
    jzfb.display_state = state_display_start;

    lcd_write(SRD_CHAIN_ADDR, virt_to_phys(frame));
    lcd_start_simple_read();

    return 0;
}

void lcdc_pan_display(struct fbdev_data *fbdev, int data_index)
{
    int ret;
    int index;

    fast_iob();

    mutex_lock(&jzfb.lock);

    jzfb.frame_index++;
    index = jzfb.frame_index % 2;

    struct srdmadesc *frame = jzfb.frames[index];
    frame->FrameBufferAddr = virt_to_phys(fbdev->srdma_cfg.fb_mem + data_index * fbdev->user.bytes_per_frame);


    if (!is_tft(jzfb.pdata))
        ret = lcdc_slcd_pan_display(frame, fbdev);
    else
        ret = lcdc_tft_pan_display(frame, fbdev);

    mutex_unlock(&jzfb.lock);

    if (ret < 0)
        return;

    if (jzfb.pan_display_sync) {
        wait_event_timeout(jzfb.wait_queue,
            jzfb.display_state == state_display_end, msecs_to_jiffies(300));
        if (jzfb.display_state != state_display_end)
            printk(KERN_EMERG "jzfb: sync lcd pan display wait timeout\n");
    }
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
    mutex_lock(&jzfb.lock);
    jzfb.is_open--;
    mutex_unlock(&jzfb.lock);
    return 0;
}

static int jzfb_blank(int blank_mode, struct fb_info *info)
{
    struct fbdev_data *fbdev = info->par;
    mutex_lock(&jzfb.lock);

    if (blank_mode == FB_BLANK_UNBLANK) {
        if (!fbdev->is_power_on) {
            enable_fb();
            fbdev->is_power_on = 1;
        }
    } else {
        if (fbdev->is_power_on) {
            disable_fb();
            fbdev->is_power_on = 0;
        }
    }

    mutex_unlock(&jzfb.lock);
    return 0;
}

static int jzfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    struct fbdev_data *fbdev = info->par;

    if (var->xres != fbdev->user.xres ||
        var->yres != fbdev->user.yres)
        return -EINVAL;

    init_var_info(&fbdev->user, var);

    return 0;
}

static int jzfb_set_par(struct fb_info *info)
{
    return jzfb_check_var(&info->var, info);
}

static int jzfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    return -1;
}

static int jzfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
    struct fbdev_data *fbdev = info->par;
    unsigned long start;
    unsigned long off;
    u32 len;

    if (fbdev->fb->fix.smem_len == 0)
        return -ENOMEM;

    off = vma->vm_pgoff << PAGE_SHIFT;

    len = fbdev->fb->fix.smem_len;
    len = ALIGN(len, PAGE_SIZE);
    if ((vma->vm_end - vma->vm_start + off) > len)
        return -EINVAL;

    start = fbdev->fb->fix.smem_start;
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


static int jzfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    struct fbdev_data *fbdev = info->par;
    struct fb_mem_info *user = &fbdev->user;
    int next_frm;
    int ret = -EINVAL;

    if (var->xoffset - info->var.xoffset) {
        printk(KERN_ERR "jzfb: No support for X panning for now\n");
        return ret;
    }

    next_frm = var->yoffset / var->yres;
    if (next_frm >= user->frame_count) {
        printk(KERN_ERR "jzfb: yoffset is out of framebuffer: %d\n", var->yoffset);
        return ret;
    }

    if (!fbdev->is_power_on) {
        printk(KERN_ERR "jzfb: fb is not enabled\n");
        ret = -EBUSY;
        return ret;
    }


    lcdc_pan_display(fbdev, next_frm);

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
    // .fb_fillrect = cfb_fillrect,
    // .fb_copyarea = cfb_copyarea,
    // .fb_imageblit = cfb_imageblit,
    .fb_ioctl = jzfb_ioctl,
    .fb_mmap = jzfb_mmap,
};


static void init_one_fb(struct fbdev_data *fbdev)
{
    struct fb_info *fb;
    struct fb_mem_info *info;
    int ret;

    fb = framebuffer_alloc(0, NULL);
    assert(fb);

    fb->par = fbdev;
    fbdev->fb = fb;
    info = &fbdev->user;
    init_video_mode(info, &fbdev->videomode);
    fb_videomode_to_modelist(&fbdev->videomode, 1, &fb->modelist);
    init_fix_info(info, &fb->fix);
    init_var_info(info, &fb->var);
    fb->fbops = &jzfb_ops;
    fb->flags = FBINFO_DEFAULT;
    fb->screen_base = info->fb_mem;
    fb->screen_size = info->bytes_per_frame;
    ret = register_framebuffer(fb);
    assert(!ret);
}

static void release_one_fb(struct fbdev_data *fbdev)
{
    int ret = unregister_framebuffer(fbdev->fb);
    assert(!ret);

    framebuffer_release(fbdev->fb);
}

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
    error_if(pdata->fb_fmt > fb_fmt_ARGB8888);

    if (is_slcd(pdata))
        error_if(check_scld_fmt(pdata));

    auto_calculate_pixel_clock(pdata);

    ret = init_gpio(pdata);
    if (ret)
        goto unlock;

    jzfb.pdata = pdata;

    lcdc_alloc_desc();

    lcdc_init_fbdev();

    lcdc_srdma_init();


    init_one_fb(&jzfb.fbdev);

unlock:
    mutex_unlock(&jzfb.lock);

    return ret;
}

void jzfb_unregister_lcd(struct lcdc_data *pdata)
{
    mutex_lock(&jzfb.lock);

    assert(pdata == jzfb.pdata);

    assert(!jzfb.is_open);

    if (jzfb.is_enabled) {
        jzfb.is_enabled = 1;
        disable_fb();
    }

    free_mem();

    release_one_fb(&jzfb.fbdev);

    jzfb_release_pins();

    jzfb.pdata = NULL;

    mutex_unlock(&jzfb.lock);
}

static int jzfb_module_init(void)
{
    int ret;

    mutex_init(&jzfb.lock);
    spin_lock_init(&jzfb.spinlock);
    init_waitqueue_head(&jzfb.wait_queue);
    jzfb.irq = IRQ_INTC_BASE + 31;

    jzfb.clk = clk_get(NULL, "gate_lcd");
    assert(!IS_ERR(jzfb.clk));

    jzfb.pclk = clk_get(NULL, "div_lcd");
    assert(!IS_ERR(jzfb.pclk));

    ret = request_irq(jzfb.irq, jzfb_irq_handler, IRQF_SHARED, "jzfb", &jzfb);
    assert(!ret);

    return 0;
}

static void jzfb_module_exit(void)
{
    assert(!jzfb.pdata);

    clk_put(jzfb.clk);
    clk_put(jzfb.pclk);

    free_irq(jzfb.irq, &jzfb);
}

module_init(jzfb_module_init);
module_exit(jzfb_module_exit);

EXPORT_SYMBOL(jzfb_register_lcd);
EXPORT_SYMBOL(jzfb_unregister_lcd);

MODULE_DESCRIPTION("Ingenic Soc FB driver");
MODULE_LICENSE("GPL");
