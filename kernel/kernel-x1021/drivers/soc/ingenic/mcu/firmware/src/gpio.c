#include <common.h>
#include <tcsm.h>
#include <timer.h>
#include <debug.h>
#include <soc/irq.h>

void handle_gpio_irq(int irq)
{
	volatile struct mailbox_pend_addr_s *mailbox_pend_addr =
			(volatile struct mailbox_pend_addr_s *)MAILBOX_PEND_ADDR;
	unsigned int pend;
	int port = IRQ_GPIO0 - irq;

	pend = readl(GPIO_IOBASE + port * GPIO_PORT_OFF + PXFLG);

	// TODO

	if(pend) {
		mailbox_pend_addr->irq_mask &=
			~(1 << (port + IRQ_PEND_GPIO0 - IRQ_MCU_BASE));
		mailbox_pend_addr->irq_state |=
			(1 << (port + IRQ_PEND_GPIO0 - IRQ_MCU_BASE));
		writel(INTC_PDMAM, INTC_IOBASE + ICMCR1);
		writel(readl(INTC_IOBASE + DMR1) | INTC_PDMAM, INTC_IOBASE + DMR1);
		writel(DMINT_S_IMSK, PDMA_IOBASE + DMINT);
		writel(0xFFFFFFFF, PDMA_IOBASE + DMNMB);
	}
}
