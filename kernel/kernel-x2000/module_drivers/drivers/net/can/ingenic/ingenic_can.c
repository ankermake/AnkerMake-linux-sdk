#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/can/dev.h>
#include <linux/can/error.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>

#include "ingenic_can.h"

#define MODNAME	"ingenic_can"
#define TX_ECHO_SKB_MAX 1

#define DMA_RX_DESC_NUM		8
#define DMA_RX_DESC_DTC		8
#define DMA_BUFF_SIZE		16			/*A frame of data is 16 bytes*/
#define DMA_RX_BUFF_SIZE	(DMA_BUFF_SIZE * DMA_RX_DESC_DTC)
#define INGENIC_CAN_FILTER_ALL_ACCEPT	0x1FFFFFFF	/*Filter Settings all pass, SocketCan have software filters*/
#define PDMA_IOBASE	(0xb3420000)
#define DMA_CH_DTC	(0x08)
#define DMA_CHA_DTC(ch)	((void __iomem *)( PDMA_IOBASE + ch * 0x20 + DMA_CH_DTC))
#define INGENIC_CAN_DEBUG


static const struct can_bittiming_const ingenic_can_bittiming_const = {
	.name           = MODNAME,
	.tseg1_min      = 1,
	.tseg1_max      = 16,
	.tseg2_min      = 1,
	.tseg2_max      = 8,
	.sjw_max        = 4,
	.brp_min        = 1,
	.brp_max        = 64,
	.brp_inc        = 1,
};


static uint32_t can_readl(struct ingenic_can_priv *priv,uint32_t reg_off)
{
	return readl(priv->reg_base + reg_off);
}


static void can_writel(struct ingenic_can_priv *priv,uint32_t val, uint32_t reg_off)
{
	writel(val, priv->reg_base + reg_off);
}

#ifdef INGENIC_CAN_DEBUG
static void ingenic_can_dump_registers(struct ingenic_can_priv *priv)
{
	void __iomem *ingenic_can_iomem = priv->reg_base;
	struct net_device *dev = priv->dev;

	netdev_info(dev, "CAN_MODE %p : 0x%08x\n",(ingenic_can_iomem + CAN_MODE),can_readl(priv, CAN_MODE));
	netdev_info(dev, "CAN_CMD %p : 0x%08x\n",(ingenic_can_iomem + CAN_CMD),can_readl(priv, CAN_CMD));
	netdev_info(dev, "CAN_STATUS %p : 0x%08x\n",(ingenic_can_iomem + CAN_STATUS),can_readl(priv, CAN_STATUS));
	netdev_info(dev, "CAN_INT %p : 0x%08x\n",(ingenic_can_iomem + CAN_INTF),can_readl(priv, CAN_INTF));
	netdev_info(dev, "CAN_BTR %p : 0x%08x\n",(ingenic_can_iomem + CAN_BTR),can_readl(priv, CAN_BTR));
	netdev_info(dev, "CAN_ECC %p : 0x%08x\n",(ingenic_can_iomem + CAN_ECC),can_readl(priv, CAN_ECC));
	netdev_info(dev, "CAN_AFID0 %p : 0x%08x\n",(ingenic_can_iomem + CAN_AFID0),can_readl(priv, CAN_AFID0));
	netdev_info(dev, "CAN_AFMK0 %p : 0x%08x\n",(ingenic_can_iomem + CAN_AFMK0),can_readl(priv, CAN_AFMK0));
	netdev_info(dev, "CAN_AFID1 %p : 0x%08x\n",(ingenic_can_iomem + CAN_AFID1),can_readl(priv, CAN_AFID1));
	netdev_info(dev, "CAN_AFMK1 %p : 0x%08x\n",(ingenic_can_iomem + CAN_AFMK1),can_readl(priv, CAN_AFMK1));
	netdev_info(dev, "CAN_AFID2 %p : 0x%08x\n",(ingenic_can_iomem + CAN_AFID2),can_readl(priv, CAN_AFID2));
	netdev_info(dev, "CAN_AFMK2 %p : 0x%08x\n",(ingenic_can_iomem + CAN_AFMK2),can_readl(priv, CAN_AFMK2));
	netdev_info(dev, "CAN_AFID3 %p : 0x%08x\n",(ingenic_can_iomem + CAN_AFID3),can_readl(priv, CAN_AFID3));
	netdev_info(dev, "CAN_AFMK3 %p : 0x%08x\n",(ingenic_can_iomem + CAN_AFMK3),can_readl(priv, CAN_AFMK3));
	netdev_info(dev, "CAN_TFIR %p : 0x%08x\n",(ingenic_can_iomem + CAN_TFIR),can_readl(priv, CAN_TFIR));
	netdev_info(dev, "CAN_TXID %p : 0x%08x\n",(ingenic_can_iomem + CAN_TXID),can_readl(priv, CAN_TXID));
	netdev_info(dev, "CAN_TXDATA %p : 0x%08x\n",(ingenic_can_iomem + CAN_TXDATA),can_readl(priv, CAN_TXDATA));
	netdev_info(dev, "CAN_TXDATA %p : 0x%08x\n",(ingenic_can_iomem + CAN_TXDATA1),can_readl(priv, CAN_TXDATA1));
	netdev_info(dev, "CAN_RFIR %p : 0x%08x\n",(ingenic_can_iomem + CAN_RFIR),can_readl(priv, CAN_RFIR));
	netdev_info(dev, "CAN_RXID %p : 0x%08x\n",(ingenic_can_iomem + CAN_RXID),can_readl(priv, CAN_RXID));
	netdev_info(dev, "CAN_RXDATA %p : 0x%08x\n",(ingenic_can_iomem + CAN_RXDATA),can_readl(priv, CAN_RXDATA));
	netdev_info(dev, "CAN_RXDATA %p : 0x%08x\n",(ingenic_can_iomem + CAN_RXDATA1), can_readl(priv, CAN_RXDATA1));
	netdev_info(dev, "CAN_SLTM %p : 0x%08x\n",(ingenic_can_iomem + CAN_SLTM),can_readl(priv, CAN_SLTM));
}
#endif

static int ingenic_can_close(struct net_device *dev)
{
	struct ingenic_can_priv *priv = netdev_priv(dev);

	can_writel(priv, CAN_MODE_RSTM, CAN_MODE);
	if(priv->can.ctrlmode == CAN_CTRLMODE_LOOPBACK) {
		can_writel(priv, CAN_SLTM_UNLOCK_CANSLKEY, CAN_SLTM);
		can_writel(priv, 0, CAN_SLTM);
	}
#ifdef CONFIG_INGENIC_CAN_USE_PDMA
	dmaengine_terminate_all(priv->rxchan);
	dmaengine_terminate_all(priv->txchan);
#endif

	priv->can.state = CAN_STATE_STOPPED;

	close_candev(dev);

	return 0;
}

static int ingenic_can_set_baudrate(struct net_device *dev)
{
	struct ingenic_can_priv *priv = netdev_priv(dev);
	struct can_bittiming brt = priv->can.bittiming;
	int reg_btr;

	/*    set can baud rate   */
	reg_btr = (((brt.sjw-1) << 6)&CAN_BTR_SJW) | ((brt.brp-1)&CAN_BTR_CANCS) | (((brt.phase_seg2-1) << 12)&CAN_BTR_TSEG2) | (((brt.phase_seg1-1) << 8)&CAN_BTR_TSEG1);

	can_writel(priv, reg_btr, CAN_BTR);

	netdev_info(dev, "set can bitrate:%d ok !\n",brt.bitrate);

	return 0;
}

static int ingenic_can_get_berr_counter(const struct net_device *dev,
		struct can_berr_counter *bec)
{
	struct ingenic_can_priv *priv = netdev_priv(dev);
	unsigned int reg_err;

	reg_err = can_readl(priv,CAN_ECC);
	bec->rxerr = (reg_err >> 16) & 0xFF;
	bec->txerr = (reg_err >> 24) & 0xFF;

	return 0;
}

#ifdef CONFIG_INGENIC_CAN_USE_PDMA
static int ingenic_can_dma_rxtx_config(struct ingenic_can_priv *priv)
{
	struct dma_slave_config tx_config;
	struct dma_slave_config rx_config;

	tx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	tx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	tx_config.src_maxburst = 4;
	tx_config.dst_maxburst = 4;
	tx_config.slave_id = 0;
	tx_config.direction = DMA_MEM_TO_DEV;
	tx_config.dst_addr = (dma_addr_t)(priv->reg_base_phy + CAN_TFIR);
	dmaengine_slave_config(priv->txchan, &tx_config);

	rx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	rx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	rx_config.src_maxburst = 16;
	rx_config.dst_maxburst = 16;
	rx_config.slave_id = 0;
	rx_config.direction = DMA_DEV_TO_MEM;
	rx_config.src_addr = (dma_addr_t)(priv->reg_base_phy + CAN_RFIR);
	dmaengine_slave_config(priv->rxchan, &rx_config);

	return 0;
}

static int ingenic_can_dma_rx_pending(struct ingenic_can_priv *priv)
{
	struct dma_async_tx_descriptor *rxdesc;
	struct dma_chan *rxchan = priv->rxchan;

	if(!rxchan) {
		netdev_err(priv->dev,"no dma rx channel\n");
		return -ENODEV;
	}

	memset(priv->buffer, 0, (DMA_BUFF_SIZE * DMA_RX_DESC_NUM));

	rxdesc = dmaengine_prep_dma_cyclic(rxchan,
			priv->buffer_dma,
			(DMA_RX_BUFF_SIZE * DMA_RX_DESC_NUM),
			DMA_RX_BUFF_SIZE,
			DMA_DEV_TO_MEM,
			DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!rxdesc) {
		netdev_err(priv->dev,"rxdesc: device_prep_dma_cyclic error\n");
		dmaengine_terminate_all(rxchan);
		return -1;
	}

	rxdesc->callback = NULL;
	rxdesc->callback_param = NULL;

	dmaengine_submit(rxdesc);
	dma_async_issue_pending(rxchan);

	return 0;
}

static int ingenic_can_dma_tx_pending(struct ingenic_can_priv *priv)
{
	struct dma_async_tx_descriptor *txdesc;
	struct dma_chan *txchan = priv->txchan;

	if(!txchan) {
		netdev_err(priv->dev,"no dma tx channel\n");
		return -ENODEV;
	}

	sg_init_one(priv->sg_tx, priv->txbuffer, DMA_BUFF_SIZE);

	if(dma_map_sg(priv->txchan->device->dev,
				priv->sg_tx, 1, DMA_TO_DEVICE) != 1) {
		netdev_err(priv->dev,"dma_map_sg tx error\n");
	}

	txdesc = dmaengine_prep_slave_sg(txchan,
			priv->sg_tx,
			1,
			DMA_MEM_TO_DEV,
			DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!txdesc) {
		netdev_err(priv->dev,"txdesc: device_prep_slave_sg error\n");
		goto unmap_sg;
	}

	txdesc->callback = NULL;
	txdesc->callback_param = NULL;

	dmaengine_submit(txdesc);
	dma_async_issue_pending(txchan);

	return 0;
unmap_sg:
	dma_unmap_sg(txchan->device->dev, priv->sg_tx, 1, DMA_MEM_TO_DEV);
	dmaengine_terminate_all(txchan);

	return -1;

}
#endif

static int ingenic_can_send_data(struct ingenic_can_priv *priv, unsigned type, unsigned id, unsigned data0, unsigned data1)
{
	unsigned int *txdata = (unsigned int*)priv->txbuffer;
	int timeout = 1000;

	while(priv->flag && timeout --) {
		udelay(10);
	}

	if(timeout <= 0) {
		netdev_err(priv->dev,"error, tx buffer full\n");
		return -EBUSY;
	}

	priv->flag = CAN_SENDING;

	txdata[0] = type & 0xFF;
	txdata[1] = id;
	txdata[2] = data0;
	txdata[3] = data1;

#ifdef CONFIG_INGENIC_CAN_USE_PDMA
	ingenic_can_dma_tx_pending(priv);

	if(priv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK)
	{
		mdelay(1);
		can_writel(priv, CAN_CMD_SRR, CAN_CMD);
	}
#else
	can_writel(priv, (type & 0xFF), CAN_TFIR);
	can_writel(priv, id, CAN_TXID);
	can_writel(priv, data0, CAN_TXDATA);
	can_writel(priv, data1, CAN_TXDATA1);
	if(priv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK) {
		mdelay(1);
		can_writel(priv, CAN_CMD_SRR, CAN_CMD);
	} else {
		can_writel(priv, CAN_CMD_TR, CAN_CMD);
	}

#endif

	return 0;
}

static int ingenic_can_set_filter(struct ingenic_can_priv *priv)
{
	struct can_filter rf[4];

	rf[0].can_id = 0;
	rf[0].can_mask = INGENIC_CAN_FILTER_ALL_ACCEPT;
	rf[1].can_id = 0;
	rf[1].can_mask = INGENIC_CAN_FILTER_ALL_ACCEPT;
	rf[2].can_id = 0;
	rf[2].can_mask = INGENIC_CAN_FILTER_ALL_ACCEPT;
	rf[3].can_id = 0;
	rf[3].can_mask = INGENIC_CAN_FILTER_ALL_ACCEPT;

	can_writel(priv, rf[0].can_id, CAN_AFID0);
	can_writel(priv, rf[0].can_mask, CAN_AFMK0);
	can_writel(priv, rf[1].can_id, CAN_AFID1);
	can_writel(priv, rf[1].can_mask, CAN_AFMK1);
	can_writel(priv, rf[2].can_id, CAN_AFID2);
	can_writel(priv, rf[2].can_mask, CAN_AFMK2);
	can_writel(priv, rf[3].can_id, CAN_AFID3);
	can_writel(priv, rf[3].can_mask, CAN_AFMK3);

	return 0;
}

static int ingenic_can_set_normal_mode(struct ingenic_can_priv *priv)
{
	/*enter reset mode*/
	can_writel(priv, CAN_MODE_RSTM, CAN_MODE);
	/*clear the state bit*/
	can_writel(priv, CAN_CMD_CTB | CAN_CMD_RRB, CAN_CMD);
#ifdef CONFIG_INGENIC_CAN_USE_PDMA
	/*set auto send and enable rx dma */
	can_writel(priv, CAN_MODE_RXCNT4 | CAN_MODE_TXAR | CAN_MODE_DMAEN, CAN_MODE);
	/*set dma and some error interrupt*/
	can_writel(priv, CAN_ALL_WARNING_INT_EN | CAN_INTE_DMAIE | CAN_INTE_TIE, CAN_INTF);
#else
	/*set auto send and enable rx dma */
	can_writel(priv, CAN_MODE_RXCNT4, CAN_MODE);
	/*set dma and some error interrupt*/
	can_writel(priv, CAN_ALL_WARNING_INT_EN | CAN_INTE_RIE | CAN_INTE_TIE, CAN_INTF);
#endif

	return 0;
}

static int ingenic_can_set_loopback_mode(struct ingenic_can_priv *priv)
{
	/*enter reset mode*/
	can_writel(priv, CAN_MODE_RSTM, CAN_MODE);
	/*unlock the CANSLKEY*/
	can_writel(priv, CAN_SLTM_UNLOCK_CANSLKEY, CAN_SLTM);
	/*enable canslen and cantxoff*/
	can_writel(priv, CAN_SLTM_UNLOCK_CANSLKEY | CAN_SLTM_CANSLEN | CAN_SLTM_CANTXOFF, CAN_SLTM);
#ifdef CONFIG_INGENIC_CAN_USE_PDMA
	/*set loopback mode*/
	can_writel(priv, CAN_MODE_RXCNT4 | CAN_MODE_STE | CAN_MODE_DMAEN, CAN_MODE);
	/*set dma and some error interrupt*/
	can_writel(priv, CAN_ALL_WARNING_INT_EN | CAN_INTE_DMAIE | CAN_INTE_TIE, CAN_INTF);
#else
	/*set loopback mode*/
	can_writel(priv, CAN_MODE_RXCNT4 | CAN_MODE_STE, CAN_MODE);
	/*set dma and some error interrupt*/
	can_writel(priv, CAN_ALL_WARNING_INT_EN | CAN_INTE_RIE | CAN_INTE_TIE, CAN_INTF);
#endif

	return 0;
}

static int ingenic_can_set_listenonly_mode(struct ingenic_can_priv *priv)
{
	/*enter reset mode*/
	can_writel(priv, CAN_MODE_RSTM, CAN_MODE);
#ifdef CONFIG_INGENIC_CAN_USE_PDMA
	/*set auto send and enable rx dma */
	can_writel(priv, CAN_MODE_LOM | CAN_MODE_RXCNT4 | CAN_MODE_DMAEN, CAN_MODE);
//	can_writel(priv, CAN_MODE_LOM | CAN_MODE_RXCNT4 | CAN_MODE_TXAR | CAN_MODE_DMAEN, CAN_MODE);
	/*set dma and some error interrupt*/
	can_writel(priv, CAN_ALL_WARNING_INT_EN | CAN_INTE_DMAIE, CAN_INTF);
#else
	/*set auto send and enable rx dma */
	can_writel(priv, CAN_MODE_LOM | CAN_MODE_RXCNT4, CAN_MODE);
	/*set dma and some error interrupt*/
	can_writel(priv, CAN_ALL_WARNING_INT_EN | CAN_INTE_RIE, CAN_INTF);
#endif

	return 0;
}


static int ingenic_can_set_work_mode(struct ingenic_can_priv *priv)
{
	if(priv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK)
		ingenic_can_set_loopback_mode(priv);
	else if(priv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY)
		ingenic_can_set_listenonly_mode(priv);
	else
		ingenic_can_set_normal_mode(priv);

	return 0;
}

static int ingenic_can_chip_start(struct ingenic_can_priv *priv)
{

	priv->rece_sum = 0;
	priv->trans_sum = 0;
	priv->flag = CAN_SEND_OVER;
#ifdef CONFIG_INGENIC_CAN_USE_PDMA
	ingenic_can_dma_rxtx_config(priv);
	ingenic_can_set_filter(priv);
	ingenic_can_set_work_mode(priv);
	ingenic_can_dma_rx_pending(priv);
	priv->can.state = CAN_STATE_ERROR_ACTIVE;
#else
	ingenic_can_set_filter(priv);
	ingenic_can_set_work_mode(priv);
	priv->can.state = CAN_STATE_ERROR_ACTIVE;
#endif

	return 0;
}

static int ingenic_can_restart_set_mode(struct net_device *dev, enum can_mode mode)
{
	struct ingenic_can_priv *priv = netdev_priv(dev);

	switch (mode) {
	case CAN_MODE_START:
#ifdef CONFIG_INGENIC_CAN_USE_PDMA
		dmaengine_terminate_all(priv->rxchan);
		dmaengine_terminate_all(priv->txchan);
#endif
		ingenic_can_chip_start(priv);
		priv->can.can_stats.restarts++;
		break;

	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static netdev_tx_t ingenic_can_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct can_frame *cf = (struct can_frame *)skb->data;
	struct net_device_stats *stats = &dev->stats;
	struct ingenic_can_priv *priv = netdev_priv(dev);
	unsigned int send[2];
	unsigned int can_tfir = 0;

	if(can_dropped_invalid_skb(dev, skb))
		return NETDEV_TX_OK;

	memset(send, 0, CAN_MAX_DLEN);

	spin_lock(&priv->txrx_lock);
	can_tfir |= cf->can_dlc;
	if(cf->can_id & CAN_EFF_FLAG)
		can_tfir |= CAN_TFIR_FF;
	if(cf->can_id & CAN_RTR_FLAG)
		can_tfir |= CAN_TFIR_RTR;
	memcpy(send, cf->data, cf->can_dlc);
	stats->tx_bytes += cf->can_dlc;
	stats->tx_packets ++;

	if(!(priv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY))
		ingenic_can_send_data(priv, can_tfir, (cf->can_id & CAN_EFF_MASK), send[0], send[1]);

	can_put_echo_skb(skb, dev, 0);

	spin_unlock(&priv->txrx_lock);

	return 0;
}

static int ingenic_can_chip_stop(struct ingenic_can_priv *priv)
{
	/*disable all interrupt*/
	can_writel(priv, 0, CAN_INTF);
	can_writel(priv, CAN_MODE_RSTM, CAN_MODE);
#ifdef CONFIG_INGENIC_CAN_USE_PDMA
	dmaengine_terminate_all(priv->rxchan);
	dmaengine_terminate_all(priv->txchan);
#endif

	return 0;
}

static void ingenic_can_irq_error(struct net_device *dev,unsigned int flag)
{
	struct ingenic_can_priv *priv = netdev_priv(dev);
	unsigned int irq_status = flag;
	enum can_state new_state = CAN_STATE_ERROR_ACTIVE;
	struct sk_buff *skb;
	struct can_frame *cf;

	if(irq_status & CAN_INTF_BEI) {
		netdev_err(dev, "%s Bus Error\n", dev->name);
		priv->can.can_stats.bus_error++;
		dev->stats.rx_errors++;
	}
	if(irq_status & CAN_INTF_ALI) {
		netdev_err(dev, "%s Arbitration Lost\n", dev->name);
		priv->can.can_stats.arbitration_lost++;
		dev->stats.rx_errors++;
	}
	if(irq_status & CAN_INTF_OFI) {
		netdev_err(dev, "%s Overload Frame\n", dev->name);
		dev->stats.rx_frame_errors++;
	}
	if(irq_status & CAN_INTF_DOI) {
		netdev_err(dev, "%s Data Overrun\n", dev->name);
		dev->stats.rx_over_errors++;
	}
	if(irq_status & CAN_INTF_EI) {
		netdev_err(dev, "%s Error Warning\n", dev->name);
		priv->can.can_stats.error_warning++;
		dev->stats.rx_errors++;
		new_state = CAN_STATE_ERROR_WARNING;
	}
	if(irq_status & CAN_INTF_EPI) {
		netdev_err(dev, "%s Error Passive\n", dev->name);
		priv->can.can_stats.error_passive++;
		dev->stats.rx_errors++;
		new_state = CAN_STATE_ERROR_PASSIVE;
	}
	if(irq_status & CAN_INTF_BOI) {
		netdev_err(dev, "%s Bus Off\n", dev->name);
		priv->can.can_stats.bus_off++;
		dev->stats.rx_errors++;
		new_state = CAN_STATE_BUS_OFF;
	}

	if(new_state == priv->can.state)
		return;
	skb = alloc_can_err_skb(dev, &cf);
	if(!skb)
		return;
	if(new_state == CAN_STATE_BUS_OFF) {
		if(!priv->can.restart_ms) {
			netdev_err(dev, "%s ingenic_can stop !!!, please reset can.\n", dev->name);
		}
		ingenic_can_chip_stop(priv);
		can_bus_off(priv->dev);
	}

	priv->can.state = new_state;
}


static int ingenic_can_read_msg(struct net_device *dev, int *rdata)
{
	struct sk_buff *skb_rx;
	struct can_frame *cf;
	struct net_device_stats *stats = &dev->stats;
	struct ingenic_can_priv *priv = netdev_priv(dev);
	unsigned long flags;

	skb_rx = alloc_can_skb(dev, &cf);

	spin_lock_irqsave(&priv->txrx_lock, flags);
	cf->can_id = rdata[1];
	if(rdata[0] & CAN_RFIR_FF)
		cf->can_id |= CAN_EFF_FLAG;
	if(rdata[0] & CAN_RFIR_RTR)
		cf->can_id |= CAN_RTR_FLAG;
	cf->can_dlc = rdata[0] & 0xF;
	memcpy(cf->data, rdata + 2, cf->can_dlc);
	stats->rx_packets++;
	stats->rx_bytes += cf->can_dlc;
	spin_unlock_irqrestore(&priv->txrx_lock, flags);

	netif_receive_skb(skb_rx);

	return 0;
}

static irqreturn_t ingenic_can_irq_handler(int irq, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct ingenic_can_priv *priv = netdev_priv(dev);
	unsigned int *rxdata = (unsigned int*)priv->buffer;
	unsigned int irq_status;
	unsigned int *rdata;
	unsigned int can_data[4];

	irq_status = can_readl(priv, CAN_INTF);

	if(irq_status & CAN_INTF_DMAI) {
		unsigned int cnt, dtc;
		unsigned int i;

		dtc = readl(DMA_CHA_DTC(priv->rxchan->chan_id));
		cnt = (priv->record >= dtc) ? (priv->record - dtc) : (DMA_RX_DESC_NUM - (dtc - priv->record));
		for(i = 0; i < cnt; i++) {
			rdata = rxdata + ((priv->rece_sum % (DMA_RX_DESC_NUM * DMA_RX_DESC_DTC)) * 4);
			priv->rece_sum ++;
			ingenic_can_read_msg(dev, rdata);
		}
		priv->record = dtc;
	}

	if(irq_status & CAN_INTF_RI) {
		can_data[0] = can_readl(priv, CAN_RFIR);
		can_data[1] = can_readl(priv, CAN_RXID);
		can_data[2] = can_readl(priv, CAN_RXDATA);
		can_data[3] = can_readl(priv, CAN_RXDATA1);
		priv->rece_sum ++;
		ingenic_can_read_msg(dev, can_data);
	}

#ifndef CONFIG_INGENIC_CAN_USE_PDMA
	can_writel(priv, CAN_CMD_RRB, CAN_CMD);
#endif

	if(irq_status & CAN_INTF_WKI) {
		netdev_info(dev, "enter can sleep interrupt \n");
	}

	if(irq_status & CAN_INTF_TI) {
		priv->flag = CAN_SEND_OVER;
		priv->trans_sum ++;
		can_get_echo_skb(dev, 0);
	}

	ingenic_can_irq_error(dev, irq_status);

	return IRQ_HANDLED;
}


static int ingenic_can_open(struct net_device *dev)
{
	struct ingenic_can_priv *priv = netdev_priv(dev);
	int err;

	err = open_candev(dev);
	if(err)
		goto out;

	ingenic_can_chip_start(priv);

	return 0;
out:
	return err;
}


static const struct net_device_ops ingenic_can_netdev_ops = {
	.ndo_open	= ingenic_can_open,
	.ndo_stop       = ingenic_can_close,
	.ndo_start_xmit = ingenic_can_start_xmit,
};


static int ingenic_can_probe(struct platform_device *pdev)
{
	struct net_device *dev;
	struct ingenic_can_priv *priv = NULL;
	struct resource *res;
	void __iomem *addr;
	int err, irq;
	dma_cap_mask_t mask;
	char clkname[16];
	unsigned int can_clk_freq;
	struct clk *clk_cgu;
	struct clk *clk_gate;

	dev_info(&pdev->dev,"ingenic_can probe init\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);
	if(!res || irq <= 0) {
		err = -ENODEV;
		goto exit;
	}

	if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
		err = -EBUSY;
		goto exit;
	}

	addr = ioremap_nocache(res->start, resource_size(res));
	if (!addr) {
		err = -ENOMEM;
		goto exit_release_mem;
	}

	pdev->id = of_alias_get_id(pdev->dev.of_node, "can");

	sprintf(clkname, "div_can%d", pdev->id);
	clk_cgu = devm_clk_get(&pdev->dev, clkname);
	if (IS_ERR(clk_cgu)) {
		dev_err(&pdev->dev, "%s Cannot get clock: %s\n", __func__, clkname);
		goto exit_iounmap;
	}

	sprintf(clkname, "gate_can%d", pdev->id);
	clk_gate = devm_clk_get(&pdev->dev, clkname);
	if (IS_ERR(clk_gate)) {
		dev_err(&pdev->dev, "%s Cannot get clock: %s\n", __func__, clkname);
		goto exit_iounmap;
	}
//	clk_set_parent(clk_get(NULL, "mux_can0"), clk_get(NULL, "mpll"));
//	clk_set_parent(clk_get(NULL, "mux_can1"), clk_get(NULL, "mpll"));

	err = of_property_read_u32(pdev->dev.of_node, "ingenic,clk-freq", &can_clk_freq);
	if(err < 0) {
		can_clk_freq = 12000000;
	}

	if((err = clk_prepare_enable(clk_cgu)) < 0) {
		dev_err(&pdev->dev, "Enable can%d cgu clk failed\n", pdev->id);
		goto exit_clk_put;
	}

	if(clk_set_rate(clk_cgu, can_clk_freq)) {
		dev_err(&pdev->dev, "Set cgu_can%d clk rate faild\n", pdev->id);
		goto exit_clk_put;
	}

	if ((err = clk_prepare_enable(clk_gate)) < 0) {
		dev_err(&pdev->dev, "Enable can%d gate clk failed\n", pdev->id);
		goto exit_clk_put;
	}

	dev = alloc_candev(sizeof(struct ingenic_can_priv), TX_ECHO_SKB_MAX);
	if (!dev) {
		err = -ENOMEM;
		goto exit_clk_put;
	}

	dev->flags |= IFF_ECHO;
	dev->netdev_ops = &ingenic_can_netdev_ops;
	dev->irq = irq;

	priv = netdev_priv(dev);
	priv->can.clock.freq = can_clk_freq;
	priv->can.bittiming_const = &ingenic_can_bittiming_const;
	priv->can.do_set_mode = ingenic_can_restart_set_mode;
	priv->can.do_set_bittiming = ingenic_can_set_baudrate;
	priv->can.do_get_berr_counter = ingenic_can_get_berr_counter;
	priv->can.ctrlmode_supported = CAN_CTRLMODE_3_SAMPLES |
		CAN_CTRLMODE_LOOPBACK | CAN_CTRLMODE_LISTENONLY;
	priv->dev = dev;
	priv->reg_base = addr;
	priv->reg_base_phy = (void __iomem *)res->start;
	priv->clk_cgu = clk_cgu;
	priv->clk_gate = clk_gate;

	dev_set_drvdata(&pdev->dev, dev);
	SET_NETDEV_DEV(dev, &pdev->dev);

	err = register_candev(dev);
	if (err) {
		dev_err(&pdev->dev, "registering netdev failed\n");
		goto exit_free_dev;
	}

	err = devm_request_irq(&pdev->dev, dev->irq, ingenic_can_irq_handler, 0, dev->name, dev);
	if (err) {
		dev_err(&pdev->dev, "request irq failed\n");
		goto exit_unregister_dev;
	}

	spin_lock_init(&priv->lock);
	spin_lock_init(&priv->txrx_lock);

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	priv->txchan = dma_request_slave_channel_reason(&pdev->dev, "tx");
	if(IS_ERR(priv->txchan)) {
		if (PTR_ERR(priv->txchan) == -EPROBE_DEFER)
			err = -EPROBE_DEFER;
		dev_err(&pdev->dev,"ingenic_can request dma tx channel failed\n");
		goto exit_unregister_dev;
	}

	priv->rxchan = dma_request_slave_channel_reason(&pdev->dev, "rx");
	if(IS_ERR(priv->rxchan)) {
		if (PTR_ERR(priv->rxchan) == -EPROBE_DEFER)
			err = -EPROBE_DEFER;
		dev_err(&pdev->dev,"ingenic_can request dma rx channel failed\n");
		goto exit_free_dma_chan;
	}

	priv->buffer = dma_alloc_coherent(&pdev->dev, (DMA_RX_BUFF_SIZE * DMA_RX_DESC_NUM), &priv->buffer_dma, GFP_KERNEL);
	if (!priv->buffer) {
		dev_err(&pdev->dev,"ingenic_can request temp dma buffer failed\n");
		goto exit_free_dma_chan;
	}

	priv->txbuffer = dma_alloc_coherent(&pdev->dev, DMA_BUFF_SIZE, &priv->txbuffer_dma, GFP_KERNEL);
	if (!priv->txbuffer) {
		dev_err(&pdev->dev,"ingenic_can request temp dma txbuffer failed\n");
		goto exit_free_request_buffer;
	}
	priv->sg_tx = devm_kmalloc(&pdev->dev,sizeof(struct scatterlist), GFP_KERNEL);
	if(!priv->sg_tx) {
		dev_err(&pdev->dev,"Failed to alloc tx scatterlist\n");
	}


	return 0;

exit_free_request_buffer:
	 if(priv->buffer)
		kfree(priv->buffer);
	 if(priv->txbuffer)
		kfree(priv->txbuffer);
exit_free_dma_chan:
	if(priv->rxchan)
		dma_release_channel(priv->rxchan);
	if(priv->txchan)
		dma_release_channel(priv->txchan);
exit_unregister_dev:
	unregister_netdev(dev);
exit_free_dev:
	free_candev(dev);
exit_clk_put:
	if(priv->clk_gate)
		devm_clk_put(&pdev->dev, priv->clk_gate);
	if(priv->clk_cgu)
		devm_clk_put(&pdev->dev, priv->clk_cgu);
exit_iounmap:
	iounmap(priv->reg_base);
exit_release_mem:
	release_mem_region(res->start, resource_size(res));
exit:
	return err;
}


static int ingenic_can_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct ingenic_can_priv *priv = netdev_priv(dev);
	struct resource *res;

	unregister_netdev(dev);
	platform_set_drvdata(pdev, NULL);
	iounmap(priv->reg_base);

	clk_disable_unprepare(priv->clk_cgu);
	clk_disable_unprepare(priv->clk_gate);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, resource_size(res));

	free_candev(dev);

	return 0;
}

#ifdef CONFIG_PM
static int ingenic_can_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct ingenic_can_priv *priv = netdev_priv(dev);

	disable_irq(dev->irq);

	if (netif_running(dev)) {
		netif_device_detach(dev);
	}
	clk_disable_unprepare(priv->clk_gate);
	clk_disable_unprepare(priv->clk_cgu);

	return 0;
}

static int ingenic_can_resume(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct ingenic_can_priv *priv = netdev_priv(dev);

	netif_device_attach(dev);
	clk_prepare_enable(priv->clk_cgu);
	clk_prepare_enable(priv->clk_gate);

	enable_irq(dev->irq);

	return 0;
}
#else
#define ingenic_can_suspend NULL
#define ingenic_can_resume  NULL
#endif

static const struct of_device_id ingenic_can_dt_ids[] = {
	{ .compatible = "ingenic,x1600-can", },
	{}
};
MODULE_DEVICE_TABLE(of, ingenic_can_dt_ids);

static struct platform_driver ingenic_can_driver = {
	.probe = ingenic_can_probe,
	.remove = ingenic_can_remove,
	.suspend = ingenic_can_suspend,
	.resume = ingenic_can_resume,
	.driver = {
		.name = MODNAME,
		.of_match_table = ingenic_can_dt_ids,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(ingenic_can_driver);

MODULE_AUTHOR("xiaozhihao<zhihao.xiao@ingenic.com>");
MODULE_DESCRIPTION("ingenci can netdevice driver");
MODULE_LICENSE("GPL");
