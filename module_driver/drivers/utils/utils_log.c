#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>

#include <linux/string.h>
#include <linux/list.h>
#include <utils/clock.h>
#include <linux/wait.h>
#include <linux/fs.h>

#include <linux/miscdevice.h>

struct ring {
    void *buf;
    int size;
};

static int ring_write(struct ring *ring, int pos, void *buf, int size)
{
    pos = pos % ring->size;

    if (pos + size > ring->size) {
        int sz = ring->size - pos;
        memcpy(ring->buf + pos, buf, sz);
        memcpy(ring->buf, buf + sz, size - sz);
    } else {
        memcpy(ring->buf + pos, buf, size);
    }

    return (pos+size)%ring->size;
}

static int ring_read(struct ring *ring, int pos, void *buf, int size)
{
    pos = pos % ring->size;

    if (pos + size > ring->size) {
        int sz = ring->size - pos;
        memcpy(buf, ring->buf + pos, sz);
        memcpy(buf + sz, ring->buf, size - sz);
    } else {
        memcpy(buf, ring->buf + pos, size);
    }

    return (pos+size)%ring->size;
}

struct msg {
    unsigned int size;
    unsigned short tag;
    uint64_t us;
    char buf[0];
};

#define TAG 11237

static LIST_HEAD(list);
static DEFINE_MUTEX(lock);

struct ring ring;

static void *buffer;

static int buffer_size = 128*1024;
module_param(buffer_size, int, 0644);

static volatile int m_start, m_end;

static DECLARE_WAIT_QUEUE_HEAD(wait_queue);

struct m_data {
    int start;
};

static void init_msg(struct msg *msg, int size)
{
    msg->size = size;
    msg->tag = TAG;
    msg->us = local_clock_us();
}

static int write_msg(int pos, int size)
{
    struct msg msg;

    init_msg(&msg, size);

    return ring_write(&ring, pos, &msg, sizeof(msg));
}

static int write_log(int pos, char *buf, int size)
{
    pos = write_msg(pos, size);

    return ring_write(&ring, pos, buf, size);
}

static int read_msg(int pos, struct msg *msg)
{
    return ring_read(&ring, pos, msg, sizeof(*msg));
}

static int read_data(int pos, char *data, int size)
{
    return ring_read(&ring, pos, data, size);
}

static ssize_t log_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    struct m_data *data = file->private_data;

    wait_event_interruptible(wait_queue, data->start != m_end);
    if (data->start == m_end)
        return -EINTR;

    mutex_lock(&lock);

    struct msg msg;

    int start;
    while (1) {
        start = read_msg(data->start, &msg);
        if (msg.tag == TAG)
            break;
        data->start += 1;
    }

    static char mem[1024+1];
    int len = msg.size;
    data->start = read_data(start, mem, len);
    mem[len] = 0;

    // printk(KERN_ERR "%d %d %d %d\n", m_start, m_end, data->start, len);

    unsigned int usec;
    unsigned long long secs = msg.us;
    usec = do_div(secs, 1000000);

    len = snprintf(buf, size, "%llu.%06u:%s\n", secs, usec, mem);

    mutex_unlock(&lock);

    return len;
}

static ssize_t log_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    if (size > 1024)
        size = 1024;

    mutex_lock(&lock);

    int start = m_end;
    int len = 0;
    struct msg msg;
    int start_is_over = 0;

    while (1) {
        read_msg(start, &msg);
        if (msg.tag != TAG)
            panic("failed to check msg: %x\n", msg.tag);

        // printk(KERN_ERR "m -> %d\n", msg.size);

        start += sizeof(msg) + msg.size;
        len += sizeof(msg) + msg.size;
        if (len >= (size+sizeof(msg)*2))
            break;
        start_is_over = 1;
    }

    m_end = write_log(m_end, (void *)buf, size);
    write_msg(m_end, len-(size+sizeof(msg))-sizeof(msg));

    if (start_is_over)
        m_start = start;

    // printk(KERN_ERR "len -> %d %d %d\n", start, m_end, len);

    wake_up_all(&wait_queue);

    mutex_unlock(&lock);

    return size;
}

static void check_init_buffer(void)
{
    if (buffer != NULL)
        return;

    buffer = vmalloc(buffer_size);
    BUG_ON(!buffer);

    ring.buf = buffer;
    ring.size = buffer_size;

    m_start = 0;

    char str[] = "-----------\n";
    m_end = write_log(m_start, str, sizeof(str));

    write_msg(m_end, buffer_size-m_end-sizeof(struct msg));
}

static int log_open(struct inode *inode, struct file *filp)
{
    struct m_data *data = vmalloc(sizeof(*data));

    mutex_lock(&lock);

    check_init_buffer();

    data->start = m_start;
    filp->private_data = data;
    mutex_unlock(&lock);

    return 0;
}

static int log_release(struct inode *inode, struct file *filp)
{
    vfree(filp->private_data);
    return 0;
}

static long log_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static struct file_operations log_misc_fops = {
    .open = log_open,
    .release = log_release,
    .read = log_read,
    .write = log_write,
    .unlocked_ioctl = log_ioctl,
};

static struct miscdevice log_mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "log_manager",
    .fops = &log_misc_fops,
};

int log_dev_register(void)
{
    int ret;
    ret = misc_register(&log_mdev);
    BUG_ON(ret < 0);

    return 0;
}

void log_dev_unregister(void)
{
    misc_deregister(&log_mdev);
}

module_init(log_dev_register);

module_exit(log_dev_unregister);
