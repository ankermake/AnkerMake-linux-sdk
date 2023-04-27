#ifndef _UTILS_CLOCK_H_
#define _UTILS_CLOCK_H_

#include <linux/kernel.h>
#include <linux/sched.h>

/**
 * 获取本地时钟的值（us级）
 * @return 返回本地时钟的值（us级）
 */
extern unsigned long long local_clock_us(void);

/**
 * 获取本地时钟的值（ms级）
 * @return 返回本地时钟的值（ms级）
 */
extern unsigned long long local_clock_ms(void);
#endif
