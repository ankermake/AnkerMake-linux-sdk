#ifndef _ASSERT_H_
#define _ASSERT_H_

void __assert(const char *expr, const char *file, unsigned int line, const char *func);

#ifndef assert
#define assert(_expr) \
    do { \
        if (!(_expr)) \
            __assert(#_expr, __FILE__, __LINE__, __func__); \
    } while (0)
#endif

#endif /* _ASSERT_H_ */