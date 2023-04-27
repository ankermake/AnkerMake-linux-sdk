#ifndef __UARTC_H__
#define __UARTC_H__

#define UART_RBR             (0x00)
#define UART_THR             (0x00)
#define UART_DLLR            (0x00)
#define UART_DLHR            (0x04)
#define UART_IER             (0x04)
#define UART_IIR             (0x08)
#define UART_FCR             (0x08)
#define UART_LCR             (0x0C)
#define UART_MCR             (0x10)
#define UART_LSR             (0x14)
#define UART_MSR             (0x18)
#define UART_SPR             (0x1C)
#define UART_ISR             (0x20)
#define UART_UMR             (0x24)
#define UART_ACR             (0x28)
#define UART_RCR             (0x40)
#define UART_TCR             (0x44)

#define IIR_NO_INT            (0x1 << 0)
#define IIR_INID              (0x7 << 1)
#define IIR_RECEIVE_TIMEOUT   (0x6 << 1)

#define LSR_TDRQ              (0x1 << 5)
#define LSR_DRY               (0x1 << 0)
#define IER_TDRIE             (0x1 << 1)
#define MSR_ANY_DELTA         0x0F
#define MSR_CTS               0x10
#define MSR_DCTS              0x01

#endif
