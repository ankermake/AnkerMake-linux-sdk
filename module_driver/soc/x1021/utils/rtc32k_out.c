#include <linux/module.h>
#include <asm/atomic.h>
#include <linux/gpio.h>
#include <utils/gpio.h>
#include <linux/spinlock.h>
#include <linux/moduleparam.h>


static int rtc32k_initialized = 0;
static DEFINE_SPINLOCK(lock);
static int refcount;
static int rtc32k_init_on = 0;

module_param(rtc32k_init_on, int, 0644);

#define rtc_gpio GPIO_PC(16)
#define rtc_gpio_func GPIO_FUNC_0

void ingenic_rtc32k_enable(void)
{
    unsigned long flags;

    if (!rtc32k_initialized) {
        pr_err("rtc32k is not init ok\n");
        return;
    }

    spin_lock_irqsave(&lock, flags);

    if (refcount++ == 0)
        gpio_set_func(rtc_gpio, rtc_gpio_func);

    spin_unlock_irqrestore(&lock, flags);
}

void ingenic_rtc32k_disable(void)
{
    unsigned long flags;

    if (!rtc32k_initialized) {
        pr_err("rtc32k is not init ok\n");
        return;
    }

    spin_lock_irqsave(&lock, flags);

    if (--refcount == 0)
        gpio_set_func(rtc_gpio, GPIO_INPUT);

    spin_unlock_irqrestore(&lock, flags);
}

void rtc32k_init(void)
{
    if (gpio_request(rtc_gpio, "clk32k_out_o")) {
        pr_err("request rtc32k gpio fail!\n");
        return;
    }

    rtc32k_initialized = 1;

    if (rtc32k_init_on) {
        printk("init enable rtc32k out\n");
        ingenic_rtc32k_enable();
    }
}

void rtc32k_exit(void)
{
    if (rtc32k_initialized)
        gpio_free(rtc_gpio);

    refcount = 0;
    rtc32k_initialized = 0;
}

EXPORT_SYMBOL(rtc32k_exit);
EXPORT_SYMBOL(ingenic_rtc32k_enable);
EXPORT_SYMBOL(ingenic_rtc32k_disable);
