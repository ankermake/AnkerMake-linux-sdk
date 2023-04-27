
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/mtd/mtd.h>
#include <linux/ioctl.h>
#include <linux/list.h>



#include "conn.h"

#define LINUX_CPU_ID    0

#define NOTIFY_WRITE_TIMEOUT_MS (10)
#define NOTIFY_READ_TIMEOUT_MS  (3 *1000)

#define MTDRAM_ERASE_SIZE (128 * 1024) // 128K

struct conn_blk_data {
    void *priv_addr;
    int start_addr;
    int size;
    char *device_name;

    struct conn_node *conn;
};

static struct conn_blk_data conn_blk_dev = {
    .device_name            = "conn_blk",
};

static struct mtd_info conn_blk_mtd_info;

enum conn_cmd {
    CMD_conn_blk_get_info,
    CMD_conn_blk_read,
    CMD_conn_blk_write,
    CMD_conn_blk_erase,

    CMD_conn_blk_conn_detect,
};

static const char *conn_cmd_str[] = {
    [CMD_conn_blk_get_info] = "conn_blk_get_info",
    [CMD_conn_blk_read] = "conn_blk_read",
    [CMD_conn_blk_write] = "conn_blk_write",
    [CMD_conn_blk_erase] = "conn_blk_erase",
    [CMD_conn_blk_conn_detect] = "conn_blk_conn_detect",
};

struct conn_blk_notify {
    enum conn_cmd cmd;
    int offset;
    int length;
    void *data;
};

static int conn_blk_send_cmd(enum conn_cmd cmd, int offset, void *data, int length)
{
    int ret;
    void **p = data;
    struct conn_blk_notify notify;
    int len = sizeof(notify);

    struct conn_node *conn = conn_blk_dev.conn;

    notify.cmd = cmd;
    notify.data = data;
    notify.offset = offset;
    notify.length = length;

    ret = conn_write(conn, &notify, len, NOTIFY_WRITE_TIMEOUT_MS);
    if (ret != len) {
        printk(KERN_ERR "CONN_BLK: %s ,failed to write conn. ret = %d. (CPU%d)\n", conn_cmd_str[cmd], ret, LINUX_CPU_ID);
        return -1;
    }

    ret = conn_read(conn, &notify, len, NOTIFY_READ_TIMEOUT_MS);
    if (ret != len) {
        printk(KERN_ERR "CONN_BLK: %s ,failed to read conn. ret = %d. (CPU%d)\n", conn_cmd_str[cmd], ret, LINUX_CPU_ID);
        return -1;
    }

    if (notify.data && data)
        *p = notify.data;

    return 0;
}

static int check_blk_rw_size(int start, int offset, int length, int size)
{
    if (start + offset + length > size)
        return -EINVAL;

    return 0;
}

static int conn_blk_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    int ret = check_blk_rw_size(conn_blk_dev.start_addr, instr->addr, instr->len, mtd->size);
    if (ret < 0) {
        printk(KERN_ERR "CONN_BLK: failed to erase blk. out of mmc_blk size. (CPU%d)\n", LINUX_CPU_ID);
        return ret;
    }

    ret = conn_blk_send_cmd(CMD_conn_blk_erase, instr->addr, NULL, instr->len);
    if (ret < 0) {
        printk(KERN_ERR "CONN_BLK: failed to erase blk. ret = %d. (CPU%d)\n", ret, LINUX_CPU_ID);
        return -1;
    }

    instr->state = MTD_ERASE_DONE;
    mtd_erase_callback(instr);

    return 0;
}

static int conn_blk_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
    int ret = check_blk_rw_size(conn_blk_dev.start_addr, 0, len, mtd->size);
    if (ret < 0) {
        printk(KERN_ERR "CONN_BLK: failed to read blk. out of mmc_blk size. (CPU%d)\n", LINUX_CPU_ID);
        return ret;
    }

    ret = conn_blk_send_cmd(CMD_conn_blk_read, from, buf, len);
    if (ret < 0) {
        printk(KERN_ERR "CONN_BLK: failed to read blk. ret = %d. (CPU%d)\n", ret, LINUX_CPU_ID);
        return -1;
    }

    *retlen = len;

    return 0;
}

static int conn_blk_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
    int ret = check_blk_rw_size(conn_blk_dev.start_addr, 0, len, mtd->size);
    if (ret < 0) {
        printk(KERN_ERR "CONN_BLK: failed to write blk. out of mmc_blk size. (CPU%d)\n", LINUX_CPU_ID);
        return ret;
    }

    ret = conn_blk_send_cmd(CMD_conn_blk_write, to, (void *)buf, len);
    if (ret < 0) {
        printk(KERN_ERR "CONN_BLK: failed to write blk. ret = %d. (CPU%d)\n", ret, LINUX_CPU_ID);
        return -1;
    }

    *retlen = len;

    return 0;
}

static int conn_blk_request(struct conn_blk_data *drv)
{
    struct conn_blk_notify notify;
    int ret;

    int len = sizeof(notify);
    int i = 3;

    struct conn_node *conn = conn_request(drv->device_name, sizeof(struct conn_blk_notify));
    if (!conn)
        return -1;

    notify.cmd = CMD_conn_blk_conn_detect;
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

    ret = conn_read(conn, &notify, len, 10);
    if (ret != len) {
        printk(KERN_ERR "CONN_BLK: %s ,failed to read conn. ret = %d. (CPU%d)\n", conn_cmd_str[CMD_conn_blk_conn_detect], ret, LINUX_CPU_ID);
        conn_release(conn);
        return -1;
    }

    drv->start_addr = (unsigned int)notify.data;
    drv->size = notify.length;
    drv->conn = conn;

    return 0;
}

int mtdconn_blk_init_device(struct conn_blk_data *drv, const char *name)
{
    struct mtd_info *mtd = &conn_blk_mtd_info;

    mtd->name = name;
    mtd->type = MTD_RAM;
    mtd->flags = MTD_CAP_RAM;
    mtd->size = drv->size;
    mtd->writesize = 1;
    mtd->writebufsize = 64;
    mtd->erasesize = MTDRAM_ERASE_SIZE;
    mtd->priv = drv->priv_addr;

    mtd->owner = THIS_MODULE;
    mtd->_erase = conn_blk_erase;
    mtd->_read = conn_blk_read;
    mtd->_write = conn_blk_write;

    if (mtd_device_register(mtd, NULL, 0))
        return -EIO;

    return 0;
}

static int conn_blk_drv_init(void)
{
    int ret;

    struct conn_blk_data *drv = &conn_blk_dev;

    ret = conn_blk_request(drv);
    if (ret < 0)
        return 0;

    printk(KERN_ERR "CONN_BLK: %s request conn succeed. (CPU%d)\n", drv->device_name, LINUX_CPU_ID);

    ret = mtdconn_blk_init_device(drv, "conn_blk_dev");
    if (ret)
        panic("CONN_BLK: %s failed to init dev. (CPU%d)\n", drv->device_name, LINUX_CPU_ID);

    return 0;
}

static void conn_blk_drv_exit(void)
{
    struct conn_blk_data *drv = &conn_blk_dev;

    conn_release(drv->conn);

    mtd_device_unregister(&conn_blk_mtd_info);
}

static int conn_blk_init(void)
{
    conn_wait_conn_inited();

    conn_blk_drv_init();

    return 0;
}

static void conn_blk_exit(void)
{
    conn_blk_drv_exit();
}

module_init(conn_blk_init);
module_exit(conn_blk_exit);

MODULE_LICENSE("GPL");
