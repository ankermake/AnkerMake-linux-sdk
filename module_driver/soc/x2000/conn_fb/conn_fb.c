#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>

#include <assert.h>

#include <common.h>
#include <bit_field.h>

#include <soc/x2000/fb/lcdc_data.h>

#include <utils/clock.h>

#include <conn.h>

#define LINUX_CPU_ID    0
#define NOTIFY_WRITE_TIMEOUT_MS 1000
#define NOTIFY_READ_TIMEOUT_MS  5000
#define CONN_FB_DEV_MAX_COUNT 5

#define CMD_set_cfg       _IOWR('i', 80, struct lcdc_layer)
#define CMD_enable_cfg    _IO('i', 81)
#define CMD_disable_cfg   _IO('i', 82)
#define CMD_read_reg      _IOWR('i', 83, unsigned long)

enum conn_cmd {
    CMD_conn_fb_conn_detect,
    CMD_conn_fb_get_info,
    CMD_conn_fb_enable,
    CMD_conn_fb_pan_display,
    CMD_conn_fb_set_cfg,
    CMD_conn_fb_enable_config,
    CMD_conn_fb_disable_config,
    CMD_conn_fb_read_reg,
};

static const char *conn_cmd_str[] = {
    [CMD_conn_fb_conn_detect]  = "conn_fb_conn_detect",
    [CMD_conn_fb_get_info]     = "conn_fb_get_info",
    [CMD_conn_fb_enable]       = "conn_fb_enable",
    [CMD_conn_fb_pan_display]  = "conn_fb_pan_display",
    [CMD_conn_fb_set_cfg]      = "conn_fb_set_cfg",
    [CMD_conn_fb_enable_config] = "conn_fb_enable_config",
    [CMD_conn_fb_disable_config] = "conn_fb_disable_config",
    [CMD_conn_fb_read_reg]     = "conn_fb_read_reg",
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

struct conn_fb_data {
    int index;
    int is_enable;
    char *device_name;
    int *frame_index;
    struct mutex lock;
    struct conn_node *conn;
    struct fb_mem_info info;
    struct lcdc_layer *config;
    struct miscdevice conn_fb_mdev;
};

static struct conn_fb_data conn_fb_dev[5] = {
    {
        .index                  = 0,
        .device_name            = "conn_fb0",
    },
    {
        .index                  = 1,
        .device_name            = "conn_fb1",
    },
    {
        .index                  = 2,
        .device_name            = "conn_fb2",
    },
    {
        .index                  = 3,
        .device_name            = "conn_fb3",
    },
    {
        .index                  = 4,
        .device_name            = "conn_fb_srdma",
    },
};

struct conn_fb_notify {
    enum conn_cmd cmd;
    void *data;
};

static inline void set_fb_bitfield(struct fb_bitfield *field, int length, int offset, int msb_right)
{
    field->length = length;
    field->offset = offset;
    field->msb_right = msb_right;
}

static int conn_fb_send_cmd(struct conn_node *conn, enum conn_cmd cmd, void *data_in, void *data_out)
{
    int ret;
    struct conn_fb_notify notify;
    int len = sizeof(notify);

    notify.cmd = cmd;
    notify.data = data_in;

    ret = conn_write(conn, &notify, len, NOTIFY_WRITE_TIMEOUT_MS);
    if (ret != len) {
        printk(KERN_ERR "CONN_FB: %s ,failed to write conn. ret = %d. (CPU%d)\n", conn_cmd_str[cmd], ret, LINUX_CPU_ID);
        return -1;
    }

    ret = conn_read(conn, &notify, len, NOTIFY_READ_TIMEOUT_MS);
    if (ret != len) {
        printk(KERN_ERR "CONN_FB: %s ,failed to read conn. ret = %d. (CPU%d)\n", conn_cmd_str[cmd], ret, LINUX_CPU_ID);
        return -1;
    }

    if (notify.data && data_out)
        *(unsigned long *)data_out = (unsigned long)notify.data;

    return 0;
}

static int conn_fb_get_info(struct conn_fb_data *drv)
{
    unsigned long data = 0;
    struct fb_mem_info *info = &drv->info;

    int ret = conn_fb_send_cmd(drv->conn, CMD_conn_fb_get_info, NULL, (void *)&data);
    if (ret < 0)
        return ret;

    *info = *(struct fb_mem_info *)data;

    return ret;
}


static int conn_fb_get_fix_info(struct conn_fb_data *drv, struct fb_fix_screeninfo *fix)
{
    fix->line_length = drv->info.bytes_per_line;
    fix->smem_len = drv->info.frame_count * drv->info.bytes_per_frame;

    return 0;
}


static int conn_fb_get_var_info(struct conn_fb_data *drv, struct fb_var_screeninfo *var)
{
    var->xres = drv->info.xres;
    var->yres = drv->info.yres;
    var->bits_per_pixel = drv->info.bytes_per_frame;
    var->yres_virtual = drv->info.yres * drv->info.frame_count;

    switch (drv->info.fb_fmt) {
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

    return 0;
}

static int conn_fb_set_config(struct conn_fb_data *drv, struct lcdc_layer *cfg)
{
    return conn_fb_send_cmd(drv->conn, CMD_conn_fb_set_cfg, (void*)cfg, NULL);
}

static int conn_fb_enable(struct conn_fb_data *drv)
{
    return conn_fb_send_cmd(drv->conn, CMD_conn_fb_enable, NULL, NULL);
}

static int conn_fb_pan_display(struct conn_fb_data *drv, int *frame_index)
{
    return conn_fb_send_cmd(drv->conn, CMD_conn_fb_pan_display, (void*)frame_index, NULL);
}

static int conn_fb_enbale_config(struct conn_fb_data *drv)
{
    return conn_fb_send_cmd(drv->conn, CMD_conn_fb_enable_config, NULL, NULL);
}

static int conn_fb_disable_config(struct conn_fb_data *drv)
{
    return conn_fb_send_cmd(drv->conn, CMD_conn_fb_disable_config, NULL, NULL);
}

static int conn_fb_open(struct inode *inode, struct file *file)
{
    struct conn_fb_data *drv = container_of(file->private_data, struct conn_fb_data, conn_fb_mdev);

    int ret = conn_fb_get_info(drv);
    if (ret) {
        printk(KERN_ERR "CONN_FB[%d]: get info fail\n", drv->index);
        return ret;
    }

    file->private_data = &drv->index;

    return 0;
}

static int conn_fb_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long conn_fb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    int index = *((int *)file->private_data);

    struct conn_fb_data *drv = &conn_fb_dev[index];
    if (!drv)
        return -EINVAL;

    mutex_lock(&drv->lock);

    switch (cmd) {
        case FBIOGET_FSCREENINFO:
            conn_fb_get_fix_info(drv, (struct fb_fix_screeninfo *)arg);
            break;

        case FBIOGET_VSCREENINFO:
            conn_fb_get_var_info(drv, (struct fb_var_screeninfo *)arg);
            break;

        case FBIOBLANK:
            conn_fb_enable(drv);
            break;

        case FBIOPAN_DISPLAY: {
            struct fb_var_screeninfo *var = (struct fb_var_screeninfo *)arg;
            int index = var->yoffset / drv->info.yres;
            int frame_count = drv->info.frame_count;
            if (index > frame_count) {
                printk(KERN_ERR "CONN_FB[%d]: frame_index(%d) > frame_count(%d)\n", drv->index, index, frame_count);
                ret = -EINVAL;
            }

            *(drv->frame_index) = index;

            conn_fb_pan_display(drv, drv->frame_index);
            break;
        }

        case CMD_set_cfg: {
            struct lcdc_layer *cfg = drv->config;
            *(cfg) = *(struct lcdc_layer *)arg;

            if (cfg->layer_order > lcdc_layer_3) {
                printk(KERN_ERR "CONN_FB[%d]: invalid order: %d\n", drv->index, cfg->layer_order);
                ret = -EINVAL;
                goto unlock;
            }
            if (cfg->alpha.value >= 256) {
                printk(KERN_ERR "CONN_FB[%d]: invalid alpha: %x\n", drv->index, cfg->alpha.value);
                ret =  -EINVAL;
                goto unlock;
            }
            if (cfg->fb_fmt > fb_fmt_NV21) {
                printk(KERN_ERR "CONN_FB[%d]: invalid fmt: %d\n", drv->index, cfg->fb_fmt);
                ret = -EINVAL;
                goto unlock;
            }
            if (cfg->fb_fmt < fb_fmt_NV12) {
                if (cfg->rgb.mem >= (void *)(512*1024*1024)) {
                    printk(KERN_ERR "CONN_FB[%d]: must be phys address\n", drv->index);
                    ret = -EINVAL;
                    goto unlock;
                }
            } else {
                if (cfg->y.mem >= (void *)(512*1024*1024)) {
                    printk(KERN_ERR "CONN_FB[%d]: must be phys address\n", drv->index);
                    ret = -EINVAL;
                    goto unlock;
                }
                if (cfg->uv.mem >= (void *)(512*1024*1024)) {
                    printk(KERN_ERR "CONN_FB[%d]: must be phys address\n", drv->index);
                    ret = -EINVAL;
                    goto unlock;
                }
            }

            if (!cfg->scaling.enable) {
                if (cfg->xres + cfg->xpos > drv->info.xres) {
                    printk(KERN_ERR "CONN_FB[%d]: invalid xres: %d %d\n", drv->index, cfg->xres, cfg->xpos);
                    ret = -EINVAL;
                    goto unlock;
                }
                if (cfg->yres + cfg->ypos > drv->info.yres) {
                    printk(KERN_ERR "CONN_FB[%d]: invalid yres: %d %d\n", drv->index, cfg->yres, cfg->ypos);
                    ret = -EINVAL;
                    goto unlock;
                }
            } else {
                if (cfg->scaling.xres + cfg->xpos > drv->info.xres) {
                    printk(KERN_ERR "CONN_FB[%d]: invalid scaling xres: %d %d\n", drv->index, cfg->scaling.xres, cfg->xpos);
                    ret = -EINVAL;
                    goto unlock;
                }
                if (cfg->scaling.yres + cfg->ypos > drv->info.yres) {
                    printk(KERN_ERR "CONN_FB[%d]: invalid scaling yres: %d %d\n", drv->index, cfg->scaling.yres, cfg->ypos);
                    ret = -EINVAL;
                    goto unlock;
                }
            }

            if (drv->config->fb_fmt < fb_fmt_NV12) {
                drv->config->rgb.mem = (void *)CKSEG0ADDR(drv->config->rgb.mem);
            } else {
                drv->config->y.mem = (void *)CKSEG0ADDR(drv->config->y.mem);
                drv->config->uv.mem = (void *)CKSEG0ADDR(drv->config->uv.mem);
            }

            ret = conn_fb_set_config(drv, drv->config);
            break;
        }

        case CMD_enable_cfg:
            conn_fb_enbale_config(drv);
            break;
        case CMD_disable_cfg:
            conn_fb_disable_config(drv);
            break;
        case CMD_read_reg:
            break;

        default:
            printk(KERN_ERR "CONN_FB[%d]: %x not support this cmd. (CPU%d)\n", index, cmd, LINUX_CPU_ID);
            ret = -EINVAL;
    }


unlock:

    mutex_unlock(&drv->lock);

    return ret;
}


static int conn_fb_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long len;
    unsigned long start;
    unsigned long off;
    int index = *((int *)file->private_data);

    struct conn_fb_data *drv = &conn_fb_dev[index];

    off = vma->vm_pgoff << PAGE_SHIFT;
    if (off) {
        printk(KERN_ERR "CONN_FB[%d]: camera offset must be 0. (CPU%d)\n", index, LINUX_CPU_ID);
        return -EINVAL;
    }

    len = drv->info.frame_count * drv->info.bytes_per_frame;

    len = ALIGN(len, PAGE_SIZE);
    if ((vma->vm_end - vma->vm_start + off) > len)
        return -EINVAL;

    start = virt_to_phys(drv->info.fb_mem);
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

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot))
        return -EAGAIN;

    return 0;
}

static struct file_operations conn_fb_fops = {
    .owner          = THIS_MODULE,
    .open           = conn_fb_open,
    .release        = conn_fb_release,
    .mmap           = conn_fb_mmap,
    .unlocked_ioctl = conn_fb_ioctl,
};


static int conn_fb_alloc(struct conn_fb_data *drv)
{
    drv->frame_index = kmalloc(sizeof(int), GFP_KERNEL);
    if (drv->frame_index == NULL) {
        printk(KERN_ERR "CONN_FB[%d]: kmalloc frame_index space fail\n", drv->index);
        return -1;
    }

    drv->config = kmalloc(sizeof(struct lcdc_layer), GFP_KERNEL);
    if (drv->config == NULL) {
        printk(KERN_ERR "CONN_FB[%d]: kmalloc config space fail\n", drv->index);
        kfree(drv->frame_index);
        return -1;
    }

    return 0;
}

static void conn_fb_free(struct conn_fb_data *drv)
{
    kfree(drv->frame_index);
    kfree(drv->config);
}

static int conn_fb_request(struct conn_fb_data *drv)
{
    struct conn_fb_notify notify;
    int ret;

    int len = sizeof(notify);
    int i = 3;

    struct conn_node *conn = conn_request(drv->device_name, sizeof(struct conn_fb_notify));
    if (!conn)
        return -1;

    notify.cmd = CMD_conn_fb_conn_detect;
    notify.data = NULL;

    while (i) {
        ret = conn_write(conn, &notify, len, NOTIFY_WRITE_TIMEOUT_MS);
        if (ret == len)
            break;

        if (!--i) {
            conn_release(conn);

            return -1;
        }

        usleep_range(10000, 10000);
    }

    drv->conn = conn;

    return 0;
}


static int conn_fb_drv_init(int index)
{
    int ret;
    assert(index < CONN_FB_DEV_MAX_COUNT);

    struct conn_fb_data *drv = &conn_fb_dev[index];

    ret = conn_fb_request(drv);
    if (ret < 0)
        return 0;

    printk("CONN_FB: %s request conn succeed. (CPU%d)\n", drv->device_name, LINUX_CPU_ID);

    drv->conn_fb_mdev.minor = MISC_DYNAMIC_MINOR;
    drv->conn_fb_mdev.name  = drv->device_name;
    drv->conn_fb_mdev.fops  = &conn_fb_fops;

    ret = misc_register(&drv->conn_fb_mdev);
    if (ret < 0)
        panic("CONN_FB: conn_fb register %s err.(CPU%d)\n", drv->device_name, LINUX_CPU_ID);

    conn_fb_alloc(drv);

    mutex_init(&drv->lock);

    drv->is_enable = 1;

    return 0;
}

static void conn_fb_drv_exit(int index)
{
    struct conn_fb_data *drv = &conn_fb_dev[index];

    if (!drv->is_enable)
        return;

    conn_fb_free(drv);

    drv->is_enable = 0;
    conn_release(drv->conn);
}

static int conn_fb_init(void)
{
    int i;

    conn_wait_conn_inited();

    for (i = 0; i < CONN_FB_DEV_MAX_COUNT; i++)
        conn_fb_drv_init(i);

    return 0;
}

static void conn_fb_exit(void)
{
    int i;

    for (i = 0; i < CONN_FB_DEV_MAX_COUNT; i++)
        conn_fb_drv_exit(i);
}

module_init(conn_fb_init);
module_exit(conn_fb_exit);

MODULE_LICENSE("GPL");