#include <linux/sched.h>
#include <linux/kernel.h>


static unsigned long long conn_local_clock_us(void)
{
    unsigned long long ns = local_clock();

    do_div(ns, 1000);

    return ns;
}

static unsigned long long conn_local_clock_ms(void)
{
    unsigned long long ns = local_clock();

    do_div(ns, 1000 * 1000);

    return ns;
}
