#ifndef __COMMON_H__
#define __COMMON_H__

#include <base.h>


#define readb(addr)     (*(volatile unsigned char *)(addr | 0xa0000000))
#define readw(addr)     (*(volatile unsigned short *)(addr | 0xa0000000))
#define readl(addr)     (*(volatile unsigned int *)(addr | 0xa0000000))
#define writeb(b, addr) (*(volatile unsigned char *)(addr | 0xa0000000)) = (b)
#define writew(b, addr) (*(volatile unsigned short *)(addr | 0xa0000000)) = (b)
#define writel(b, addr) (*(volatile unsigned int *)(addr | 0xa0000000)) = (b)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


#endif
