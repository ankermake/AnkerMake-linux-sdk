/*
 * Copyright (C) 2016 Ingenic Semiconductor
 *
 * HuangLihong(Regen) <lihong.huang@ingenic.com>
 *
 */

#include <common.h>

void uart_init(void)
{
	REG32(TCSM_UART_TBUF_WP) = 0;
	REG32(TCSM_UART_TBUF_RP) = 0;

	REG32(TCSM_UART_RBUF_WP) = 0;
	REG32(TCSM_UART_RBUF_RP) = 0;
}

#define UART_REG32(addr)  REG32(0x10030000 + REG32(TCSM_UART_DEVICE_NUM) * 0x1000 + (addr))
#define UART_REG8(addr)   REG8(0x10030000 + REG32(TCSM_UART_DEVICE_NUM) * 0x1000 + (addr))
#define INTC_UART0_BIT 19
#define INTC_UART(n) (1 << (INTC_UART0_BIT - n))

static void start_tx()
{
	UART_REG8(UART_IER) |= IER_TDRIE;
}

static void stop_tx()
{
	UART_REG8(UART_IER) &= ~IER_TDRIE;
}

static int is_rx_buf_full(unsigned int waddr, unsigned int raddr)
{
	int isFull;
	if (waddr < raddr)
		isFull = (waddr == raddr - 2);
	else
		isFull = (waddr == raddr + TCSM_UART_RBUF_LEN - 2);

	return isFull;
}

static int is_buf_empty(unsigned int waddr, unsigned int raddr)
{
	return (waddr == raddr);
}

static int need_notify_cpu_to_read()
{
	unsigned int rx_buf_raddr = REG32(TCSM_UART_RBUF_RP);
	unsigned int rx_buf_waddr = REG32(TCSM_UART_RBUF_WP);

	if (!is_buf_empty(rx_buf_waddr, rx_buf_raddr))
		return 1;

	return 0;
}

static volatile struct mailbox_pend_addr_s *mailbox_pend_addr;
static void receive_chars(unsigned int lsr)
{
	volatile unsigned char *buf = (volatile unsigned char *)TCSM_UART_RBUF_ADDR;
	unsigned int waddr = REG32(TCSM_UART_RBUF_WP);
	unsigned int raddr = REG32(TCSM_UART_RBUF_RP);

	/* The delay according to the TIMEOUT value should bigger than 3.3us,
	 * which is transmission time of 10 bits (1 start bit, 8 data bits and 1 stop bit)
	 * in 3M baud rate.
	 */
#define TIMEOUT (50)
	short timeout = TIMEOUT;
	do {
		while (lsr & LSR_DRY) {
			if (is_rx_buf_full(waddr, raddr)) {
				break;
			}

			buf[waddr] = UART_REG8(UART_RBR); // data
			buf[waddr + 1] = lsr; // status
			waddr = (waddr + 2) & (TCSM_UART_RBUF_LEN - 1);

			lsr = UART_REG8(UART_LSR);
			timeout = TIMEOUT;
		}

		raddr = REG32(TCSM_UART_RBUF_RP);
		if (is_rx_buf_full(waddr, raddr)) {
			break;
		}
		lsr = UART_REG8(UART_LSR);
	} while (timeout--);

	REG32(TCSM_UART_RBUF_WP) = waddr;
}

static int need_notify_cpu_to_write()
{
	unsigned int tx_buf_waddr = REG32(TCSM_UART_TBUF_WP);
	unsigned int tx_buf_raddr = REG32(TCSM_UART_TBUF_RP);

	return is_buf_empty(tx_buf_waddr, tx_buf_raddr);
}

static void transmit_chars()
{
	volatile unsigned char *buf = (volatile unsigned char *)TCSM_UART_TBUF_ADDR;
	unsigned int raddr = REG32(TCSM_UART_TBUF_RP);
	unsigned int waddr = REG32(TCSM_UART_TBUF_WP);
	int i = 0;

	while (!is_buf_empty(waddr, raddr) && i < 32 ) {
		UART_REG8(UART_THR) = buf[raddr];
		i++;
		raddr = (raddr + 1) & (TCSM_UART_TBUF_LEN - 1);
	}

	REG32(TCSM_UART_TBUF_RP) = raddr;
}

static int check_modem_status(void)
{
	unsigned int msr = UART_REG32(UART_MSR);
	int hw_stopped;

	if (msr & MSR_CTS) {
		start_tx();
		hw_stopped = 0;
	} else {
		hw_stopped = 1;
		stop_tx();
	}

	if (msr & MSR_ANY_DELTA)
		mailbox_pend_addr->mcu_state |= msr << TCSM_UART_CTS_CHANGE_BIT;

	return hw_stopped;
}

void handle_uart_irq(void)
{
	mailbox_pend_addr = (volatile struct mailbox_pend_addr_s *)MAILBOX_PEND_ADDR;
	unsigned int dpr1 = readl(INTC_IOBASE + DPR1);

	if (dpr1 & INTC_UART(REG32(TCSM_UART_DEVICE_NUM))) {
		int hw_stopped;

		unsigned int iir = UART_REG32(UART_IIR);
		unsigned int lsr = UART_REG32(UART_LSR);

		if (iir & IIR_NO_INT)
			return;

		if (lsr & LSR_DRY)
			receive_chars(lsr);

		hw_stopped = check_modem_status();

		if ((lsr & LSR_TDRQ) && !hw_stopped)
			transmit_chars();

		if(need_notify_cpu_to_write())
			mailbox_pend_addr->mcu_state |= TCSM_UART_NEED_WRITE;

		if(need_notify_cpu_to_read())
			mailbox_pend_addr->mcu_state |= TCSM_UART_NEED_READ;
	}
}
