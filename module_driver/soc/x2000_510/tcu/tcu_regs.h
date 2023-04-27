#ifndef _TCU_REGS_H_
#define _TCU_REGS_H_

#define IRQ_TCU0         (IRQ_INTC_BASE + 27)

#define TCU_IOBASE 0x10002000
#define TCU_ADDR(reg) ((volatile unsigned long *)KSEG1ADDR(TCU_IOBASE + (reg)))

/* 这四个寄存器是每个通道都有的寄存器，所以需要加上偏移N*0x40 */
#define CHN_TDFR	(0x0)    /*channel N data full register*/
#define CHN_TDHR	(0x4)    /*channel N data half register*/
#define CHN_TCNT	(0x8)    /*channel N counter register*/
#define CHN_TCSR	(0xc)    /*channel N control register*/

#define TCU_TSR		(0x1C)   /* Timer Stop Register */
#define TCU_TSSR	(0x2C)   /* Timer Stop Set Register */
#define TCU_TSCR	(0x3C)   /* Timer Stop Clear Register */
#define TCU_TER		(0x10)   /* Timer Counter Enable Register */
#define TCU_TESR	(0x14)   /* Timer Counter Enable Set Register */
#define TCU_TECR	(0x18)   /* Timer Counter Enable Clear Register */
#define TCU_TFR		(0x20)   /* Timer Flag Register */
#define TCU_TFSR	(0x24)   /* Timer Flag Set Register */
#define TCU_TFCR	(0x28)   /* Timer Flag Clear Register */
#define TCU_TMR		(0x30)   /* Timer Mask Register */
#define TCU_TMSR	(0x34)   /* Timer Mask Set Register */
#define TCU_TMCR	(0x38)   /* Timer Mask Clear Register */

#define HFLAG_OFFSET    16
#define FFLAG_OFFSET    0

#define CH_TDFR(n)	(0x40 + (n)*0x10) /* Timer Data Full Reg */
#define CH_TDHR(n)	(0x44 + (n)*0x10) /* Timer Data Half Reg */
#define CH_TCNT(n)	(0x48 + (n)*0x10) /* Timer Counter Reg */
#define CH_TCSR(n)	(0x4C + (n)*0x10) /* Timer Control Reg */

#define CHN_CAP(n)	(0xc0 + (n)*0x04) /*Capture register*/
#define CHN_CAP_VAL(n)	(0xe0 + (n)*0x04) /*Capture Value register*/
#define CHN_FIL_VAL(n)	(0x1a0 + (n)*0x04) /*filter value*/

//OST control
#define TER_OSTEN	(1 << 15)   /* enable the counter in ost */
#define TMR_OSTM	(1 << 15)   /* ost comparison match interrupt mask */
#define TFR_OSTF	(1 << 15)   /* ost interrupt flag */
#define	TSR_OSTS	(1 << 15)   /*the clock supplies to osts is stopped */

//watchdog bit
#define TSR_WDTS	(1 << 16)   /*the clock supplies to wdt is stopped */
#define TCU_FLAG_RD	(1 << 24)   /*HALF comparison match flag of WDT. (TCNT = TDHR) */

//timer control Register bit operation
#define CSR_EXT_EN	(1 << 2)  /* select extal as the timer clock input */
#define CSR_DIV1	(0x0 << 3) /*select the TCNT count clock frequency*/
#define CSR_DIV4	(0x1 << 3)
#define CSR_DIV16	(0x2 << 3)
#define CSR_DIV64	(0x3 << 3)
#define CSR_DIV256	(0x4 << 3)
#define CSR_DIV1024	(0x5 << 3)
#define CSR_DIV_MSK	(0x7 << 3)

#define CSR_DIR_HIGH	(0x0 << 8)  /*0:direction signal hold on 1*/
#define CSR_DIR_CLK	(0x1 << 8)  /*1:use clock with direction signal*/
#define CSR_DIR_GPIO0	(0x2 << 8)  /*2:use gpio0 with direction signal*/
#define CSR_DIR_GPIO1	(0x3 << 8)  /*3:use gpio1 with direction signal*/
#define CSR_DIR_QUADRATURE	(0x4 << 8)  /*4:use gpio0 and gpio1 quadrature result with direction signal*/
#define CSR_DIR_MSK	(0x7 << 8)

#define CSR_GATE_LOW	(0x0 << 11)  /*0:gate signal hold on 0*/
#define CSR_GATE_CLK	(0x1 << 11)  /*1:use clock with gate signal*/
#define CSR_GATE_GPIO0	(0x2 << 11)  /*2:use gpio0 with gate signal*/
#define CSR_GATE_GPIO1	(0x3 << 11)  /*3:use gpio1 with gate signal*/
#define CSR_GATE_MSK	(0x3 << 11)

#define CLK_POS		(16)
#define CLK_NEG		(17)
#define GPIO0_POS	(18)
#define GPIO0_NEG	(19)
#define GPIO1_POS	(20)
#define GPIO1_NEG	(21)


#define CSR_CM_FCZ	(0x0 << 22)  /*count range 0 <-> full with clear to 0*/
#define CSR_CM_MCZ	(0x1 << 22)  /*count range 0 <-> 0xffff with clear to 0*/
#define CSR_CM_MH	(0x2 << 22)  /*count range 0 <-> 0xffff with hold on*/
#define CSR_CM_MSK	(0x3 << 22)

#define TCU_CONTROL_BIT(n)	(1 << n)

//Capture register bit operation
#define CAP_SEL_CLK	(0x0 << 16)  /*TCU will capture clock*/
#define CAP_SEL_GPIO0	(0x1 << 16)  /*TCU will capture gpio0*/
#define CAP_SEL_GPIO1	(0x2 << 16)  /*TCU will capture gpio1*/
/*if capture mode is disable then these bits will:*/
#define CAP_SEL_CCG0P	(0x1 << 16)  /*counter will clear when gpio0 posedge*/
#define CAP_SEL_CCG1P	(0x2 << 16)  /*counter will clear when gpio1 posedge*/
#define CAP_SEL_MSK	(0x7 << 16)
#define CAP_NUM_MSK	(0xff)

//filter value register bit operation
#define  FIL_VAL_GPIO1_MSK	(0x3ff << 16)  /*Gpio1 filter counter value*/
#define  FIL_VAL_GPIO0_MSK	(0x3ff)	       /*Gpio0 filter counter value*/

//Common 1 bit offset
#define ONE_BIT_OFFSET(n)	TCU_CONTROL_BIT(n)
#define NR_TCU_CHNS  8

#define TCU_CHN_TDFR	(0x0)
#define TCU_CHN_TDHR	(0x4)
#define TCU_CHN_TCNT	(0x8)
#define TCU_CHN_TCSR	(0xc)
#define TCU_FULL0	(0x40)
#define TCU_CHN_OFFSET	(0x10)

#endif /* _TCU_REGS_H_ */