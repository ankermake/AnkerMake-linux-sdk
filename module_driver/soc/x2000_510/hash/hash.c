#include <linux/module.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <assert.h>
#include <bit_field.h>
#include "hash_regs.h"

#define CMD_hash_init        _IOWR('h', 0, int)
#define CMD_hash_write       _IOWR('h', 1, unsigned long)
#define CMD_hash_deinit      _IOWR('h', 2, unsigned long)

#define HASH_IOBASE      0x13470000
#define HASH_ADDR(reg)   ((volatile unsigned long *)((KSEG1ADDR(HASH_IOBASE)) + (reg)))
#define IRQ_HASH         (IRQ_INTC_BASE + 22)

enum encryption_mode {
    MD5,
    SHA1,
    SHA224,
    SHA256,
};

enum dma_state {
    DMA_DONE,
    DMA_TRANSMIT,
};

struct hash_device {
    wait_queue_head_t wait;
    struct clk* clk;
    unsigned int *mem;
    struct miscdevice *mdev;
    enum dma_state dma_state;
    struct mutex hash_write;
    struct mutex hash_work;
} jz_hash;

struct data_attr {
    enum encryption_mode mode;
    int total_input_size;
    int output_size;
    int buffer_size;
    char buffer[64];
} hash_data;


static inline void jz_hash_write_reg(unsigned int reg, unsigned int value)
{
    *HASH_ADDR(reg) = value;
}

static inline unsigned int jz_hash_read_reg(unsigned int reg)
{
    return *HASH_ADDR(reg);
}

static inline void jz_hash_set_bit(unsigned int reg, int start, int end, unsigned int value)
{
    set_bit_field(HASH_ADDR(reg), start, end, value);
}

static inline unsigned int jz_hash_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(HASH_ADDR(reg), start, end);
}

static inline void *m_dma_alloc_coherent(struct device *dev, int dma_size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(dev, dma_size, &dma_handle, GFP_KERNEL);
    assert(mem);

    return (void *)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(struct device *dev, void *mem, int dma_size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    dma_free_coherent(dev, dma_size, (void *)CKSEG1ADDR(mem), dma_handle);
}

static inline void m_cache_sync(void *mem, int dma_size)
{
    dma_cache_wback((unsigned long)mem, dma_size);
}

static void hash_set_dma_addr(void *virt_address)
{
    unsigned int phys_address = virt_to_phys(virt_address);
    jz_hash_write_reg(HSSA, phys_address);
}

static void md5_init_setting(void)
{
    jz_hash_set_bit(HSCR, HSCR_SEL, 0);
    jz_hash_set_bit(HSCR, HSCR_DORVS, 1);
    jz_hash_set_bit(HSCR, HSCR_DIRVS, 0);
    jz_hash_set_bit(HSCR, HSCR_DMAE, 1);
    jz_hash_set_bit(HSCR, HSCR_EN, 1);
}

static void sha1_init_setting(void)
{
    jz_hash_set_bit(HSCR, HSCR_SEL, 1);
    jz_hash_set_bit(HSCR, HSCR_DORVS, 0);
    jz_hash_set_bit(HSCR, HSCR_DIRVS, 1);
    jz_hash_set_bit(HSCR, HSCR_DMAE, 1);
    jz_hash_set_bit(HSCR, HSCR_EN, 1);
}

static void sha224_init_setting(void)
{
    jz_hash_set_bit(HSCR, HSCR_SEL, 2);
    jz_hash_set_bit(HSCR, HSCR_DORVS, 0);
    jz_hash_set_bit(HSCR, HSCR_DIRVS, 1);
    jz_hash_set_bit(HSCR, HSCR_DMAE, 1);
    jz_hash_set_bit(HSCR, HSCR_EN, 1);
}

static void sha256_init_setting(void)
{
    jz_hash_set_bit(HSCR, HSCR_SEL, 3);
    jz_hash_set_bit(HSCR, HSCR_DORVS, 0);
    jz_hash_set_bit(HSCR, HSCR_DIRVS, 1);
    jz_hash_set_bit(HSCR, HSCR_DMAE, 1);
    jz_hash_set_bit(HSCR, HSCR_EN, 1);
}

static unsigned int swap_endian(unsigned int value)
{
    value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
    return (value << 16) | (value >> 16);
}

static void hash_data_dma_transform(unsigned char *input)
{
    int ret;
    jz_hash_write_reg(HSTC, 1);

    memcpy(jz_hash.mem, input, 64);
    m_cache_sync(jz_hash.mem, 64);

    jz_hash_set_bit(HSINTM, HSINTM_MR_INT_M, 1);
    jz_hash_set_bit(HSCR, HSCR_DMAS, 1);
    jz_hash.dma_state = DMA_TRANSMIT;

    ret = wait_event_timeout(jz_hash.wait, jz_hash.dma_state == DMA_DONE, msecs_to_jiffies(100));
    if (!ret)
        printk(KERN_ERR "hash data transmit timeout\n");
    hash_data.total_input_size += 64;
}

static void hash_data_transform_end(unsigned char *input, unsigned int input_size)
{
    unsigned int bits0, bits1, temp;
    unsigned char dma_array[64];
    unsigned int total_size = hash_data.total_input_size + input_size;

    memcpy(dma_array, input, input_size);
    dma_array[input_size] = 0x80;
    input_size = input_size + 1;

    memset(dma_array+input_size, 0, 64-input_size);
    if (input_size > 56) {
        hash_data_dma_transform(dma_array);
        memset(dma_array, 0, 64);
    }

    bits0 = total_size << 3;
    bits1 = total_size >> 29;
    if (hash_data.mode != MD5) {
        bits0 = swap_endian(bits0);
        bits1 = swap_endian(bits1);
        temp = bits0;
        bits0 = bits1;
        bits1 = temp;
    }
    memcpy(dma_array+56, &bits0, 4);
    memcpy(dma_array+60, &bits1, 4);
    hash_data_dma_transform(dma_array);
}

static irqreturn_t hash_irq_handler(int irq, void *data)
{
    jz_hash_set_bit(HSSR, HSSR_MRD, 1);
    jz_hash.dma_state = DMA_DONE;
    wake_up(&jz_hash.wait);

    return IRQ_HANDLED;
}

static void jz_hash_encryption_deinit(void)
{
    jz_hash_set_bit(HSCR, HSCR_DMAE, 0);
    jz_hash_set_bit(HSCR, HSCR_EN, 0);
    clk_disable_unprepare(jz_hash.clk);
}

static int jz_hash_encryption_init(void)
{
    clk_prepare_enable(jz_hash.clk);
    hash_data.total_input_size = 0;

    switch(hash_data.mode) {
        case MD5:
            md5_init_setting();
            hash_data.output_size = 16;
            break;
        case SHA1:
            sha1_init_setting();
            hash_data.output_size = 20;
            break;
        case SHA224:
            sha224_init_setting();
            hash_data.output_size = 28;
            break;
        case SHA256:
            sha256_init_setting();
            hash_data.output_size = 32;
            break;
        default:
            printk(KERN_ERR "not support this mode\n");
            jz_hash_encryption_deinit();
            return -1;
    }
    hash_set_dma_addr(jz_hash.mem);
    return 0;
}

static void jz_hash_encryption_write(unsigned char *input, unsigned int input_size)
{
    int remain_size;
    if (hash_data.buffer_size) {
        remain_size = 64 - hash_data.buffer_size;
        if (input_size >= remain_size) {
            memcpy(hash_data.buffer + hash_data.buffer_size, input, remain_size);
            hash_data.buffer_size = 0;
            hash_data_dma_transform(hash_data.buffer);
            input += remain_size;
            input_size -= remain_size;
        }
    }

    while (input_size >= 64) {
        hash_data_dma_transform(input);
        input += 64;
        input_size -= 64;
    }

    if (input_size) {
        memcpy(hash_data.buffer + hash_data.buffer_size, input, input_size);
        hash_data.buffer_size += input_size;
    }
}

static void jz_hash_encryption_read(unsigned int *result)
{
    int i;
    hash_data_transform_end(hash_data.buffer, hash_data.buffer_size);
    hash_data.buffer_size = 0;
    jz_hash_set_bit(HSCR, HSCR_INIT, 1);

    for (i = 0; i < hash_data.output_size / 4; i++) {
        result[i] = jz_hash_read_reg(HSDO);
        result[i] = swap_endian(result[i]);
    }
}

static int hash_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int hash_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long hash_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    switch (cmd) {
    case CMD_hash_init: {
        mutex_lock(&jz_hash.hash_work);
        hash_data.mode = (enum encryption_mode)arg;

        ret = jz_hash_encryption_init();
        if (ret) {
            printk(KERN_ERR "HASH: hash_init failed\n");
            mutex_unlock(&jz_hash.hash_work);
            return -1;
        }
        return 0;
    }

    case CMD_hash_write: {
        mutex_lock(&jz_hash.hash_write);
        unsigned long data[2];
        ret = copy_from_user(data, (void *)arg, sizeof(data));
        if (ret) {
            printk(KERN_ERR "HASH: copy_from_user failed\n");
            mutex_unlock(&jz_hash.hash_write);
            return -1;
        }

        unsigned char *input = (unsigned char *)data[0];
        unsigned int input_size = (unsigned int)data[1];
        jz_hash_encryption_write(input, input_size);
        mutex_unlock(&jz_hash.hash_write);
        return 0;
    }

    case CMD_hash_deinit: {
        unsigned long data[2];
        unsigned int result[16];
        ret = copy_from_user(data, (void *)arg, sizeof(data));
        if (ret) {
            printk(KERN_ERR "HASH: copy_from_user failed\n");
            mutex_unlock(&jz_hash.hash_work);
            return -1;
        }

        unsigned char *output = (unsigned char *)data[0];
        unsigned int output_size = (unsigned int)data[1];
        jz_hash_encryption_read(result);

        ret = copy_to_user(output, result, output_size);
        if (ret) {
            printk(KERN_ERR "HASH: copy_to_user failed\n");
            mutex_unlock(&jz_hash.hash_work);
            return -1;
        }

        jz_hash_encryption_deinit();
        mutex_unlock(&jz_hash.hash_work);
        return 0;
    }

    default:
        printk(KERN_ERR "HASH: not support this cmd: %x\n", cmd);
        return -EINVAL;
    }
}

static struct file_operations hash_fops = {
    .owner            = THIS_MODULE,
    .open             = hash_open,
    .release          = hash_release,
    .unlocked_ioctl   = hash_ioctl,
};

static struct miscdevice hash_mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "jz_hash",
    .fops  = &hash_fops,
};

static int ingenic_hash_probe(struct platform_device *pdev)
{
    int ret;
    mutex_init(&jz_hash.hash_write);
    mutex_init(&jz_hash.hash_work);
    init_waitqueue_head(&jz_hash.wait);

    jz_hash.clk = clk_get(NULL, "gate_hash");
    if (!jz_hash.clk) {
        printk(KERN_ERR "get hash clk fail!\n");
        return -1;
    }

    jz_hash.mdev = &hash_mdev;
    ret = misc_register(jz_hash.mdev);
    if (ret) {
        printk(KERN_ERR "misc register fail\n");
        return -1;
    }

    ret = request_irq(IRQ_HASH, hash_irq_handler, 0, "hash", NULL);
    if (ret) {
        printk(KERN_ERR "request irq fail!\n");
        return -1;
    }

    jz_hash.mem = m_dma_alloc_coherent(&pdev->dev, 64);
    return 0;
}

static int ingenic_hash_remove(struct platform_device *pdev)
{
    free_irq(IRQ_HASH, NULL);
    misc_deregister(jz_hash.mdev);
    clk_put(jz_hash.clk);
    m_dma_free_coherent(&pdev->dev, jz_hash.mem, 64);

    return 0;
}

static struct platform_driver ingenic_hash_driver = {
    .probe = ingenic_hash_probe,
    .remove = ingenic_hash_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ingenic-hash",
    },
};

/* stop no dev release warning */
static void jz_hash_dev_release(struct device *dev){}

struct platform_device ingenic_hash_device = {
    .name = "ingenic-hash",
    .dev  = {
        .release = jz_hash_dev_release,
    },
};

static int __init jz_hash_init(void)
{
    int ret = platform_device_register(&ingenic_hash_device);
    if (ret)
        return ret;

    return platform_driver_register(&ingenic_hash_driver);
}
module_init(jz_hash_init);

static void __exit jz_hash_exit(void)
{
    platform_device_unregister(&ingenic_hash_device);

    platform_driver_unregister(&ingenic_hash_driver);
}
module_exit(jz_hash_exit);

MODULE_DESCRIPTION("X2000 Soc HASH driver");
MODULE_LICENSE("GPL");