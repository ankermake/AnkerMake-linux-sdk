#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>

#include "common.h"
#include "lcdc_data.h"

static int frame_num = 0;

module_param(frame_num, int, 0644);

static struct {
    struct mutex mutex;

    struct lcdc_data *data;

    struct fb_videomode videomode;
    struct fb_info *fb;

    int spi_fb_en;
    int is_stop;
    int is_open;

    void *fb_mem;
    unsigned int xres;
    unsigned int yres;
    unsigned int bytes_per_line;
    unsigned int bytes_per_frame;
    unsigned int frame_count;
    unsigned int frame_alloc_size;
}spi_fb;

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

static void spi_fb_init_video_mode(struct fb_videomode *mode)
{
    struct lcdc_data *data = spi_fb.data;

    mode->name = data->name;
    mode->refresh = data->refresh;
    mode->xres = data->xres;
    mode->yres = data->yres;
    mode->pixclock = 0;
    mode->left_margin = 0;
    mode->right_margin = 0;
    mode->upper_margin = 0;
    mode->lower_margin = 0;
    mode->hsync_len = 0;
    mode->vsync_len = 0;
    mode->sync = 0;
    mode->vmode = 0;
    mode->flag = 0;
}

static void spi_fb_init_fix_info(struct fb_fix_screeninfo *fix)
{
    strcpy(fix->id, "spi_fb");
    fix->type = FB_TYPE_PACKED_PIXELS;
    fix->visual = FB_VISUAL_TRUECOLOR;
    fix->xpanstep = 0;
    fix->ypanstep = 1;
    fix->ywrapstep = 0;
    fix->accel = FB_ACCEL_NONE;
    fix->line_length = spi_fb.bytes_per_line;
    fix->smem_start = virt_to_phys(spi_fb.fb_mem);
    fix->smem_len = spi_fb.bytes_per_frame * spi_fb.frame_count;
    fix->mmio_start = 0;
    fix->mmio_len = 0;
}

static inline void set_fb_bitfield(struct fb_bitfield *field, int length, int offset, int msb_right)
{
    field->length = length;
    field->offset = offset;
    field->msb_right = msb_right;
}

static void spi_fb_init_var_info(struct fb_var_screeninfo *var)
{
    struct lcdc_data *data = spi_fb.data;

    var->xres = spi_fb.xres;
    var->yres = spi_fb.yres;
    var->xres_virtual = spi_fb.xres;
    var->yres_virtual = spi_fb.yres * spi_fb.frame_count;
    var->xoffset = 0;
    var->yoffset = 0;
    var->height = data->height;
    var->width = data->width;
    var->pixclock = data->pixclock;
    var->left_margin = data->left_margin;
    var->right_margin = data->right_margin;
    var->upper_margin = data->upper_margin;
    var->lower_margin = data->lower_margin;
    var->hsync_len = data->hsync_len;
    var->vsync_len = data->vsync_len;
    var->sync = 0;
    var->vmode = 0;

    var->bits_per_pixel = bits_per_pixel(data->fb_format);
    switch (data->fb_format) {
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
    default:
        printk(KERN_ERR "spi_fb: No support for fb_fmt's type %d\n", data->fb_format);
        assert(0);
    }
}

static int spi_fb_enable(void)
{
    if (spi_fb.spi_fb_en)
        return 0;
    spi_fb.spi_fb_en = 1;

    if (spi_fb.data->power_on)
        spi_fb.data->power_on(NULL);

    return 0;
}

static void spi_fb_disable(void)
{
    if (!spi_fb.spi_fb_en)
        return;
    spi_fb.spi_fb_en = 0;

    if (spi_fb.data->power_off)
        spi_fb.data->power_off(NULL);
}

static int spi_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
    unsigned long start;
    unsigned long off;
    u32 len;

    off = vma->vm_pgoff << PAGE_SHIFT;

    /* frame buffer memory */
    if (spi_fb.fb->fix.smem_len == 0)
        return -ENOMEM;

    len = spi_fb.fb->fix.smem_len;
    len = ALIGN(len, PAGE_SIZE);
    if ((vma->vm_end - vma->vm_start + off) > len)
        return -EINVAL;

    start = spi_fb.fb->fix.smem_start;
    start &= PAGE_MASK;
    off += start;

    vma->vm_pgoff = off >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;

    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= 0;

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start, vma->vm_page_prot))
    {
        return -EAGAIN;
    }

    return 0;
}

static int spi_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    if (var->xres != spi_fb.xres ||
        var->yres != spi_fb.yres)
        return -EINVAL;

    spi_fb_init_var_info(var);

    return 0;
}

static int spi_fb_set_par(struct fb_info *info)
{
    return spi_fb_check_var(&info->var, info);
}

static int spi_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    printk(KERN_ERR "spi_fb: No support for command %x\n", cmd);
    return -1;
}

static int spi_fb_open(struct fb_info *info, int user)
{
    mutex_lock(&spi_fb.mutex);

    spi_fb.is_open++;

    mutex_unlock(&spi_fb.mutex);

    return 0;
}

static int spi_fb_release(struct fb_info *info, int user)
{
    mutex_lock(&spi_fb.mutex);

    spi_fb.is_open--;

    mutex_unlock(&spi_fb.mutex);

    return 0;
}

static int spi_fb_blank(int blank_mode, struct fb_info *info)
{
    mutex_lock(&spi_fb.mutex);

    if (blank_mode == FB_BLANK_UNBLANK)
        spi_fb_enable();
    else
        spi_fb_disable();

    mutex_unlock(&spi_fb.mutex);

    return 0;
}

static int spi_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    int next_frm;
    if (var->xoffset - info->var.xoffset) {
        printk(KERN_ERR "spi_fb: No support for X panning for now\n");
        return -EINVAL;
    }

    next_frm = var->yoffset / var->yres;
    if (next_frm >= spi_fb.frame_count) {
        printk(KERN_ERR "spi_fb: yoffset is out of framebuffer: %d\n", var->yoffset);
        return -EINVAL;
    }

    mutex_lock(&spi_fb.mutex);

    if (!spi_fb.spi_fb_en)
        goto unlock;

    spi_fb.data->write_fb_data(spi_fb.fb_mem + next_frm * spi_fb.bytes_per_frame);

unlock:
    mutex_unlock(&spi_fb.mutex);

    return 0;
}

static struct fb_ops spi_fb_ops = {
    .owner = THIS_MODULE,
    .fb_open = spi_fb_open,
    .fb_release = spi_fb_release,
    .fb_check_var = spi_fb_check_var,
    .fb_set_par = spi_fb_set_par,
    .fb_blank = spi_fb_blank,
    .fb_pan_display = spi_fb_pan_display,
    // .fb_fillrect = cfb_fillrect,
    // .fb_copyarea = cfb_copyarea,
    // .fb_imageblit = cfb_imageblit,
    .fb_ioctl = spi_fb_ioctl,
    .fb_mmap = spi_fb_mmap,
};

static int spi_fb_alloc_mem(void)
{
    struct lcdc_data *data = spi_fb.data;
    void *mem = NULL;

    spi_fb.xres = data->xres;
    spi_fb.yres = data->yres;
    spi_fb.bytes_per_line = data->xres * bytes_per_pixel(data->fb_format);
    spi_fb.bytes_per_frame = spi_fb.bytes_per_line * data->yres;
    spi_fb.frame_count = frame_num;
    spi_fb.frame_alloc_size = ALIGN(spi_fb.bytes_per_frame * spi_fb.frame_count, PAGE_SIZE);

    mem = m_dma_alloc_coherent(spi_fb.frame_alloc_size);
    if (mem == NULL) {
        printk(KERN_ERR "spi_fb: failed to alloc fb mem\n");
        return -ENOMEM;
    }

    spi_fb.fb_mem = mem;
    return 0;
}

static void init_fb(void)
{
    int ret = 0;

    spi_fb.fb = framebuffer_alloc(0, NULL);
    assert(spi_fb.fb);

    spi_fb_init_video_mode(&spi_fb.videomode);
    spi_fb_init_fix_info(&spi_fb.fb->fix);
    spi_fb_init_var_info(&spi_fb.fb->var);
    fb_videomode_to_modelist(&spi_fb.videomode, 1, &spi_fb.fb->modelist);

    spi_fb.fb->fbops = &spi_fb_ops;
    spi_fb.fb->flags = FBINFO_DEFAULT;
    spi_fb.fb->screen_base = spi_fb.fb_mem;
    spi_fb.fb->screen_size = spi_fb.bytes_per_frame;

    ret = register_framebuffer(spi_fb.fb);
    assert(!ret);
}

int spi_fb_register_lcd(struct lcdc_data *pdata)
{
    int ret = 0;

    mutex_init(&spi_fb.mutex);

    mutex_lock(&spi_fb.mutex);

    spi_fb.data = pdata;

    ret = spi_fb_alloc_mem();
    if (ret) {
        spi_fb.data = NULL;
        goto unlock;
    }

    init_fb();

    goto unlock;

unlock:
    mutex_unlock(&spi_fb.mutex);

    return ret;
}

void spi_fb_unregister_lcd(struct lcdc_data *pdata)
{
    mutex_lock(&spi_fb.mutex);

    assert(pdata == spi_fb.data);
    assert(!spi_fb.is_open);

    spi_fb_disable();

    unregister_framebuffer(spi_fb.fb);

    m_dma_free_coherent(spi_fb.fb_mem, spi_fb.frame_alloc_size);

    framebuffer_release(spi_fb.fb);

    spi_fb.data = NULL;

    mutex_unlock(&spi_fb.mutex);
}

EXPORT_SYMBOL(spi_fb_register_lcd);
EXPORT_SYMBOL(spi_fb_unregister_lcd);

MODULE_DESCRIPTION("Ingenic Soc SPI_FB driver");
MODULE_LICENSE("GPL");