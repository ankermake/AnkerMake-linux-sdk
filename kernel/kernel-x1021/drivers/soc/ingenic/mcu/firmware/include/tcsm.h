#ifndef __TCSM_H__
#define __TCSM_H__

#include <irq.h>

#define PDMA_TO_CPU(addr)       (0xB3422000 + (addr & 0xFFFF))
#define CPU_TO_PDMA(addr)       (0xF4000000 + (addr & 0xFFFF) - 0x2000)

struct mailbox_pend_addr_s {
	unsigned int irq_state;
	unsigned int irq_mask;
	unsigned int cpu_state;
	unsigned int mcu_state;
};

#define TCSM_BASE_ADDR       0xF4000000

// IRQ Pending Flag Space : up to 64 irq
#define MAILBOX_PEND_ADDR       0xF4001800

// I2C Allocate Space
#define ARGS     0x1B00

// UART Allocate Space
#define TCSM_UART_TBUF_WP       (MAILBOX_PEND_ADDR + 0x10)
#define TCSM_UART_TBUF_RP       (TCSM_UART_TBUF_WP + 0x4)
#define TCSM_UART_RBUF_WP       (TCSM_UART_TBUF_RP + 0x4)
#define TCSM_UART_RBUF_RP       (TCSM_UART_RBUF_WP + 0x4)
#define TCSM_UART_DEVICE_NUM    (TCSM_UART_RBUF_RP + 0x4)
#define TCSM_UART_DEBUG         (TCSM_UART_DEVICE_NUM + 0x4)

#define TCSM_UART_TBUF_ADDR     (MAILBOX_PEND_ADDR + 0x100)
#define TCSM_UART_TBUF_LEN      0x100
#define TCSM_UART_RBUF_ADDR     (TCSM_UART_TBUF_ADDR + TCSM_UART_TBUF_LEN)
#define TCSM_UART_RBUF_LEN      0x100

#define TCSM_UART_NEED_READ     (0x1 << 0)
#define TCSM_UART_NEED_WRITE    (0x1 << 1)
#define TCSM_UART_CTS_CHANGE_BIT 8
#define TCSM_UART_CTS_CHANGE_MASK 0xff
#define TCSM_UART_CTS_CHANGE    (TCSM_UART_CTS_CHANGE_MASK << TCSM_UART_CTS_CHANGE_BIT)

// IRQ Pending
enum {
	IRQ_PEND_I2C0 = IRQ_MCU_BASE,
	IRQ_PEND_I2C1,
	IRQ_PEND_I2C2,
	IRQ_PEND_UART0,
	IRQ_PEND_UART1,
	IRQ_PEND_UART2,
	IRQ_PEND_GPIO0,
	IRQ_PEND_GPIO1,
	IRQ_PEND_GPIO2,
	IRQ_PEND_GPIO3,
	IRQ_PEND_DMA,
};

#endif
