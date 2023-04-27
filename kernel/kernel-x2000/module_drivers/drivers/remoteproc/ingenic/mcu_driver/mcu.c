#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
//#include <common.h>
#include <linux/list.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <soc/base.h>
//#include <bit_field.h>
#include <linux/workqueue.h>
#include <linux/sched.h>

#include "utils/clock.h"

#include "ring_mem.h"
#include "mcu_firmware.h"
//#include "assert.h"

#define CPM_SFTINT 0xbc

#define IRQ_MCU         (IRQ_INTC_BASE + 5)

#define DMCS     0x1030
#define DMNMB    0x1034
#define DMSMB    0x1038
#define DMINT    0x103c

#define SLEEP 31, 31
#define SCMD 30, 30
#define SC_OFF 8, 23
#define BTB_INV 4, 4
#define SC_CALL 3, 3
#define SW_RST 0, 0

#define S_IP 17, 17
#define N_IP 16, 16
#define S_IMSK 1, 1
#define N_IMSK 0, 0

#define TCSM_MAX_SIZE (32 * 1024)

#define TCSM_IO_BASE 0x13422000
#define TCSM_MCU_VIRT_BASE 0xF4000000

#define TCSM_ADDR ((volatile unsigned long *)CKSEG1ADDR(TCSM_IO_BASE))

#define MCU_ADDR(reg) ((volatile unsigned long *)CKSEG1ADDR(PDMA_IOBASE+reg))

#define CPM_ADDR(reg) ((volatile unsigned long *)CKSEG1ADDR(CPM_IOBASE+reg))

static inline void mcu_write_reg(int reg, unsigned int value)
{
    *MCU_ADDR(reg) = value;
}

static inline unsigned int mcu_read_reg(int reg)
{
    return *MCU_ADDR(reg);
}

static inline void set_bit_field(int reg, int start, int end, unsigned int value)
{
	unsigned int set_value = 0;

	set_value = readl(reg);
	if(value){
		set_value |= (1 << start);
	}
	else{
		set_value &= ~(1 << start);
	}
	/*1. keep reset*/
	writel(set_value, reg);
}

static inline void mcu_set_bit(int reg, int start, int end, unsigned int value)
{
    set_bit_field(MCU_ADDR(reg), start, end, value);
}

static inline void cpm_write_reg(int reg, unsigned int value)
{
    *CPM_ADDR(reg) = value;
}

static inline unsigned int cpm_read_reg(int reg)
{
    return *CPM_ADDR(reg);
}

struct mcu_data {
    struct clk *clk;
    struct clk *intc_clk;
    int is_up;
    wait_queue_head_t wait;
    int msg_status;
} mcu;

// static DEFINE_SPINLOCK(spinlock);
static DEFINE_MUTEX(m_lock);

static int debug;
module_param(debug, int, 0644);

static void mailbox_enable(void)
{
    unsigned long dmint = mcu_read_reg(DMINT);
    set_bit_field(&dmint, N_IP, 0);
    set_bit_field(&dmint, N_IMSK, 0);
    mcu_write_reg(DMINT, dmint);
}

static irqreturn_t mcu_irq_handler(int irq, void *data)
{
    unsigned long dmint;

    /*1. mask mailbox int*/
    /*2. clear mailbox pending*/
    dmint = mcu_read_reg(DMINT);
    set_bit_field(&dmint, N_IP, 0);
    set_bit_field(&dmint, N_IMSK, 1);
    mcu_write_reg(DMINT, dmint);

    printk(KERN_ERR "received msg: %d\n", mcu_read_reg(DMNMB));
    mcu.msg_status = mcu_read_reg(DMNMB);

    /*3. unmask mailbox int*/
    set_bit_field(&dmint, N_IMSK, 0);
    mcu_write_reg(DMINT, dmint);

    wake_up_interruptible(&mcu.wait);

    return IRQ_HANDLED;
}

static void *to_host_addr(unsigned long addr)
{
    addr -= TCSM_MCU_VIRT_BASE;
    addr += TCSM_IO_BASE;
    return (void *)CKSEG1ADDR(addr);
}

static struct ring_mem *mcu_get_ring_mem_for_host_write(void)
{
    struct mcu_firmware_header *header = (void *) TCSM_ADDR;
    return to_host_addr(header->ring_mem_for_host_write);
}

static struct ring_mem *mcu_get_ring_mem_for_host_read(void)
{
    struct mcu_firmware_header *header = (void *) TCSM_ADDR;
    return to_host_addr(header->ring_mem_for_host_read);
}

static void host_notify_mcu(void)
{
    if (!cpm_read_reg(CPM_SFTINT))
        cpm_write_reg(CPM_SFTINT, 1);
}

static int wait_mcu_wakeup(unsigned long *timeout)
{
    if (!*timeout)
        return 0;

    ktime_t until;
    struct timespec start, ts, now;

	if((*timeout) > 1 * 1000 * 1000){
	    ts.tv_sec = (*timeout) / (1 * 1000 * 1000);
		ts.tv_nsec = (*timeout) % (1 * 1000 * 1000);
	}else{
	    ts.tv_sec = 0;
		ts.tv_nsec = (*timeout);
	}
    until = timespec_to_ktime(ts);

    int ret = wait_event_hrtimeout(mcu.wait, mcu.msg_status, until);
    if (ret < 0) {
        printk(KERN_ERR "mcu: mcu read data timeout\n");
        return -ETIMEDOUT;
    }

    return 0;
}

void mcu_shutdown(void)
{
    mutex_lock(&m_lock);

    mcu_set_bit(DMCS, SW_RST, 1);

    mcu.is_up = 0;

    mutex_unlock(&m_lock);
}
EXPORT_SYMBOL(mcu_shutdown);

void mcu_bootup(void)
{
    mutex_lock(&m_lock);

    mcu.is_up = 1;

    mcu_set_bit(DMCS, SW_RST, 0);

    mailbox_enable();

    mutex_unlock(&m_lock);
}
EXPORT_SYMBOL(mcu_bootup);

void mcu_reset(void)
{
    mutex_lock(&m_lock);

    mcu.is_up = 1;

    mcu_set_bit(DMCS, SW_RST, 1);

    udelay(10);

    mcu_set_bit(DMCS, SW_RST, 0);

    mailbox_enable();

    mutex_unlock(&m_lock);
}
EXPORT_SYMBOL(mcu_reset);

int mcu_write_data(void *buf, unsigned int size, unsigned long timeout)
{
    int w_size;
    int ret = size;
    unsigned long min_timeout = 100;
    unsigned long long now;

    struct ring_mem *ring = mcu_get_ring_mem_for_host_write();

    mutex_lock(&m_lock);

    if (!mcu.is_up) {
        printk(KERN_ERR "mcu: mcu is not up\n");
        ret = -ENODEV;
        goto unlock;
    }

    if (!ring->mem_size) {
        printk(KERN_ERR "mcu: ring mem is not inited\n");
        ret = -EBUSY;
        goto unlock;
    }

    if (!size)
        goto unlock;

    ring_mem_set_virt_addr_for_write(ring, to_host_addr(ring->mem_addr));

    if (timeout && timeout < min_timeout)
        timeout = min_timeout;

    now = local_clock_us();
    while (size) {
        w_size = ring_mem_write(ring, buf, size);
        if (!w_size) {
            if (!timeout)
                break;

            if (local_clock_us() - now > timeout) {
                printk(KERN_ERR "mcu: mcu write mem timeout\n");
                break;
            }
            usleep_range(min_timeout, min_timeout);
        }
        size -= w_size;
        buf += w_size;
    }

    ret = ret - size;
    if (ret)
        host_notify_mcu();

unlock:
    mutex_unlock(&m_lock);

    return ret;
}
EXPORT_SYMBOL(mcu_write_data);

int mcu_read_data(void *buf, unsigned int size, unsigned long timeout)
{
    int r_size;
    int ret = size;
    unsigned long min_timeout = 100;

    struct ring_mem *ring = mcu_get_ring_mem_for_host_read();

    mutex_lock(&m_lock);

    if (!mcu.is_up) {
        printk(KERN_ERR "mcu: mcu is not up\n");
        ret = -ENODEV;
        goto unlock;
    }

    if (!ring->mem_size) {
        printk(KERN_ERR "mcu: ring mem is not inited\n");
        ret = -EBUSY;
        goto unlock;
    }

    if (!size)
        goto unlock;

    ring_mem_set_virt_addr_for_read(ring, to_host_addr(ring->mem_addr));

    if (ring_mem_readable_size(ring))
        mcu.msg_status = 1;

    if (timeout && timeout < min_timeout)
        timeout = min_timeout;

    if (wait_mcu_wakeup(&timeout)) {
        ret = -ETIMEDOUT;
        goto unlock;
    }

    mcu.msg_status = 0;

    while (size) {
        r_size = ring_mem_read(ring, buf, size);
        if (!r_size) {
            if (!timeout)
                break;

            if (wait_mcu_wakeup(&timeout)) {
                printk(KERN_ERR "mcu: mcu read mem timeout\n");
                break;
            }
        }
        size -= r_size;
        buf += r_size;
    }

    ret = ret - size;

unlock:
    mutex_unlock(&m_lock);

    return ret;
}
EXPORT_SYMBOL(mcu_read_data);

#define MCU_MAGIC_NUMBER    'M'

#define MCU_SHUTDOWN      _IO(MCU_MAGIC_NUMBER, 112)
#define MCU_RESET         _IO(MCU_MAGIC_NUMBER, 113)
#define MCU_BOOTUP        _IO(MCU_MAGIC_NUMBER, 114)
#define MCU_WRITE_MEM     _IOW(MCU_MAGIC_NUMBER, 115, void *)
// #define MCU_WRITE_DATA    _IOW(MCU_MAGIC_NUMBER, 116, void *)
// #define MCU_READ_DATA     _IOW(MCU_MAGIC_NUMBER, 117, void *)
#define MCU_WRITE_DATA_TIMEOUT    _IOW(MCU_MAGIC_NUMBER, 118, void *)
#define MCU_READ_DATA_TIMEOUT     _IOW(MCU_MAGIC_NUMBER, 119, void *)

static long mcu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case MCU_SHUTDOWN:
        mcu_shutdown();
        return 0;

    case MCU_BOOTUP:
        mcu_bootup();
        return 0;

    case MCU_RESET:
        mcu_reset();
        return 0;

    case MCU_WRITE_MEM: {
        unsigned long *array = (void *) arg;
        unsigned int offset = array[0];
        void *src = (void *)array[1];
        int len = array[2];
        void *dst = (void *)TCSM_ADDR + offset;

        if (len < 0 || (offset + len) > TCSM_MAX_SIZE) {
            printk(KERN_ERR "mcu: len out of range: %d %d\n", offset, len);
            return -EINVAL;
        }

        memcpy(dst, src, len);

        return 0;
    }

    case MCU_WRITE_DATA_TIMEOUT: {
        unsigned long *array = (void *) arg;
        void *src = (void *)array[0];
        int len = array[1];
        unsigned long timeout_us = array[2];

        return mcu_write_data(src, len, timeout_us);
    }

    case MCU_READ_DATA_TIMEOUT: {
        unsigned long *array = (void *) arg;
        void *dst = (void *)array[0];
        int len = array[1];
        unsigned long timeout_us = array[2];

        return mcu_read_data(dst, len, timeout_us);
    }

    default:
        printk(KERN_ERR "mcu: do not support this cmd: %x\n", cmd);
        break;
    }

    return -ENODEV;
}

static int mcu_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int mcu_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations mcu_misc_fops = {
    .open = mcu_open,
    .release = mcu_release,
    .unlocked_ioctl = mcu_ioctl,
};

static struct miscdevice mcu_mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "mcu",
    .fops = &mcu_misc_fops,
};

static int __init mcu_init(void)
{
    int ret;

    mcu.clk = clk_get(NULL, "gate_pdma");
    if(IS_ERR(mcu.clk)){
		return -1;
	}

    mcu.intc_clk = clk_get(NULL, "gate_intc");
    if(IS_ERR(mcu.intc_clk)){
		return -1;
	}

    clk_prepare_enable(mcu.clk);

    clk_prepare_enable(mcu.intc_clk);

    ret = request_irq(IRQ_MCU, mcu_irq_handler, 0, "mcu", NULL);
    if(ret){
		return -1;
	}

    init_waitqueue_head(&mcu.wait);

    ret = misc_register(&mcu_mdev);
    if(ret){
		return -1;
	}

	printk("MCU init ok!\n");
    return 0;
}
module_init(mcu_init);

static void mcu_exit(void)
{
    misc_deregister(&mcu_mdev);

    free_irq(IRQ_MCU, NULL);

    clk_put(mcu.clk);

    clk_put(mcu.intc_clk);
}
module_exit(mcu_exit);

MODULE_LICENSE("GPL");
