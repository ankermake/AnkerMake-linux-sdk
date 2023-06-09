/*
 * Startup Code for MIPS32 XBURST X1621 CPU-core
 *
 * Copyright (c) 2017 Ingenic Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <version.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm/cacheops.h>
#include <asm/arch/base.h>

#define RESERVED_FOR_SC(x) .space 1536, x

	.set noreorder
	.globl _start
	.section .start_section
_start:
#ifdef CONFIG_SPL_JZMMC_SUPPORT
	/* magic value ("MSPL") */
	.word 0x4d53504c
	.space 508, 0
	RESERVED_FOR_SC(0)
#elif defined(CONFIG_SPL_SFC_SUPPORT)
	.word 0x03040506
	.word 0x55aa5502
  #ifdef CONFIG_SPL_SFC_NOR
	.word 0x000000aa
  #else /*CONFIG_SPL_SFC_NAND*/
     #define SSI_PPB	(CONFIG_SPI_NAND_PPB / 32)
     #define SSI_BPP (CONFIG_SPI_NAND_BPP / 1024)
	.word (0x00000000 | (SSI_PPB<<16) | (SSI_BPP<<24))
  #endif
	.word 0x00000000
  #ifdef CONFIG_SPL_VERSION
	.word (0x00000000 | CONFIG_SPL_VERSION)
	.space (512-20),0
  #else
	.space (512-16),0
  #endif
	RESERVED_FOR_SC(0)
#elif defined(CONFIG_SPL_NOR_SUPPORT)
	.word 0
#endif /* CONFIG_SPL_NOR_SUPPORT */



#if 0

loop:
	la      v0, 0xb0012000
	li      t0, 1 << 17
	sw      t0, 0x74(v0)
	sw      t0, 0x18(v0)
	sw      t0, 0x24(v0)
	sw      t0, 0x38(v0)
	sw      t0, 0x48(v0)
	li      t1, 0x7fffff
loop2:
	subu    t1, t1, 1
	nop
	bgtz    t1, loop2
	nop
	sw      t0, 0x44(v0)
	li      t1, 0x7fffff
loop3:
	subu    t1, t1, 1
	nop
	bgtz    t1, loop3
	nop
	b       loop
	nop
#endif



	/* Invalidate BTB */
	mfc0	v0, CP0_CONFIG, 7
	nop
	ori	v0, v0, 2
	mtc0	v0, CP0_CONFIG, 7
	nop

        mfc0	v0, $12, 2
        nop
        li	v1, 0x7fffffff
        and	v0, v0, v1
        mtc0	v0, $12, 2
        nop

	/*
	 * CU0=UM=EXL=IE=0, BEV=ERL=1, IP2~7=1
	 */
	li	t0, 0x0040FC04
	mtc0	t0, CP0_STATUS

	/* CAUSE register */
	/* IV=1, use the specical interrupt vector (0x200) */
	li	t1, 0x00800000
	mtc0	t1, CP0_CAUSE

	/* enable bridge radical mode */
//	la	t0, CPM_BASE
//	lw	t1, 0x24(t0)
//	ori	t1, t1, 0x22
//	sw	t1, 0x24(t0)

#ifdef CONFIG_PALLADIUM
	.set	mips32
init_caches:
	la	$25, 0x80000000
        li      $2, 3                   // cacheable for kseg0 access
        mtc0    $2, $16                 // CP0_CONFIG
        nop

#if 1
//	li      $2, 0x20000000          // enable idx-store-data cache insn
//	mtc0    $2, $26                 // CP0_ERRCTL
#else
	mtc0    $0, $26                 // CP0_ERRCTL
#endif
	mfc0    $3, $4,7		//enable rdhwr
	ori     $3, $3,1
	mtc0    $3, $4,7

        ori     $2, $25, 0              // start address
        ori     $3, $2, CONFIG_SYS_DCACHE_SIZE          // end address, total 16KB
        mtc0    $0, $28, 0              // CP0_TAGLO
        mtc0    $0, $28, 1              // CP0_DATALO
cache_clear_a_line:
        cache   0x8, 0($2)              // Index_Store_Tag_I
        cache   0x9, 0($2)              // Index_Store_Tag_D
        bne     $2, $3, cache_clear_a_line
        addiu   $2, $2, 32              // increment CACHE_LINE_SIZE
        ori     $2, $25, 0              // start address
        ori     $3, $2, CONFIG_SYS_ICACHE_SIZE          // end address, total 16KB spl stack space
        la      $4, 0x1ffff000          // physical address and 4KB page mask
cache_alloc_a_line:
        and     $5, $2, $4
        ori     $5, $5, 1               // V bit of the physical tag
        mtc0    $5, $28, 0              // CP0_TAGLO
        cache   0x8, 0($2)              // Index_Store_Tag_I
        cache   0x9, 0($2)              // Index_Store_Tag_D
        bne     $2, $3, cache_alloc_a_line
        addiu   $2, $2, 32              // increment CACHE_LINE_SIZE

	li	sp, 0x80008000
#endif

	/* Set up stack */
#ifdef CONFIG_SPL_STACK
	li	sp, CONFIG_SPL_STACK
#endif
#ifdef CONFIG_SPL_NOR_SUPPORT
	.extern __data_start
	.extern __data_end
	/*Init data section, nor spl*/
	la	t0, __data_start
	la	t1, __data_end
	la	t2, 0x80000000
1:
	lw	t3, 0(t0)
	addiu	t0, t0, 4
	sw	t3, 0(t2)
	addiu	t2, t2, 4
	bne	t0, t1, 1b
	nop
#endif
	j	board_init_f
	nop

