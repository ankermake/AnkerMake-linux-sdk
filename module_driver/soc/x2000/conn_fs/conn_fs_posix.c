/*
 * RTOS 和 Linux 共用存储分区，RTOS中文件系统挂载在ramdisk中,没有存储介质的写入操作，所有修改存储介质相关的操作均在Linux端实现
 *
 * 该驱动实现依赖于RTOS端 ramdisk模拟存储介质
 *
 * RTOS端 擦除/写入等修改存储介质相关操作 通过核间通讯发送给Linux端,由Linux执行相关操作
 * RTOS端 读取/查询等获取存储介质相关操作 通过核间通讯发送给Linux端,由Linux执行相关操作，更新到RTOS端指定的内存空间中
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <assert.h>
#include <linux/vmalloc.h>
#include <linux/miscdevice.h>
#include <common.h>
#include <bit_field.h>
#include <linux/wait.h>
#include "drivers/rmem_manager/rmem_manager.h"

#include <conn.h>
#include "conn_fs_posix.h"

//#define CONN_FS_POSIX_CMDS_DEBUG

#define CONN_FS_POSIX_DEVICE_NAME       "conn_fs"
#define LINUX_CPU_ID                    0

struct m_private_data {
    unsigned int service_running;
    struct conn_fs_posix_data *data;
    void *map_mem;
};

struct conn_fs_posix_data {
    wait_queue_head_t wait_cmds;
    wait_queue_head_t wait_return;
    struct mutex lock;

    void *mem;  /* 用户空间和内核空间传参数/返回值， 已经大量数据 */
    uint32_t mem_size;
    uint32_t event_cmds;   /* 应用层等待RTOS 发送posix cmds(ioctl magic number) =0 等待, =1 继续运行 */
    uint32_t event_return; /* 等待应用层posix指令执行完成并返回      =0 等待, =1 继续运行 */
    struct conn_fs_posix_cmds_arguments *args;
    struct conn_fs_posix_cmds_result *res;
};

int conn_fs_is_connected(void);

static struct m_private_data *pri_data = NULL;


#include "conn_fs_posix_open.c"
#include "conn_fs_posix_close.c"
#include "conn_fs_posix_read.c"
#include "conn_fs_posix_write.c"
#include "conn_fs_posix_lseek.c"


int conn_fs_drv_get_service_status(void)
{
    int ret = 0;

    if (pri_data)
        ret = pri_data->service_running;

    return ret;
}

static void conn_fs_drv_update_service_status(struct conn_fs_posix_data *data, int status)
{
    if (pri_data)
        pri_data->service_running = status;
}

static int conn_fs_drv_get_mem_size(struct conn_fs_posix_data *data, unsigned int arg)
{
    uint32_t mem_size = data->mem_size;
    int ret = copy_to_user((void __user *)arg, &mem_size, sizeof(mem_size));
    if (ret)
        printk(KERN_ERR "conn fs drv can not get mem size\n");

    return ret;
}

static int conn_fs_drv_ops_get_event_command(struct conn_fs_posix_data *data, unsigned int arg)
{
    uint32_t cmds = data->event_cmds;

    int ret = copy_to_user((void __user *)arg, &cmds, sizeof(cmds));
    if (ret)
        printk(KERN_ERR "conn fs drv can not get event commands(0x%x)\n", cmds);

    return ret;
}


static int conn_fs_drv_ops_return(struct conn_fs_posix_data *data, unsigned int arg)
{
    /*
     * 将应用层POSIX API执行结构更新到 data->res中
     * data->res变量的地址 在接收到RTOS指令时赋值 conn_fs_posix_rtos_ioctl()
     */
    int ret = copy_from_user(data->res, (void __user *)arg, sizeof(struct conn_fs_posix_cmds_result));
    if (ret)
        printk(KERN_ERR "conn fs drv ops return error\n");

    return ret;
}

static int conn_fs_drv_wakeup_event_cmds(struct conn_fs_posix_data *data)
{
    wake_up_interruptible(&data->wait_cmds);

    return 0;
}

static int conn_fs_drv_set_wait_event_cmds(struct conn_fs_posix_data *data, uint32_t ioctl_cmds, void *args, void *res)
{
    data->event_cmds = ioctl_cmds;
    data->args = args;
    data->res = res;

    return 0;
}

static int conn_fs_drv_clear_wait_event_cmds(struct conn_fs_posix_data *data)
{
    data->event_cmds = 0;

    return 0;
}

static int conn_fs_drv_waitting_event_cmds(struct conn_fs_posix_data *data)
{
    int ret = 0;

    /*
     * conn_fs 是否建立连接，
     * 没有建立连接则等待建立连接
     * 有建立连接，判断是否有需要处理的指令
     */
    ret = conn_fs_is_connected();

    if (!ret)
        return 0;

    /*
     * 没有接收到RTOS conn_fs发送的指令，则等待
     * 否则接收到RTOS conn_fs 的指令， 处理该指令
     */
    ret = data->event_cmds ? 1 : 0;

    return ret;
}

int conn_fs_drv_wakeup_event_return(struct conn_fs_posix_data *data)
{
    wake_up_interruptible(&data->wait_return);

    return 0;
}

int conn_fs_drv_set_wait_event_return(struct conn_fs_posix_data *data, uint32_t ioctl_cmds)
{
    data->event_return = ioctl_cmds;

    return 0;
}

static int conn_fs_drv_clear_wait_event_return(struct conn_fs_posix_data *data)
{
    data->event_return = 0;

    return 0;
}

static int conn_fs_drv_waitting_event_return(struct conn_fs_posix_data *data)
{
    int ret = 0;

    /*
     * conn_fs 是否建立连接，
     * 没有建立连接则等待建立连接
     * 有建立连接，判断是否有需要处理的指令
     */
    ret = conn_fs_is_connected();

    if (!ret)
        return 0;

    /*
     * 没有接收到Linux 应用层posix 执行结果 则等待
     * 否则接收到Linux 应用层posxi 执行结果 则将结果发送给RTOS端(conn_fs)
     */
    ret = data->event_return ? 1 : 0;

    return ret;
}

#define POSIX_MAGIC_OFFSET              150

#define POSIX_conn_fs_ops_get_event         _IOR('T',  POSIX_MAGIC_OFFSET + 0, unsigned int)
#define POSIX_conn_fs_ops_return            _IOW('T',  POSIX_MAGIC_OFFSET + 1, struct conn_fs_posix_cmds_result)

#define POSIX_conn_fs_conn_detect           _IO('T',   POSIX_MAGIC_OFFSET + 2)
#define POSIX_conn_fs_conn_release          _IO('T',   POSIX_MAGIC_OFFSET + 3)
#define POSIX_conn_fs_conn_get_mem_size     _IOR('T',  POSIX_MAGIC_OFFSET + 4, unsigned int)

#define POSIX_conn_fs_posix_open            _IOR('T',  POSIX_MAGIC_OFFSET + 5, struct conn_fs_args_open)
#define POSIX_conn_fs_posix_close           _IOR('T',  POSIX_MAGIC_OFFSET + 6, struct conn_fs_args_close)
#define POSIX_conn_fs_posix_read            _IOR('T',  POSIX_MAGIC_OFFSET + 7, struct conn_fs_args_read)
#define POSIX_conn_fs_posix_read_update_buf _IOW('T',  POSIX_MAGIC_OFFSET + 8, struct conn_fs_args_read)
#define POSIX_conn_fs_posix_write           _IOR('T',  POSIX_MAGIC_OFFSET + 9, struct conn_fs_args_write)
#define POSIX_conn_fs_posix_lseek           _IOR('T',  POSIX_MAGIC_OFFSET + 10, struct conn_fs_args_lseek)

static long conn_fs_drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct m_private_data *pdata = filp->private_data;
    struct conn_fs_posix_data *data = pdata->data;
    int ret = 0;

    mutex_lock(&data->lock);

    switch (cmd) {
    case POSIX_conn_fs_ops_get_event:
        conn_fs_drv_clear_wait_event_cmds(data);
        mutex_unlock(&data->lock);

        ret = wait_event_interruptible(data->wait_cmds, conn_fs_drv_waitting_event_cmds(data));

        mutex_lock(&data->lock);

        if (!ret && arg)
            ret = conn_fs_drv_ops_get_event_command(data, arg);
        else
            ret = -EINVAL;

        break;

    case POSIX_conn_fs_ops_return:
        conn_fs_drv_ops_return(data, arg);
        conn_fs_drv_set_wait_event_return(data, POSIX_conn_fs_ops_return);
        conn_fs_drv_wakeup_event_return(data);
        break;

    case POSIX_conn_fs_conn_detect:
        conn_fs_drv_update_service_status(data, 1);
        break;

    case POSIX_conn_fs_conn_release:
        conn_fs_drv_update_service_status(data, 0);
        break;

    case POSIX_conn_fs_conn_get_mem_size:
        conn_fs_drv_get_mem_size(data, arg);
        break;

    case POSIX_conn_fs_posix_open: {
        ret = conn_fs_drv_service_posix_open(data, arg);
        break;
    }

    case POSIX_conn_fs_posix_close:
        ret = conn_fs_drv_service_posix_close(data, arg);
        break;

    case POSIX_conn_fs_posix_read:
        ret = conn_fs_drv_service_posix_read(data, arg);
        break;

    case POSIX_conn_fs_posix_read_update_buf:
        /* read的长度超过mmap映射缓存长度,将read内容更新到RTOS指定的buf中,继续读 */
        ret = conn_fs_drv_service_posix_read_update_buf(data, arg);
        break;

    case POSIX_conn_fs_posix_write:
        ret = conn_fs_drv_service_posix_write(data, arg);
        break;

    case POSIX_conn_fs_posix_lseek:
        ret = conn_fs_drv_service_posix_lseek(data, arg);
        break;

    default:
        printk(KERN_ERR "CONN_FS_POSIX: %x not support this cmd. (CPU%d)\n", cmd, LINUX_CPU_ID);
        ret = -EINVAL;
    }

    mutex_unlock(&data->lock);

    return ret;
}


static int conn_fs_drv_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct m_private_data *pdata = file->private_data;
    struct conn_fs_posix_data *data = pdata->data;
    unsigned long len;
    unsigned long start;
    unsigned long off;

    off = vma->vm_pgoff << PAGE_SHIFT;
    if (off) {
        printk(KERN_ERR "CONN_FS_POSIX: fs offset must be 0. (CPU%d)\n", LINUX_CPU_ID);
        return -EINVAL;
    }

    len = data->mem_size;

    len = ALIGN(len, PAGE_SIZE);
    if ((vma->vm_end - vma->vm_start + off) > len)
        return -EINVAL;

    start = virt_to_phys(data->mem);
    start &= PAGE_MASK;
    off += start;

    vma->vm_pgoff = off >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;

    /*
     * 0: cachable,write through (cache + cacheline对齐写穿)
     * 1: uncachable,write Acceleration (uncache + 硬件写加速)
     * 2: uncachable
     * 3: cachable
     * 把非cacheline 对齐的内存同步一下
     */
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= 0;

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot))
        return -EAGAIN;

    pdata->map_mem = (void *)vma->vm_start;
    return 0;
}


static struct conn_fs_posix_data *conn_fs_posix_drv_init(void)
{
    struct conn_fs_posix_data *data = kzalloc(sizeof(struct conn_fs_posix_data), GFP_KERNEL);
    if (!data) {
        printk(KERN_ERR "CONN_FS_POSIX: conn_fs_posix_drv init malloc failed. (CPU%d)\n", LINUX_CPU_ID);
        return NULL;
    }

    init_waitqueue_head(&data->wait_cmds);
    init_waitqueue_head(&data->wait_return);

    mutex_init(&data->lock);
    conn_fs_drv_clear_wait_event_cmds(data);
    conn_fs_drv_clear_wait_event_return(data);

    return data;
}

static void conn_fs_posix_drv_deinit(struct conn_fs_posix_data *data)
{
    conn_fs_drv_set_wait_event_cmds(data, POSIX_conn_fs_conn_release, NULL, NULL);
    conn_fs_drv_set_wait_event_return(data, POSIX_conn_fs_conn_release);

    conn_fs_drv_wakeup_event_cmds(data);
    conn_fs_drv_wakeup_event_return(data);

    kfree(data);
}

static int conn_fs_alloc_mem(struct conn_fs_posix_data *data)
{
    data->mem_size = PAGE_SIZE;

    data->mem = rmem_alloc_aligned(data->mem_size, PAGE_SIZE);
    if (data->mem == NULL) {
        printk(KERN_ERR "CONN_FS_POSIX : fs failed to alloc mem: %u\n", data->mem_size);
        return -ENOMEM;
    }

    return 0;
}

static int conn_fs_free_mem(struct conn_fs_posix_data *data)
{
    rmem_free(data->mem, data->mem_size);

    return 0;
}

static int conn_fs_drv_open(struct inode *inode, struct file *filp)
{
    /* 当前该服务只允许打开一次 */
    if (pri_data)
        return -EBUSY;

    struct m_private_data *pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);

    pdata->data = conn_fs_posix_drv_init();

    conn_fs_alloc_mem(pdata->data);

    filp->private_data = pdata;

    pri_data = pdata;

    return 0;
}

static int conn_fs_drv_release(struct inode *inode, struct file *filp)
{
    struct m_private_data *pdata = filp->private_data;

    conn_fs_free_mem(pdata->data);

    conn_fs_posix_drv_deinit(pdata->data);

    kfree(pdata);

    pri_data = NULL;

    return 0;
}


static struct file_operations conn_fs_drv_fops= {
    .owner          = THIS_MODULE,
    .open           = conn_fs_drv_open,
    .release        = conn_fs_drv_release,
    .mmap           = conn_fs_drv_mmap,
    .unlocked_ioctl = conn_fs_drv_ioctl,
};


int conn_fs_posix_register(struct miscdevice *mdev)
{
    int ret;

    mdev->minor = MISC_DYNAMIC_MINOR;
    mdev->name  = CONN_FS_POSIX_DEVICE_NAME;
    mdev->fops  = &conn_fs_drv_fops;

    ret = misc_register(mdev);
    if (ret < 0)
        panic("CONN_FS_POSIX: conn_fs register %s device node err.(CPU%d)\n", CONN_FS_POSIX_DEVICE_NAME, LINUX_CPU_ID);
    return 0;
}


int conn_fs_posix_unregister(struct miscdevice *mdev)
{
     misc_deregister(mdev);

     return 0;
}

/*****************************************************************************/

/*
 * RTOS端定义的操作指令序列
 */
enum conn_fs_posix_cmds {
    conn_fs_posix_cmds_open,
    conn_fs_posix_cmds_close,
    conn_fs_posix_cmds_read,
    conn_fs_posix_cmds_write,
    conn_fs_posix_cmds_lseek,
};

static inline void conn_fs_posix_rtos_debug_info(struct conn_fs_posix_cmds_arguments *args)
{
#ifdef CONN_FS_POSIX_CMDS_DEBUG
    int cmds = args->cmd_index;

    switch (cmds) {
    case conn_fs_posix_cmds_open:
        conn_fs_posix_debug_rtos_args_open(args);
        break;
    case conn_fs_posix_cmds_close:
        conn_fs_posix_debug_rtos_args_close(args);
        break;
    case conn_fs_posix_cmds_read:
        conn_fs_posix_debug_rtos_args_read(args);
        break;
    case conn_fs_posix_cmds_write:
        conn_fs_posix_debug_rtos_args_write(args);
        break;
    case conn_fs_posix_cmds_lseek:
        conn_fs_posix_debug_rtos_args_lseek(args);
        break;
    default:
        printk("args:rtos ioctl cmd(%x) can not handle\n", cmds);
        break;
    }
#endif
}

/*
 * conn_fs.c 中接收到RTOS端发送的操作命令, 唤醒应用层service进程
 * 处理指令,并返回执行结果
 */
int conn_fs_posix_rtos_ioctl(void *arg, void *result)
{
    struct conn_fs_posix_cmds_arguments *args = (struct conn_fs_posix_cmds_arguments *)arg;
    struct conn_fs_posix_cmds_result *res = (struct conn_fs_posix_cmds_result *)result;
    struct m_private_data *pdata = pri_data;
    void *result_buf = NULL;
    int ret;

    if (!pdata) {
        printk(KERN_ERR "CONN_FS_POSIX: conn_fs_posix service is not ready. (CPU%d)\n", LINUX_CPU_ID);
        return -EIO;
    }

    struct conn_fs_posix_data *data = pdata->data;

    mutex_lock(&data->lock);

    conn_fs_posix_rtos_debug_info(args);

    switch (args->cmd_index) {
    case conn_fs_posix_cmds_open:
        conn_fs_drv_set_wait_event_cmds(data, POSIX_conn_fs_posix_open, args, res);
        break;
    case conn_fs_posix_cmds_close:
        conn_fs_drv_set_wait_event_cmds(data, POSIX_conn_fs_posix_close, args, res);
        break;
    case conn_fs_posix_cmds_read:
        result_buf = conn_fs_posix_get_rtos_buffer(args->argv);
        conn_fs_drv_set_wait_event_cmds(data, POSIX_conn_fs_posix_read, args, res);
        break;
    case conn_fs_posix_cmds_write:
        conn_fs_drv_set_wait_event_cmds(data, POSIX_conn_fs_posix_write, args, res);
        break;
    case conn_fs_posix_cmds_lseek:
        conn_fs_drv_set_wait_event_cmds(data, POSIX_conn_fs_posix_lseek, args, res);
        break;
    default:
        printk("args:%x can not handle\n", args->cmd_index);
        break;
    }

    /* 唤醒POSIX service等待命令事件 */
    conn_fs_drv_wakeup_event_cmds(data);

    /* 清楚POSIX service执行API完成标志,使其进入等待状态 */
    conn_fs_drv_clear_wait_event_return(data);

    mutex_unlock(&data->lock);

    ret = wait_event_interruptible(data->wait_return, conn_fs_drv_waitting_event_return(data));

    /* 更新返回buf地址为RTOS传递的buffer地址:当没有内容返回是该地址为NULL */
    res->result = result_buf;
    return 0;
}