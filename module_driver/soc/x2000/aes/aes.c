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
#include <linux/string.h>
#include <bit_field.h>
#include "aes_hal.h"

#define CMD_aes_get_key         _IOWR('a', 0, void *)
#define CMD_aes_transfer_data   _IOWR('a', 1, void *)

#define IRQ_AES         (IRQ_INTC_BASE + 23)

#define MCU_BOOT        0xb3422000
#define DMCS            0xb3421030
#define boot_up_mcu()   *(volatile unsigned int *)(DMCS) = 0;
#define reset_mcu()     *(volatile unsigned int *)(DMCS) = 1;

/* must align with 16 bytes */
#define AES_DMA_LEN     1024
#define AES_ALIGN_LEN   16
#define AES_ALIGN(d,a)  (((d)+((a)-1))/(a)*(a))

struct aes_config {
    enum aes_keyl keyl;     /* 密钥长度: 0:128 1:196 2:256 */
    enum aes_mode mode;     /* 编解码是否前后相关: 0:ecb 1:cbc */
    enum aes_endian endian; /* 编解码数据输入大小端模式: 0:little 1:big */
    unsigned char ukey[33]; /* 用户传入的密钥 */
    unsigned char iv[17];   /* 初始化向量(cbc模式使用) */
};

struct aes_dev {
    struct aes_config *config;  /* 密钥长度,编解码模式,大小端,密钥,初始化向量 */
    enum aes_dece dece;         /* 编码(0)/解码(1) */
    unsigned int *key;          /* 转化完成密钥存放地址 */
    unsigned char *src;         /* 需要加解密的数据地址 */
    unsigned char *dst;         /* 加解密完成数据存放地址 */
    unsigned int enable_dma;    /* 默认使用dma模式 */
    unsigned int tc;            /* dma模式下传输次数, 每次传输128位 */
};

enum aes_state {
    aes_IDLE,
    aes_BUSY,
};

struct jz_aes_drv {
    int irq;
    struct clk* clk;

    struct mutex aes_mutex;
    wait_queue_head_t dma_wait;
    enum aes_state dma_state;   /* 0: idle 1: busy */
    struct miscdevice aes_mdev;
    struct aes_dev dev;
};

static struct jz_aes_drv aes_drv;

static inline void *m_dma_alloc_coherent(int dma_size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(NULL, dma_size, &dma_handle, GFP_KERNEL);
    assert(mem);

    return (void*)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(void *mem, int dma_size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    dma_free_coherent(NULL, dma_size, (void *)CKSEG1ADDR(mem), dma_handle);
}

static unsigned int aes_set_configs(struct aes_dev *dev)
{
    aes_hal_enable_encrypt();

    aes_hal_mask_all_interrupt();
    aes_hal_clear_all_done_status();

    aes_hal_set_page_size(0x3); /* keep default */

    if (dev->dece != ENCRYPTION && dev->dece != DECRYPTION) {
        printk(KERN_ERR "AES: failed to get dece! dece: %d\n", dev->dece);
        return -1;
    }

    if (dev->config->mode != ECB_MODE && dev->config->mode != CBC_MODE) {
        printk(KERN_ERR "AES: failed to get mode(ecb/cbc)! mode: %d\n", dev->config->mode);
        return -1;
    }

    if (dev->config->endian != ENDIAN_LITTLE && dev->config->endian != ENDIAN_BIG) {
        printk(KERN_ERR "AES: failed to get mode(ecb/cbc)! mode: %d\n", dev->config->mode);
        return -1;
    }

    aes_hal_set_data_input_endian(!!dev->config->endian);
    aes_hal_select_dece(!!dev->dece);
    aes_hal_select_mode(!!dev->config->mode);
    aes_hal_set_key_length(dev->config->keyl & 0x3);

    aes_hal_clear_iv_keys();

    if (dev->enable_dma) {
        aes_hal_enable_dma();
        aes_hal_set_dma_src_addr(dev->src);
        aes_hal_set_dma_dst_addr(dev->dst);
        aes_hal_set_transfer_count(dev->tc);
    }

    return 0;
}

inline unsigned int aes_get_key_bits(enum aes_keyl keyl)
{
    switch (keyl)
    {
    case AES128:
        return 128;

    case AES192:
        return 192;

    case AES256:
        return 256;

    default:
        printk(KERN_ERR "AES: get key bits failed! keyl: %d\n", keyl);
        break;
    }
    return 0;
}

/* please ensure the key_length should conrresponding to keyl */
int AES_get_key_expansion(unsigned char *ukey, enum aes_keyl keyl, unsigned int *key, enum aes_endian endian)
{
    if (ukey == NULL || key == NULL) {
        printk(KERN_ERR "AES: key_expansion get ukey/key failed!\n");
        return -1;
    }

    if (keyl < AES128 || keyl > AES256) {
        printk(KERN_ERR "AES: key_expansion get keyl failed!\n");
        return -2;
    }

    int i = 0;
    for (i = 0; i < aes_get_key_bits(keyl) / 8 / 4; i++)
        key[i] = GETU32(ukey + 4 * i, endian);

    return 0;
}

void aes_set_key_expansion(unsigned int *key, enum aes_keyl keyl)
{
    aes_hal_set_keys(key, keyl);

    aes_hal_start_key_expansion();

    while(!aes_hal_get_key_expansion_done_status());

    aes_hal_clear_key_expansion_done_status();
}

unsigned int AES_get_aes_dataout(unsigned char *src, unsigned char *dst)
{
    if (aes_drv.dev.enable_dma) {
        aes_hal_enable_dma();
        aes_hal_start_dma();

        aes_drv.dma_state = aes_BUSY;
        aes_hal_enable_dma_done_interrupt();

        unsigned int timeout = wait_event_timeout(aes_drv.dma_wait, aes_drv.dma_state == aes_IDLE, HZ);
        aes_hal_mask_dma_done_interrupt();
        if (timeout == 0) {
            printk(KERN_ERR "AES: aes encrypt/decrypt dma timeout\n");
            return -1;
        }

        aes_hal_clear_dma_done_status();
        aes_hal_disable_dma();
    } else {
        aes_hal_data_input(src, aes_drv.dev.config->endian);

        aes_hal_start_encrypt();
        while(!aes_hal_get_aes_done_status());

        aes_hal_clear_aes_done_status();
        aes_hal_data_output(dst, aes_drv.dev.config->endian);
    }

    return 0;
}

unsigned int AES_transfer_data(struct aes_dev *dev)
{
    clk_prepare_enable(aes_drv.clk);

    int ret = aes_set_configs(dev);
    if (ret < 0)
        goto err;

    if (dev->config->mode == CBC_MODE) {
        aes_hal_set_iv(dev->config->iv, dev->config->endian);
        aes_hal_initial_iv();
    }

    aes_set_key_expansion(dev->key, dev->config->keyl);

    ret = AES_get_aes_dataout(dev->src, dev->dst);

err:
    aes_hal_disable_encrypt();

    clk_disable_unprepare(aes_drv.clk);

    return ret;
}

static irqreturn_t aes_intr_handler(int irq, void *dev)
{
    unsigned int status = aes_hal_get_all_done_status();
    unsigned int mask = aes_hal_get_all_interrupt();
    aes_hal_clear_all_done_status();
    status = status & mask;

    /* dma_done_status */
    if (status & 0x4) {
        aes_drv.dma_state = aes_IDLE;
        wake_up(&aes_drv.dma_wait);
    }

    return IRQ_HANDLED;
}

static int aes_open(struct inode *inode, struct file *filp)
{
    struct jz_aes_drv *drv = container_of(filp->private_data,
            struct jz_aes_drv, aes_mdev);

    filp->private_data = &drv->dev;

    return 0;
}

static int aes_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long aes_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned long *array = (void *)arg;

    switch (cmd)
    {
    case CMD_aes_get_key: {
        unsigned char *ukey = (unsigned char *)array[0];
        unsigned int *key = (unsigned int *)array[1];
        enum aes_keyl keyl = array[2];
        enum aes_endian endian = array[3];

        ret = AES_get_key_expansion(ukey, keyl, key, endian);
        break;
    }

    case CMD_aes_transfer_data: {
        mutex_lock(&aes_drv.aes_mutex);
        struct aes_dev *dev = (struct aes_dev *)filp->private_data;
        dev->config = (struct aes_config *)array[0];
        dev->dece = array[1];
        dev->key = (unsigned int *)array[2];
        unsigned char *src = (unsigned char *)array[3];
        unsigned char *dst = (unsigned char *)array[4];
        int len = array[5];

        unsigned int N = dev->enable_dma ? AES_DMA_LEN : AES_ALIGN_LEN;

        while (len) {
            int n = len > N ? N : len;
            int size = AES_ALIGN(n, AES_ALIGN_LEN);
            dev->tc = size / AES_ALIGN_LEN;

            memcpy(dev->src, src, n);
            if (size - n)
                memset(&dev->src[n], 0, size - n);
            dma_cache_sync(NULL, dev->src, size, DMA_TO_DEVICE);
            dma_cache_sync(NULL, dev->dst, size, DMA_FROM_DEVICE);

            ret = AES_transfer_data(dev);
            if (ret < 0) {
                printk(KERN_ERR "AES: AES_transfer_data failed\n");
                break;
            }

            if (dev->config->mode == CBC_MODE) {
                if (dev->dece == ENCRYPTION)
                    memcpy(dev->config->iv, &dev->dst[size-AES_ALIGN_LEN], AES_ALIGN_LEN);
                else if (dev->dece == DECRYPTION)
                    memcpy(dev->config->iv, &dev->src[size-AES_ALIGN_LEN], AES_ALIGN_LEN);
            }

            memcpy(dst, dev->dst, size);
            len -= n;
            src += n;
            dst += size;
        }
        mutex_unlock(&aes_drv.aes_mutex);
        break;
    }

    default: {
        printk(KERN_ERR "AES: do not support this cmd: %x\n", cmd);
        ret = -EINVAL;
    }
    }

    return ret;
}

static struct file_operations aes_fops = {
    .owner          = THIS_MODULE,
    .open           = aes_open,
    .release        = aes_release,
    .unlocked_ioctl = aes_ioctl,
};

static noinline void pdma_wait(void)
{
	__asm__ volatile (
		"	.set	push		\n\t"
		"	.set	noreorder	\n\t"
		"	.set	mips32		\n\t"
		"	li	$26, 0		\n\t"
		"	mtc0	$26, $12	\n\t"
		"	nop			\n\t"
		"1:				\n\t"
		"	wait			\n\t"
		"	b	1b		\n\t"
		"	nop			\n\t"
		"	.set	reorder		\n\t"
		"	.set	pop		\n\t"
		);
}

static int __init jz_aes_init(void)
{
    int ret;
    struct jz_aes_drv *drv = &aes_drv;
    memset(drv, 0, sizeof(struct jz_aes_drv));

    init_waitqueue_head(&drv->dma_wait);

    drv->clk = clk_get(NULL, "gate_aes");
    BUG_ON(IS_ERR(drv->clk));

    clk_prepare_enable(drv->clk);

    reset_mcu();
    memcpy((void*)MCU_BOOT, pdma_wait, 64);
    boot_up_mcu();

    mutex_init(&drv->aes_mutex);

    drv->aes_mdev.minor = MISC_DYNAMIC_MINOR;
    drv->aes_mdev.name  = "jz_aes";
    drv->aes_mdev.fops  = &aes_fops;

    ret = misc_register(&drv->aes_mdev);
    if (ret < 0) {
        printk(KERN_ERR "AES: %s register err!\n", drv->aes_mdev.name);
        return -1;
    }

    drv->irq = IRQ_AES;
    ret = request_irq(drv->irq, aes_intr_handler, 0, "aes", NULL);
    BUG_ON(ret);

    /* changing enable_dma can control whether dma mode(1) or normal mode(0) is used */
    drv->dev.enable_dma = 1;
    drv->dev.src = m_dma_alloc_coherent(AES_DMA_LEN);
    drv->dev.dst = m_dma_alloc_coherent(AES_DMA_LEN);

    if (drv->dev.src == NULL || drv->dev.dst == NULL)
        ret = -ENOMEM;

    return 0;
}

static void __exit jz_aes_exit(void)
{
    struct jz_aes_drv *drv = &aes_drv;

    misc_deregister(&drv->aes_mdev);
    free_irq(IRQ_AES, NULL);
    clk_disable_unprepare(drv->clk);
    clk_put(drv->clk);
    m_dma_free_coherent(drv->dev.src, AES_DMA_LEN);
    m_dma_free_coherent(drv->dev.dst, AES_DMA_LEN);
}

module_init(jz_aes_init);
module_exit(jz_aes_exit);

MODULE_DESCRIPTION("JZ x2000 AES driver");
MODULE_LICENSE("GPL");
