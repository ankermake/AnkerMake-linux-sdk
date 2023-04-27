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
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/mtd/mtd.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/mtd/mtd.h>
#include <linux/buffer_head.h>
#include <assert.h>
#include <linux/vmalloc.h>
#include <linux/miscdevice.h>
#include <common.h>
#include <bit_field.h>

#include <utils/clock.h>

#include <conn.h>
#include "conn_fs_posix.h"

#define LINUX_CPU_ID                    0

#define NOTIFY_WRITE_TIMEOUT_MS         (10)
#define NOTIFY_READ_TIMEOUT_MS          (3 *1000)
#define BLK_DEV_SECTOR_SIZE             (512)

//#define CONN_FS_POSIX_CMDS_DEBUG

struct conn_fs_data {
    char *device_name;              /* conn name */
    void *ramdisk_start_addr;       /* RTOS ramdisk起始地址 */
    uint32_t partition_size;        /* RTOS 分区大小 */

    struct miscdevice mdev;
    struct conn_node *conn;
};

enum conn_fs_cmd {
    CMD_conn_fs_get_posix_service_status,
    CMD_conn_fs_ops_posix_ioctl,

    CMD_conn_fs_conn_detect,
};

struct conn_fs_notify {
    enum conn_fs_cmd cmd;
    void *data;
};

static const char *conn_cmd_str[] = {
    [CMD_conn_fs_get_posix_service_status] = "conn_fs_get_posix_service_status",
    [CMD_conn_fs_ops_posix_ioctl] = "conn_fs_ops_posix_ioctl",

    [CMD_conn_fs_conn_detect] = "conn_fs_conn_detect",
};

static struct conn_fs_data conn_fs_dev = {
    .device_name            = "conn_fs",
};


int conn_fs_is_connected(void)
{
    struct conn_fs_data *drv = &conn_fs_dev;

    return drv->conn ? 1 : 0;
}


static int conn_fs_send_cmd(struct conn_fs_data *drv, enum conn_fs_cmd cmd, void *data)
{
    int ret;
    struct conn_fs_notify notify;
    int len = sizeof(notify);

    struct conn_node *conn = drv->conn;

    notify.cmd = cmd;
    notify.data = data;

    ret = conn_write(conn, &notify, len, NOTIFY_WRITE_TIMEOUT_MS);
    if (ret != len) {
        printk(KERN_ERR "CONN_FS: %s ,failed to write conn. ret = %d. (CPU%d)\n", conn_cmd_str[cmd], ret, LINUX_CPU_ID);
        return -1;
    }

    return 0;
}


static int conn_fs_get_posix_service_status(struct conn_fs_data *drv, void *args)
{
    int ret;
    int *status = (int *)args;

    /* 设置返回值 */
    *status = conn_fs_drv_get_service_status();

    ret = conn_fs_send_cmd(drv, CMD_conn_fs_get_posix_service_status, &status);
    if (ret < 0)
        printk(KERN_ERR "CONN_FS: %s ,failed read blk. ret = %d. (CPU%d)\n", __func__, ret, LINUX_CPU_ID);

    return 0;
}

static void conn_fs_ops_posix_debug_result(struct conn_fs_posix_cmds_result *res)
{
#ifdef CONN_FS_POSIX_CMDS_DEBUG
    /*
     * res.result
     * 应用和驱动交换数据使用mmaped_mem作为临时缓存,
     * 交互过程中有相应的操作更新到RTOS传递来的地址空间中
     * 最后结果显示使用RTOS端的地址打印
     */
    printk(KERN_ERR "conn_fs_drv_result:res address             : %p\n", res);
    printk(KERN_ERR "conn_fs_drv_result:res.index               : 0x%x\n", res->cmd_index);
    printk(KERN_ERR "conn_fs_drv_result:res.res_val             : %d\n", res->res_val);
    printk(KERN_ERR "conn_fs_drv_result:res.result_len          : %d\n", res->result_len);
    printk(KERN_ERR "conn_fs_drv_result:res.result(RTOS)        : %p\n", res->result);

    if (res->result_len > 4096) {
        printk(KERN_ERR "conn_fs_drv_result: result len is too long set result len to 4096\n");
        res->result_len = 4096;
    }

    unsigned char *buf = (void *)res->result;
    int i = 0;

    for (i = 0; i < res->result_len; i++) {
        if ( i != 0 && i % 16 == 0) {
            printk("\n");
        }
        printk("%02x:", buf[i]);
    }
    printk(KERN_ERR "\n");
#endif
}

static int conn_fs_ops_posix_ioctl(struct conn_fs_data *drv, void *args)
{
    int ret;
    struct conn_fs_posix_cmds_result res;

    /* 初始化默认返回结果 */
    memset(&res, 0x00, sizeof(struct conn_fs_posix_cmds_result));
    res.res_val = -1;

    ret = conn_fs_posix_rtos_ioctl(args, (void *)&res);

    conn_fs_ops_posix_debug_result(&res);

    ret = conn_fs_send_cmd(drv, CMD_conn_fs_ops_posix_ioctl, &res);
    if (ret < 0)
        printk(KERN_ERR "CONN_FS: %s ,failed read blk. ret = %d. (CPU%d)\n", __func__, ret, LINUX_CPU_ID);

    return 0;
}

static void conn_fs_detect(struct conn_fs_data *drv, struct conn_fs_notify *notify)
{
    int ret = conn_fs_send_cmd(drv, CMD_conn_fs_conn_detect, NULL);
    if (ret < 0)
        printk(KERN_ERR "CONN_FS: %s ,failed get info. ret = %d. (CPU%d)\n", __func__, ret, LINUX_CPU_ID);
}


static int conn_fs_request(struct conn_fs_data *drv)
{
    struct conn_node *conn = conn_request(drv->device_name, sizeof(struct conn_fs_notify));
    if (!conn)
        return -1;

    drv->conn = conn;

    return 0;
}

static int conn_fs_notify_process(void *data)
{
    struct conn_fs_data *drv = (struct conn_fs_data *)data;
    int ret;
    struct conn_fs_notify notify;
    int len = sizeof(notify);

    while (1) {
        ret = conn_read(drv->conn, &notify, len, NOTIFY_READ_TIMEOUT_MS);
        if (ret != len)
            continue;

        switch (notify.cmd) {
            case CMD_conn_fs_get_posix_service_status:
                conn_fs_get_posix_service_status(drv, notify.data);
                break;
            case CMD_conn_fs_ops_posix_ioctl:
                conn_fs_ops_posix_ioctl(drv, notify.data);
                break;
            case CMD_conn_fs_conn_detect:
                conn_fs_detect(drv, &notify);
                break;
            default:
                break;
        }
    }

    return 0;
}

static int conn_fs_drv_init(void)
{
    static struct task_struct *notify_fs_thread;
    struct conn_fs_data *drv = &conn_fs_dev;
    int ret;

    ret = conn_fs_request(drv);
    if (ret < 0)
        return ret;

    ret = conn_fs_posix_register(&drv->mdev);

    notify_fs_thread = kthread_create(conn_fs_notify_process, drv, "conn_fs_notify_process");
    wake_up_process(notify_fs_thread);

    printk(KERN_ERR "CONN_FS: %s request conn succeed. (CPU%d)\n", drv->device_name, LINUX_CPU_ID);

    return 0;
}

static void conn_fs_drv_exit(void)
{
    struct conn_fs_data *drv = &conn_fs_dev;

    conn_fs_posix_unregister(&drv->mdev);

    conn_release(drv->conn);
    drv->conn = NULL;
}

static int conn_fs_init(void)
{
    conn_wait_conn_inited();

    conn_fs_drv_init();

    return 0;
}

static void conn_fs_exit(void)
{
    conn_fs_drv_exit();
}

module_init(conn_fs_init);
module_exit(conn_fs_exit);

MODULE_LICENSE("GPL");
