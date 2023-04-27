#ifndef __BARRIER_H__
#define __BARRIER_H__

#define CKUSEG                  0x00000000
#define CKSEG0                  0x80000000
#define CKSEG1                  0xa0000000
#define CKSEG2                  0xc0000000
#define CKSEG3                  0xe0000000

#define __sync()                                \
	__asm__ __volatile__(                   \
		".set   push\n\t"               \
		".set   noreorder\n\t"          \
		".set   mips2\n\t"              \
		"sync\n\t"                      \
		".set   pop"                    \
		: /* no output */               \
		: /* no input */                \
		: "memory")


#define __fast_iob()                            \
	__asm__ __volatile__(                   \
		".set   push\n\t"               \
		".set   noreorder\n\t"          \
		"lw     $0,%0\n\t"              \
		"nop\n\t"                       \
		".set   pop"                    \
		: /* no output */               \
		: "m" (*(int *)CKSEG1)          \
		: "memory")


#define fast_mb()      __sync()
#define mb()           fast_mb()

#endif
