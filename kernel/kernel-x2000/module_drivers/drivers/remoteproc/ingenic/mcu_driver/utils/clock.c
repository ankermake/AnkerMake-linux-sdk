#include <linux/sched.h>
#include <linux/kernel.h>
#include "clock.h"

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
