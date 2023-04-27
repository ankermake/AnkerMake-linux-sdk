#ifndef __CONN_FS_POSIX_H__
#define __CONN_FS_POSIX_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <linux/wait.h>
#include <linux/miscdevice.h>

struct conn_fs_posix_cmds_arguments {
    uint32_t cmd_index;     /* 指定命令索引号，Linux端按此转发到posix应用层对应操作 */
    int argc;               /* argument count */
    void **argv;            /* argument vector */
};

struct conn_fs_posix_cmds_result {
    uint32_t cmd_index;     /* 返回Linux应用层和驱动交互的magic Number. 暂无用处 */
    int res_val;            /* 返回值, posix API操作结果 */
    uint32_t result_len;    /* 返回内容的长度, */
    void *result;           /* 返回内存存放buffer, Linux应用层中被mapped_mem取代，Linux驱动中被mem取代. RTOS存放有效数据 */
};


int conn_fs_drv_get_service_status(void);

int conn_fs_posix_rtos_ioctl(void *arg, void *result);
int conn_fs_posix_register(struct miscdevice *mdev);
int conn_fs_posix_unregister(struct miscdevice *mdev);

#ifdef __cplusplus
}
#endif

#endif /* __CONN_FS_POSIX_H__ */