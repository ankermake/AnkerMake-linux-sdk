#include <linux/kernel.h>

static void __assert(const char *expr, const char *file, unsigned int line, const char *func)
{
    panic("assert %s failed in %s %d %s\n", expr, file, line, func);
}

#ifndef assert
#define assert(_expr) \
    do { \
        if (!(_expr)) \
            __assert(#_expr, __FILE__, __LINE__, __func__); \
    } while (0)
#endif

