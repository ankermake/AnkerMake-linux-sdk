#include <linux/version.h>
#include <linux/sched.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10, 0)
#include <linux/sched/clock.h>
#endif
#include <linux/kernel.h>
#include <utils/clock.h>

unsigned long long local_clock_us(void)
{
    unsigned long long ns = local_clock();

    do_div(ns, 1000);

    return ns;
}
EXPORT_SYMBOL(local_clock_us);

unsigned long long local_clock_ms(void)
{
    unsigned long long ns = local_clock();

    do_div(ns, 1000 * 1000);

    return ns;
}
EXPORT_SYMBOL(local_clock_ms);
