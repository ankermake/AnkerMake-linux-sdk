#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <assert.h>
#include <common.h>
#include <bit_field.h>
#include <linux/wait.h>

#include <conn.h>
#include "conn_fs_posix.h"

#define LINUX_CPU_ID                    0

#ifdef CONN_FS_DRV_TAG
#undef CONN_FS_DRV_TAG
#endif

#define CONN_FS_DRV_TAG                "conn_fs_drv_write:"

/*
 * 该结构体和kernel driver保持一致， 和 RTOS端传递的结构体不一致
 * 用户空间和kernel交互数据buffer 大小固定，需要多次交换数据
 */
struct conn_fs_args_write {
    int argc;
    int fd;
    void *buf;  /* 暂时未使用该变量, 用户空间和kernel 通过mmaped_mem地址交互数据  */
    size_t totle_len; /* 总大小 */
    size_t tranfer_len; /* 本次传输大小 */
};

#ifdef CONN_FS_POSIX_CMDS_DEBUG
static void conn_fs_posix_debug_rtos_args_write(struct conn_fs_posix_cmds_arguments *args)
{
    uint32_t *argv0 = args->argv[0];
    uint32_t *argv1 = args->argv[1];
    uint32_t *argv2 = args->argv[2];

    int fd = *argv0;
    void *buf = (void *)(*argv1);
    uint32_t len = *argv2;

    printk(KERN_ERR CONN_FS_DRV_TAG "rtos args address        : %p\n", args);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argv address        : %p\n",args->argv);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos cmd_index           : %x\n",args->cmd_index);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argc                : %d\n",args->argc);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argv[0]:fd          : (%p)(%p)%d\n",argv0, &fd, fd);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argv[1]:buf[0]      : (%p)(%p)%x\n",argv1, buf, ((uint32_t *)buf)[0]);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argv[2]:len         : (%p)(%p)%x\n",argv2, &len, len);
    printk(KERN_ERR "\n");
}
#endif

static void conn_fs_posix_fill_api_args_write(struct conn_fs_posix_data *data, struct conn_fs_args_write *args)
{
    struct conn_fs_posix_cmds_arguments *cmds = data->args;
    struct conn_fs_posix_cmds_result *res = data->res;

    void **argv = cmds->argv;

    args->argc = cmds->argc;

    int argv0 = *(uint32_t *)argv[0];
    args->fd = argv0;

    void *argv1 = (void *)(*(uint32_t *)argv[1]);
    args->buf = argv1;

    uint32_t argv2 = *(uint32_t *)argv[2];
    args->totle_len = argv2;

    /*
     * 交互的内容在应用层读取/写入到mmaped_mem映射空间
     * 从RTOS端指定的buffer中copy到映射空间中
     */
    int offset = res->result_len; /* 已传输/copy完数据的长度：result_len当全局变量使用,当return命令执行时,res的内容会更新覆盖 */
    int totle_len = args->totle_len;  /* 需要read传输数据总长度 */

    /* 更新要写入的内容 */
    args->tranfer_len = totle_len - offset;
    if (args->tranfer_len > data->mem_size)
        args->tranfer_len = data->mem_size;

    char *buf = args->buf;
    memcpy(data->mem, &buf[offset], args->tranfer_len);

    /* 更新写入的长度 */
    res->result_len += args->tranfer_len;

#ifdef CONN_FS_POSIX_CMDS_DEBUG
    printk(KERN_ERR CONN_FS_DRV_TAG "posix args address       : %p\n", args);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix argc               : %d\n",args->argc);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix argv[0]:fd         : (%p)%d\n",&args->fd, args->fd);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix argv[1]:buf[0]     : (%p)%x\n",args->buf, ((uint32_t *)args->buf)[0]);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix argv[2]:totle_len  : (%p)%x\n",&args->totle_len, args->totle_len);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix argv[2]:tranfer_len: (%p)%x\n",&args->tranfer_len, args->tranfer_len);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix transfered length  : (%p)%x\n",&offset, offset);

    printk(KERN_ERR "\n");
#endif
}

static int conn_fs_drv_service_posix_write(struct conn_fs_posix_data *data, unsigned int arg)
{
    struct conn_fs_args_write args;

    conn_fs_posix_fill_api_args_write(data, &args);

    int ret = copy_to_user((void __user *)arg, &args, sizeof(struct conn_fs_args_write));
    if (ret)
        printk(KERN_ERR "conn fs drv service posix write copy to user error\n");

    return ret;
}
