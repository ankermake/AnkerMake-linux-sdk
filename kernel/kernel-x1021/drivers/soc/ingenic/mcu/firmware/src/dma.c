#include <common.h>
#include <pdma.h>
#include <tcsm.h>
#include <timer.h>
#include <debug.h>

/*
 * 0xf4001b00 --- req
 * 0xf4001b10 --- channel | srcaddr | dstaddr | len
 */

static unsigned int channel;
static unsigned int srcaddr;
static unsigned int dstaddr;
static unsigned int len;

static int dma_prep_memcpy(unsigned int channel)
{
	int tmp;

    //DCSn.NDES=1
	tmp = readl(PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCCS);
	tmp &= ~(DCCS_CDOA_MASK | DCCS_TOC_MASK
			| DCCS_DES8 | DCCS_NDES);
	tmp |= DCCS_NDES;
	writel(tmp, PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCCS);

	//DCMn
	tmp = readl(PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCMD);
	tmp &= ~(DCMD_TSZ_MASK | DCMD_DWDH_MASK | DCMD_SWDH_MASK
			| DCMD_RDIL_MASK | DCMD_DAI | DCMD_SAI);
	tmp |= (DCMD_TIE | DCMD_TSZ_32BIT | DCMD_DAI | DCMD_SAI);
	writel(tmp, PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCMD);

	//DRTn.RT=auto
	tmp = DRT_RT_AUTO;
	writel(tmp, PDMA_IOBASE + channel * PDMA_CHAN_OFF + DRT);

	//DCMn.TIE=1
	tmp = readl(PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCMD);
	tmp |= DCMD_TIE;
	writel(tmp, PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCMD);

	//DMACP.DCPn=1
	tmp = readl(PDMA_IOBASE + DMACP);
	tmp |= (1 << channel);
	writel(tmp, PDMA_IOBASE + DMACP);

	//DCIRQM.CIRQMn=0
	tmp = readl(PDMA_IOBASE + DCIRQM);
	tmp &= ~(1 << channel);
	writel(tmp, PDMA_IOBASE + DCIRQM);

	return 0;
}

static int dma_start_memcpy(unsigned int channel, unsigned int srcaddr,
		unsigned int dstaddr, unsigned int len)
{
	int dataunit = 4;
	int tmp;

	//DSAn.SA=srcaddr
	writel(srcaddr,PDMA_IOBASE + channel * PDMA_CHAN_OFF + DSA);

	//DTAn.TA=dstaddr
	writel(dstaddr,PDMA_IOBASE + channel * PDMA_CHAN_OFF + DTA);

	//DTCn.TC=count
	tmp = (len + dataunit - 1) / dataunit;
	writel(tmp, PDMA_IOBASE + channel * PDMA_CHAN_OFF + DTC);

	//DCSn.CTE=1
	tmp = readl(PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCCS);
	tmp |= DCCS_CTE;
	writel(tmp, PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCCS);

	return 0;
}

static int dma_memcpy(unsigned int channel, unsigned int srcaddr,
		unsigned int dstaddr, unsigned int len)
{
	dma_prep_memcpy(channel);
	dma_start_memcpy(channel, srcaddr, dstaddr, len);
	return 0;
}

void handle_dma_soft_irq(void)
{
}

void handle_dma_channel_irq(void)
{
	int tmp;

	//DCSn.TT=0
	tmp = readl(PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCCS);
	tmp &= ~DCCS_TT;
	writel(tmp, PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCCS);

	tmp = readl(PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCCS);
	tmp &= ~DCCS_CTE;
	writel(tmp, PDMA_IOBASE + channel * PDMA_CHAN_OFF + DCCS);

	writel(INTC_PDMAM, INTC_IOBASE + ICMCR1);
	writel(readl(INTC_IOBASE + DMR1) | INTC_PDMAM, INTC_IOBASE + DMR1);
	writel(DMINT_S_IMSK, PDMA_IOBASE + DMINT);
	writel(0xFFFFFFFF, PDMA_IOBASE + DMNMB);
}

void handle_dma_irq(void)
{
	writel(0xff, PDMA_IOBASE + DCIRQM);
	channel = readl(TCSM_BASE_ADDR + ARGS + 0x10);
	srcaddr = readl(TCSM_BASE_ADDR + ARGS + 0x14);
	dstaddr = readl(TCSM_BASE_ADDR + ARGS + 0x18);
	len = readl(TCSM_BASE_ADDR + ARGS + 0x1c);
	dma_memcpy(channel, srcaddr, dstaddr, len);
}

