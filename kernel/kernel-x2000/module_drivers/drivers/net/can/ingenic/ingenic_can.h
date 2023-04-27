#ifndef	__INGENIC_CAN_H__
#define __INGENIC_CAN_H__

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>


//CAN Mode Configuration Register
#define	CAN_MODE		0x0
#define CAN_MODE_RSTM		(0x01 << 0)
#define	CAN_MODE_LOM		(0x01 << 1)
#define CAN_MODE_STE		(0x01 << 2)
#define CAN_MODE_ERAR		(0x01 << 3)
#define CAN_MODE_PHYOFF		(0x01 << 4)
#define CAN_MODE_DMAEN		(0x01 << 8)
#define CAN_MODE_SLEEP		(0x01 << 9)
#define CAN_MODE_TXAR		(0x01 << 13)
#define CAN_MODE_RXCNT4		(0x01 << 14)


//CAN Command Register
#define	CAN_CMD			0x04
#define CAN_CMD_TR		(0x01 << 0)
#define CAN_CMD_AT		(0x01 << 1)
#define CAN_CMD_RRB		(0x01 << 2)
#define CAN_CMD_CDO		(0x01 << 3)
#define CAN_CMD_SRR		(0x01 << 4)
#define CAN_CMD_OFR		(0x01 << 5)
#define CAN_CMD_CTB		(0x01 << 6)

//CAN Status Register
#define CAN_STATUS		0x08
#define CAN_STATUS_RBS		(0x01 << 0)
#define CAN_STATUS_DOS		(0x01 << 1)
#define CAN_STATUS_TBS		(0x01 << 2)
#define CAN_STATUS_TCS		(0x01 << 3)
#define CAN_STATUS_RS		(0x01 << 4)
#define CAN_STATUS_TS		(0x01 << 5)
#define CAN_STATUS_ES		(0x01 << 6)
#define CAN_STATUS_BOS		(0x01 << 7)
#define CAN_STATUS_TBF		(0x01 << 8)

//CAN Interrupt Flag Register and Interrupt Enable Register
#define CAN_INTF		0x0C
#define CAN_INTF_RI		(0x01 << 0)
#define CAN_INTF_TI		(0x01 << 1)
#define CAN_INTF_EI		(0x01 << 2)
#define CAN_INTF_DOI		(0x01 << 3)
#define CAN_INTF_BOI		(0x01 << 4)
#define CAN_INTF_EPI		(0x01 << 5)
#define CAN_INTF_ALI		(0x01 << 6)
#define CAN_INTF_BEI		(0x01 << 7)
#define CAN_INTF_WKI		(0x01 << 8)
#define CAN_INTF_DMAI		(0x01 << 9)
#define CAN_INTF_TBI		(0x01 << 10)
#define CAN_INTF_OFI		(0x01 << 11)

//CAN Interrupt Enable Register
#define CAN_INTE		0x0C
#define CAN_INTE_RIE		(0x01 << 16)
#define CAN_INTE_TIE		(0x01 << 17)
#define CAN_INTE_EIE		(0x01 << 18)
#define CAN_INTE_DOIE		(0x01 << 19)
#define CAN_INTE_BOIE           (0x01 << 20)
#define CAN_INTE_EPIE           (0x01 << 21)
#define CAN_INTE_ALIE           (0x01 << 22)
#define CAN_INTE_BEIE           (0x01 << 23)
#define CAN_INTE_WKIE           (0x01 << 24)
#define CAN_INTE_DMAIE          (0x01 << 25)
#define CAN_INTE_TBIE           (0x01 << 26)
#define CAN_INTE_OFIE           (0x01 << 27)
#define CAN_ALL_WARNING_INT_EN		(CAN_INTE_EIE|CAN_INTE_DOIE|CAN_INTE_BOIE|CAN_INTE_EPIE|CAN_INTE_ALIE|CAN_INTE_BEIE|CAN_INTE_OFIE)

//CAN Bus Timing Register
#define CAN_BTR			0x10
#define CAN_BTR_SJW		(0x03 << 6)
#define CAN_BTR_CANCS		(0x3f << 0)
#define CAN_BTR_TSEG1		(0x0F << 8)
#define CAN_BTR_TSEG2		(0x07 << 12)
#define CAN_BTR_SAM		(0x01 << 15)


//CAN ID Filter Enable Register
#define CAN_FTER		0x18
#define CAN_FTER_FTER0		(0x01 << 0)
#define CAN_FTER_FTER1		(0x01 << 1)
#define CAN_FTER_FTER2		(0x01 << 2)
#define CAN_FTER_FTER3		(0x01 << 3)
//CAN Acceptance Filter Match Status Register
#define CAN_DSR			0x18
#define CAN_DSR_FMS0		(0x01 << 8)
#define CAN_DSR_FMS1		(0x01 << 9)
#define CAN_DSR_FMS2		(0x01 << 10)
#define CAN_DSR_FMS3		(0x01 << 11)
//CAN Arbitration Lost Capture Register
#define CAN_ALC			0x18


//CAN Error Code Capture Register
#define CAN_ECC			0x1C
#define CAN_ECC_SEG		(0x1F << 0)
#define CAN_ECC_DIR		(0x01 << 5)
#define CAN_ECC_ERRC		(0x03 << 6)
//CAN Error Warning Limit Register
#define CAN_EWLR		0x1C
//CAN Receive Error Count Register
#define CAN_REC			0x1C
//CAN Transmit Error Count Register
#define CAN_TEC			0x1C


//Acceptance Filter 0
#define CAN_AFID0		0x20
//Acceptance Filter 0 Mask
#define CAN_AFMK0		0x24

//Acceptance Filter 1
#define CAN_AFID1		0x28
//Acceptance Filter 1 Mask
#define CAN_AFMK1		0x2C

//Acceptance Filter 2
#define CAN_AFID2		0x30
//Acceptance Filter 2 Mask
#define CAN_AFMK2		0x34

//Acceptance Filter 3
#define CAN_AFID3		0x38
//Acceptance Filter 3 Mask
#define CAN_AFMK3		0x3C


//CAN Transmit Frame Information Register
#define CAN_TFIR		0x40
#define CAN_TFIR_DLC		(0x0F << 0)
//#define CAN_TFIR_BRS		(0x01 << 4)
//#define CAN_TFIR_FDF		(0x01 << 5)
#define CAN_TFIR_RTR		(0x01 << 6)
#define CAN_TFIR_FF		(0x01 << 7)
//CAN Transmit Identifier Register
#define CAN_TXID		0x44
//Transmit DATA Register
#define CAN_TXDATA		0x48
#define CAN_TXDATA1		0x4C

//CAN Receive Frame Information Register
#define CAN_RFIR		0x90
#define CAN_RFIR_DLC		(0x0F << 0)
//#define CAN_RFIR_BRS		(0x01 << 4)
//#define CAN_RFIR_FDF		(0x01 << 5)
#define CAN_RFIR_RTR		(0x01 << 6)
#define CAN_RFIR_FF		(0x01 << 7)
//#define CAN_RFIR_ESI		(0x01 << 8)
#define CAN_RFIR_RXFMS		(0x0F << 16)

//CAN Receive Identifier Register
#define CAN_RXID		0x94
//Receive DATA Register
#define CAN_RXDATA		0x98
#define CAN_RXDATA1		0x9C

//CAN Self-Loop Security Key Register
#define CAN_SLTM			0xFC
#define CAN_SLTM_CANSLKEY		(0x0F << 16)
#define CAN_SLTM_UNLOCK_CANSLKEY	(0x60000&CAN_SLTM_CANSLKEY)
#define CAN_SLTM_CANSLEN		(0x01 << 24)
#define CAN_SLTM_CANTXOFF		(0x01 << 25)
#define CAN_SLTM_IPRESET		(0x01 << 31)

#define CAN_SEND_OVER		0
#define CAN_SENDING		1


struct ingenic_can_priv {
	struct can_priv can;		/* must be the first member! */
	struct net_device *dev;
	struct napi_struct napi;

	void __iomem *reg_base;
	void __iomem *reg_base_phy;

	void *buffer;
	void *txbuffer;
	dma_addr_t buffer_dma;
	dma_addr_t txbuffer_dma;
	void *tdesc;
	dma_addr_t tdesc_phy;
	void *rdesc;
	dma_addr_t rdesc_phy;

	unsigned int record;

	struct dma_chan *txchan;
	struct dma_chan *rxchan;
	struct scatterlist *sg_tx; /* I/O scatter list */

	unsigned int rece_sum;
	unsigned int trans_sum;
	int flag;

	spinlock_t      lock;
	spinlock_t      txrx_lock;

	struct clk	*clk_gate;
	struct clk	*clk_cgu;
};

#endif
