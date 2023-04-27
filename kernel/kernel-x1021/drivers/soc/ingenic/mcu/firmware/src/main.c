/*
 * main.c
 */
#include <common.h>
#include <pdma.h>
#include <tcsm.h>
#include <timer.h>
#include <uart.h>

static void handle_init(void)
{
	ost_timer();
#if defined(HANDLE_UART)
	uart_init();
#endif
}

static noinline void mcu_init(void)
{
	/* clear mailbox irq pending */
	writel(0, PDMA_IOBASE + DMNMB);
	writel(0, PDMA_IOBASE + DMSMB);
	writel(DMINT_S_IMSK, PDMA_IOBASE + DMINT);
}

static void mcu_sleep()
{
	volatile struct mailbox_pend_addr_s *mailbox_pend_addr =
			(volatile struct mailbox_pend_addr_s *)MAILBOX_PEND_ADDR;

	__pdma_irq_disable();
	if(!(readl(PDMA_IOBASE + DMINT) & DMINT_N_IP))
	{
		if(mailbox_pend_addr->mcu_state)
		{
			mailbox_pend_addr->cpu_state = mailbox_pend_addr->mcu_state;
			mailbox_pend_addr->mcu_state = 0;
			writel(0xFFFFFFFF, PDMA_IOBASE + DMNMB); // Create an normal mailbox irq to CPU
		} else {
			__pdma_mwait();
		}
	}
	__pdma_irq_enable();
}

int main(void)
{
	mcu_init();
	handle_init();
	while(1) {
		mcu_sleep();
	}
	return 0;
}

