#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>
#include <linux/sched.h>

#include "adc_poll_hal.h"
#include "adc_poll.h"

#define INGENIC_ADC_TEST

#define MAX_TRY_COUNT 100000

#define ADC_MAGIC_NUMBER    'A'
#define ADC_GET_VALUE       _IOWR(ADC_MAGIC_NUMBER, 60, unsigned int)

#define CLKDIV                          (120 - 1)
#define CLKDIV_US                       (2 - 1)
#define CLKDIV_MS                       (100 - 1)

#define STABLE_TIME                     1
#define REPEAT_TIME                     1

#define ADC_SAMPLE_TIMEOUT              100
#define ADC_MAX_CHANNELS                4

#define IRQ_SADC                        (IRQ_INTC_BASE + 11)

struct adc_dev {
    struct clk* clk;

    struct mutex mutex;

    unsigned int ch_status;
    wait_queue_head_t wq;

    struct miscdevice *mdev;
};

static struct adc_dev adc_device;

static int adc_busy;

static void jz_adc_enable(void)
{
    if (adc_busy++ == 0) {
        adc_hal_enable_controller();

        adc_hal_clean_all_interrupt_flag();
        adc_hal_enable_all_interrupt();
    }
}

static void jz_adc_disable(void)
{
    if (--adc_busy == 0) {
        adc_hal_disable_controller();

        adc_hal_mask_all_interrupt();
        adc_hal_clean_all_interrupt_flag();
    }
}

static unsigned int jz_adc_read_channel_value(unsigned int channel)
{
    return adc_hal_read_channel_data(channel);
}

static irqreturn_t adc_irq_handler(int irq, void *data)
{
    unsigned int ch_status;

    ch_status = adc_hal_get_all_interrupt_flag();

    adc_hal_clean_all_interrupt_flag();

    if(ch_status & adc_device.ch_status) {
        adc_device.ch_status = 0;
        wake_up_interruptible(&adc_device.wq);
    }

    return IRQ_HANDLED;
}

static int adc_enable(void)
{
    mutex_lock(&adc_device.mutex);

    jz_adc_enable();

    mutex_unlock(&adc_device.mutex);

    return 0;
}

static int adc_open(struct inode *inode, struct file *filp)
{
    adc_enable();

    return 0;
}

static int adc_disable(void)
{
    mutex_lock(&adc_device.mutex);

    jz_adc_disable();

    mutex_unlock(&adc_device.mutex);

    return 0;
}

static int adc_release(struct inode *inode, struct file *filp)
{
    adc_disable();

    return 0;
}

static int adc_read_channel_value(unsigned int channel)
{
    int ret = 0;
    unsigned int sadc_val = 0;

    adc_device.ch_status = BIT(channel);

    adc_hal_enable_channel(channel, MAX_TRY_COUNT);

    ret = wait_event_interruptible_timeout(
        adc_device.wq, !adc_device.ch_status, msecs_to_jiffies(20));
    if (ret == 0) {
        printk(KERN_ERR "%s:adc get value timeout!\n", __func__);
        return -EBUSY;
    }
    sadc_val = jz_adc_read_channel_value(channel);

    return sadc_val;
}

static long adc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct miscdevice* mdev = filp->private_data;
    int channel = mdev - adc_device.mdev;

    /* handle ioctls */
    switch (cmd) {
    case ADC_GET_VALUE:
        /*
         * Cannot modify the JZ_REG_ADC_ENABLE if the controller is working,
         * so need time division multiplexing.
         */
        mutex_lock(&adc_device.mutex);

        ret = adc_read_channel_value(channel);

        mutex_unlock(&adc_device.mutex);
        break;
    default:
        printk(KERN_ERR "%s:unsupported ioctl cmd %x\n",__func__, cmd);
        ret = -EINVAL;
        break;
    }

    return ret;
}

static struct file_operations adc_fops= {
    .owner= THIS_MODULE,
    .open= adc_open,
    .release= adc_release,
    .unlocked_ioctl= adc_ioctl,
};

struct miscdevice adc_mdev[ADC_MAX_CHANNELS] = {
    {
        .minor   = MISC_DYNAMIC_MINOR,
        .name    = "jz_adc_aux_0",
        .fops    = &adc_fops,
    },
    {
        .minor   = MISC_DYNAMIC_MINOR,
        .name    = "jz_adc_aux_1",
        .fops    = &adc_fops,
    },
    {
        .minor   = MISC_DYNAMIC_MINOR,
        .name    = "jz_adc_aux_2",
        .fops    = &adc_fops,
    },
    {
        .minor   = MISC_DYNAMIC_MINOR,
        .name    = "jz_adc_aux_3",
        .fops    = &adc_fops,
    }
};

static int __init jz_adc_init(void)
{
    int ret, i;

    adc_device.clk = clk_get(NULL, "gate_sadc");
    clk_prepare_enable(adc_device.clk);

    adc_hal_disable_controller();
    adc_hal_mask_all_interrupt();
    adc_hal_clean_all_interrupt_flag();

    adc_hal_set_clkdiv(CLKDIV, CLKDIV_US, CLKDIV_MS);
    adc_hal_set_wait_sampling_stable_time(STABLE_TIME);

    mutex_init(&adc_device.mutex);

    init_waitqueue_head(&adc_device.wq);

    ret = request_irq(IRQ_SADC, adc_irq_handler, 0, "adc", NULL);
    BUG_ON(ret);

    adc_device.mdev = adc_mdev;

    for (i = 0; i < ADC_MAX_CHANNELS; i++) {
        ret = misc_register(&adc_mdev[i]);
        BUG_ON(ret < 0);
    }

#ifdef INGENIC_ADC_TEST
    int value;
    int cnt = 100;

    test_jz_adc_enable();

    while(cnt--) {
        test_jz_adc_channel_sample(0, 1);
        value = test_jz_adc_read_value(0, 1);

        printk(KERN_ERR "====================================jz_adc_read_value = %d\n", value);
    }
#endif

    return 0;
}
module_init(jz_adc_init);

static void __exit jz_adc_exit(void)
{
    int i;
    for (i = 0; i < ADC_MAX_CHANNELS; i++) {
        misc_deregister(&adc_mdev[i]);
    }

    free_irq(IRQ_SADC, NULL);
    clk_disable_unprepare(adc_device.clk);
    clk_put(adc_device.clk);

    adc_busy = 0;
}
module_exit(jz_adc_exit);



void test_jz_adc_enable(void)
{
    adc_enable();
    adc_hal_enable_bits_channels(0xf, -1);
}
EXPORT_SYMBOL(test_jz_adc_enable);


void test_jz_adc_disable(void)
{
    adc_disable();
    adc_hal_enable_bits_channels(0x0, -1);
}
EXPORT_SYMBOL(test_jz_adc_disable);



unsigned int test_jz_adc_read_value(unsigned int channel, unsigned int block)
{
    BUG_ON(channel > ADC_MAX_CHANNELS);

    /*block 为 1 若没使能， 死等*/
    if (block)
        while(!adc_hal_get_channel_status(channel));

    /*block 为0 ，也不判断是否使能了, 也不等中断*/
    return jz_adc_read_channel_value(channel);
}
EXPORT_SYMBOL(test_jz_adc_read_value);



unsigned int test_jz_adc_channel_sample(unsigned int channel, unsigned int block)
{
    int ret;
    BUG_ON(channel > ADC_MAX_CHANNELS);
    ret = adc_hal_get_all_channel_status();

    /*block 若为 1， 等待所有channels 不使能*/
    if (block && ret)
        while(adc_hal_get_all_channel_status());

    if(!block && ret)
        return ret;

    if (block)
        adc_hal_enable_channel(channel, -1);
    else
        adc_hal_enable_channel(channel, MAX_TRY_COUNT);

    return 0;
}
EXPORT_SYMBOL(test_jz_adc_channel_sample);



unsigned int test_jz_adc_bit_channels_sample(unsigned int bit_channels, unsigned int block)
{
    int ret = adc_hal_get_all_channel_status();
    BUG_ON(bit_channels > 0xF);

    /*block 若为 1， 等待所有channels 不使能*/
    if (block && ret)
        while(adc_hal_get_all_channel_status());

    if(!block && ret)
        return ret;

    if (block)
        adc_hal_enable_bits_channels(bit_channels, -1);
    else
        adc_hal_enable_bits_channels(bit_channels, MAX_TRY_COUNT);

    return 0;
}
EXPORT_SYMBOL(test_jz_adc_bit_channels_sample);


MODULE_DESCRIPTION("JZ x1600 ADC driver");
MODULE_LICENSE("GPL");