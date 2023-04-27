#include <assert.h>
#include <linux/kernel.h>
//#include <stdio.h>

void hang(void)
{
    // (*(volatile unsigned int *)0) = 0;
    while (1);
}

void __assert(const char *expr, const char *file, int line)
{
    printk("assert %s failed in %s %d\n", expr, file, line);
    hang();
}

void __assert_func(const char *file, int line, const char *func, const char *expr)
{
    printk("assert %s failed in %s %d %s\n", expr, file, line, func);
    hang();
}

void __assert_no_file(const char *func, int line, const char *expr)
{
    printk("assert %s failed in %s %d\n", expr, func, line);
    hang();
}
