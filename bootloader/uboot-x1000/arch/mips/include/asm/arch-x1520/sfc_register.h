#ifndef __SFC_REGISTER_H
#define __SFC_REGISTER_H

/* SFC register */

#define	SFC_GLB				(0x0000)
#define	SFC_DEV_CONF			(0x0004)
#define	SFC_DEV_STA_EXP			(0x0008)
#define	SFC_DEV_STA_RT			(0x000c)
#define	SFC_DEV_STA_MSK			(0x0010)
#define	SFC_TRAN_CONF(n)		(0x0014 + (n * 4))
#define	SFC_TRAN_CONF0			(0x0014)
#define	SFC_TRAN_LEN			(0x002c)
#define	SFC_DEV_ADDR(n)			(0x0030 + (n * 4))
#define	SFC_DEV_ADDR0			(0x0030)
#define	SFC_DEV_ADDR_PLUS(n)		(0x0048 + (n * 4))
#define	SFC_DEV_ADDR_PLUS0		(0x0048)
#define	SFC_MEM_ADDR			(0x0060)
#define	SFC_TRIG			(0x0064)
#define	SFC_SR				(0x0068)
#define	SFC_SCR				(0x006c)
#define	SFC_INTC			(0x0070)
#define	SFC_FSM				(0x0074)
#define	SFC_CGE				(0x0078)
#define	SFC_RM_DR			(0x1000)

/* For SFC_GLB */
#define	GLB_TRAN_DIR_OFFSET			(13)
#define	GLB_TRAN_DIR			(1 << GLB_TRAN_DIR_OFFSET)
#define GLB_TRAN_DIR_WRITE		(1)
#define GLB_TRAN_DIR_READ		(0)

#define	GLB_THRESHOLD_OFFSET		(7)
#define GLB_THRESHOLD_MSK		(0x3f << GLB_THRESHOLD_OFFSET)
#define GLB_OP_MODE			(1 << 6)
#define SLAVE_MODE			(0x0)
#define DMA_MODE			(0x1)
#define GLB_PHASE_NUM_OFFSET		(3)
#define GLB_PHASE_NUM_MSK		(0x7  << GLB_PHASE_NUM_OFFSET)
#define GLB_WP_EN			(1 << 2)
#define GLB_BURST_MD_OFFSET		(0)
#define GLB_BURST_MD_MSK		(0x3  << GLB_BURST_MD_OFFSET)

/* For SFC_DEV_CONF */
#define	DEV_CONF_ONE_AND_HALF_CYCLE_DELAY	(3)
#define	DEV_CONF_ONE_CYCLE_DELAY	(2)
#define	DEV_CONF_HALF_CYCLE_DELAY	(1)
#define	DEV_CONF_NO_DELAY	        (0)
#define	DEV_CONF_SMP_DELAY_OFFSET	(16)
#define	DEV_CONF_SMP_DELAY_MSK		(0x3 << DEV_CONF_SMP_DELAY_OFFSET)
#define DEV_CONF_CMD_TYPE		(0x1 << 15)
#define DEV_CONF_STA_TYPE_OFFSET	(13)
#define DEV_CONF_STA_TYPE_MSK		(0x1 << DEV_CONF_STA_TYPE_OFFSET)
#define DEV_CONF_THOLD_OFFSET		(11)
#define	DEV_CONF_THOLD_MSK		(0x3 << DEV_CONF_THOLD_OFFSET)
#define DEV_CONF_TSETUP_OFFSET		(9)
#define DEV_CONF_TSETUP_MSK		(0x3 << DEV_CONF_TSETUP_OFFSET)
#define DEV_CONF_TSH_OFFSET		(5)
#define DEV_CONF_TSH_MSK		(0xf << DEV_CONF_TSH_OFFSET)
#define DEV_CONF_CPHA			(0x1 << 4)
#define DEV_CONF_CPOL			(0x1 << 3)
#define DEV_CONF_CEDL			(0x1 << 2)
#define DEV_CONF_HOLDDL			(0x1 << 1)
#define DEV_CONF_WPDL			(0x1 << 0)

/* For SFC_TRAN_CONF */
#define	TRAN_CONF_TRAN_MODE_OFFSET	(29)
#define	TRAN_CONF_TRAN_MODE_MSK		(0x7)
#define	TRAN_CONF_ADDR_WIDTH_OFFSET	(26)
#define TRAN_CONF_FMAT_OFFSET		(23)
#define TRAN_CONF_DATEEN_OFFSET		(16)
#define	TRAN_CONF_ADDR_WIDTH_MSK	(0x7 << ADDR_WIDTH_OFFSET)
#define TRAN_CONF_POLLEN		(1 << 25)
#define TRAN_CONF_POLL_OFFSET		(25)
#define TRAN_CONF_CMDEN			(1 << 24)
#define TRAN_CONF_FMAT			(1 << 23)
#define TRAN_CONF_FMAT_OFFSET           (23)
#define TRAN_CONF_DMYBITS_OFFSET	(17)
#define TRAN_CONF_DMYBITS_MSK		(0x3f << DMYBITS_OFFSET)
#define TRAN_CONF_DATEEN		(1 << 16)
#define	TRAN_CONF_CMD_OFFSET		(0)
#define	TRAN_CONF_CMD_MSK		(0xffff << CMD_OFFSET)
#define	TRAN_CONF_CMD_LEN		(1 << 15)

/* For SFC_TRIG */
#define TRIG_FLUSH			(1 << 2)
#define TRIG_STOP			(1 << 1)
#define TRIG_START			(1 << 0)

//For SFC_SR
#define FIFONUM_OFFSET  (16)
#define FIFONUM_MSK     (0x7f << FIFONUM_OFFSET)
#define BUSY_OFFSET     (5)
#define BUSY_MSK        (0x3 << BUSY_OFFSET)
#define END             (1 << 4)
#define TRAN_REQ        (1 << 3)
#define RECE_REQ        (1 << 2)
#define OVER            (1 << 1)
#define UNDER           (1 << 0)

/* For SFC_SCR */
#define	CLR_END			(1 << 4)
#define CLR_TREQ		(1 << 3)
#define CLR_RREQ		(1 << 2)
#define CLR_OVER		(1 << 1)
#define CLR_UNDER		(1 << 0)

/* For SFC_INTC */
#define MASK_END                (1 << 4)
#define MASK_TREQ               (1 << 3)
#define MASK_RREQ               (1 << 2)
#define MASK_OVER               (1 << 1)
#define MASK_UNDR               (1 << 0)

/* For SFC_TRAN_CONFx */
#define	TRAN_MODE_OFFSET	(29)
#define	TRAN_MODE_MSK		(0x7 << TRAN_MODE_OFFSET)
#define TRAN_SPI_STANDARD   (0x0)
#define TRAN_SPI_DUAL   (0x1 )
#define TRAN_SPI_QUAD   (0x5 )
#define TRAN_SPI_IO_QUAD   (0x6 )

//For SFC_FSM
#define FSM_AHB_OFFSET      (16)
#define FSM_AHB_MSK         (0xf << FSM_AHB_OFFSET)
#define FSM_SPI_OFFSET      (11)
#define FSM_SPI_MSK         (0x1f << FSM_SPI_OFFSET)
#define FSM_CLK_OFFSET      (6)
#define FSM_CLK_MSK         (0xf << FSM_CLK_OFFSET)
#define FSM_DMAC_OFFSET     (3)
#define FSM_DMAC_MSK        (0x7 << FSM_DMAC_OFFSET)
#define FSM_RMC_OFFSET      (0)
#define FSM_RMC_MSK         (0x7 << FSM_RMC_OFFSET)

//For SFC_CGE
#define CG_EN           (1 << 0)

#define	ADDR_WIDTH_OFFSET	(26)
#define	ADDR_WIDTH_MSK		(0x7 << ADDR_WIDTH_OFFSET)
#define POLLEN			(1 << 25)
#define CMDEN			(1 << 24)
#define FMAT			(1 << 23)
#define DMYBITS_OFFSET		(17)
#define DMYBITS_MSK		(0x3f << DMYBITS_OFFSET)
#define DATEEN			(1 << 16)
#define	CMD_OFFSET		(0)
#define	CMD_MSK			(0xffff << CMD_OFFSET)

#define N_MAX				6
#define SFC_FIFO_LEN		(63)


#define ENABLE			1
#define DISABLE			0

#define COM_CMD			1	// common cmd
#define POLL_CMD		2	// the cmd will poll the status of flash,ext: read status

#define DMA_OPS			1
#define CPU_OPS			0

#define TM_STD_SPI		0
#define TM_DI_DO_SPI	1
#define TM_DIO_SPI		2
#define TM_FULL_DIO_SPI	3
#define TM_QI_QO_SPI	5
#define TM_QIO_SPI		6
#define	TM_FULL_QIO_SPI	7

#define DEFAULT_ADDRSIZE	3

#define CPHA			(1 << 4)
#define CPOL			(1 << 3)
#define CEDL			(1 << 2)
#define HOLDDL			(1 << 1)
#define WPDL			(1 << 0)

#define THRESHOLD		31

#define DEF_ADDR_LEN    3
#define DEF_TCHSH       5
#define DEF_TSLCH       5
#define DEF_TSHSL_R     20
#define DEF_TSHSL_W     50

#endif
