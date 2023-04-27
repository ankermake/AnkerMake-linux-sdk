#ifndef _ASSERT_H_
#define _ASSERT_H_

//#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void hang(void) __attribute__ ((__noreturn__));

void __assert ( const char *, const char *, int)
	    __attribute__ ((__noreturn__));

void __assert_func (const char *, int, const char *, const char *)
	    __attribute__ ((__noreturn__));

void __assert_no_file(const char *func, int line, const char *expr)
	    __attribute__ ((__noreturn__));

#ifdef APP_libmcu_lib_assert_no_file_name
#define assert(_expr) \
    do { \
        if (!(_expr)) \
            __assert_no_file(__func__, __LINE__, #_expr); \
    } while (0)
#else
#define assert(_expr) \
    do { \
        if (!(_expr)) \
            __assert_func(__FILE__, __LINE__, __func__, #_expr); \
    } while (0)
#endif

#ifndef panic
#define panic(x...) \
    do { \
        printf(x); \
        hang(); \
    } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASSERT_H_ */
