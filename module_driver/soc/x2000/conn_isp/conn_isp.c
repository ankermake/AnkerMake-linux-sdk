
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
#include <linux/kthread.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <assert.h>

#include <common.h>
#include <bit_field.h>

#include <utils/clock.h>

#include <conn.h>
#include <camera.h>
#include <camera_isp.h>
#include <camera_sensor.h>

#define LINUX_CPU_ID    0

#define NOTIFY_WRITE_TIMEOUT_MS 10
#define NOTIFY_READ_TIMEOUT_MS  300

#define MSCALER_MAX_CH 3

#define CONN_ISP_MAX_FRAME_NUMS 32
#define CONN_ISP_DEV_MAX_COUNT 2

#define notify_status_send 1
#define notify_status_recv 2

enum conn_cmd {
    CMD_conn_isp_get_info,
    CMD_conn_isp_power_on,
    CMD_conn_isp_power_off,
    CMD_conn_isp_stream_on,
    CMD_conn_isp_stream_off,
    CMD_conn_isp_set_format,
    CMD_conn_isp_get_format,
    CMD_conn_isp_request_buffer,
    CMD_conn_isp_free_buffer,
    CMD_conn_isp_get_max_scaler_size,
    CMD_conn_isp_get_line_align_size,

    CMD_conn_isp_wait_frame,
    CMD_conn_isp_get_frame,
    CMD_conn_isp_put_frame,
    CMD_conn_isp_get_frame_count,
    CMD_conn_isp_skip_frames,

    CMD_conn_isp_get_sensor_info,
    CMD_conn_isp_get_sensor_reg,
    CMD_conn_isp_set_sensor_reg,

    CMD_conn_isp_conn_detect,

    CMD_conn_isp_dqbuf,
    CMD_conn_isp_dqbuf_wait,
    CMD_conn_isp_qbuf,
};

struct conn_isp_notify {
    enum conn_cmd cmd;
    int id;
    int mscaler_ch;
    void *data;
};

struct mscaler_channel {
    int index;
    int channel;

    unsigned int mem_cnt;
    unsigned int frm_size;

    struct mutex frms_lock;
    void *frm_mem[CONN_ISP_MAX_FRAME_NUMS];
    void *mem;

    struct miscdevice mdev;
    struct conn_node *conn;
};

struct notify_data {
    struct conn_isp_notify notify;
    struct list_head link;
    wait_queue_head_t queue;
    int status;
};

struct conn_isp_data {
    int index;
    int is_enable;

    char *device_name;
    void *data;  /* tmp data */
    struct mutex list_lock;
    struct mscaler_channel mscaler_ch[MSCALER_MAX_CH];

    wait_queue_head_t read_queue;
    wait_queue_head_t stop_queue;

    struct task_struct *thread;

    int is_stop_read;
    atomic_t start_read;
    atomic_t id;

    struct list_head wait_list;
    struct conn_node *conn;
};

static struct conn_isp_data conn_isp_dev[2] = {
    {
        .index                  = 0,
        .device_name            = "conn_isp0",
    },

    {
        .index                  = 1,
        .device_name            = "conn_isp1",
    },
};


static void wake_up_user(struct conn_isp_data *drv, struct list_head *list, struct conn_isp_notify *notify)
{
    if (list_empty(list))
        return;

    int is_found = 0;
    struct notify_data *data;

    mutex_lock(&drv->list_lock);

    list_for_each_entry(data, list, link) {
        if (data->notify.cmd == notify->cmd && data->notify.id == notify->id) {
            is_found = 1;
            break;
        }
    }

    if (!is_found) {
        printk(KERN_ERR "conn_isp: failed to get notify data\n");
        mutex_unlock(&drv->list_lock);
        return;
    }

    data->notify = *notify;
    data->status = notify_status_recv;

    wake_up(&data->queue);
    mutex_unlock(&drv->list_lock);

    return;
}

static void add_user(struct conn_isp_data *drv,  struct list_head *list, struct notify_data *data)
{
    mutex_lock(&drv->list_lock);

    list_add_tail(&data->link, list);

    mutex_unlock(&drv->list_lock);
}

static void remove_user(struct conn_isp_data *drv, struct notify_data *data)
{
    mutex_lock(&drv->list_lock);

    list_del(&data->link);

    mutex_unlock(&drv->list_lock);
}


#define ISP_CMD_MAGIC                   'C'
#define ISP_CMD_MAGCIC_NR               120

#define CMD_get_info                    _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 0, struct camera_info)
#define CMD_power_on                    _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 1)
#define CMD_power_off                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 2)
#define CMD_stream_on                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 3)
#define CMD_stream_off                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 4)
#define CMD_set_format                  _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 5, struct frame_image_format)
#define CMD_get_format                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 6)
#define CMD_request_buffer              _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 7, struct frame_image_format)
#define CMD_free_buffer                 _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 8)
#define CMD_get_max_scaler_size         _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 9, unsigned int)
#define CMD_get_line_align_size         _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 10, unsigned int)

#define CMD_wait_frame                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 20)
#define CMD_get_frame                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 21)
#define CMD_put_frame                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 22)
#define CMD_dqbuf                       _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 23)
#define CMD_dqbuf_wait                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 24)
#define CMD_qbuf                        _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 25)
#define CMD_skip_frames                 _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 26)
#define CMD_get_frame_count             _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 27)

#define CMD_get_sensor_info             _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 30, struct camera_info)
#define CMD_get_sensor_reg              _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 31, struct sensor_dbg_register)
#define CMD_set_sensor_reg              _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 32, struct sensor_dbg_register)


static const char *conn_cmd_str[] = {
    [CMD_conn_isp_get_info] = "conn_isp_get_info",
    [CMD_conn_isp_power_on] = "conn_isp_power_on",
    [CMD_conn_isp_power_off] = "conn_isp_power_off",
    [CMD_conn_isp_stream_on] = "conn_isp_stream_on",
    [CMD_conn_isp_stream_off] = "conn_isp_stream_off",
    [CMD_conn_isp_set_format] = "conn_isp_set_format",
    [CMD_conn_isp_get_format] = "conn_isp_get_format",
    [CMD_conn_isp_request_buffer] = "conn_isp_request_buffer",
    [CMD_conn_isp_free_buffer] = "conn_isp_free_buffer",
    [CMD_conn_isp_get_max_scaler_size] = "conn_isp_get_max_scaler_size",
    [CMD_conn_isp_get_line_align_size] = "conn_isp_get_line_align_size",
    [CMD_conn_isp_wait_frame] = "conn_isp_wait_frame",
    [CMD_conn_isp_get_frame] = "conn_isp_get_frame",
    [CMD_conn_isp_put_frame] = "conn_isp_put_frame",
    [CMD_conn_isp_get_frame_count] = "conn_isp_get_frame_count",
    [CMD_conn_isp_skip_frames] = "conn_isp_skip_frames",
    [CMD_conn_isp_get_sensor_info] = "conn_isp_get_sensor_info",
    [CMD_conn_isp_get_sensor_reg] = "conn_isp_get_sensor_reg",
    [CMD_conn_isp_set_sensor_reg] = "conn_isp_set_sensor_reg",
    [CMD_conn_isp_conn_detect] = "conn_isp_conn_detect",
};

struct m_private_data {
    struct mscaler_channel *data;
    unsigned int index;
    unsigned int channel;
    void *map_mem;
};



static int check_frame_mem(struct mscaler_channel *data, void *mem, void *base)
{
    unsigned int size = mem - base;

    if (mem < base)
        return -1;

    if (size % data->frm_size)
        return -1;

    if (size / data->frm_size >= data->mem_cnt)
        return -1;

    return 0;
}

int conn_isp_notify_process(void *data)
{
    struct conn_isp_data *drv = data;

    int ret;
    struct conn_isp_notify notify;
    int len = sizeof(notify);

    while (1) {
        wait_event(drv->read_queue, atomic_read(&drv->start_read));
        atomic_sub(1, &drv->start_read);

        if (drv->is_stop_read)
            break;

        ret = conn_read(drv->conn, &notify, len, NOTIFY_READ_TIMEOUT_MS);
        if (ret != len)
            continue;

        wake_up_user(drv, &drv->wait_list, &notify);
    }

    drv->is_stop_read = 0;

    wake_up_all(&drv->stop_queue);

    return 0;
}

static int conn_isp_wait_cmd(struct conn_isp_data *drv, struct notify_data *n_data, void *data)
{
    void **p = data;

    atomic_add(1, &drv->start_read);
    wake_up(&drv->read_queue);

    wait_event_timeout(n_data->queue, n_data->status == notify_status_recv, NOTIFY_READ_TIMEOUT_MS);

    remove_user(drv, n_data);

    if (n_data->status != notify_status_recv) {
        printk(KERN_ERR "conn isp wait %s timeout\n", conn_cmd_str[n_data->notify.cmd]);
        return -1;
    }

    if (n_data->notify.data && data)
        *p = n_data->notify.data;

    return 0;
}

static int conn_isp_send_cmd(struct mscaler_channel *ch_data, enum conn_cmd cmd, void *data)
{
    int ret;

    struct notify_data n_data;
    int len = sizeof(n_data.notify);

    struct conn_node *conn = ch_data->conn;
    struct conn_isp_data *drv = &conn_isp_dev[ch_data->index];

    n_data.notify.id = atomic_add_return(1, &drv->id);
    n_data.notify.cmd = cmd;
    n_data.notify.mscaler_ch = ch_data->channel;
    n_data.notify.data = data;
    n_data.status = notify_status_send;
    init_waitqueue_head(&n_data.queue);

    add_user(drv, &drv->wait_list, &n_data);

    ret = conn_write(conn, &n_data.notify, len, NOTIFY_WRITE_TIMEOUT_MS);
    if (ret != len) {
        remove_user(drv, &n_data);
        printk(KERN_ERR "CONN_ISP[%d]: %s ,failed to write conn. ret = %d. (CPU%d)\n", ch_data->index, conn_cmd_str[cmd], ret, LINUX_CPU_ID);
        return -1;
    }

    return conn_isp_wait_cmd(drv, &n_data, data);
}

static int isp_get_info(struct mscaler_channel *ch_data, struct camera_info *arg)
{
    int ret;
    struct camera_info *info;

    void *data = NULL;

    if (!arg)
        return -EINVAL;

    info = arg;

    ret = conn_isp_send_cmd(ch_data, CMD_conn_isp_get_info, (void *)&data);
    if (ret < 0)
        return ret;

    *info = *(struct camera_info *)data;

    ch_data->frm_size = info->frame_align_size;
    ch_data->mem_cnt = info->frame_nums;
    ch_data->mem = phys_to_virt(info->phys_mem);

    ret = 0;

    return ret;
}

static int isp_get_sensor_info(struct mscaler_channel *ch_data, struct camera_info *arg)
{
    int ret;
    struct camera_info *info;

    void *data = NULL;

    if (!arg)
        return -EINVAL;

    info = arg;

    ret = conn_isp_send_cmd(ch_data, CMD_conn_isp_get_sensor_info, (void *)&data);
    if (ret < 0)
        return ret;

    *info = *(struct camera_info *)data;

    ret = 0;

    return ret;
}

static int isp_get_sensor_reg(struct mscaler_channel *ch_data, struct sensor_dbg_register *reg)
{
    return conn_isp_send_cmd(ch_data, CMD_conn_isp_get_sensor_reg, (void *)reg);;
}

static int isp_set_sensor_reg(struct mscaler_channel *ch_data, struct sensor_dbg_register *reg)
{
    return conn_isp_send_cmd(ch_data, CMD_conn_isp_set_sensor_reg, (void *)reg);
}

static int isp_power_on(struct mscaler_channel *data)
{
    return conn_isp_send_cmd(data, CMD_conn_isp_power_on, NULL);
}

static int isp_power_off(struct mscaler_channel *data)
{
    return conn_isp_send_cmd(data, CMD_conn_isp_power_off, NULL);
}

static int isp_stream_on(struct mscaler_channel *data)
{
    return conn_isp_send_cmd(data, CMD_conn_isp_stream_on, NULL);
}

static int isp_stream_off(struct mscaler_channel *data)
{
    return conn_isp_send_cmd(data, CMD_conn_isp_stream_off, NULL);
}

static inline void conn_isp_add_frm_mem_table(struct mscaler_channel *data, void *mem)
{
    int i;

    if (!mem)
        return;

    mutex_lock(&data->frms_lock);

    for (i = 0; i < CONN_ISP_MAX_FRAME_NUMS; i++) {
        if (!data->frm_mem[i]) {
            data->frm_mem[i] = mem;
            break;
        }
    }

    mutex_unlock(&data->frms_lock);
}

static inline void conn_isp_remove_frm_mem_table(struct mscaler_channel *data, void *mem)
{
    int i;

    mutex_lock(&data->frms_lock);

    for (i = 0; i < CONN_ISP_MAX_FRAME_NUMS; i++) {
        if (data->frm_mem[i] == mem) {
            data->frm_mem[i] = NULL;
            break;
        }
    }
    mutex_unlock(&data->frms_lock);
}

static void *isp_get_frame(struct mscaler_channel *data, unsigned int cmd)
{
    void *mem = NULL;
    enum conn_cmd conn_cmd;
    if (cmd == CMD_get_frame)
        conn_cmd = CMD_conn_isp_get_frame;
    else
        conn_cmd = CMD_conn_isp_wait_frame;


    int ret = conn_isp_send_cmd(data, conn_cmd, (void *)&mem);
    if (ret < 0)
        return NULL;

    conn_isp_add_frm_mem_table(data, mem);

    return mem;
}
/**
 * dq_buf 相关操作，若出现timeout的情况，kfree 之后小系统可能会操作对应的内存，是会有问题的
 *
 **/
static int isp_dqbuf(struct mscaler_channel *data, struct frame_info *frame)
{
    int ret;
    struct frame_info dq_data = {0};

    ret = conn_isp_send_cmd(data, CMD_conn_isp_dqbuf, (void *)&dq_data);
    if (!dq_data.vaddr)
        return -1;

    *frame = dq_data;

    return ret;
}

static int isp_dqbuf_wait(struct mscaler_channel *data, struct frame_info *frame)
{
    int ret;
    struct frame_info dq_data = {0};

    ret = conn_isp_send_cmd(data, CMD_conn_isp_dqbuf_wait, (void *)&dq_data);
    if (!dq_data.vaddr)
        return -1;

    *frame = dq_data;

    return ret;
}

static int isp_qbuf(struct mscaler_channel *data, struct frame_info *frame)
{
    int ret;
    struct frame_info q_data = *frame;

    ret = conn_isp_send_cmd(data, CMD_conn_isp_qbuf, (void *)&q_data);

    return ret;
}

static int isp_put_frame(struct mscaler_channel *data, void *mem)
{
    int ret = conn_isp_send_cmd(data, CMD_conn_isp_put_frame, mem);
    if (ret < 0)
        return ret;

    conn_isp_remove_frm_mem_table(data, mem);

    return 0;
}

static int isp_get_available_frame_count(struct mscaler_channel *data)
{
    int ret;
    int p;

    ret = conn_isp_send_cmd(data, CMD_conn_isp_get_frame_count, (void *)&p);
    if (ret < 0)
        return ret;

    ret = *((int *)p);

    return ret;
}

static int isp_skip_frames(struct mscaler_channel *data, unsigned int frames)
{
    return conn_isp_send_cmd(data, CMD_conn_isp_skip_frames, (void *)frames);
}

static int isp_set_format(struct mscaler_channel *data, struct frame_image_format *fmt)
{
    return conn_isp_send_cmd(data, CMD_conn_isp_set_format, (void *)fmt);
}

static int isp_get_format(struct mscaler_channel *data, unsigned int args)
{
    return conn_isp_send_cmd(data, CMD_conn_isp_get_format, (void *)args);
}

static int isp_request_buffer(struct mscaler_channel *data, struct frame_image_format *fmt)
{
    return conn_isp_send_cmd(data, CMD_conn_isp_request_buffer, (void *)fmt);
}

static int isp_free_buffer(struct mscaler_channel *data)
{
    return conn_isp_send_cmd(data, CMD_conn_isp_free_buffer, NULL);
}

static int isp_get_max_scaler_size(struct mscaler_channel *data, unsigned int args)
{
    return conn_isp_send_cmd(data, CMD_conn_isp_get_max_scaler_size, (void *)args);
}

static int isp_get_line_align_size(struct mscaler_channel *data, unsigned int args)
{
    return conn_isp_send_cmd(data, CMD_conn_isp_get_line_align_size, (void *)args);
}

static long conn_isp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct m_private_data *pdata = filp->private_data;
    struct mscaler_channel *data = pdata->data;
    unsigned int index = data->index;
    void *map_mem = pdata->map_mem;
    int ret = 0;

    struct conn_isp_data *drv = &conn_isp_dev[index];
    if (!drv)
        return -EINVAL;

    switch (cmd) {
        case CMD_get_info:
            ret = isp_get_info(data, (struct camera_info *)arg);
            break;

        case CMD_power_on:
            ret = isp_power_on(data);
            break;

        case CMD_power_off:
            ret = isp_power_off(data);
            break;

        case CMD_stream_on:
            ret = isp_stream_on(data);
            break;

        case CMD_stream_off:
            ret = isp_stream_off(data);
            break;

        case CMD_get_frame:
        case CMD_wait_frame:{
            if (!map_mem) {
                printk(KERN_ERR "CONN_ISP[%d]: please mmap first. (CPU%d)\n", index, LINUX_CPU_ID);
                ret = -EINVAL;
                break;
            }

            void **mem_p = (void *)arg;

            void *mem = isp_get_frame(data, cmd);
            if (!mem) {
                ret = -EBUSY;
                break;
            }

            *mem_p = map_mem + (unsigned long)(mem - data->mem);

            break;
        }

        case CMD_put_frame: {
            if (!map_mem) {
                printk(KERN_ERR "CONN_ISP[%d]: please mmap first. (CPU%d)\n", index, LINUX_CPU_ID);
                ret = -EINVAL;
                break;
            }

            void *mem = (void *)arg;

            if (check_frame_mem(data, mem, map_mem)) {
                ret = -EINVAL;
                break;
            }

            ret = isp_put_frame(data, data->mem + (unsigned long)(mem - map_mem));

            break;
        }

        case CMD_get_frame_count: {
            int *count_p = (void *)arg;
            if (!count_p) {
                ret = -EINVAL;
                break;
            }

            int count = isp_get_available_frame_count(data);
            if (count < 0) {
                ret = -1;
                break;
            }

            *count_p = count;

            break;
        }

        case CMD_skip_frames:
            ret = isp_skip_frames(data, (unsigned int)arg);
            break;
        case CMD_set_format: {
            struct frame_image_format *fmt = (struct frame_image_format *)drv->data;
            *fmt = *(struct frame_image_format *)arg;
            ret = isp_set_format(data, fmt);
            break;
        }

        case CMD_get_format: {
            ret = isp_get_format(data, (unsigned int)drv->data);
            *(struct frame_image_format *)arg = *(struct frame_image_format *)drv->data;
            break;
        }

        case CMD_request_buffer: {
            struct frame_image_format *fmt = (struct frame_image_format *)drv->data;
            *fmt = *(struct frame_image_format *)arg;
            ret = isp_request_buffer(data, fmt);
            break;
        }
        case CMD_free_buffer:
            ret = isp_free_buffer(data);
            break;
        case CMD_get_max_scaler_size: {
            ret = isp_get_max_scaler_size(data, (unsigned int)drv->data);
            *(int *)arg = *((int *)drv->data);
            *((int *)arg + 1) = *((int *)drv->data + 1);
            break;
        }

        case CMD_get_line_align_size: {
            ret = isp_get_line_align_size(data, (unsigned int)drv->data);
            *(int *)arg = *((int *)drv->data);
            break;
        }

        case CMD_get_sensor_info:
            ret = isp_get_sensor_info(data, (struct camera_info *)arg);
            break;
        case CMD_get_sensor_reg: {
            struct sensor_dbg_register *reg = (struct sensor_dbg_register *)drv->data;
            ret = isp_get_sensor_reg(data, reg);
            *(struct sensor_dbg_register *)arg = *reg;
            break;
        }

        case CMD_set_sensor_reg: {
            struct sensor_dbg_register *reg = (struct sensor_dbg_register *)drv->data;
            *reg = *(struct sensor_dbg_register *)arg;
            ret = isp_set_sensor_reg(data, reg);
            break;
        }

        case CMD_dqbuf: {
            struct frame_info *frame = (struct frame_info *)arg;
            ret = isp_dqbuf(data, frame);
            if (ret < 0)
                break;

            frame->vaddr = map_mem + (unsigned long)(frame->vaddr - data->mem);
            break;
        }
        case CMD_dqbuf_wait: {
            struct frame_info *frame = (struct frame_info *)arg;
            ret = isp_dqbuf_wait(data, frame);
            if (ret < 0)
                break;

            frame->vaddr = map_mem + (unsigned long)(frame->vaddr - data->mem);
            break;
        }
        case CMD_qbuf: {
            struct frame_info *frame = (struct frame_info *)arg;
            *frame = *(struct frame_info *)arg;

            frame->vaddr = data->mem + (unsigned long)(frame->vaddr - map_mem);
            ret = isp_qbuf(data, frame);
            break;
        }

        default:
            printk(KERN_ERR "CONN_ISP[%d]: %x not support this cmd. (CPU%d)\n", index, cmd, LINUX_CPU_ID);
            ret = -EINVAL;
    }


    return ret;
}

static int conn_isp_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long len;
    unsigned long start;
    unsigned long off;

    struct m_private_data *pdata = file->private_data;
    struct mscaler_channel *data = pdata->data;
    int index = data->index;

    off = vma->vm_pgoff << PAGE_SHIFT;

    if (off) {
        printk(KERN_ERR "CONN_ISP[%d]: camera offset must be 0. (CPU%d)\n", index, LINUX_CPU_ID);
        return -EINVAL;
    }

    len = data->frm_size * data->mem_cnt;

    if ((vma->vm_end - vma->vm_start) != len) {
        printk(KERN_ERR "CONN_ISP[%d]: camera size must be total size. (CPU%d)\n", index, LINUX_CPU_ID);
        return -EINVAL;
    }

    /* frame buffer memory */
    start = virt_to_phys(data->mem);
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

    pdata->map_mem = (void *)vma->vm_start;

    return 0;
}

static int conn_isp_open(struct inode *inode, struct file *filp)
{
    struct m_private_data *pdata = kmalloc(sizeof(*pdata), GFP_KERNEL);
    struct mscaler_channel *data = container_of(filp->private_data,
            struct mscaler_channel, mdev);

    pdata->data = data;

    filp->private_data = pdata;

    return 0;
}

static int conn_isp_release(struct inode *inode, struct file *filp)
{
    struct m_private_data *pdata = filp->private_data;
    struct mscaler_channel *data = pdata->data;

    int i;

    for (i = 0; i < CONN_ISP_MAX_FRAME_NUMS; i++) {
        if (data->frm_mem[i]) {
            isp_put_frame(data, (void *)data->frm_mem[i]);
        }
    }

    kfree(pdata);

    return 0;
}

static struct file_operations conn_isp_fops= {
    .owner          = THIS_MODULE,
    .open           = conn_isp_open,
    .release        = conn_isp_release,
    .mmap           = conn_isp_mmap,
    .unlocked_ioctl = conn_isp_ioctl,
};

static int conn_isp_request(struct conn_isp_data *drv)
{
    struct conn_isp_notify notify;
    int ret;

    int len = sizeof(notify);
    int i = 3;

    struct conn_node *conn = conn_request(drv->device_name, sizeof(struct conn_isp_notify));
    if (!conn)
        return -1;

    notify.cmd = CMD_conn_isp_conn_detect;
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

static void conn_isp_init_recv(struct conn_isp_data *drv)
{
    atomic_set(&drv->start_read, 0);
    atomic_set(&drv->id, 0);
    mutex_init(&drv->list_lock);

    INIT_LIST_HEAD(&drv->wait_list);

    init_waitqueue_head(&drv->read_queue);
    init_waitqueue_head(&drv->stop_queue);

    drv->thread = kthread_create(conn_isp_notify_process, drv, "conn isp recv notify process");
    wake_up_process(drv->thread);
}

static int conn_isp_drv_init(int index)
{
    int ret;

    assert(index < 2);

    struct conn_isp_data *drv = &conn_isp_dev[index];

    ret = conn_isp_request(drv);
    if (ret < 0)
        return 0;

    printk("CONN_ISP: %s request conn succeed. (CPU%d)\n", drv->device_name, LINUX_CPU_ID);

    int ch;

    for (ch = 0; ch < MSCALER_MAX_CH; ch++) {
        char name[16];
        sprintf(name, "%s-ch%d", drv->device_name, ch);
        drv->mscaler_ch[ch].channel = ch;
        drv->mscaler_ch[ch].index = index;
        drv->mscaler_ch[ch].conn = drv->conn;

        struct miscdevice *mdev = &drv->mscaler_ch[ch].mdev;
        mdev->minor = MISC_DYNAMIC_MINOR;
        mdev->name  = name;
        mdev->fops  = &conn_isp_fops;

        mutex_init(&drv->mscaler_ch[ch].frms_lock);
        memset(drv->mscaler_ch[ch].frm_mem, 0, sizeof(drv->mscaler_ch[ch].frm_mem));

        ret = misc_register(mdev);
        if (ret < 0)
            panic("CONN_ISP: conn_isp register %s err.(CPU%d)\n", name, LINUX_CPU_ID);
    }

    drv->data = kmalloc(sizeof(struct frame_image_format *), GFP_KERNEL);
    if (!drv->data)
        panic("CONN_ISP: conn_isp malloc buf %s err.(CPU%d)\n", drv->device_name, LINUX_CPU_ID);

    conn_isp_init_recv(drv);

    drv->is_enable = 1;

    return 0;
}

static void conn_isp_drv_exit(int index)
{
    assert(index < 2);

    struct conn_isp_data *drv = &conn_isp_dev[index];

    if (!drv->is_enable)
        return;

    drv->is_stop_read = 1;
    atomic_add(1, &drv->start_read);
    wake_up_all(&drv->read_queue);

    wait_event(drv->stop_queue, !drv->is_stop_read);

    kfree(drv->data);

    conn_release(drv->conn);

    int ch;
    for (ch = 0; ch < MSCALER_MAX_CH; ch++)
        misc_deregister(&drv->mscaler_ch[ch].mdev);
}

static int conn_isp_init(void)
{
    int i;

    conn_wait_conn_inited();

    for (i = 0; i < CONN_ISP_DEV_MAX_COUNT; i++)
        conn_isp_drv_init(i);

    return 0;
}

static void conn_isp_exit(void)
{
    int i;

    for (i = 0; i < CONN_ISP_DEV_MAX_COUNT; i++)
        conn_isp_drv_exit(i);
}

module_init(conn_isp_init);
module_exit(conn_isp_exit);

MODULE_LICENSE("GPL");
