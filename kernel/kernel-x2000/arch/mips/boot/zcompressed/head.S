/*
 *  linux/arch/mips/boot/compressed/head.S
 *
 *  Copyright (C) 2005-2008 Ingenic Semiconductor Inc.
 */

#include <asm/asm.h>
#include <asm/cacheops.h>
#include <asm/cachectl.h>
#include <asm/regdef.h>

#define IndexInvalidate_I       0x00
#define IndexWriteBack_D        0x01

	.set noreorder
	LEAF(startup)
startup:

	move	s0, a0		/* Save the boot loader transfered args */
	move	s1, a1
	move	s2, a2
	move	s3, a3

	la	a0, _edata
	la	a1, _end
1:	sw	zero, 0(a0)	/* Clear BSS section */
	bne	a1, a0, 1b
	addu	a0, 4

	la	sp, (.stack + 8192)

	la	a0, __image_begin
	la	a1, IMAGESIZE
	la	a2, LOADADDR
	la	ra, 1f
	la	k0, decompress_kernel
	jr	k0
	nop
1:

	move	a0, s0
	move	a1, s1
	move	a2, s2
	move	a3, s3

	li	k0, KERNEL_ENTRY

	jr	k0
	nop

2:
	b 2b

	END(startup)


	LEAF(flushcaches)
	li	k0, 0x80000000  # start address
	li	k1, 0x80008000  # end address (32KB I-Cache)
	la	t0, 1f
	la	t1, 0xa0000000
	or	t0, t0, t1
	jr.hb	t0
	nop
1:
	cache	IndexWriteBack_D, 0(k0)
	cache	IndexWriteBack_D, 32(k0)
	cache	IndexWriteBack_D, 64(k0)
	cache	IndexWriteBack_D, 96(k0)
	cache	IndexInvalidate_I, 0(k0)
	cache	IndexInvalidate_I, 32(k0)
	cache	IndexInvalidate_I, 64(k0)
	cache	IndexInvalidate_I, 96(k0)

	addu	k0, k0, 128
	bne	k0, k1, 1b
	nop
	sync
	la	t0, 3f
	jr.hb	t0
	nop
3:
	jr	ra
	nop
	END(flushcaches)

	.comm .stack,4096*2,4
