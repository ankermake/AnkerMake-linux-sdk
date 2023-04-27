#include <linux/module.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <soc/base.h>
#include <bit_field.h>
#include <linux/miscdevice.h>

#include <utils/clock.h>

#define DTRNG_CFG       0x00
#define DTRNG_RANDOMNUM 0x04
#define DTRNG_STAT      0x08

#define DTRNGCFG_Reserved   26, 31
#define DTRNGCFG_LINE_EN    16, 25
#define DTRNGCFG_Reserved1  13, 15
#define DTRNGCFG_RDY_CLR    12, 12
#define DTRNGCFG_INT_MASK   11, 11
#define DTRNGCFG_DIV_NUM    1,  10
#define DTRNGCFG_GEN_EN     0,  0

#define DTRNGSTAT_Reserved0         1, 31
#define DTRNGSTAT_RANDOM_RDY        0, 0

#define DTRNG_DIV   0
#define DTRNG_ENABLE_ALL_INVERTER   0x3ff
#define IRQ_DTRNG       IRQ_INTC_BASE + 34

#define DTRNG_REG_BASE  0xB0072000
#define DTRNG_ADDR(reg) ((volatile unsigned long *)(DTRNG_REG_BASE + reg))

struct dtrng_device
{
    struct clk* clk;
    struct mutex mutex;
    wait_queue_head_t wq;
    unsigned int wq_status;
    struct miscdevice *mdev;
};

static struct dtrng_device dtrng_dev;

static inline void dtrng_write_reg(unsigned int reg, unsigned int value)
{
    *DTRNG_ADDR(reg) = value;
}

static inline unsigned int dtrng_read_reg(unsigned int reg)
{
    return *DTRNG_ADDR(reg);
}

static inline void dtrng_set_bits(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(DTRNG_ADDR(reg), start, end, val);
}

static inline unsigned int dtrng_get_bits(unsigned int reg, int start, int end)
{
    return get_bit_field(DTRNG_ADDR(reg), start, end);
}

/****************************************************************/

static void dtrng_enable(void)
{
    dtrng_set_bits(DTRNG_CFG, DTRNGCFG_GEN_EN, 1);
}

static void dtrng_disable(void)
{
    dtrng_set_bits(DTRNG_CFG, DTRNGCFG_GEN_EN, 0);
}

static void dtrng_enable_irq(void)
{
    dtrng_set_bits(DTRNG_CFG, DTRNGCFG_INT_MASK, 0);
}

static void dtrng_disable_irq(void)
{
    dtrng_set_bits(DTRNG_CFG, DTRNGCFG_INT_MASK, 1);
}

static void dtrng_clean_irq_flag(void)
{
    dtrng_set_bits(DTRNG_CFG, DTRNGCFG_RDY_CLR, 1);
    dtrng_set_bits(DTRNG_CFG, DTRNGCFG_RDY_CLR, 0);
}

static void jz_dtrng_enable(void)
{
    dtrng_enable();
    dtrng_clean_irq_flag();
}


static void jz_dtrng_disable(void)
{
    dtrng_disable();
    dtrng_clean_irq_flag();
}

static int dtrng_read_random_data(unsigned int* value)
{
    int ret;

    dtrng_dev.wq_status = 1;

    dtrng_enable_irq();

    ret = wait_event_interruptible_timeout(dtrng_dev.wq, !dtrng_dev.wq_status, msecs_to_jiffies(20));
    if (ret <= 0) {
        printk(KERN_ERR "get random data timeout!\n");
        return -EBUSY;
    }

    *value = dtrng_read_reg(DTRNG_RANDOMNUM);

    return ret;
}

static irqreturn_t dtrng_irq_handler(int irq, void *data)
{
    dtrng_disable_irq();
    dtrng_clean_irq_flag();

    if (dtrng_dev.wq_status) {
        dtrng_dev.wq_status = 0;
        wake_up_interruptible(&dtrng_dev.wq);
    }

    return IRQ_HANDLED;
}


static int dtrng_open(struct inode *inode, struct file *filp)
{
    mutex_lock(&dtrng_dev.mutex);

    jz_dtrng_enable();

    mutex_unlock(&dtrng_dev.mutex);

    return 0;
}

static int dtrng_release(struct inode *inode, struct file *filp)
{
    mutex_lock(&dtrng_dev.mutex);

    jz_dtrng_disable();

    mutex_unlock(&dtrng_dev.mutex);

    return 0;
}

static ssize_t dtrng_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
    int ret;
    unsigned int dtrng_val;

    mutex_lock(&dtrng_dev.mutex);

    ret = dtrng_read_random_data(&dtrng_val);
    if (ret < 0) {
        mutex_unlock(&dtrng_dev.mutex);
        return ret;
    }

    mutex_unlock(&dtrng_dev.mutex);

    if (copy_to_user(buf, &dtrng_val, sizeof(int)))
        return -EFAULT;

    return sizeof(int);
}

static struct file_operations dtrng_fops = {
    .owner   = THIS_MODULE,
    .open    = dtrng_open,
    .read    = dtrng_read,
    .release = dtrng_release,
};

static struct miscdevice dtrng_mdev = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "jz_dtrng",
    .fops   = &dtrng_fops,
};

static int __init jz_dtrng_init(void)
{
    int ret;

    mutex_init(&dtrng_dev.mutex);
    init_waitqueue_head(&dtrng_dev.wq);

    dtrng_dev.clk = clk_get(NULL, "gate_dtrng");
    if (IS_ERR(dtrng_dev.clk)) {
        printk(KERN_ERR "get dtrng clk fail!\n");
        return -1;
    }

    clk_prepare_enable(dtrng_dev.clk);

    dtrng_set_bits(DTRNG_CFG, DTRNGCFG_LINE_EN, DTRNG_ENABLE_ALL_INVERTER);
    dtrng_set_bits(DTRNG_CFG, DTRNGCFG_DIV_NUM, DTRNG_DIV);

    dtrng_dev.mdev = &dtrng_mdev;
    ret = misc_register(&dtrng_mdev);
    BUG_ON(ret < 0);

    ret = request_irq(IRQ_DTRNG, dtrng_irq_handler, 0, "dtrng", NULL);
    BUG_ON(ret);

    return 0;
}
module_init(jz_dtrng_init);

static void __exit jz_dtrng_exit(void)
{
    misc_deregister(&dtrng_mdev);
    free_irq(IRQ_DTRNG, NULL);
    clk_disable_unprepare(dtrng_dev.clk);
    clk_put(dtrng_dev.clk);
}
module_exit(jz_dtrng_exit);

MODULE_DESCRIPTION("X2000 SoC DTRNG driver");
MODULE_LICENSE("GPL");