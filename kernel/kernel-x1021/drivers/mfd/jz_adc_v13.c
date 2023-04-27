#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>

#define ADC_MAGIC_NUMBER        'A'
#define ADC_GET_VALUE           _IOWR(ADC_MAGIC_NUMBER, 60, unsigned int)

#define ADC_MAX_CHANNEL         7

#define JZ_REG_ADC_ENABLE       0x00
#define JZ_REG_ADC_CTRL         0x08
#define JZ_REG_ADC_STATUS       0x0c

#define JZ_REG_ADC_AUX_BASE     0x10
#define JZ_REG_ADC_CLKDIV       0x20

#define JZ_REG_ADC_STABLE       0x24
#define JZ_REG_ADC_REPEAT_TIME  0x28

#define CLKDIV              (120 - 1)
#define CLKDIV_US           (2 - 1)
#define CLKDIV_MS           (100 - 1)

#define STABLE_TIME 1
#define REPEAT_TIME 1

#define DEV_NAME "jz-adc"

struct jz_adc {
    struct resource* mem;
    void __iomem* base;

    int irq;

    struct clk* clk;
    atomic_t enable_count;

    struct mutex mmutex;
    struct miscdevice mdev;
    unsigned int status;
    wait_queue_head_t status_wait;
};

static void jz_adc_enable(struct jz_adc* adc)
{
    if (atomic_inc_return(&adc->enable_count) == 1) {
        unsigned int val;
        do {
            writel(0x0, adc->base + JZ_REG_ADC_ENABLE);
            val = readl(adc->base + JZ_REG_ADC_ENABLE);
        } while(0x00 != val);
        msleep(2);

        writel(0xff, adc->base + JZ_REG_ADC_STATUS);
        writel(0x0, adc->base + JZ_REG_ADC_CTRL);
    }
}

static void jz_adc_disable(struct jz_adc* adc)
{
    if (atomic_dec_return(&adc->enable_count) == 0) {
        unsigned int val;
        do {
            writel(0x8000, adc->base + JZ_REG_ADC_ENABLE);
            val = readl(adc->base + JZ_REG_ADC_ENABLE);
        } while(0x8000 != val);

        writel(0xff, adc->base + JZ_REG_ADC_CTRL);
        writel(0xff, adc->base + JZ_REG_ADC_STATUS);
    }
}

static void jz_adc_channels_sample(struct jz_adc* adc, unsigned int bit_channels)
{
    unsigned int val;
    bit_channels = bit_channels  & 0xFF;

    do {
        writel(bit_channels, adc->base + JZ_REG_ADC_ENABLE);
        val = readl(adc->base + JZ_REG_ADC_ENABLE);
    } while(~val & bit_channels);

}

static unsigned int jz_adc_read_channel_value(struct jz_adc* adc, unsigned int channel)
{
    unsigned int val;

    val = readl(adc->base + JZ_REG_ADC_AUX_BASE + (channel / 2) * 4);
    val = val >> ((channel % 2) * 16);
    val = val & 0x3ff;

    return val;
}

static irqreturn_t jz_adc_irq_handler(int irq, void *devid)
{
    unsigned int status;
    struct jz_adc* adc = (struct jz_adc *)devid;

    status = readl(adc->base + JZ_REG_ADC_STATUS);
    writel(status, adc->base + JZ_REG_ADC_STATUS);

    if(status & adc->status) {
        adc->status = 0;
        wake_up_interruptible(&adc->status_wait);
    }
    return IRQ_HANDLED;
}

static int jz_adc_open(struct inode *inode, struct file *filp)
{
    struct miscdevice* mdev = filp->private_data;
    struct jz_adc* adc = container_of(mdev, struct jz_adc, mdev);

    mutex_lock(&adc->mmutex);
    jz_adc_enable(adc);
    mutex_unlock(&adc->mmutex);

    return 0;
}

static int jz_adc_release(struct inode *inode, struct file *filp)
{
    struct miscdevice* mdev = filp->private_data;
    struct jz_adc* adc = container_of(mdev, struct jz_adc, mdev);

    mutex_lock(&adc->mmutex);
    jz_adc_disable(adc);
    mutex_unlock(&adc->mmutex);

    return 0;
}

static long jz_adc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct miscdevice* mdev = filp->private_data;
    struct jz_adc* adc = container_of(mdev, struct jz_adc, mdev);

    /* handle ioctls */
    switch (cmd) {
    case ADC_GET_VALUE:
        if(arg > ADC_MAX_CHANNEL) {
            ret = -EINVAL;
        } else {
            /*
             * Cannot modify the JZ_REG_ADC_ENABLE if the controller is working,
             * so need time division multiplexing.
            */
            mutex_lock(&adc->mmutex);

            adc->status = BIT(arg);
            jz_adc_channels_sample(adc, BIT(arg));
            wait_event_interruptible(adc->status_wait, !adc->status);
            ret = jz_adc_read_channel_value(adc, arg);

            mutex_unlock(&adc->mmutex);
        }
        break;
    default:
        /* could not handle ioctl */
        ret = -EINVAL;
        break;
    }

    return ret;
}

static struct file_operations jz_adc_fops= {
    .owner= THIS_MODULE,
    .open= jz_adc_open,
    .release= jz_adc_release,
    .unlocked_ioctl= jz_adc_ioctl,
};

static struct jz_adc* test_adc;

void test_jz_adc_enable(void)
{
    BUG_ON(test_adc == NULL);

    jz_adc_enable(test_adc);
}
EXPORT_SYMBOL(test_jz_adc_enable);

void test_jz_adc_disable(void)
{
    BUG_ON(test_adc == NULL);

    jz_adc_disable(test_adc);
}
EXPORT_SYMBOL(test_jz_adc_disable);


/*
* set and enable sample for some bit format channels
* bit_channels[in]:  bit character channels of ADC, eg. channels = 0x00000003 means sample adc0 and adc1
* block[in]:    the way of set sample bit;
*               0: noneblock, abandon this time of sample if set enable faile and HW would not resample
*               1: block, wiat untill set sample enable for this time
* return:   0: succeed to set sample enable bit of the channel
*           others: those combination of enabled bit(channel) have been set and in sampling
* ATTENTION:    if some bit(channel) has been aready set and not be cleared, set any bit enable, writew(1),
*               would fail even this bit is not the one already set
*/
unsigned int test_jz_adc_bit_channels_sample(unsigned int bit_channels, unsigned int block)
{
    unsigned int val;
    unsigned int retry_count = 100;

    BUG_ON(test_adc == NULL);
    BUG_ON(bit_channels > 0xFF);

    do {
        val = readl(test_adc->base + JZ_REG_ADC_ENABLE);
        val = val & 0xFF;
    } while(block && val);

    if((!block) && (val))
        return val;

    do {
        writel(bit_channels, test_adc->base + JZ_REG_ADC_ENABLE);
        val = readl(test_adc->base + JZ_REG_ADC_ENABLE);
        retry_count--;
    } while(retry_count && (~val & bit_channels));

    if (!retry_count)
        printk(KERN_ERR "%s bit_channels = 0x%02x, retry_count=%u \n",__func__, bit_channels, retry_count);

    return (~val & bit_channels);
}

EXPORT_SYMBOL(test_jz_adc_bit_channels_sample);


/*
* set and enable sample for a proper channel once
* channel[in]:  channel of ADC, 0 for ADC0, 1 for ADC1，......
* block[in]:    the way of set sample bit;
*               0: noneblock, abandon this time of sample if set enable faile and HW would not resample
*               1: block, wiat untill set sample enable for this time
* return:   0: succeed to set sample enable bit of the channel
*           others: those combination of enabled bit(channel) have been set and in sampling
* ATTENTION:    if some bit(channel) has been aready set and not be cleared, set any bit enable, writew(1),
*               would fail even if this bit is not the one already set
*/
unsigned int test_jz_adc_channel_sample(unsigned int channel, unsigned int block)
{
    unsigned int val;
    unsigned int retry_count = 100;

    BUG_ON(test_adc == NULL);
    BUG_ON(channel > ADC_MAX_CHANNEL);

    do {
        val = readl(test_adc->base + JZ_REG_ADC_ENABLE);
        val = val & 0xFF;
    } while(block && val);

    if((!block) && (val))
        return val;

    do {
        writel(BIT(channel), test_adc->base + JZ_REG_ADC_ENABLE);
        val = readl(test_adc->base + JZ_REG_ADC_ENABLE);
        retry_count--;
    } while(retry_count && (~val & BIT(channel)));

    if (!retry_count)
        printk(KERN_ERR "%s channel = %d, retry_count=%u \n",__func__, channel, retry_count);

    return (~val & BIT(channel));
}
EXPORT_SYMBOL(test_jz_adc_channel_sample);

/* No blocking may read old value
*  Attention: channel here is not bit character. It is different from channels of test_jz_adc_channel_sample.
*  eg. channel_0 is 0 while channel_1 is 1
*/
unsigned int test_jz_adc_read_value(unsigned int channel, unsigned int block)
{
    BUG_ON(test_adc == NULL);
    BUG_ON(channel > ADC_MAX_CHANNEL);

    if(block) {
        unsigned int val;

        do {
            val = readl(test_adc->base + JZ_REG_ADC_ENABLE);
        } while(val & BIT(channel));
    }

    return jz_adc_read_channel_value(test_adc, channel);
}
EXPORT_SYMBOL(test_jz_adc_read_value);

static int jz_adc_probe(struct platform_device* pdev)
{
    int ret;
    unsigned int val;
    struct jz_adc* adc;
    struct resource* mem_base;

    adc = kzalloc(sizeof(struct jz_adc), GFP_KERNEL);
    if (adc == NULL) {
        dev_err(&pdev->dev, "Failed to allocate driver structre\n");
        return -ENOMEM;
    }

    adc->irq = platform_get_irq(pdev, 0);
    if (adc->irq < 0) {
        ret = adc->irq;
        dev_err(&pdev->dev, "Failed to get platform irq: %d\n", ret);
        goto err_free;
    }

    mem_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (mem_base == NULL) {
        ret = -ENOENT;
        dev_err(&pdev->dev, "Failed to get platform mmio resource");
        goto err_free;
    }

    adc->mem = request_mem_region(mem_base->start, JZ_REG_ADC_STATUS,
        pdev->name);
    if (adc->mem == NULL) {
        ret = -EBUSY;
        dev_err(&pdev->dev, "Failed to request mmio memory region\n");
        goto err_free;
    }

    adc->base = ioremap_nocache(adc->mem->start, resource_size(adc->mem));
    if (!adc->base) {
        ret = -EBUSY;
        dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
        goto err_release_mem_region;
    }

    adc->clk = clk_get(&pdev->dev, "sadc");
    if (IS_ERR(adc->clk)) {
        ret = PTR_ERR(adc->clk);
        dev_err(&pdev->dev, "Failed to get clock: %d\n", ret);
        goto err_iounmap;
    }
    clk_enable(adc->clk);

    writel(0x8000, adc->base + JZ_REG_ADC_ENABLE);
    writel(0xff, adc->base + JZ_REG_ADC_CTRL);
    writel(0xff, adc->base + JZ_REG_ADC_STATUS);

    val = CLKDIV | (CLKDIV_US << 8) | (CLKDIV_MS << 16);
    writel(val, adc->base + JZ_REG_ADC_CLKDIV);

    writel(STABLE_TIME, adc->base + JZ_REG_ADC_STABLE);
    writel(REPEAT_TIME, adc->base + JZ_REG_ADC_REPEAT_TIME);

    mutex_init(&adc->mmutex);
    init_waitqueue_head(&adc->status_wait);
    atomic_set(&adc->enable_count, 0);

    ret = request_irq(adc->irq, jz_adc_irq_handler, 0, DEV_NAME, adc);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request irq %d\n", ret);
        goto err_clk_put;
    }

    adc->mdev.minor = MISC_DYNAMIC_MINOR;
    adc->mdev.name = DEV_NAME;
    adc->mdev.fops = &jz_adc_fops;

    ret = misc_register(&adc->mdev);
    if (ret < 0) {
        dev_err(&pdev->dev, "misc_register failed\n");
        goto err_free_irq;
    }

    platform_set_drvdata(pdev, adc);

    test_adc = adc;
    return 0;

err_free_irq:
    free_irq(adc->irq, adc);
err_clk_put:
    clk_disable(adc->clk);
    clk_put(adc->clk);
err_iounmap:
    iounmap(adc->base);
err_release_mem_region:
    release_mem_region(adc->mem->start, resource_size(adc->mem));
err_free:
    kfree(adc);
    return ret;
}

static int jz_adc_remove(struct platform_device* pdev)
{
    struct jz_adc* adc = platform_get_drvdata(pdev);

    test_adc = NULL;
    platform_set_drvdata(pdev, NULL);
    misc_deregister(&adc->mdev);
    free_irq(adc->irq, adc);
    clk_disable(adc->clk);
    clk_put(adc->clk);
    iounmap(adc->base);
    release_mem_region(adc->mem->start, resource_size(adc->mem));
    kfree(adc);

    return 0;
}

struct platform_driver jz_adc_driver = {
    .probe = jz_adc_probe,
    .remove = jz_adc_remove,
    .driver = {
        .name = DEV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init jz_adc_init(void)
{
    return platform_driver_register(&jz_adc_driver);
}
module_init(jz_adc_init);

static void __exit jz_adc_exit(void)
{
    platform_driver_unregister(&jz_adc_driver);
}
module_exit(jz_adc_exit);

MODULE_DESCRIPTION("JZ x1021 ADC driver");
MODULE_AUTHOR("xinshuan <shuan.xin@ingenic.com>");
MODULE_LICENSE("GPL");
