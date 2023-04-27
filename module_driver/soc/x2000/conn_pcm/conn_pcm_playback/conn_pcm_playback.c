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

#define LINUX_CPU_ID    0
#define NOTIFY_WRITE_TIMEOUT_MS 10
#define NOTIFY_READ_TIMEOUT_MS  1000
#define CONN_PCM_PLAYBACK_DEV_MAX_COUNT 1

//  copy from rtos pcm.h
typedef enum {
    pcm_fmt_S8,
    pcm_fmt_U8,
    pcm_fmt_S16LE,
    pcm_fmt_S16BE,
    pcm_fmt_U16LE,
    pcm_fmt_U16BE,
    pcm_fmt_S24LE,
    pcm_fmt_S24BE,
    pcm_fmt_U24LE,
    pcm_fmt_U24BE,
    pcm_fmt_S32LE,
    pcm_fmt_S32BE,
    pcm_fmt_U32LE,
    pcm_fmt_U32BE,

    pcm_fmt_nums,
} pcm_data_fmt;

//  copy from rtos pcm.h
typedef enum {
    pcm_rate_5512,
    pcm_rate_8000,
    pcm_rate_11025,
    pcm_rate_12000,
    pcm_rate_16000,
    pcm_rate_22050,
    pcm_rate_24000,
    pcm_rate_32000,
    pcm_rate_44100,
    pcm_rate_48000,
    pcm_rate_64000,
    pcm_rate_88200,
    pcm_rate_96000,
    pcm_rate_176400,
    pcm_rate_192000,

    pmc_rate_nums,
} pcm_sample_rate;

struct pcm_data_info {
    void *data;
    int size;
};

struct pcm_param_info {
    int channel;
    pcm_data_fmt data_fmt;
    pcm_sample_rate sample_rate;
    int volume;
    int frame_byte;
    int per_frame_size;
    int count;
};

struct pcm_info {
    struct pcm_data_info pcm_data;
    struct pcm_param_info pcm_param;
};

struct conn_pcm_playback_data {
    int index;
    int is_enable;

    struct pcm_info *conn_pcm_info;

    char *device_name;
    struct mutex lock;
    struct miscdevice conn_pcm_playback_mdev;

    struct conn_node *conn;
};

static struct conn_pcm_playback_data conn_pcm_playback_dev[CONN_PCM_PLAYBACK_DEV_MAX_COUNT] = {
    {
        .index                  = 0,
        .device_name            = "conn_pcm_icodec_playback",
    },
};

#define CMD_alloc_mem                   _IOWR('A', 100, int)
#define CMD_free_mem                    _IO('A', 101)
#define CMD_enable                          _IO('A', 102)
#define CMD_disable                          _IO('A', 103)
#define CMD_write_frame                 _IOWR('A', 104, struct pcm_data_info)
#define CMD_set_param                        _IOWR('A', 105, struct pcm_param_info)

enum conn_cmd {
    CMD_conn_pcm_playback_alloc_mem,
    CMD_conn_pcm_playback_free_mem,
    CMD_conn_pcm_playback_enable,
    CMD_conn_pcm_playback_disable,
    CMD_conn_pcm_playback_write_frame,
    CMD_conn_pcm_playback_set_param,
    CMD_conn_pcm_playback_detect,
};

static const char *conn_cmd_str[] = {
    [CMD_conn_pcm_playback_alloc_mem] = "conn_pcm_playback_alloc_mem",
    [CMD_conn_pcm_playback_free_mem] = "conn_pcm_playback_free_mem",
    [CMD_conn_pcm_playback_enable] = "conn_pcm_playback_enable",
    [CMD_conn_pcm_playback_disable] = "conn_pcm_playback_disable",
    [CMD_conn_pcm_playback_write_frame] = "conn_pcm_playback_write_frame",
    [CMD_conn_pcm_playback_set_param] = "CMD_conn_pcm_playback_set_param",
    [CMD_conn_pcm_playback_detect] = "conn_pcm_playback_detect",
};

struct m_private_data {
    unsigned int index;
    void *map_mem;
};

struct conn_pcm_playback_notify {
    enum conn_cmd cmd;
    void *data;
};

static int conn_pcm_playback_send_cmd(struct conn_pcm_playback_data *drv, enum conn_cmd cmd, void *data)
{
    int ret;
    void **p = data;
    struct conn_pcm_playback_notify notify;
    int len = sizeof(notify);

    struct conn_node *conn = drv->conn;

    notify.cmd = cmd;
    notify.data = data;

    ret = conn_write(conn, &notify, len, NOTIFY_WRITE_TIMEOUT_MS);
    if (ret != len) {
        printk(KERN_ERR "CONN_PCM_PLAYBACK[%d]: %s ,failed to write conn. ret = %d. (CPU%d)\n", drv->index, conn_cmd_str[cmd], ret, LINUX_CPU_ID);
        return -1;
    }

    ret = conn_read(conn, &notify, len, NOTIFY_READ_TIMEOUT_MS);
    if (ret != len) {
        printk(KERN_ERR "CONN_PCM_PLAYBACK[%d]: %s ,failed to read conn. ret = %d. (CPU%d)\n", drv->index, conn_cmd_str[cmd], ret, LINUX_CPU_ID);
        return -1;
    }

    if (notify.data && data)
        *p = notify.data;

    return 0;
}

static int conn_pcm_playback_alloc_mem(struct conn_pcm_playback_data *drv, int size)
{
    drv->conn_pcm_info->pcm_data.data = kmalloc(size, GFP_KERNEL);
    if (drv->conn_pcm_info->pcm_data.data == NULL) {
        printk(KERN_ERR "CONN_PCM_PLAYBACK[%d]: kmalloc mem space fail\n", drv->index);
        return -1;
    }

    return 0;
}

static int conn_pcm_playback_free_mem(struct conn_pcm_playback_data *drv)
{
    if (drv->conn_pcm_info->pcm_data.data != NULL) {
        kfree(drv->conn_pcm_info->pcm_data.data);
        drv->conn_pcm_info->pcm_data.data = NULL;
        printk("CONN_PCM_PLAYBACK[%d]: kfree mem space success\n", drv->index);
    } else {
        printk(KERN_ERR "CONN_PCM_PLAYBACK[%d]: kfree mem space fail\n", drv->index);
        return -1;
    }

    return 0;
}

static int conn_pcm_playback_enable(struct conn_pcm_playback_data *drv)
{
    conn_pcm_playback_send_cmd(drv, CMD_conn_pcm_playback_enable, NULL);

    return 0;
}

static int conn_pcm_playback_disable(struct conn_pcm_playback_data *drv)
{
    conn_pcm_playback_send_cmd(drv, CMD_conn_pcm_playback_disable, NULL);

    return 0;
}

static int conn_pcm_playback_set_param(struct conn_pcm_playback_data *drv, struct pcm_param_info *param)
{
    drv->conn_pcm_info->pcm_param.channel = param->channel;
    drv->conn_pcm_info->pcm_param.count = param->count;
    drv->conn_pcm_info->pcm_param.data_fmt = param->data_fmt;
    drv->conn_pcm_info->pcm_param.frame_byte = param->frame_byte;
    drv->conn_pcm_info->pcm_param.per_frame_size = param->per_frame_size;
    drv->conn_pcm_info->pcm_param.sample_rate = param->sample_rate;
    drv->conn_pcm_info->pcm_param.volume = param->volume;

    conn_pcm_playback_send_cmd(drv, CMD_conn_pcm_playback_set_param, &drv->conn_pcm_info->pcm_param);

    return 0;
}

static int conn_pcm_playback_write_frame(struct conn_pcm_playback_data *drv, struct pcm_data_info *data_info)
{
    drv->conn_pcm_info->pcm_data.size = data_info->size;
    // 应用层拷贝过一次到mmap地址
    // memcpy(drv->conn_pcm_info->pcm_data.data, data_info->data, data_info->size);

    conn_pcm_playback_send_cmd(drv, CMD_conn_pcm_playback_write_frame, &drv->conn_pcm_info->pcm_data);

    return 0;
}

static int conn_pcm_playback_open(struct inode *inode, struct file *filp)
{
    struct m_private_data *data = kmalloc(sizeof(*data), GFP_KERNEL);
    struct conn_pcm_playback_data *drv = container_of(filp->private_data,
            struct conn_pcm_playback_data, conn_pcm_playback_mdev);

    drv->conn_pcm_info = kmalloc(sizeof(struct pcm_info), GFP_KERNEL);
    data->index = drv->index;
    filp->private_data = data;

    return 0;
}

static int conn_pcm_playback_release(struct inode *inode, struct file *filp)
{
    struct m_private_data *data = filp->private_data;
    struct conn_pcm_playback_data * drv = &conn_pcm_playback_dev[data->index];
    if (!drv)
        return -EINVAL;

    if (drv->conn_pcm_info->pcm_data.data != NULL) {
        kfree(drv->conn_pcm_info->pcm_data.data);
        drv->conn_pcm_info->pcm_data.data = NULL;
        drv->conn_pcm_info->pcm_data.size = 0;
        printk(KERN_ERR "CONN_PCM_PLAYBACK[%d]: kfree mem space success\n", data->index);
    }

    kfree(drv->conn_pcm_info);
    kfree(data);

    return 0;
}

static int conn_pcm_playback_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long start;
    unsigned long off;
    struct m_private_data *data = file->private_data;
    int index = data->index;
    struct conn_pcm_playback_data *drv = &conn_pcm_playback_dev[index];

    off = vma->vm_pgoff << PAGE_SHIFT;

    if (off) {
        printk(KERN_ERR "CONN_PCM_PLAYBACK[%d]: camera offset must be 0. (CPU%d)\n", index, LINUX_CPU_ID);
        return -EINVAL;
    }

    /* frame buffer memory */
    start = virt_to_phys(drv->conn_pcm_info->pcm_data.data);
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

static long conn_pcm_playback_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct m_private_data *data = filp->private_data;
    unsigned int index = data->index;
    int ret = 0;

    struct conn_pcm_playback_data *drv = &conn_pcm_playback_dev[index];
    if (!drv)
        return -EINVAL;

    mutex_lock(&drv->lock);

    switch (cmd) {
        case CMD_alloc_mem :
            conn_pcm_playback_alloc_mem(drv, (int)arg);
            break;

        case CMD_free_mem :
            conn_pcm_playback_free_mem(drv);
            break;

        case CMD_enable :
            conn_pcm_playback_enable(drv);
            break;

        case CMD_disable :
            conn_pcm_playback_disable(drv);
            break;

        case CMD_write_frame :
            conn_pcm_playback_write_frame(drv, (struct pcm_data_info *)arg);
            break;

        case CMD_set_param :
            conn_pcm_playback_set_param(drv, (struct pcm_param_info *)arg);
            break;

        default :
            printk(KERN_ERR "CONN_PCM_PLAYBACK[%d]: %x not support this cmd.\n", cmd, index);
            ret = -EINVAL;
            break;
    }
    mutex_unlock(&drv->lock);

    return ret;
}

static struct file_operations conn_pcm_playback_fops= {
    .owner          = THIS_MODULE,
    .open           = conn_pcm_playback_open,
    .release        = conn_pcm_playback_release,
    .mmap           = conn_pcm_playback_mmap,
    .unlocked_ioctl = conn_pcm_playback_ioctl,
};

static int conn_pcm_playback_request(struct conn_pcm_playback_data *drv)
{
    struct conn_pcm_playback_notify notify;
    int ret;

    int len = sizeof(notify);
    int i = 3;

    struct conn_node *conn = conn_request(drv->device_name, sizeof(struct conn_pcm_playback_notify));
    if (!conn)
        return -1;

    notify.cmd = CMD_conn_pcm_playback_detect;
    notify.data = NULL;

    while (1) {
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

static int conn_pcm_playback_drv_init(int index)
{
    int ret;

    struct conn_pcm_playback_data *drv = &conn_pcm_playback_dev[index];
    ret = conn_pcm_playback_request(drv);
    if (ret < 0)
        return 0;
    printk("CONN_PCM_PLAYBACK: %s request conn succeed. (CPU%d)\n", drv->device_name, LINUX_CPU_ID);

    drv->conn_pcm_playback_mdev.minor = MISC_DYNAMIC_MINOR;
    drv->conn_pcm_playback_mdev.name  = drv->device_name;
    drv->conn_pcm_playback_mdev.fops  = &conn_pcm_playback_fops;

    ret = misc_register(&drv->conn_pcm_playback_mdev);
    if (ret < 0)
        panic("CONN_PCM_PLAYBACK: conn_pcm_playback register %s err.(CPU%d)\n", drv->device_name, LINUX_CPU_ID);

    mutex_init(&drv->lock);

    drv->is_enable = 1;

    return 0;
}

static void conn_pcm_playback_drv_exit(int index)
{
    struct conn_pcm_playback_data *drv = &conn_pcm_playback_dev[index];

    if (!drv->is_enable)
        return;

    drv->is_enable = 0;

    misc_deregister(&drv->conn_pcm_playback_mdev);
    conn_release(drv->conn);
}

static int conn_pcm_playback_init(void)
{
    int i;

    conn_wait_conn_inited();

    for (i = 0; i < CONN_PCM_PLAYBACK_DEV_MAX_COUNT; i++)
        conn_pcm_playback_drv_init(i);

    return 0;
}

static void conn_pcm_playback_exit(void)
{
    int i;

    for (i = 0; i < CONN_PCM_PLAYBACK_DEV_MAX_COUNT; i++)
        conn_pcm_playback_drv_exit(i);
}

module_init(conn_pcm_playback_init);
module_exit(conn_pcm_playback_exit);

MODULE_LICENSE("GPL");