#ifndef _COMMON_H_
#define _COMMON_H_

#include <linux/kernel.h>
#include <linux/io.h>
#include <utils/gpio.h>
#include <utils/string.h>
#include <assert.h>

#define assert_range(x, start, end) assert(((x) >= (start)) && ((x) <= (end)))

#define assert_bool(x) assert((x) >= 0 && (x) <= 1)

#define phys_to_page(phys) pfn_to_page((phys) >> PAGE_SHIFT)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_A) (sizeof(_A) / sizeof((_A)[0]))
#endif /* ARRAY_SIZE */

#endif /* _COMMON_H_ */
