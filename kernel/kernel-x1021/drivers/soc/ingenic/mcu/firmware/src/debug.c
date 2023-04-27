#include <common.h>
#include <pdma.h>

#ifdef DEBUG

#define LSR_TEMT (1 << 6)
#define DEBUG_UART_REG8(addr)   REG8(0x10030000 + 2 * 0x1000 + (addr))
static void putc(const char c)
{
    if (c == '\n')
        putc('\r');

    DEBUG_UART_REG8(UART_THR) = (u8)c;

    /* Wait for fifo to shift out some bytes */
    while (!((DEBUG_UART_REG8(UART_LSR) &
            (LSR_TDRQ | LSR_TEMT)) == 0x60))
        continue;
}

void puts(const char *s)
{
    while (*s)
        putc(*s++);
}

void put_hex(const unsigned int d)
{
        char c[9];
        int i;
        for (i = 0; i < 8; i++) {
                c[i] = (d >> ((7 - i) * 4)) & 0xf;
                if (c[i] < 10)
                        c[i] += 0x30;
                else
                        c[i] += (0x41 - 10);
        }
        c[8] = 0;
        puts(c);
        puts("\n");
}

#endif
