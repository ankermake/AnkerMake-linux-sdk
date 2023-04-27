#include <linux/mm.h>
#include <linux/cpu.h>
#include <linux/nmi.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/lockdep.h>
#include <linux/notifier.h>
#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/smpboot.h>
#include <linux/sched/rt.h>

#include <asm/irq_regs.h>
#include <linux/kvm_para.h>
#include <linux/perf_event.h>
#include <soc/cpm.h>

#include <linux/spinlock.h>
#include <linux/clk.h>
#include <linux/miscdevice.h>

#define TCU_TSSTR       0x2C
#define TCU_TSCLR       0x3C

#define WDT_TDR     0x0
#define WDT_TCER    0x4
#define WDT_TCNT    0x8
#define WDT_TCSR    0xC

#define WDT_CLK_DIV_1       0
#define WDT_CLK_DIV_4       1
#define WDT_CLK_DIV_16      2
#define WDT_CLK_DIV_64      3
#define WDT_CLK_DIV_256     4
#define WDT_CLK_DIV_1024    5

#define WDT_MAX_COUNT       (0xFFFF)

#define OPCR_ERCS_BIT       2

#define RTC_EN      2

#define WDT_IDLE    0
#define WDT_BUSY    1

#define WATCHDOG_MAGIC_NUMBER   'W'
#define WATCHDOG_START              _IOW(WATCHDOG_MAGIC_NUMBER, 13, unsigned long)
#define WATCHDOG_STOP               _IO(WATCHDOG_MAGIC_NUMBER, 14)
#define WATCHDOG_FEED               _IO(WATCHDOG_MAGIC_NUMBER, 15)
#define WATCHDOG_RESET              _IO(WATCHDOG_MAGIC_NUMBER, 16)

#define WDT_ADDR(reg) ((volatile unsigned long *)(KSEG1ADDR(WDT_IOBASE) + reg))

static inline void wdt_write_reg(unsigned int reg, unsigned int value)
{
    *WDT_ADDR(reg) = value;
}

static inline unsigned int wdt_read_reg(unsigned int reg)
{
    return *WDT_ADDR(reg);
}


static DEFINE_SPINLOCK(lock);

static unsigned int status;
static struct clk *ext1_clk;

unsigned long get_rtc_internal_clk_rate(void)
{
    unsigned int rtc_32k_is_on = cpm_test_bit(OPCR_ERCS_BIT, CPM_OPCR);

    if (!rtc_32k_is_on) {
        unsigned long rate;

        ext1_clk = clk_get(NULL, "ext1");
        BUG_ON(IS_ERR(ext1_clk));
        rate = clk_get_rate(ext1_clk) / 512;

        return rate;
    }

    return 32768;
}

static int jz_wdt_set_timeout(unsigned long ms)
{
    unsigned int val, us;
    unsigned long count = ms;
    unsigned int clock_div = 0;

    unsigned long rate = get_rtc_internal_clk_rate();

    us = 1000000 / rate;

    count = ms * 1000 / us;

    while (count > WDT_MAX_COUNT) {
        if (clock_div == WDT_CLK_DIV_1024)
            return -1;

        count /= 4;
        clock_div += 1;
    }

    wdt_write_reg(WDT_TCER, 0);

    val = (clock_div << 3) | RTC_EN;

    wdt_write_reg(WDT_TCSR, val);

    wdt_write_reg(WDT_TDR, count);

    wdt_write_reg(WDT_TCNT, 0);

    wdt_write_reg(WDT_TCER, 1);

    return 0;
}

static int wdt_start(unsigned long ms)
{
    if (status != WDT_IDLE) {
        printk(KERN_ERR "WDT: watchdog is running ! \n");
        return -1;
    }

    status = WDT_BUSY;

    wdt_write_reg(TCU_TSCLR, 1 << 16);//使能看门狗计数器
    if (jz_wdt_set_timeout(ms)) {
        printk(KERN_ERR "error: count more than the WATCHDOG_MAX_COUNT!\n");
        return -1;
    }

    wdt_write_reg(WDT_TCER, 1);

    return 0;
}

int soc_wdt_start(unsigned long ms)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);

    ret = wdt_start(ms);

    spin_unlock_irqrestore(&lock, flags);

    return ret;
}

static int wdt_stop(void)
{
    if (status == WDT_IDLE) {
        printk(KERN_ERR "WDT: watchdog already stopped ! \n");
        return -1;
    }

    wdt_write_reg(WDT_TCER, 0);
    wdt_write_reg(TCU_TSSTR, 1 << 16);//失能看门狗计数器

    status = WDT_IDLE;

    return 0;
}

int soc_wdt_stop(void)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);

    ret = wdt_stop();

    spin_unlock_irqrestore(&lock, flags);

    return ret;
}

int soc_wdt_feed(void)
{
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);

    wdt_write_reg(WDT_TCNT, 0);

    spin_unlock_irqrestore(&lock, flags);

    return 0;
}

void soc_reset(void)
{
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);

    if (status != WDT_IDLE)
        wdt_stop();

    wdt_start(0);

    while (1) {
        mdelay(10);
        printk(KERN_ERR "WDT: wait for reset\n");
    }

    spin_unlock_irqrestore(&lock, flags);
}


static int watchdog_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int watchdog_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static long watchdog_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned long ms;

    switch (cmd) {
        case WATCHDOG_START: {
            if (copy_from_user(&ms, (unsigned long *)arg, sizeof(ms))) {
                printk(KERN_ERR "WATCHDOG: copy_from_user err!\n");
                return -1;
            }

            ret = soc_wdt_start(ms);
            break;
        }
        case WATCHDOG_STOP: {
            ret = soc_wdt_stop();
            break;
        }
        case WATCHDOG_FEED: {
            ret = soc_wdt_feed();
            break;
        }
        case WATCHDOG_RESET: {
            soc_reset();
            break;
        }
        default:
            return -1;
    }

    return ret;
}

static struct file_operations watchdog_fops = {
    .owner= THIS_MODULE,
    .open= watchdog_open,
    .release = watchdog_close,
    .unlocked_ioctl = watchdog_ioctl,
};

struct miscdevice watchdog_mdevice = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "jz_watchdog",
    .fops   = &watchdog_fops,
};

static int jz_watchdog_init(void)
{
    int ret;

    ret = misc_register(&watchdog_mdevice);
    if (ret < 0)
        panic("watchdog: %s, watchdog register misc dev error !\n", __func__);

    return 0;
}

module_init(jz_watchdog_init);

static void jz_watchdog_exit(void)
{
    if (ext1_clk)
        clk_put(ext1_clk);

    misc_deregister(&watchdog_mdevice);
}

module_exit(jz_watchdog_exit);

MODULE_DESCRIPTION("Ingenic SoC WATCHDOG driver");
MODULE_LICENSE("GPL");
