#include <linux/module.h>
#include <asm/atomic.h>
#include <linux/gpio.h>
#include <utils/gpio.h>
#include <linux/spinlock.h>
#include <linux/moduleparam.h>


static int clk24m_initialized = 0;
static DEFINE_SPINLOCK(lock);
static int refcount;
static int clk24m_init_on = 0;

module_param(clk24m_init_on, int, 0644);

#define clk_gpio GPIO_PB(27)
#define clk_gpio_func GPIO_FUNC_0

void clk24m_enable(void)
{
    unsigned long flags;

    if (!clk24m_initialized) {
        pr_err("clk24m is not init ok\n");
        return;
    }

    spin_lock_irqsave(&lock, flags);

    if (refcount++ == 0)
        gpio_set_func(clk_gpio, clk_gpio_func);

    spin_unlock_irqrestore(&lock, flags);
}

void clk24m_disable(void)
{
    unsigned long flags;

    if (!clk24m_initialized) {
        pr_err("clk24m is not init ok\n");
        return;
    }

    spin_lock_irqsave(&lock, flags);

    if (--refcount == 0)
        gpio_set_func(clk_gpio, GPIO_INPUT);

    spin_unlock_irqrestore(&lock, flags);
}

void clk24m_init(void)
{
    if (gpio_request(clk_gpio, "clk24m_out_o")) {
        pr_err("request clk24m gpio fail!\n");
        return;
    }

    clk24m_initialized = 1;

    if (clk24m_init_on) {
        printk("init enable clk24m out\n");
        clk24m_enable();
    }
}

void clk24m_exit(void)
{
    if (clk24m_initialized)
        gpio_free(clk_gpio);

    refcount = 0;
    clk24m_initialized = 0;
}

EXPORT_SYMBOL(clk24m_exit);
EXPORT_SYMBOL(clk24m_enable);
EXPORT_SYMBOL(clk24m_disable);
