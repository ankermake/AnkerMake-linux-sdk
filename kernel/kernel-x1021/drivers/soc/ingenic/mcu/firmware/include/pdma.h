#ifndef __PDMA_H__
#define __PDMA_H__

#define PDMA_BCH_CHANNEL        0
#define PDMA_IO_CHANNEL         1
#define PDMA_DDR_CHANNEL        2
#define PDMA_MSG_CHANNEL	3
#define PDMA_MOVE_CHANNEL	4

#define NEMC_TO_TCSM	(1 << 0)
#define TCSM_TO_NEMC	(1 << 1)
#define BCH_TO_TCSM	(1 << 2)
#define TCSM_TO_BCH	(1 << 3)
#define DDR_TO_TCSM	(1 << 4)
#define TCSM_TO_DDR	(1 << 5)
#define TCSM_TO_NEMC_FILL	(1 << 6)
#define DMA_WAIT_FINISH       (1 << 16)

/* the descriptor of pdma is defined*/
#define DESC_DCM            0x00
#define DESC_DSA            0x01
#define DESC_DTA            0x02
#define DESC_DTC            0x03

/* trans Sys32 VA to Physics Address */
//#define CPHYSADDR(a)	((a) & 0x1fffffff)
#define CPHYSADDR(a)	(a)
/* trans Sys32 TCSM VA to Physics Address */
#define TCSMVAOFF	0xE0BDE000
#define TPHYSADDR(a)	((a) - TCSMVAOFF)

#define __pdma_write_cp0(value,cn,slt)		\
	__asm__ __volatile__(			\
			"mtc0\t%0, $%1, %2\n"	\
			: /* no output */	\
			:"r"(value),"i"(cn),"i"(slt))

#define __pdma_read_cp0(cn,slt)		\
	({				\
	 unsigned int _res_;		\
	 				\
	 __asm__ __volatile__(		\
		 "mfc0\t%0, $%1, %2\n"	\
		 :"=r"(_res_)		\
		 :"i"(cn), "i"(slt));	\
					\
	 _res_;				\
	 })

#define __pdma_mwait() 	__asm__ __volatile__("wait")

#define __pdma_irq_enable()					     \
	do{							     \
		volatile unsigned int tmp = __pdma_read_cp0(12,0);   \
		__pdma_write_cp0((tmp | 0x1), 12, 0);		     \
	}while(0)

#define __pdma_irq_disable()					      \
	do{							      \
		volatile unsigned int tmp = __pdma_read_cp0(12,0);    \
		__pdma_write_cp0((tmp & (~1)), 12, 0);		      \
	}while(0)

#endif /* __PDMA_H__ */
