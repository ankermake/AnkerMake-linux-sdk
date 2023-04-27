#include <linux/kernel.h>
#include <assert.h>

void __assert(const char *expr, const char *file, unsigned int line, const char *func)
{
    panic("assert %s failed in %s %d %s\n", expr, file, line, func);
}

EXPORT_SYMBOL(__assert);
