
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/kthread.h>


#include "ring_mem.h"

#include "conn.h"

#include "assert.c"
#include "mailbox.c"

#define NOTIFY_TIMEOUT_MS 1000

#define LINUX_CPU_ID    0

static DEFINE_MUTEX(lock);

enum conn_cmd {
    CONN_bind = 0xac,
    CONN_unbind = 0xab,
    CONN_unbind_confirm = 0xad,
    CONN_writeable = 0xae,
    CONN_readable = 0xaf,
};

struct conn_notify {
    enum conn_cmd cmd;
    struct conn_node *conn;
};

struct conn_node {
    int id;
    char *name;
    struct ring_mem ring;

    char *conn_buf;
    unsigned int is_writing;
    struct mutex write_mutex;
    struct mutex read_mutex;

    wait_queue_head_t write_wait;
    wait_queue_head_t read_wait;
    wait_queue_head_t write_complete_wait;
    wait_queue_head_t unbind_confirm_wait;
};

struct conn_table {
    volatile struct conn_node *this_side;
    volatile struct conn_node *other_side;
};

struct conn_table *conn_table;

static int conn_max_link_count = 16;

static int conn_is_init = 0;

static wait_queue_head_t inited_wait;

module_param_named(conn_max_link_count, conn_max_link_count, int, 0644);

static int conn_send_notify(struct conn_node *conn, enum conn_cmd cmd)
{
    int ret;
    int size;
    struct conn_notify notify;

    notify.cmd = cmd;
    notify.conn = conn;

    size = sizeof(notify);

    ret = mailbox_send(&notify, size, NOTIFY_TIMEOUT_MS);
    if (ret != size) {
        printk(KERN_ERR "CONN: send notify timeout.(CPU%d)\n", LINUX_CPU_ID);
        return -1;
    }

    return 0;
}

static int get_useable_id_by_name(const char *name)
{
    int i;

    for (i = 0; i < conn_max_link_count; i++) {
        const char *conn_name;
        if (conn_table[i].this_side)
            conn_name = conn_table[i].this_side->name;
        else if (conn_table[i].other_side)
            conn_name = conn_table[i].other_side->name;
        else
            continue;

        if (!strcmp(name, conn_name))
            return i;
    }

    return -1;
}

static int get_unused_id(void)
{
    int i;

    for (i = 0; i < conn_max_link_count; i++) {
        if (!conn_table[i].this_side && !conn_table[i].other_side)
            return i;
    }
    return -1;
}

static int get_useable_id(const char *name)
{
    int id;

    /* 优先选择同名的conn设备进行匹配,并返回对应的ID */
    id = get_useable_id_by_name(name);
    if (id >= 0 && id < conn_max_link_count)
        return id;

    /* 若没有同名设备存在,选择一个没被用到的ID用作设备ID */
    return get_unused_id();
}

static int conn_other_side_bind(struct conn_notify *notify)
{
    int id;
    struct conn_node *conn = notify->conn;

    mutex_lock(&lock);

    id = get_useable_id(conn->name);
    if (id < 0) {
        printk(KERN_ERR "CONN: conn bind failed, not enough id to be used !(CPU%d)\n", LINUX_CPU_ID);
        goto unlock;
    }

    conn_table[id].other_side = conn;

unlock:
    mutex_unlock(&lock);

    return 0;
}

static int conn_other_side_unbind(struct conn_notify *notify)
{
    int id;
    int ret;
    struct conn_node *this_side;
    struct conn_node *other_side = notify->conn;

    mutex_lock(&lock);

    id = get_useable_id_by_name(other_side->name);
    if (id < 0) {
        printk(KERN_ERR "CONN: failed to get conn(%s)!(CPU%d)\n", other_side->name, LINUX_CPU_ID);
        goto unlock;
    }

    this_side = (struct conn_node *)conn_table[id].this_side;

    conn_table[id].other_side = NULL;

    if (this_side) {
        wake_up_all(&this_side->write_wait);

        wake_up_all(&this_side->read_wait);

        wait_event_timeout(this_side->write_complete_wait,
                        this_side->is_writing == 0,
                        msecs_to_jiffies(100));
    }

    ret = conn_send_notify(other_side, CONN_unbind_confirm);
    if (ret < 0)
        printk(KERN_ERR "CONN: conn(%s) failed to send unbind_confirm cmd !(CPU%d)\n", other_side->name, LINUX_CPU_ID);

unlock:
    mutex_unlock(&lock);

    return 0;
}

static void conn_this_side_unbind_confirm(struct conn_notify *notify)
{
    struct conn_node *conn = notify->conn;
    if (!conn)
        return;

    mutex_lock(&lock);

    conn_table[conn->id].this_side = NULL;

    wake_up_all(&conn->unbind_confirm_wait);

    mutex_unlock(&lock);
}

static void conn_wake_up_writeable(struct conn_notify *notify)
{
    struct conn_node *conn = notify->conn;
    if (!conn)
        return;

    wake_up_all(&conn->write_wait);
}

static void conn_wake_up_readable(struct conn_notify *notify)
{
    struct conn_node *conn = notify->conn;
    if (!conn)
        return;

    wake_up_all(&conn->read_wait);
}

int conn_write(struct conn_node *conn, const void *buf, unsigned int size, int timeout_ms)
{
    int id;
    int ret;
    struct ring_mem *ring;
    int total_size = size;
    struct conn_node *other_side;

    uint64_t start_time;

    assert(conn && buf && size);

    mutex_lock(&conn->write_mutex);

    start_time = conn_local_clock_ms();

    mutex_lock(&lock);

    id = conn->id;

    if (!conn_table[id].other_side) {
        mutex_unlock(&lock);
        goto unlock;
    }

    other_side = (struct conn_node *)conn_table[id].other_side;

    ring = &other_side->ring;

    conn->is_writing = 1;

    mutex_unlock(&lock);

    while (1) {
        if (!conn_table[id].other_side)
            break;

        int other_side_need_wake_up = (ring_mem_readable_size(ring) == 0);

        ret = ring_mem_write(ring, (void *)buf, size);

        size -= ret;
        buf += ret;

        if (other_side_need_wake_up)
            conn_send_notify(other_side, CONN_readable);

        if (!size)
            break;

        int timeout = timeout_ms - (conn_local_clock_ms() - start_time);
        if (timeout <= 0) {
            printk(KERN_ERR "CONN: conn(%s) write timeout.(CPU%d)\n", conn->name, LINUX_CPU_ID);
            break;
        }

        wait_event_timeout(conn->write_wait,
                            ring_mem_writable_size(ring),
                            msecs_to_jiffies(timeout));
    }

unlock:
    conn->is_writing = 0;
    if (!conn_table[id].other_side)
        wake_up_all(&conn->write_complete_wait);

    mutex_unlock(&conn->write_mutex);

    return total_size - size;
}
EXPORT_SYMBOL(conn_write);

int conn_read(struct conn_node *conn, void *buf, unsigned int size, int timeout_ms)
{
    int ret;
    int id;
    struct ring_mem *ring;
    int total_size = size;

    uint64_t start_time;

    assert(conn && buf && size);

    mutex_lock(&conn->read_mutex);

    start_time = conn_local_clock_ms();

    id = conn->id;

    ring = &conn->ring;

    while (1) {
        if (!conn_table[id].other_side)
            break;

        int other_side_need_wake_up = ring_mem_readable_size(ring) == ring->mem_size;

        ret = ring_mem_read(&conn->ring, buf, size);

        size -= ret;
        buf += ret;

        if (other_side_need_wake_up)
            conn_send_notify((struct conn_node *)conn_table[id].other_side, CONN_writeable);

        if (!size)
            break;

        int timeout = timeout_ms - (conn_local_clock_ms() - start_time);
        if (timeout <= 0) {
            printk(KERN_ERR "CONN: conn(%s) read timeout.(CPU%d)\n", conn->name, LINUX_CPU_ID);
            break;
        }

        wait_event_timeout(conn->read_wait,
                            ring_mem_readable_size(&conn->ring),
                            msecs_to_jiffies(timeout));
    }

    mutex_unlock(&conn->read_mutex);

    return total_size - size;
}
EXPORT_SYMBOL(conn_read);

static void conn_do_release(struct conn_node *conn)
{
    ring_mem_clean(&conn->ring);

    kfree(conn->name);
    kfree(conn->conn_buf);
    kfree(conn);
}

void conn_wait_conn_inited(void)
{
    wait_event(inited_wait, conn_is_init);
}
EXPORT_SYMBOL(conn_wait_conn_inited);

void conn_release(struct conn_node *conn)
{
    int ret;
    int id;
    int cpu_id = LINUX_CPU_ID;

    assert(conn);

    ret = conn_send_notify(conn, CONN_unbind);
    if (ret < 0) {
        printk(KERN_ERR "CONN: conn(%s) send unbind notify failed.(CPU%d)\n", conn->name, cpu_id);
        printk(KERN_ERR "CONN: conn(%s) release failed.(CPU%d)\n", conn->name, cpu_id);
        return;
    }

    mutex_lock(&conn->write_mutex);

    mutex_lock(&conn->read_mutex);

    mutex_lock(&lock);

    id = conn->id;

    while (1) {
        if (!conn_table[id].this_side)
            break;

        mutex_unlock(&lock);

        ret = wait_event_timeout(conn->unbind_confirm_wait,
                            conn_table[id].this_side == NULL,
                            msecs_to_jiffies(1000));
        if (!ret)
            printk(KERN_ERR "CONN: conn(%s) wait unbind_confirm timeout.(CPU%d)\n", conn->name, cpu_id);

        mutex_lock(&lock);
    }

    conn_do_release(conn);

    mutex_unlock(&lock);
}
EXPORT_SYMBOL(conn_release);

struct conn_node *conn_request(const char *name, unsigned int buf_size)
{
    int id;
    int ret;
    int name_len;
    struct conn_node *conn = NULL;
    int cpu_id = LINUX_CPU_ID;

    mutex_lock(&lock);

    if (!conn_is_init)
        goto unlock;

    if (name == NULL) {
        printk(KERN_ERR "CONN: name cannot be NULL.(CPU%d)\n", cpu_id);
        goto unlock;
    }

    id = get_useable_id(name);
    if (id < 0) {
        printk(KERN_ERR "CONN: no enough id to be used !(CPU%d)\n", cpu_id);
        goto unlock;
    }

    if (conn_table[id].this_side) {
        printk(KERN_ERR "CONN: conn(%s) has been requested !(CPU%d)\n", name, cpu_id);
        goto unlock;
    }

    conn = kmalloc(sizeof(*conn), GFP_KERNEL);
    if (conn == NULL) {
        printk(KERN_ERR "CONN: conn(%s) malloc failed.(CPU%d)\n", name, cpu_id);
        goto unlock;
    }

    conn->conn_buf = kmalloc(buf_size, GFP_KERNEL);
    if (conn->conn_buf == NULL) {
        printk(KERN_ERR "CONN: conn(%s) malloc buf failed.(CPU%d)\n", name, cpu_id);
        goto malloc_buf_err;
    }

    name_len = strlen(name) + 1;
    conn->id = id;

    conn->name = kmalloc(name_len, GFP_KERNEL);
    if (conn->name == NULL) {
        printk(KERN_ERR "CONN: conn(%s) malloc conn name failed.(CPU%d)\n", name, cpu_id);
        goto malloc_name_err;
    }
    memcpy(conn->name, name, name_len);

    ring_mem_init(&conn->ring, conn->conn_buf, buf_size);

    mutex_init(&conn->write_mutex);
    mutex_init(&conn->read_mutex);

    init_waitqueue_head(&conn->write_wait);
    init_waitqueue_head(&conn->read_wait);
    init_waitqueue_head(&conn->write_complete_wait);
    init_waitqueue_head(&conn->unbind_confirm_wait);

    ret = conn_send_notify(conn, CONN_bind);
    if (ret < 0) {
        printk(KERN_ERR "CONN: conn(%s) send bind notify failed.(CPU%d)\n", name, cpu_id);
        goto send_bind_err;
    }

    conn_table[id].this_side = conn;
    conn_table[id].this_side->name = conn->name;

unlock:
    mutex_unlock(&lock);

    return conn;

send_bind_err:
    kfree(conn->name);
malloc_name_err:
    kfree(conn->conn_buf);
malloc_buf_err:
    kfree(conn);

    conn = NULL;

    mutex_unlock(&lock);

    return conn;
}
EXPORT_SYMBOL(conn_request);

static int conn_notify_process(void *data)
{
    int ret;
    struct conn_notify notify;
    int size = sizeof(notify);

    while (1) {
        ret = mailbox_receive(&notify, size, NOTIFY_TIMEOUT_MS);
        if (ret != size)
            continue;

        switch (notify.cmd) {
            case CONN_bind:
                conn_other_side_bind(&notify);
                break;
            case CONN_unbind:
                conn_other_side_unbind(&notify);
                break;
            case CONN_unbind_confirm:
                conn_this_side_unbind_confirm(&notify);
                break;
            case CONN_writeable:
                conn_wake_up_writeable(&notify);
                break;
            case CONN_readable:
                conn_wake_up_readable(&notify);
                break;
            default:
                break;
        }
    }

    return 0;
}

static int conn_init_thread(void *data)
{
    static struct task_struct *notify_thread;

    conn_table = kzalloc(conn_max_link_count * sizeof(*conn_table), GFP_KERNEL);
    if (conn_table == NULL)
        panic("CONN: malloc conn_table failed.(CPU%d)\n", LINUX_CPU_ID);

    mailbox_init();

    notify_thread = kthread_create(conn_notify_process, NULL, "conn_notify_process");
    wake_up_process(notify_thread);

    mutex_lock(&lock);

    conn_is_init = 1;

    mutex_unlock(&lock);

    wake_up_all(&inited_wait);

    printk("CONN: conn init succeed.(CPU%d)\n", LINUX_CPU_ID);

    return 0;
}

int conn_init(void)
{
    struct task_struct *init_thread;

    init_waitqueue_head(&inited_wait);

    init_thread = kthread_create(conn_init_thread, NULL, "conn_init_thread");

    wake_up_process(init_thread);

    return 0;
}
module_init(conn_init);

MODULE_LICENSE("GPL");
