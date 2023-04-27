
#define INTC_ICSR0	(0x00)
#define INTC_ICSR1	(0x20)
#define INTC_ICMR0	(0x04)
#define INTC_ICMR1	(0x24)
#define INTC_ICMSR0	(0x08)
#define INTC_ICMSR1	(0x28)
#define INTC_ICMCR0	(0x0c)
#define INTC_ICMCR1	(0x28)
#define INTC_ICPR0	(0x10)
#define INTC_ICPR1	(0x30)



enum {
// interrupt controller interrupts

/* int0 */
	IRQ_AUDIO = 0,				/* 0 */
	IRQ_OTG,				/* 1 */
	IRQ_RESERVED0_2,			/* 2 */
	IRQ_PDMA,				/* 3 */
	IRQ_PDMAD,				/* 4 */
	IRQ_PDMAM,				/* 5 */
	IRQ_PWM,				/* 6 */
	IRQ_SFC,				/* 7 */
	IRQ_SSI1,				/* 8 */
	IRQ_SSI0,				/* 9 */
	IRQ_MIPI_DSI,				/* 10 */
	IRQ_SADC,				/* 11 */
	IRQ_MIPI_CSI_4,				/* 12 */
	IRQ_GPIO4,				/* 13 */
	IRQ_GPIO3,				/* 14 */
	IRQ_GPIO2,				/* 15 */
	IRQ_GPIO1,				/* 16 */
	IRQ_GPIO0,				/* 17 */
#define IRQ_GPIO_PORT(N) (IRQ_GPIO0 - (N))
	IRQ_VIC1,				/* 18 */
	IRQ_VIC0,				/* 19 */
	IRQ_ISP1,				/* 20 */
	IRQ_ISP0,				/* 21 */
	IRQ_HASH,				/* 22 */
	IRQ_AES,				/* 23 */
	IRQ_RSA,				/* 24 */
	IRQ_TCU2,				/* 25 */
	IRQ_TCU1,				/* 26 */
	IRQ_TCU0,				/* 27 */
	IRQ_MIPI_CSI_2,				/* 28 */
	IRQ_ROTATE,				/* 29 */
	IRQ_CIM,				/* 30 */
	IRQ_LCD,				/* 31 */

/* int1 */
	IRQ_RTC,				/* 32 */
	IRQ_SOFT,			/* 33 */
	IRQ_DTRNG,				/* 34 */
	IRQ_SCC,				/* 35 */
	IRQ_MSC1,				/* 36 */
	IRQ_MSC0,				/* 37 */
	IRQ_UART9,				/* 38 */
	IRQ_UART8,				/* 39 */
	IRQ_UART7,				/* 40 */
	IRQ_UART6,				/* 41 */
	IRQ_UART5,				/* 42 */
	IRQ_UART4,				/* 43 */
	IRQ_UART3,				/* 44 */
	IRQ_UART2,				/* 45 */
	IRQ_UART1,				/* 46 */
	IRQ_UART0,				/* 47 */
	IRQ_MSC2,				/* 48 */
	IRQ_HARB2,				/* 49 */
	IRQ_HARB0,				/* 50 */
	IRQ_CPM,				/* 51 */
	IRQ_DDR,				/* 52 */
	IRQ_GMAC1,				/* 53 */
	IRQ_EFUSE,				/* 54 */
	IRQ_GMAC0,				/* 55 */
	IRQ_I2C5,				/* 56 */
	IRQ_I2C4,				/* 57 */
	IRQ_I2C3,				/* 58 */
	IRQ_I2C2,				/* 59 */
	IRQ_I2C1,				/* 60 */
	IRQ_I2C0,				/* 61 */
	IRQ_HELIX,				/* 62 */
	IRQ_FELIX,				/* 63 */

#define IRQ_MCU_GPIO_PORT(N) (IRQ_MCU_GPIO0 + (N))
	IRQ_MCU_GPIO0,
	IRQ_MCU_GPIO1,
	IRQ_MCU_GPIO2,
	IRQ_MCU_GPIO3,
	IRQ_MCU_GPIO4,
	IRQ_MCU_GPIO5,


};






#define intc_readl(off)		readl(INTC_IOBASE + off)
#define intc_writel(val, off)	writel(val, INTC_IOBASE + off)


