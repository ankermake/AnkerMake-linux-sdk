/*
 * common.h
 */
#ifndef __COMMON_H__
#define __COMMON_H__

#include <soc/base.h>
#include <soc/tcu.h>
#include <tcsm.h>
#include <pdmac.h>
#include <intc.h>
#include <uartc.h>
#include <gpioc.h>

#ifndef noinline
#define noinline __attribute__((noinline))
#endif

#ifndef __section
# define __section(S) __attribute__ ((__section__(#S)))
#endif

#define NULL		0
#define ALIGN_ADDR_WORD(addr)	(void *)((((unsigned int)(addr) >> 2) + 1) << 2)

#define REG8(addr)	*((volatile unsigned char *)(addr))
#define REG16(addr)	*((volatile unsigned short *)(addr))
#define REG32(addr)	*((volatile unsigned int *)(addr))

#define writel(value, addr)    (*(volatile unsigned int *) (addr) = (value))
#define readl(addr)    (*(volatile unsigned int *) (addr))

#define MCU_SOFT_IRQ		(1 << 2)
#define MCU_CHANNEL_IRQ		(1 << 1)
#define MCU_INTC_IRQ		(1 << 0)

typedef		char s8;
typedef	unsigned char	u8;
typedef		short s16;
typedef unsigned short	u16;
typedef		int s32;
typedef unsigned int	u32;

#endif /* __COMMON_H__ */
