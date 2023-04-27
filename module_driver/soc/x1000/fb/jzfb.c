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
#include <soc/irq.h>

#include "common.h"

#include "lcdc_data.h"
#include "lcdc.c"
#include "lcd_gpio.c"

#define JZFB_MAX_OPEN_COUNT 10

static int pan_display_sync = 0;
static int frame_num = 0;

module_param(pan_display_sync, int, 0644);
module_param(frame_num, int, 0644);

enum jzfb_display_state {
    state_display_clear,
    state_display_start,
    state_display_end,
};

static struct {
    struct clk *clk_lcd;
    struct clk *clk_cgu;

    int irq;
    int te_irq;
    void *fb_mem_mapped;

    struct lcdc_frame_desc *data_framedesc;
    struct lcdc_frame_desc *cmd_framedesc;

    struct mutex mutex;
    wait_queue_head_t wait_queue;
    struct lcdc_data *data;

    struct jzfb_lcd_msg lcd;
    struct jzfb_lcd_msg user;

    struct fb_videomode videomode;
    struct fb_info *fb;

    int jzfb_en;
    int is_stop;
    int is_open;
    int te_en;

    enum jzfb_display_state display_state;

    struct jzfb_user_data {
        pid_t tgid;
        void *map_addr;
        int count;
    } user_data[JZFB_MAX_OPEN_COUNT];

}jzfb;

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

static inline int bytes_per_pixel(enum fb_fmt fmt)
{
    if (fmt == fb_fmt_RGB888)
        return 4;
    return 2;
}

static inline int bits_per_pixel(enum fb_fmt fmt)
{
    if (fmt == fb_fmt_RGB888)
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

static void jzfb_init_user_info(void)
{
    jzfb.user.xres = jzfb.data->xres;
    jzfb.user.yres = jzfb.data->yres;
    jzfb.user.bytes_per_line = jzfb.lcd.bytes_per_line;
    jzfb.user.bytes_per_frame = jzfb.lcd.bytes_per_frame;
    jzfb.user.frame_count = frame_num;
    jzfb.user.fb_mem = jzfb.lcd.fb_mem;
}

static void jzfb_init_video_mode(struct fb_videomode *mode)
{
    struct lcdc_data *data = jzfb.data;

    mode->name = data->name;
    mode->refresh = data->refresh;
    mode->xres = data->xres;
    mode->yres = data->yres;
    mode->pixclock = data->pixclock;
    mode->left_margin = data->left_margin;
    mode->right_margin = data->right_margin;
    mode->upper_margin = data->upper_margin;
    mode->lower_margin = data->lower_margin;
    mode->hsync_len = data->hsync_len;
    mode->vsync_len = data->vsync_len;
    mode->sync = 0;
    mode->vmode = 0;
    mode->flag = 0;
}

static void jzfb_init_fix_info(struct fb_fix_screeninfo *fix)
{
    struct jzfb_lcd_msg *user = &jzfb.user;

    strcpy(fix->id, "jzfb");
    fix->type = FB_TYPE_PACKED_PIXELS;
    fix->visual = FB_VISUAL_TRUECOLOR;
    fix->xpanstep = 0;
    fix->ypanstep = 1;
    fix->ywrapstep = 0;
    fix->accel = FB_ACCEL_NONE;
    fix->line_length = user->bytes_per_line;
    fix->smem_start = virt_to_phys(user->fb_mem);
    fix->smem_len = user->bytes_per_frame * user->frame_count;
    fix->mmio_start = 0;
    fix->mmio_len = 0;
}

static inline void set_fb_bitfield(struct fb_bitfield *field, int length, int offset, int msb_right)
{
    field->length = length;
    field->offset = offset;
    field->msb_right = msb_right;
}

static void jzfb_init_var_info(struct fb_var_screeninfo *var)
{
    struct jzfb_lcd_msg *user = &jzfb.user;
    struct lcdc_data *data = jzfb.data;

    var->xres = user->xres;
    var->yres = user->yres;
    var->xres_virtual = user->xres;
    var->yres_virtual = user->yres * user->frame_count;
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
        assert(0);
    }
}

static struct jzfb_user_data *jzfb_find_user_data(pid_t tgid)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(jzfb.user_data); i++) {
        if (jzfb.user_data[i].tgid == tgid)
            return &jzfb.user_data[i];
    }

    return NULL;
}

static void jzfb_set_user_data(pid_t tgid, void *map_addr)
{
    struct jzfb_user_data *data = jzfb_find_user_data(tgid);

    assert(data);
    data->map_addr = data->count == 1 ? map_addr : 0;
}

static int jzfb_add_user_data(pid_t tgid)
{
    struct jzfb_user_data *data = jzfb_find_user_data(tgid);

    if (data) {
        data->map_addr = 0;
        data->count++;
        return 0;
    }

    data = jzfb_find_user_data(0);
    if (data) {
        data->tgid = tgid;
        data->count = 1;
        data->map_addr = 0;
        return 0;
    }

    return -ENOMEM;
}

static void jzfb_del_user_data(pid_t tgid)
{
    struct jzfb_user_data *data = jzfb_find_user_data(tgid);

    if (data && data->count-- == 1) {
        data->tgid = 0;
        data->map_addr = 0;
    }
}

static void jzfb_process_slcd_data_table(struct smart_lcd_data_table *table, unsigned int length)
{
    int i = 0;

    for (; i < length; i++) {
        switch (table[i].type) {
        case SMART_CONFIG_CMD:
            lcdc_send_cmd(table[i].value);
            break;
        case SMART_CONFIG_PRM:
        case SMART_CONFIG_DATA:
            lcdc_send_data(table[i].value);
            break;
        case SMART_CONFIG_UDELAY:
            if (table[i].value >= 500)
                usleep_range(table[i].value, table[i].value);
            else
                udelay(table[i].value);

            break;
        default:
            assert(0);
            break;
        }
    }

    if (lcdc_wait_busy(10 * 1000))
        panic("lcdc busy\n");

}

static void lcdc_start_framedesc(void)
{
    lcdc_write_framedesc(virt_to_phys(jzfb.cmd_framedesc));
    lcdc_enable();
    lcdc_start_dma();
}

static void jzfb_enable_lcd(void)
{
    clk_enable(jzfb.clk_lcd);

    clk_set_rate(jzfb.clk_cgu, jzfb.data->pixclock_when_init);
    clk_enable(jzfb.clk_cgu);

    lcdc_hal_init(jzfb.data);

    if (jzfb.data->power_on)
        jzfb.data->power_on();

    lcdc_enable_register_mode();

    jzfb_process_slcd_data_table(jzfb.data->slcd_data_table, jzfb.data->slcd_data_table_length);

    lcdc_disable_register_mode(jzfb.data);

    lcdc_set_te(jzfb.data);

    clk_disable(jzfb.clk_cgu);
    clk_set_rate(jzfb.clk_cgu, jzfb.data->pixclock);
    clk_enable(jzfb.clk_cgu);

#ifdef DEBUG
    lcdc_dump_regs();
#endif
}

static int jzfb_enable(void)
{
    if (jzfb.jzfb_en)
        return 0;
    jzfb.jzfb_en = 1;

    jzfb_enable_lcd();
    jzfb.display_state = state_display_clear;

    return 0;
}

static void jzfb_disable(void)
{
    if (!jzfb.jzfb_en)
        return;
    jzfb.jzfb_en = 0;

    lcdc_disable();

    if (jzfb.data->power_off)
        jzfb.data->power_off();

    clk_disable(jzfb.clk_lcd);
    clk_disable(jzfb.clk_cgu);
}

static irqreturn_t jzfb_irq_handler(int irq, void *data)
{
    unsigned long flage;

    if (lcdc_check_interrupt(&flage, FRAME_END)) {
        lcdc_clear_interrupt(FRAME_END);
        jzfb.display_state = state_display_end;
        wake_up_all(&jzfb.wait_queue);
        return IRQ_HANDLED;
    }

    if (lcdc_check_interrupt(&flage, FRAME_START)) {
        lcdc_clear_interrupt(FRAME_START);
        return IRQ_HANDLED;
    }

    if (lcdc_check_interrupt(&flage, QUICK_STOP)) {
        lcdc_clear_interrupt(QUICK_STOP);
        return IRQ_HANDLED;
    }

    printk(KERN_ERR "why this irq: %lx\n", flage);
    return IRQ_HANDLED;

}

static irqreturn_t jzfb_te_irq_handler(int irq, void *data)
{
    if (jzfb.te_en) {
        disable_irq_nosync(jzfb.te_irq);
        lcdc_start_framedesc();
        jzfb.te_en = 0;
    }

    return IRQ_HANDLED;
}

static int jzfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
    unsigned long start;
    unsigned long off;
    u32 len;

    off = vma->vm_pgoff << PAGE_SHIFT;

    /* frame buffer memory */
    start = jzfb.fb->fix.smem_start;
    len = jzfb.fb->fix.smem_len;
    start &= PAGE_MASK;

    len = ALIGN(len, PAGE_SIZE);
    if ((vma->vm_end - vma->vm_start + off) > len)
        return -EINVAL;

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

    mutex_lock(&jzfb.mutex);
    jzfb_set_user_data(current->tgid, (void *)vma->vm_start);
    mutex_unlock(&jzfb.mutex);

    return 0;
}

static int jzfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    if (var->xres != jzfb.user.xres ||
        var->yres != jzfb.user.yres)
        return -EINVAL;

    jzfb_init_var_info(var);

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

static int jzfb_open(struct fb_info *info, int user)
{
    int ret = 0;

    mutex_lock(&jzfb.mutex);

    ret = jzfb_add_user_data(current->tgid);
    if (ret)
        goto unlock;

    jzfb.is_open++;
unlock:
    mutex_unlock(&jzfb.mutex);

    return ret;
}

static int jzfb_release(struct fb_info *info, int user)
{
    mutex_lock(&jzfb.mutex);

    jzfb_del_user_data(current->tgid);
    jzfb.is_open--;

    mutex_unlock(&jzfb.mutex);

    return 0;
}

static int jzfb_blank(int blank_mode, struct fb_info *info)
{
    mutex_lock(&jzfb.mutex);

    if (blank_mode == FB_BLANK_UNBLANK)
        jzfb_enable();
    else
        jzfb_disable();

    mutex_unlock(&jzfb.mutex);

    return 0;
}

static int jzfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    int next_frm;

    if (var->xoffset - info->var.xoffset) {
        printk(KERN_ERR "jzfb: No support for X panning for now\n");
        return -EINVAL;
    }

    next_frm = var->yoffset / var->yres;
    if (next_frm >= jzfb.user.frame_count) {
        printk(KERN_ERR "jzfb: yoffset is out of framebuffer: %d\n", var->yoffset);
        return -EINVAL;
    }

    /*通过设置var->yoffset来决定刷新哪一帧缓冲区*/
    jzfb.data_framedesc->buffer_addr = virt_to_phys(jzfb.lcd.fb_mem + next_frm * jzfb.lcd.bytes_per_frame);


    mutex_lock(&jzfb.mutex);

    if (!jzfb.jzfb_en)
        goto unlock;

    fast_iob();

    if (jzfb.display_state != state_display_clear) {
        wait_event_timeout(jzfb.wait_queue, jzfb.display_state == state_display_end, msecs_to_jiffies(300));
        if (jzfb.display_state != state_display_end) {
            printk(KERN_EMERG "jzfb: lcd pan display wait timeout\n");
            goto unlock;
        }
    }

    jzfb.display_state = state_display_start;


    if (lcdc_wait_busy(200 * 1000))
        panic("fb pan display wait busy error !\n");

    if (jzfb.data->slcd.te_pin_mode != TE_GPIO_IRQ_TRIGGER) {
        lcdc_start_framedesc();
    }

    if (jzfb.data->slcd.te_pin_mode == TE_GPIO_IRQ_TRIGGER) {
        jzfb.te_en = 1;
        enable_irq(jzfb.te_irq);
    }

    if (pan_display_sync) {
        wait_event_timeout(jzfb.wait_queue, jzfb.display_state == state_display_end, msecs_to_jiffies(300));
        if (jzfb.display_state != state_display_end) {
            printk(KERN_EMERG "jzfb: lcd pan display wait irqasldfjklas timeout\n");
            goto unlock;
        }
    }

unlock:
    mutex_unlock(&jzfb.mutex);

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

static int init_gpio(struct lcdc_data *data)
{
    int ret;
    switch (data->lcd_mode) {
    case SLCD_6800:
    case SLCD_8080: {
        int use_cs = 0;
        int use_te = data->slcd.te_pin_mode == TE_LCDC_TRIGGER;
        int width = max(data->slcd.mcu_cmd_width, data->slcd.mcu_data_width);

        if (width != MCU_WIDTH_8BITS) {
            printk(KERN_ERR "just have 8 data lines!\n");
            return -EPERM;
        }

        ret = slcd_init_gpio_data8(use_te, use_cs);
        if (ret < 0)
            return -EIO;
        break;
    }

    default:
        printk(KERN_ERR "This mode is not currently implemented: %d\n", data->lcd_mode);
        return -EPERM;
    }

    return 0;
}

static int jzfb_init_lcd_msg(void)
{
    struct lcdc_data *data = jzfb.data;
    void *mem = NULL;

    jzfb.lcd.xres = data->xres;
    jzfb.lcd.yres = data->yres;

    jzfb.lcd.bytes_per_line = bytes_per_line(data->xres, data->fb_format);
    jzfb.lcd.bytes_per_frame = bytes_per_frame(jzfb.lcd.bytes_per_line, data->yres);
    jzfb.lcd.frame_count = frame_num;
    jzfb.lcd.frame_alloc_size = ALIGN(jzfb.lcd.bytes_per_frame * jzfb.lcd.frame_count, PAGE_SIZE);

    mem = m_dma_alloc_coherent(jzfb.lcd.frame_alloc_size);
    if (mem == NULL) {
        printk(KERN_ERR "jzfb: failed to alloc fb mem\n");
        return -ENOMEM;
    }

    jzfb.lcd.fb_mem = mem;
    return 0;
}

static int init_te_gpio_irq(struct lcdc_data *data)
{
    int ret;

    if (data->slcd.te_pin_mode != TE_GPIO_IRQ_TRIGGER)
        return 0;

    if (data->slcd.te_gpio < 0) {
        data->slcd.te_pin_mode = TE_NOT_EANBLE;
        return 0;
    }

    ret = gpio_request(data->slcd.te_gpio, "lcd te");
    if (ret) {
        printk(KERN_ERR "can's request lcd te_irq gpio\n");
        return -EIO;
    }

    gpio_direction_input(data->slcd.te_gpio);

    jzfb.te_irq = gpio_to_irq(data->slcd.te_gpio);

    int irq_flags = IRQF_DISABLED;

    if (data->slcd.te_data_transfered_edge == AT_RISING_EDGE)
        irq_flags |= IRQ_TYPE_EDGE_FALLING;

    if (data->slcd.te_data_transfered_edge == AT_FALLING_EDGE)
        irq_flags |= IRQ_TYPE_EDGE_RISING;

    ret = request_irq(jzfb.te_irq, jzfb_te_irq_handler, irq_flags, "slcd_te", NULL);
    assert(!ret);

    disable_irq(jzfb.te_irq);

    data->slcd.te_pin_mode = TE_GPIO_IRQ_TRIGGER;

    return 0;

}

static int slcd_pixclock_cycle(struct lcdc_data *data)
{
    int cycle = 0;
    int width = data->slcd.mcu_data_width;
    int pix_fmt = data->out_format;
    if (width == MCU_WIDTH_8BITS) {
        if (pix_fmt == OUT_FORMAT_565)
            cycle = 2;
        if (pix_fmt == OUT_FORMAT_888)
            cycle = 3;
    }
    if (width == MCU_WIDTH_9BITS) {
        cycle = 2;
    }
    if (width == MCU_WIDTH_16BITS) {
        cycle = 1;
    }

    assert(cycle);

    return cycle;
}

static void auto_calculate_pixel_clock(struct lcdc_data *data)
{
    if (!data->refresh)
        data->refresh = 40;

    if (!data->pixclock) {
        data->pixclock = data->xres * data->yres * data->refresh;
        data->pixclock *= slcd_pixclock_cycle(data);
    }

    if (!data->pixclock_when_init)
        data->pixclock_when_init = data->xres * data->yres;
}

static void init_fb(void)
{
    int ret = 0;

    jzfb.fb = framebuffer_alloc(0, NULL);
    assert(jzfb.fb);

    jzfb_init_video_mode(&jzfb.videomode);
    jzfb_init_fix_info(&jzfb.fb->fix);
    jzfb_init_var_info(&jzfb.fb->var);
    fb_videomode_to_modelist(&jzfb.videomode, 1, &jzfb.fb->modelist);

    jzfb.fb->fbops = &jzfb_ops;
    jzfb.fb->flags = FBINFO_DEFAULT;
    jzfb.fb->screen_base = jzfb.user.fb_mem;
    jzfb.fb->screen_size = jzfb.user.bytes_per_frame;

    ret = register_framebuffer(jzfb.fb);
    assert(!ret);
}


static void init_framedesc(void)
{
    struct lcdc_data *data = jzfb.data;
    void *cmd_buf;

    cmd_buf = m_dma_alloc_coherent(32);

    jzfb.cmd_framedesc = m_dma_alloc_coherent(sizeof(struct lcdc_frame_desc) * 2);
    jzfb.data_framedesc = jzfb.cmd_framedesc + 1;

    init_cmd_framedesc(data, cmd_buf, jzfb.cmd_framedesc, jzfb.data_framedesc);
    init_data_framedesc(data, &jzfb.lcd, jzfb.data_framedesc, jzfb.cmd_framedesc);
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

    mutex_lock(&jzfb.mutex);

    error_if(jzfb.data != NULL);
    error_if(pdata == NULL);
    error_if(pdata->name == NULL);
    error_if(pdata->power_on == NULL);
    error_if(pdata->power_off == NULL);
    error_if(pdata->xres < 32 || pdata->xres >= 2048);
    error_if(pdata->yres < 32 || pdata->yres >= 2048);
    error_if(pdata->fb_format > fb_fmt_RGB888);

    ret = init_gpio(pdata);
    if (ret)
        goto unlock;

    ret = init_te_gpio_irq(pdata);
    if (ret)
        goto te_gpio_init_err;

    auto_calculate_pixel_clock(pdata);

    jzfb.data = pdata;

    ret = jzfb_init_lcd_msg();
    if (ret) {
        jzfb.data = NULL;
        goto lcd_msg_init_err;
    }

    init_framedesc();

    jzfb_init_user_info();

    init_fb();

    goto unlock;


lcd_msg_init_err:
    if (jzfb.te_irq) {
        jzfb.te_irq = 0;
        disable_irq(jzfb.te_irq);
        free_irq(jzfb.te_irq, NULL);
        gpio_free(pdata->slcd.te_gpio);
    }

te_gpio_init_err:
    slcd_free_gpio();

unlock:
    mutex_unlock(&jzfb.mutex);

    return ret;

}

void jzfb_unregister_lcd(struct lcdc_data *pdata)
{
    mutex_lock(&jzfb.mutex);

    int ret;
    assert(pdata == jzfb.data);
    assert(!jzfb.is_open);

    if (jzfb.te_irq) {
        jzfb.te_irq = 0;
        disable_irq(jzfb.te_irq);
        free_irq(jzfb.te_irq, NULL);
        gpio_free(jzfb.data->slcd.te_gpio);
    }

    jzfb_disable();

    ret = unregister_framebuffer(jzfb.fb);
    assert(!ret);

    m_dma_free_coherent(jzfb.lcd.fb_mem, jzfb.lcd.frame_alloc_size);
    m_dma_free_coherent(jzfb.data_framedesc, sizeof(struct lcdc_frame_desc) * 2);

    framebuffer_release(jzfb.fb);

    slcd_free_gpio();

    jzfb.data = NULL;

    mutex_unlock(&jzfb.mutex);

}

static int jzfb_module_init(void)
{
    int ret;
    mutex_init(&jzfb.mutex);
    init_waitqueue_head(&jzfb.wait_queue);

    jzfb.clk_lcd = clk_get(NULL, "lcd");
    assert(!IS_ERR(jzfb.clk_lcd));

    jzfb.clk_cgu = clk_get(NULL, "cgu_lcd");
    assert(!IS_ERR(jzfb.clk_cgu));

    jzfb.irq = IRQ_LCD;
    ret = request_irq(jzfb.irq, jzfb_irq_handler, IRQF_DISABLED, "lcdc", NULL);
    assert(!ret);

    return 0;
}

static void jzfb_module_exit(void)
{
    assert(!jzfb.data);

    disable_irq(jzfb.irq);

    clk_put(jzfb.clk_cgu);
    clk_put(jzfb.clk_lcd);

    free_irq(jzfb.irq, NULL);
}

module_init(jzfb_module_init);

module_exit(jzfb_module_exit);

EXPORT_SYMBOL(jzfb_register_lcd);
EXPORT_SYMBOL(jzfb_unregister_lcd);

MODULE_DESCRIPTION("Ingenic Soc FB driver");
MODULE_LICENSE("GPL");
