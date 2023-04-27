/*
 * start.S
 *
 * Entry point of the firmware.
 * The firmware code are executed in the ICache.
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 *
 */
#define TCSM_STACK_TOP	(0xF4002000 - 0x10)
#define TCSM_BANK0_PA	0xF4000000

	.text

	.globl _reset
	.globl _intr
	.globl _start
	.globl _intr_entry
	.globl _gp

	.extern main
	.extern trap_entry

	.set noreorder
	/* Reset Entry 0xF4000000 */
_reset:
	/* Jump to _start */
	b	_start
	nop

	/* Interrupt Entry 0xF4000100 */
	.org 0x100
_intr:
	/* Jump to Interrupt */
	b	_intr_entry
	nop

_start:
	/* Init mcu */
	li	$2, 0x0
	mtc0	$2, $12, 0 //cp0 status
	li	$2, 0x0
	mtc0	$2, $13, 0 //cp0 cause
	li	$2, TCSM_BANK0_PA
	mtc0	$2, $30, 0 //cp0 errorPC

	/*
	 * Clear BSS
	 */
	la  $9, __bss_start  # $9 <-- __bss_start
	la  $10, __bss_end   # $10 <-- __bss_end
1:
	sw   $0, 0($9)
	blt  $9, $10, 1b
	addi $9, 4

	/* setup stack, jump to C code */
	la	$28, _gp // Init gp
	la	$29, TCSM_STACK_TOP // Init sp
	j	main
	nop

_intr_entry:
	.set noat

	/* Push register to stack */
	subu  	$29, $29, 0x80 // 32 words
	sw	$1, 1*4($29)
	sw	$2, 2*4($29)
	sw	$3, 3*4($29)
	sw	$4, 4*4($29)
	sw	$5, 5*4($29)
	sw	$6, 6*4($29)
	sw	$7, 7*4($29)
	sw	$8, 8*4($29)
	sw	$9, 9*4($29)
	sw	$10, 10*4($29)
	sw	$11, 11*4($29)
	sw	$12, 12*4($29)
	sw	$13, 13*4($29)
	sw	$14, 14*4($29)
	sw	$15, 15*4($29)
	sw	$24, 24*4($29)
	sw	$25, 25*4($29)
	mfhi	$26
	sw	$26, 26*4($29)
	mflo	$27
	sw	$27, 27*4($29)
	sw	$28, 28*4($29)
	sw	$29, 29*4($29)
	sw	$30, 30*4($29)
	sw	$31, 31*4($29)

	//move	$4, $29
	jal 	trap_entry
	nop

	/* Pop stack to register */
	lw	$1, 1*4($29)
	lw	$2, 2*4($29)
	lw	$3, 3*4($29)
	lw	$4, 4*4($29)
	lw	$5, 5*4($29)
	lw	$6, 6*4($29)
	lw	$7, 7*4($29)
	lw	$8, 8*4($29)
	lw	$9, 9*4($29)
	lw	$10, 10*4($29)
	lw	$11, 11*4($29)
	lw	$12, 12*4($29)
	lw	$13, 13*4($29)
	lw	$14, 14*4($29)
	lw	$15, 15*4($29)
	lw	$24, 24*4($29)
	lw	$25, 25*4($29)
	lw	$26, 26*4($29)
	mthi	$26
	lw	$27, 27*4($29)
	mtlo	$27
	lw	$28, 28*4($29)
	lw	$29, 29*4($29)
	lw	$30, 30*4($29)
	lw	$31, 31*4($29)

	addu	$29, $29, 0x80 // 32 words
	eret // irq return

	.set at
	.set reorder
