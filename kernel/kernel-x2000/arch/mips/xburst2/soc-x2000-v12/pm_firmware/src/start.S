#include <regdef.h>

	.set        noreorder
	.globl      _start
	.ent        _start
	.text
_start:
	    //--------  a0 <- load address
	    //-------   -T 0
	addiu       sp,sp,-28
	sw          gp,16(sp)
	sw          ra,20(sp)
	sw          t9,24(sp)
	                    /* Initialize $gp */
	bal         1f
	nop
	.word       _gp
	.word       __rel_dyn_end
	.word       __rel_dyn_start
	.word       __image_copy_end
	.word       _GLOBAL_OFFSET_TABLE_
	.word       num_got_entries:

1:
	lw          gp, 0(ra)
	addu        gp,gp,a0
	//----------------------------------------------------------------------
	lw          t3, 20(ra)          # t3 <-- num_got_entries
	lw          t4, 16(ra)          # t4 <-- _GLOBAL_OFFSET_TABLE_
	addu        t4, a0,t4           # t4 now holds relocated _G_O_T_
	addi        t4, t4, 8           # skipping first two entries
	li          t2, 2
1:
	lw          t1, 0(t4)
	addu        t1, a0
	sw          t1, 0(t4)
2:
	addi        t2, 1
	blt         t2, t3, 1b
	addi        t4, 4
	//----------------------------------------------------------------------
	lw          t2, 4(ra)   // <-  __rel_dyn_end
	lw          t1, 8(ra)   // <- __rel_dyn_start

	addu        t2,t2,a0
	addu        t1,t1,a0

	b           2f                  # skip first reserved entry
	addiu       t1, 8

1:
	lw          t3, -4(t1)          # t3 <-- relocation info

	sub         t3, 3
	bnez        t3, 2f              # skip non R_MIPS_REL32 entries
	nop

	lw          t3, -8(t1)          # t3 <-- location to fix up in FLASH
	addu        t3, a0              # t3 <-- location to fix up in RAM

	lw          t4, 0(t3)           # t4 <-- original pointer
	addu        t4, a0              # t4 <-- adjusted pointer
	sw          t4, 0(t3)

2:
	blt         t1, t2, 1b
	addi        t1, 8               # each rel.dyn entry is 8 bytes


	la          t1, __bss_start     # t1 <-- __bss_start
	la          t2, __bss_end       # t2 <-- __bss_end
1:
	sw          zero, 0(t1)
	blt         t1, t2, 1b
	addi        t1, 4

	la          t0,p_slp_param
	sw          t0,0(a1)

	lw          gp,16(sp)
	lw          ra,20(sp)
	lw          t9,24(sp)
	addu        sp,sp,28

	jr          ra
	nop
	.end        _start

g_stack:
	.space      4096
	nop

	.macro	BUILD_CALLBACK_FUNC func
pm_\func:
	.global pm_\func
	.ent	pm_\func
	.type   pm_\func,@function
	.set    noreorder

	addiu   sp,sp,-32
	sw	gp,20(sp)
	sw	ra,24(sp)
	sw	t9,28(sp)

	bal   1f
	nop
1:
	addiu   t9,ra,4
	.cpload t9
// save sp to g_stack + 4096 - 4 and change sp into g_stack + 4096 - 8
	or   t0,sp,zero
	la   sp,(g_stack + 4096 - 4)
	sw   t0,0(sp)
	addiu sp,sp,-16

	la	 t9,\func
	jalr	t9
	nop

	la  t0,(g_stack + 4096 - 4)
	lw  sp,0(t0)


	lw	gp,20(sp)
	lw 	ra,24(sp)
	lw	t9,28(sp)
	addiu    sp,sp,32
	jr ra
	nop
	.end pm_\func
	.endm

	BUILD_CALLBACK_FUNC valid
	BUILD_CALLBACK_FUNC begin
	BUILD_CALLBACK_FUNC prepare
	BUILD_CALLBACK_FUNC enter
	BUILD_CALLBACK_FUNC finish
	BUILD_CALLBACK_FUNC end
	BUILD_CALLBACK_FUNC poweroff


cpu_resume_bootup:
	.global cpu_resume_bootup
	.ent cpu_resume_bootup
	.type cpu_resume_bootup,@function
	.set noreorder

	bal   1f
	nop
	1:
	addiu   t9,ra,4
	.cpload t9
	la	t9, cpu_resume
	jr.hb	t9
	nop
	.end cpu_resume_bootup


fastboot_cpu_resume:
	.global fastboot_cpu_resume
	.ent fastboot_cpu_resume
	.type fastboot_cpu_resume,@function
	.set noreorder

	bal   1f
	nop
	1:
	addiu   t9,ra,4
	.cpload t9

	la t9, restore_goto
	jr.hb t9
	nop

	.end fastboot_cpu_resume


