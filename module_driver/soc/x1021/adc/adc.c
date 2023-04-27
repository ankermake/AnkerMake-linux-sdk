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

#include <soc/irq.h>
#include "adc_hal.h"

#define ADC_MAGIC_NUMBER    'A'
#define ADC_ENABLE            _IO(ADC_MAGIC_NUMBER, 11)
#define ADC_DISABLE            _IO(ADC_MAGIC_NUMBER, 22)
#define ADC_SET_VREF        _IOW(ADC_MAGIC_NUMBER, 33, unsigned int)
#define ADC_GET_VREF        _IOWR(ADC_MAGIC_NUMBER, 44, unsigned int)
#define ADC_GET_VALUE       _IOWR(ADC_MAGIC_NUMBER, 60, unsigned int)

#define CLKDIV              (120 - 1)
#define CLKDIV_US           (2 - 1)
#define CLKDIV_MS           (100 - 1)

#define STABLE_TIME 1
#define REPEAT_TIME 1

#define ADC_SAMPLE_TIMEOUT      100
#define ADC_MAX_CHANNELS             5

struct adc_dev {
    struct clk* clk;

    struct mutex mutex;

    unsigned int ch_status;
    wait_queue_head_t wq;

    struct miscdevice *mdev;
};

static unsigned int VREF_ADC = 1800;
module_param_named(adc_vref, VREF_ADC, int, 0644);

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

int adc_enable(void)
{
    mutex_lock(&adc_device.mutex);

    jz_adc_enable();

    mutex_unlock(&adc_device.mutex);

    return 0;
}
EXPORT_SYMBOL(adc_enable);

static int adc_open(struct inode *inode, struct file *filp)
{
    adc_enable();

    return 0;
}

int adc_disable(void)
{
    mutex_lock(&adc_device.mutex);

    jz_adc_disable();

    mutex_unlock(&adc_device.mutex);

    return 0;
}
EXPORT_SYMBOL(adc_disable);

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

    adc_hal_enable_channel(channel);

    ret = wait_event_interruptible_timeout(
        adc_device.wq, !adc_device.ch_status, msecs_to_jiffies(20));
    if (ret == 0) {
        printk(KERN_ERR "%s:adc get value timeout!\n", __func__);
        return -EBUSY;
    }
    sadc_val = jz_adc_read_channel_value(channel);

    return sadc_val;
}

int adc_read_channel_voltage(unsigned int channel)
{
    unsigned int sadc_val = 0;

    mutex_lock(&adc_device.mutex);

    sadc_val = adc_read_channel_value(channel);
    sadc_val = sadc_val * VREF_ADC / 1024;

    mutex_unlock(&adc_device.mutex);

    return sadc_val;
}
EXPORT_SYMBOL(adc_read_channel_voltage);

ssize_t adc_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
    unsigned int sadc_val = 0;
    struct miscdevice* mdev = filp->private_data;
    unsigned int channel = mdev - adc_device.mdev;

    BUG_ON(channel >= ADC_MAX_CHANNELS);

    mutex_lock(&adc_device.mutex);

    sadc_val = adc_read_channel_value(channel);
    if (sadc_val < 0) {
        mutex_unlock(&adc_device.mutex);
        return sadc_val;
    }

    sadc_val = sadc_val * VREF_ADC / 1024;

    mutex_unlock(&adc_device.mutex);

    if (copy_to_user(buf, &sadc_val, sizeof(int)))
        return -EFAULT;

    return sizeof(int);
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
    case ADC_ENABLE:
    case ADC_DISABLE:
        break;
    case ADC_SET_VREF:
        VREF_ADC = *(unsigned int *)arg;
        break;
    case ADC_GET_VREF:
        ret = VREF_ADC;
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
    .read = adc_read,
    .release= adc_release,
    .unlocked_ioctl= adc_ioctl,
};

struct miscdevice adc_mdev[ADC_MAX_CHANNELS] = {
    {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "jz_adc_aux_0",
        .fops = &adc_fops,
    },
    {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "jz_adc_aux_1",
        .fops = &adc_fops,
    },
    {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "jz_adc_aux_2",
        .fops = &adc_fops,
    },
    {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "jz_adc_aux_3",
        .fops = &adc_fops,
    },
    {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "jz_adc_aux_4",
        .fops = &adc_fops,
    }
};

static int __init jz_adc_init(void)
{
    int ret, i;

    adc_device.clk = clk_get(NULL, "sadc");
    clk_enable(adc_device.clk);

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
    clk_disable(adc_device.clk);
    clk_put(adc_device.clk);

    adc_busy = 0;
}
module_exit(jz_adc_exit);

MODULE_DESCRIPTION("JZ x1021 ADC driver");
MODULE_LICENSE("GPL");
