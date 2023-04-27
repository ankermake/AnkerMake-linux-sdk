/*
 * traps.c  :  IRQ handler
 */

#include <common.h>
#include <tcsm.h>
#include <pdma.h>
#include <bitops.h>
#include <i2c.h>
#include <dma.h>
#include <uart.h>
#include <gpio.h>
#include <debug.h>
#include <soc/irq.h>

static int handle_mcu_intc_irq(int irq)
{
	volatile struct mailbox_pend_addr_s *mailbox_pend_addr =
			(volatile struct mailbox_pend_addr_s *)MAILBOX_PEND_ADDR;
	unsigned int dsr2, dmr2;
	int irq_mcu;

	switch(irq) {
	case IRQ_PDMAM:
		dsr2 = mailbox_pend_addr->irq_state;
		dmr2 = mailbox_pend_addr->irq_mask;
		irq_mcu = ffs(dsr2 & ~dmr2) + IRQ_MCU_BASE - 1;
		switch(irq_mcu) {
		case IRQ_PEND_I2C0:
		case IRQ_PEND_I2C1:
		case IRQ_PEND_I2C2:
#if defined(HANDLE_I2C)
			handle_i2c_irq();
#endif
			break;
		case IRQ_PEND_UART0:
		case IRQ_PEND_UART1:
		case IRQ_PEND_UART2:
#if defined(HANDLE_UART)
			handle_uart_irq();
#endif
			break;
		case IRQ_PEND_DMA:
#if defined(HANDLE_DMA)
			handle_dma_irq();
#endif
			break;
		default:
			break;

		}
		break;
	case IRQ_GPIO0:
	case IRQ_GPIO1:
	case IRQ_GPIO2:
	case IRQ_GPIO3:
#if defined(HANDLE_GPIO)
		handle_gpio_irq(irq);
#endif
		break;
	default:
		break;
	}
	return 0;
}

void trap_entry(void)
{
	unsigned int intctl;
	unsigned int dpr0, dpr1;

	__pdma_irq_disable();
	intctl = __pdma_read_cp0(12, 1);
	// Handler irq
	if (intctl & MCU_INTC_IRQ) {
		dpr0 = readl(INTC_IOBASE + DPR0);
		dpr1 = readl(INTC_IOBASE + DPR1);
		writel(DMINT_S_IMSK, PDMA_IOBASE + DMINT);
		if(dpr0) {
			handle_mcu_intc_irq(ffs(dpr0) + IRQ_INTC_BASE - 1);
		}
		if(dpr1) {
			handle_mcu_intc_irq(ffs(dpr1) + IRQ_INTC_BASE + 31);
		}
	}
	else if (intctl & MCU_CHANNEL_IRQ) {
#if defined(HANDLE_DMA)
		handle_dma_channel_irq();
#endif
	}
	else if (intctl & MCU_SOFT_IRQ) {
#if defined(HANDLE_DMA)
		handle_dma_soft_irq();
#endif
	}
	__pdma_irq_enable();
}
