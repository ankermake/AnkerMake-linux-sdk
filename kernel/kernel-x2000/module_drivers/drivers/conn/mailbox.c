#include <dt-bindings/interrupt-controller/mips-irq.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include "ring_mem.h"

#include "clock.c"

/* kernel 只能跑在 CPU0 */

#define CCU_IO_BASE 0x12200000

#define CCU_CCCR 0x0000
#define CCU_CSSR 0x0020
#define CCU_CSRR 0x0040
#define CCU_MSCR 0x0060
#define CCU_MSIR 0x0064
#define CCU_CCR  0x0070
#define CCU_PIPR 0x0100
#define CCU_PIMR 0x0120
#define CCU_MIPR 0x0140
#define CCU_MIMR 0x0160
#define CCU_OIPR 0x0180
#define CCU_OIMR 0x01a0
#define CCU_DIPR 0x01c0
#define CCU_GDIMR 0x01e0
#define CCU_LDIMR(N) (0x0300+(N)*32)
#define CCU_RER  0x0f00
#define CCU_CSLR 0x0fa0
#define CCU_CSAR 0x0fa4
#define CCU_GIMR 0x0fc0
#define CCU_CFCR 0x0fe0
#define CCU_MBR(N) (0x1000+(N)*4)
#define CCU_BCER 0x1f00

#define CCU_ADDR(reg) ((volatile unsigned long *)(KSEG1ADDR(CCU_IO_BASE) + reg))

static inline void ccu_write_reg(int reg, unsigned long value)
{
    *CCU_ADDR(reg) = value;
}

static inline unsigned long ccu_read_reg(int reg)
{
    return *CCU_ADDR(reg);
}

static inline void ccu_spin_unlock(unsigned int id)
{
    unsigned long cslr;

    cslr = ccu_read_reg(CCU_CSLR);
    if ((cslr & BIT(31)) && ((cslr & 0x7fffffff) == id))
        ccu_write_reg(CCU_CSLR, 0);

}

static unsigned long flags;

static void ccu_spin_lock_critical(unsigned int id)
{
    unsigned long cslr;

    local_irq_save(flags);

    while (1) {
        ccu_write_reg(CCU_CSAR, id);
        cslr = ccu_read_reg(CCU_CSLR);
        if ((cslr & BIT(31)) && ((cslr & 0x7fffffff) == id))
            break;
    }
}

static void ccu_spin_unlock_critical(unsigned int id)
{
    unsigned long cslr;

    cslr = ccu_read_reg(CCU_CSLR);
    if ((cslr & BIT(31)) && ((cslr & 0x7fffffff) == id))
        ccu_write_reg(CCU_CSLR, 0);

    local_irq_restore(flags);
}

static wait_queue_head_t mailbox_send_wait;
static wait_queue_head_t mailbox_receive_wait;

struct ring_mem *write_to_rtos;
struct ring_mem *read_from_rtos;

static DEFINE_MUTEX(send_mutex);
static DEFINE_MUTEX(receive_mutex);

#define LINUX_CPU_ID    0
#define RTOS_CPU_ID     1

#define MAILBOX_PACK_HEADER     0x12345677
#define MAILBOX_PACK_RECV_DATA  0x14753125
#define MAILBOX_PACK_TAIL       0x14753123

static void mailbox_notify(void)
{
    unsigned long mbr;
    unsigned int id = LINUX_CPU_ID;

    ccu_spin_lock_critical(id);

    mbr = ccu_read_reg(CCU_MBR(id));
    if (mbr == 0)
        ccu_write_reg(CCU_MBR(RTOS_CPU_ID), id + 1);

    ccu_spin_unlock_critical(id);
}

static int mailbox_send(const void *buf, int size, int timeout_ms)
{
    int ret;
    int send_len;
    int total_size = size;

    uint64_t now;

    mutex_lock(&send_mutex);

    while (size) {
        now = conn_local_clock_ms();

        send_len = ring_mem_write(write_to_rtos, (void *)buf, size);
        if (!send_len) {
            ret = wait_event_timeout(mailbox_send_wait,
                                ring_mem_writable_size(write_to_rtos),
                                msecs_to_jiffies(timeout_ms));
            if (!ret) {
                send_len = total_size - size;
                goto unlock;
            }
        }

        size -= send_len;
        buf += send_len;

        timeout_ms = timeout_ms - (conn_local_clock_ms() - now);
        if (timeout_ms < 0)
            timeout_ms = 0;
    }

unlock:
    mutex_unlock(&send_mutex);

    if (send_len)
        mailbox_notify();

    return send_len;
}

static int mailbox_receive(void *buf, int size, int timeout_ms)
{
    int ret;
    int receive_len;
    int total_size = size;

    uint64_t now;

    mutex_lock(&receive_mutex);

    while (size) {
        now = conn_local_clock_ms();

        receive_len = ring_mem_read(read_from_rtos, buf, size);
        if (!receive_len) {
            ret = wait_event_timeout(mailbox_receive_wait,
                                            ring_mem_readable_size(read_from_rtos),
                                            msecs_to_jiffies(timeout_ms));
            if (!ret) {
                receive_len = total_size - size;
                goto unlock;
            }
        }

        size -= receive_len;
        buf += receive_len;

        timeout_ms = timeout_ms - (conn_local_clock_ms() - now);
        if (timeout_ms < 0)
            timeout_ms = 0;
    }

unlock:
    mutex_unlock(&receive_mutex);

    if (receive_len)
        mailbox_notify();

    return receive_len;
}

static irqreturn_t mailbox_irq_handler(int irq, void *data)
{
    unsigned int linux_cpu_id = LINUX_CPU_ID;

    ccu_spin_lock_critical(linux_cpu_id);

    ccu_write_reg(CCU_MBR(linux_cpu_id), 0);

    ccu_spin_unlock_critical(linux_cpu_id);

    if (ring_mem_writable_size(write_to_rtos))
        wake_up(&mailbox_send_wait);

    if (ring_mem_readable_size(read_from_rtos))
        wake_up(&mailbox_receive_wait);

    return IRQ_HANDLED;
}

static void mailbox_communication_prepare(void)
{
    int ret;
    unsigned long read_addr;
    unsigned long write_addr;
    unsigned long tail_addr;
    int linux_cpu_id = LINUX_CPU_ID;
    int rtos_cpu_id = RTOS_CPU_ID;
    unsigned long timeout_us = 300;
    unsigned long timeout = timeout_us * 1000 * 10;

    uint64_t start_time = conn_local_clock_us();

    /* 发送包头信息， 并等待回复包头信息 */
    ccu_write_reg(CCU_MBR(rtos_cpu_id), MAILBOX_PACK_HEADER);
    while (1) {
        ret = ccu_read_reg(CCU_MBR(linux_cpu_id));
        if (ret == MAILBOX_PACK_HEADER)
            break;

        if (conn_local_clock_us() - start_time > timeout) {
            start_time = conn_local_clock_us();
            printk("MAILBOX: CPU%d: read faild\n", linux_cpu_id);
        }

        usleep_range(timeout_us, timeout_us);
    }

    while (1) {
        /* 接收邮箱信息 */
        ret = ccu_read_reg(CCU_MBR(linux_cpu_id));
        if (ret != MAILBOX_PACK_HEADER)
            break;

        usleep_range(timeout_us, timeout_us);
    }

    /* 将邮箱收到的数据信息写回CPU1表示确认收到 */
    read_addr = *(unsigned long *)ret;
    write_addr = *((unsigned long *)ret + 1);
    tail_addr = *((unsigned long *)ret + 2);

    read_from_rtos = (struct ring_mem *)read_addr;
    write_to_rtos = (struct ring_mem *)write_addr;

    /* 发送 MAILBOX_PACK_RECV_DATA 表示缓冲区的相关信息传输完成 */
    ccu_write_reg(CCU_MBR(rtos_cpu_id), MAILBOX_PACK_RECV_DATA);
    while (1) {
        /* 接收邮箱信息 */
        ret = ccu_read_reg(CCU_MBR(linux_cpu_id));
        if (ret == MAILBOX_PACK_RECV_DATA)
            break;

        usleep_range(timeout_us, timeout_us);
    }

    /* 告诉 CPU1 MAILBOX 准备完成 */
    *(unsigned int *)tail_addr = MAILBOX_PACK_TAIL;
}

static int mailbox_init(void)
{
    int ret;

    mailbox_communication_prepare();

    init_waitqueue_head(&mailbox_send_wait);
    init_waitqueue_head(&mailbox_receive_wait);

    ret = request_percpu_irq(CORE_MAILBOX_IRQ, mailbox_irq_handler, "mailbox", "no use but can't be null");
    if (ret)
        panic("mailbox: failed to request irq: %d\n", ret);

    ccu_write_reg(CCU_MIMR, 3 << 0);
    enable_percpu_irq(CORE_MAILBOX_IRQ, IRQ_TYPE_NONE);

    return 0;
}
