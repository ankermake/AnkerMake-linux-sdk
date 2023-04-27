#include <uart.h>
#include <common.h>

unsigned int U_IOBASE = 0;


void serial_puts (const char *s)
{
	while (*s){
		if(*s == '\n') {
			TCSM_PCHAR('\r');
		}
		TCSM_PCHAR(*s++);
	}

}





/**********************************************************************************************/

static struct baudtoregs_t
{
	unsigned int baud;
	unsigned short div;
	unsigned int umr:5;
	unsigned int uacr:12;
} baudtoregs[] = {
	{50,0x7530,0x10,0x0},
	{75,0x4e20,0x10,0x0},
	{110,0x3521,0x10,0x0},
	{134,0x2b9d,0x10,0x0},
	{150,0x2710,0x10,0x0},
	{200,0x1d4c,0x10,0x0},
	{300,0x1388,0x10,0x0},
	{600,0x9c4,0x10,0x0},
	{1200,0x4e2,0x10,0x0},
	{1800,0x340,0x10,0x0},
	{2400,0x271,0x10,0x0},
	{4800,0x138,0x10,0x0},
	{9600,0x9c,0x10,0x0},
	{19200,0x4e,0x10,0x0},
	{38400,0x27,0x10,0x0},
	{57600,0x1a,0x10,0x0},
	{115200,0xd,0x10,0x0},
	{230400,0x6,0x11,0x252},
	{460800,0x3,0x11,0x252},
	{500000,0x3,0x10,0x0},
	{576000,0x3,0xd,0xfef},
	{921600,0x2,0xd,0x0},
	{1000000,0x2,0xc,0x0},
	{1152000,0x1,0x14,0xefb},
	{1500000,0x1,0x10,0x0},
	{2000000,0x1,0xc,0x0},
	{2500000,0x1,0x9,0x6b5},
	{3000000,0x1,0x8,0x0},
	{3500000,0x1,0x6,0xbf7},
	{4000000,0x1,0x6,0x0},
};


void firmware_uart_set_baud(unsigned int uart_idx, unsigned int baud)
{
	unsigned int i;
	unsigned int val;
	unsigned int umr;
	unsigned int uacr;
	unsigned int baud_div;

	unsigned int ubase = UART0_IOBASE + uart_idx * UART_OFF;

	for (i = 0; i < ARRAY_SIZE(baudtoregs); i++) {
		if (baud == baudtoregs[i].baud) {
			umr = baudtoregs[i].umr;
			uacr = baudtoregs[i].uacr;
			baud_div = baudtoregs[i].div;
		}
	}

	val = readb(ubase + ULCR);
	val |= UART_LCR_DLAB;
	writeb(val, ubase + ULCR);

	writeb((baud_div >> 8) & 0xff, ubase + UDLHR);
	writeb(baud_div & 0xff, ubase + UDLLR);

	val &= ~UART_LCR_DLAB;
	writeb(val, ubase + ULCR);

	writeb(umr, ubase + UMR);
	writeb(uacr, ubase + UACR);

}

void firmware_uart_init(unsigned int uart_idx)
{
	unsigned int ubase = UART0_IOBASE + uart_idx * UART_OFF;

	writeb(0, ubase + UIER);

	writeb(~UART_FCR_UUE, ubase + UFCR);

	writeb(~(SIRCR_RSIRE | SIRCR_TSIRE), ubase + ISR);

	writeb(UART_LCR_WLEN_8 | UART_LCR_STOP_1, ubase + ULCR);

//	firmware_uart_set_baud(uart_idx, baud);

	writeb(UART_FCR_UUE | UART_FCR_FE | UART_FCR_TFLS | UART_FCR_RFLS, ubase + UFCR);

}


void firmware_uart_putc(unsigned int uart_idx, char c)
{
	unsigned int ubase = UART0_IOBASE + uart_idx * UART_OFF;

	writeb(c, ubase + URBR);

	while(!((readb(ubase + ULSR) &(UART_LSR_TDRQ | UART_LSR_TEMT)) == (UART_LSR_TDRQ | UART_LSR_TEMT)));
}

void firmware_uart_send(unsigned int uart_idx, char *s)
{
	while (*s) {
		if (*s == '\n') {
			firmware_uart_putc(uart_idx, '\r');
		}
		firmware_uart_putc(uart_idx, *s++);
	}
}



#if 0
void firmware_uart_test(unsigned int uart_idx, unsigned int baud, char *s)
{
	firmware_uart_init(uart_idx);

	firmware_uart_set_baud(uart_idx, baud);

	firmware_uart_send(uart_idx, s);

}
#endif




























