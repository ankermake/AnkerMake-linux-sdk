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

#define CONN_FS_DRV_TAG                "conn_fs_drv_close:"

struct conn_fs_args_close {
    int argc;
    int fd;
};

#ifdef CONN_FS_POSIX_CMDS_DEBUG
static void conn_fs_posix_debug_rtos_args_close(struct conn_fs_posix_cmds_arguments *args)
{
    uint32_t *argv0 = args->argv[0];

    int fd = *argv0;

    printk(KERN_ERR CONN_FS_DRV_TAG "rtos args address        : %p\n", args);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argv address        : %p\n",args->argv);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos cmd_index           : %x\n",args->cmd_index);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argc                : %d\n",args->argc);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argv[0]:fd          : (%p)(%p)%x\n",argv0, &fd, fd);
    printk(KERN_ERR "\n");
}
#endif

static void conn_fs_posix_fill_api_args_close(struct conn_fs_posix_data *data, struct conn_fs_args_close *args)
{
    struct conn_fs_posix_cmds_arguments *cmds = data->args;

    void **argv = cmds->argv;

    args->argc = cmds->argc;

    int argv0 = *(uint32_t *)argv[0];
    args->fd = argv0;
#ifdef CONN_FS_POSIX_CMDS_DEBUG
    printk(KERN_ERR CONN_FS_DRV_TAG "posix args address       : %p\n", args);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix argc               : %d\n",args->argc);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix argv[0]:fd         : (%p)%x\n",&args->fd, args->fd);
    printk(KERN_ERR "\n");
#endif
}

static int conn_fs_drv_service_posix_close(struct conn_fs_posix_data *data, unsigned int arg)
{
    struct conn_fs_args_close args;

    conn_fs_posix_fill_api_args_close(data, &args);

    int ret = copy_to_user((void __user *)arg, &args, sizeof(struct conn_fs_args_close));
    if (ret)
        printk(KERN_ERR "conn fs drv service posix close copy to user error\n");

    return ret;
}
