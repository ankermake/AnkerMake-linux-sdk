
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
#include <assert.h>

#include <common.h>
#include <bit_field.h>

#include <utils/clock.h>

#include <conn.h>
#include <camera.h>

#define LINUX_CPU_ID    0

#define NOTIFY_WRITE_TIMEOUT_MS 10
#define NOTIFY_READ_TIMEOUT_MS  300

#define CONN_VIC_MAX_FRAME_NUMS 3
#define CONN_VIC_DEV_MAX_COUNT 2

static void *frm_mem_table[CONN_VIC_MAX_FRAME_NUMS];

struct conn_vic_data {
    int index;
    int is_enable;

    void *mem;
    unsigned int mem_cnt;
    unsigned int frm_size;

    char *device_name;
    struct mutex lock;
    struct miscdevice vic_mdev;

    struct conn_node *conn;
};

static struct conn_vic_data conn_vic_dev[2] = {
    {
        .index                  = 0,
        .device_name            = "conn_vic0",
    },

    {
        .index                  = 1,
        .device_name            = "conn_vic1",
    },
};

#define CMD_get_info                _IOWR('C', 120, struct camera_info)
#define CMD_power_on                _IO('C', 121)
#define CMD_power_off               _IO('C', 122)
#define CMD_stream_on               _IO('C', 123)
#define CMD_stream_off              _IO('C', 124)
#define CMD_wait_frame              _IO('C', 125)
#define CMD_put_frame               _IO('C', 126)
#define CMD_get_frame_count         _IO('C', 127)
#define CMD_skip_frames             _IO('C', 128)
#define CMD_get_frame               _IO('C', 131)

enum conn_cmd {
    CMD_conn_vic_get_info,
    CMD_conn_vic_power_on,
    CMD_conn_vic_power_off,
    CMD_conn_vic_stream_on,
    CMD_conn_vic_stream_off,
    CMD_conn_vic_wait_frame,
    CMD_conn_vic_put_frame,
    CMD_conn_vic_get_frame_count,
    CMD_conn_vic_skip_frames,
    CMD_conn_vic_conn_detect,
};

static const char *conn_cmd_str[] = {
    [CMD_conn_vic_get_info] = "conn_vic_get_info",
    [CMD_conn_vic_power_on] = "conn_vic_power_on",
    [CMD_conn_vic_power_off] = "conn_vic_power_off",
    [CMD_conn_vic_stream_on] = "conn_vic_stream_on",
    [CMD_conn_vic_stream_off] = "conn_vic_stream_off",
    [CMD_conn_vic_wait_frame] = "conn_vic_wait_frame",
    [CMD_conn_vic_put_frame] = "conn_vic_put_frame",
    [CMD_conn_vic_get_frame_count] = "conn_vic_get_frame_count",
    [CMD_conn_vic_skip_frames] = "conn_vic_skip_frames",
};

struct m_private_data {
    unsigned int index;
    void *map_mem;
};

struct conn_vic_notify {
    enum conn_cmd cmd;
    void *data;
};

static int check_frame_mem(int index, void *mem, void *base)
{
    struct conn_vic_data *drv = &conn_vic_dev[index];
    unsigned int size = mem - base;

    if (mem < base)
        return -1;

    if (size % drv->frm_size)
        return -1;

    if (size / drv->frm_size >= drv->mem_cnt)
        return -1;

    return 0;
}

static int conn_vic_send_cmd(struct conn_vic_data *drv, enum conn_cmd cmd, void *data)
{
    int ret;
    void **p = data;
    struct conn_vic_notify notify;
    int len = sizeof(notify);

    struct conn_node *conn = drv->conn;

    notify.cmd = cmd;
    notify.data = data;

    ret = conn_write(conn, &notify, len, NOTIFY_WRITE_TIMEOUT_MS);
    if (ret != len) {
        printk(KERN_ERR "CONN_VIC[%d]: %s ,failed to write conn. ret = %d. (CPU%d)\n", drv->index, conn_cmd_str[cmd], ret, LINUX_CPU_ID);
        return -1;
    }

    ret = conn_read(conn, &notify, len, NOTIFY_READ_TIMEOUT_MS);
    if (ret != len) {
        printk(KERN_ERR "CONN_VIC[%d]: %s ,failed to read conn. ret = %d. (CPU%d)\n", drv->index, conn_cmd_str[cmd], ret, LINUX_CPU_ID);
        return -1;
    }

    if (notify.data && data)
        *p = notify.data;

    return 0;
}

static int camera_get_info(struct conn_vic_data *drv, struct camera_info *arg)
{
    int ret;
    struct camera_info *info;

    void *data = NULL;

    if (!arg)
        return -EINVAL;

    info = arg;

    ret = conn_vic_send_cmd(drv, CMD_conn_vic_get_info, (void *)&data);
    if (ret < 0)
        return ret;

    *info = *(struct camera_info *)data;

    drv->frm_size = info->frame_align_size;
    drv->mem_cnt = info->frame_nums;
    drv->mem = phys_to_virt(info->phys_mem);

    ret = 0;

    return ret;
}

static int camera_power_on(struct conn_vic_data *drv)
{
    return conn_vic_send_cmd(drv, CMD_conn_vic_power_on, NULL);
}

static int camera_power_off(struct conn_vic_data *drv)
{
    return conn_vic_send_cmd(drv, CMD_conn_vic_power_off, NULL);
}

static int camera_stream_on(struct conn_vic_data *drv)
{
    return conn_vic_send_cmd(drv, CMD_conn_vic_stream_on, NULL);
}

static int camera_stream_off(struct conn_vic_data *drv)
{
    return conn_vic_send_cmd(drv, CMD_conn_vic_stream_off, NULL);
}

static inline void conn_vic_add_frm_mem_table(struct conn_vic_data *drv, void *mem)
{
    int i;

    if (!mem)
        return;

    for (i = 0; i < drv->mem_cnt; i++) {
        if (!frm_mem_table[i]) {
            frm_mem_table[i] = mem;
            break;
        }
    }
}

static inline void conn_vic_remove_frm_mem_table(struct conn_vic_data *drv, void *mem)
{
    int i;

    for (i = 0; i < drv->mem_cnt; i++) {
        if (frm_mem_table[i] == mem) {
            frm_mem_table[i] = 0;
            break;
        }
    }
}

static void *camera_wait_frame(struct conn_vic_data *drv)
{
    void *mem = NULL;

    int ret = conn_vic_send_cmd(drv, CMD_conn_vic_wait_frame, (void *)&mem);
    if (ret < 0)
        return NULL;

    conn_vic_add_frm_mem_table(drv, mem);

    return mem;
}

static int camera_put_frame(struct conn_vic_data *drv, void *mem)
{
    int ret = conn_vic_send_cmd(drv, CMD_conn_vic_put_frame, mem);
    if (ret < 0)
        return ret;

    conn_vic_remove_frm_mem_table(drv, mem);

    return 0;
}

static int camera_get_available_frame_count(struct conn_vic_data *drv)
{
    int ret;
    int p;

    ret = conn_vic_send_cmd(drv, CMD_conn_vic_get_frame_count, (void *)&p);
    if (ret < 0)
        return ret;

    ret = *((int *)p);

    return ret;
}

static int camera_skip_frames(struct conn_vic_data *drv, unsigned int frames)
{
    return conn_vic_send_cmd(drv, CMD_conn_vic_skip_frames, (void *)frames);
}

static long conn_vic_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct m_private_data *data = filp->private_data;
    unsigned int index = data->index;
    void *map_mem = data->map_mem;
    int ret = 0;

    struct conn_vic_data *drv = &conn_vic_dev[index];
    if (!drv)
        return -EINVAL;

    mutex_lock(&drv->lock);

    switch (cmd) {
        case CMD_get_info:
            ret = camera_get_info(drv, (struct camera_info *)arg);
            break;

        case CMD_power_on:
            ret = camera_power_on(drv);
            break;

        case CMD_power_off:
            ret = camera_power_off(drv);
            break;

        case CMD_stream_on:
            ret = camera_stream_on(drv);
            break;

        case CMD_stream_off:
            ret = camera_stream_off(drv);
            break;

        case CMD_get_frame:
        case CMD_wait_frame:{
            if (!map_mem) {
                printk(KERN_ERR "CONN_VIC[%d]: please mmap first. (CPU%d)\n", index, LINUX_CPU_ID);
                ret = -EINVAL;
                break;
            }

            void **mem_p = (void *)arg;

            void *mem = camera_wait_frame(drv);
            if (!mem) {
                ret = -EBUSY;
                break;
            }

            *mem_p = map_mem + (unsigned long)(mem - drv->mem);

            break;
        }

        case CMD_put_frame: {
            if (!map_mem) {
                printk(KERN_ERR "CONN_VIC[%d]: please mmap first. (CPU%d)\n", index, LINUX_CPU_ID);
                ret = -EINVAL;
                break;
            }

            void *mem = (void *)arg;

            if (check_frame_mem(index, mem, map_mem)) {
                ret = -EINVAL;
                break;
            }

            ret = camera_put_frame(drv, drv->mem + (unsigned long)(mem - map_mem));

            break;
        }

        case CMD_get_frame_count: {
            int *count_p = (void *)arg;
            if (!count_p) {
                ret = -EINVAL;
                break;
            }

            int count = camera_get_available_frame_count(drv);
            if (count < 0) {
                ret = -1;
                break;
            }

            *count_p = count;

            break;
        }

        case CMD_skip_frames:
            ret = camera_skip_frames(drv, (unsigned int)arg);
            break;

        default:
            printk(KERN_ERR "CONN_VIC[%d]: %x not support this cmd. (CPU%d)\n", cmd, index, LINUX_CPU_ID);
            ret = -EINVAL;
    }

    mutex_unlock(&drv->lock);

    return 0;
}

static int conn_vic_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long len;
    unsigned long start;
    unsigned long off;
    struct m_private_data *data = file->private_data;
    int index = data->index;
    struct conn_vic_data *drv = &conn_vic_dev[index];

    off = vma->vm_pgoff << PAGE_SHIFT;

    if (off) {
        printk(KERN_ERR "CONN_VIC[%d]: camera offset must be 0. (CPU%d)\n", index, LINUX_CPU_ID);
        return -EINVAL;
    }

    len = drv->frm_size * drv->mem_cnt;

    if ((vma->vm_end - vma->vm_start) != len) {
        printk(KERN_ERR "CONN_VIC[%d]: camera size must be total size. (CPU%d)\n", index, LINUX_CPU_ID);
        return -EINVAL;
    }

    /* frame buffer memory */
    start = virt_to_phys(drv->mem);
    off += start;

    vma->vm_pgoff = off >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;

    /*
     * 0: cachable,write through (cache + cacheline对齐写穿)
     * 1: uncachable,write Acceleration (uncache + 硬件写加速)
     * 2: uncachable
     * 3: cachable
     */
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_NO_WA;

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
        return -EAGAIN;
    }

    data->map_mem = (void *)vma->vm_start;

    return 0;
}

static int conn_vic_open(struct inode *inode, struct file *filp)
{
    struct m_private_data *data = kmalloc(sizeof(*data), GFP_KERNEL);
    struct conn_vic_data *drv = container_of(filp->private_data,
            struct conn_vic_data, vic_mdev);

    data->index = drv->index;
    filp->private_data = data;

    return 0;
}

static int conn_vic_release(struct inode *inode, struct file *filp)
{
    struct m_private_data *data = filp->private_data;
    struct conn_vic_data *drv = &conn_vic_dev[data->index];
    int i;

    for (i = 0; i < drv->mem_cnt; i++) {
        if (frm_mem_table[i])
            camera_put_frame(drv, (void *)frm_mem_table[i]);
    }

    kfree(data);

    return 0;
}

static struct file_operations conn_vic_fops= {
    .owner          = THIS_MODULE,
    .open           = conn_vic_open,
    .release        = conn_vic_release,
    .mmap           = conn_vic_mmap,
    .unlocked_ioctl = conn_vic_ioctl,
};

static int conn_vic_request(struct conn_vic_data *drv)
{
    struct conn_vic_notify notify;
    int ret;

    int len = sizeof(notify);
    int i = 3;

    struct conn_node *conn = conn_request(drv->device_name, sizeof(struct conn_vic_notify));
    if (!conn)
        return -1;

    notify.cmd = CMD_conn_vic_conn_detect;
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

static int conn_vic_drv_init(int index)
{
    int ret;

    assert(index < 2);

    struct conn_vic_data *drv = &conn_vic_dev[index];

    ret = conn_vic_request(drv);
    if (ret < 0)
        return 0;
    printk("CONN_VIC: %s request conn succeed. (CPU%d)\n", drv->device_name, LINUX_CPU_ID);

    drv->vic_mdev.minor = MISC_DYNAMIC_MINOR;
    drv->vic_mdev.name  = drv->device_name;
    drv->vic_mdev.fops  = &conn_vic_fops;

    ret = misc_register(&drv->vic_mdev);
    if (ret < 0)
        panic("CONN_VIC: conn_vic register %s err.(CPU%d)\n", drv->device_name, LINUX_CPU_ID);

    mutex_init(&drv->lock);

    drv->is_enable = 1;

    return 0;
}

static void conn_vic_drv_exit(int index)
{
    assert(index < 2);

    struct conn_vic_data *drv = &conn_vic_dev[index];

    if (!drv->is_enable)
        return;

    conn_release(drv->conn);

    misc_deregister(&drv->vic_mdev);
}

static int conn_vic_init(void)
{
    int i;

    conn_wait_conn_inited();

    for (i = 0; i < CONN_VIC_DEV_MAX_COUNT; i++)
        conn_vic_drv_init(i);

    return 0;
}

static void conn_vic_exit(void)
{
    int i;

    for (i = 0; i < CONN_VIC_DEV_MAX_COUNT; i++)
        conn_vic_drv_exit(i);
}

module_init(conn_vic_init);
module_exit(conn_vic_exit);

MODULE_LICENSE("GPL");
