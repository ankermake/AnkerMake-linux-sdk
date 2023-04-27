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

#define CONN_FS_DRV_TAG                "conn_fs_drv_open:"

struct conn_fs_args_open {
    int argc;
    char file_name[256];
    int flags;
    mode_t mode;
};

#ifdef CONN_FS_POSIX_CMDS_DEBUG
static void conn_fs_posix_debug_rtos_args_open(struct conn_fs_posix_cmds_arguments *args)
{
    uint32_t *argv0 = args->argv[0];
    uint32_t *argv1 = args->argv[1];
    uint32_t *argv2 = args->argv[2];

    char *file_name = (char *)(*argv0);
    uint32_t flags = *argv1;
    uint32_t mode = *argv2;

    printk(KERN_ERR CONN_FS_DRV_TAG "rtos args address         : %p\n", args);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argv address         : %p\n",args->argv);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos cmd_index            : %x\n",args->cmd_index);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argc                 : %d\n",args->argc);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argv[0]:file_name    : (%p)(%p)%s\n",argv0, file_name, file_name);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argv[1]:flags        : (%p)(%p)%x\n",argv1, &flags, flags);
    printk(KERN_ERR CONN_FS_DRV_TAG "rtos argv[2]:mode         : (%p)(%p)%x\n",argv2, &mode, mode);
    printk(KERN_ERR "\n");
}
#endif

static int conn_fs_posix_fill_api_args_open(struct conn_fs_posix_data *data, struct conn_fs_args_open *args)
{
    struct conn_fs_posix_cmds_arguments *cmds = data->args;

    void **argv = cmds->argv;

    args->argc = cmds->argc;

    char *argv0 = (char *)*(uint32_t *)argv[0];
    if (strlen(argv0) >= sizeof(args->file_name)) {
        printk(KERN_ERR "conn fs drv service posix open file name length(%d) > max length(%d)\n",strlen(argv0), sizeof(args->file_name));
        return -EIO;
    }
    strcpy(args->file_name, argv0);

    uint32_t argv1 = *(uint32_t *)argv[1];
    args->flags = argv1;

    uint32_t argv2 = *(uint32_t *)argv[2];
    args->mode = argv2;

#ifdef CONN_FS_POSIX_CMDS_DEBUG
    printk(KERN_ERR CONN_FS_DRV_TAG "posix args address        : %p\n", args);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix argc                : %d\n",args->argc);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix argv[0]:file_name   : (%p)%s\n",args->file_name, args->file_name);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix argv[1]:flags       : (%p)%x\n",&args->flags, args->flags);
    printk(KERN_ERR CONN_FS_DRV_TAG "posix argv[2]:mode        : (%p)%x\n",&args->mode, args->mode);
    printk(KERN_ERR "\n");
#endif

    return 0;
}

static int conn_fs_drv_service_posix_open(struct conn_fs_posix_data *data, unsigned int arg)
{
    struct conn_fs_args_open args;

    int ret = conn_fs_posix_fill_api_args_open(data, &args);
    if (ret < 0) {
        printk(KERN_ERR "conn fs drv service posix open params is invalid\n");
        return ret;
    }

    ret = copy_to_user((void __user *)arg, &args, sizeof(struct conn_fs_args_open));
    if (ret)
        printk(KERN_ERR "conn fs drv service posix open copy to user error\n");

    return ret;
}