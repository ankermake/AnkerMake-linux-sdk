
#ifndef __MACH_JZMCU_H__
#define __MACH_JZMCU_H__

/* MCU of PDMA */
#define DMCS	0x1030
#define DMNMB	0x1034
#define DMSMB	0x1038
#define DMINT	0x103C

#define DMINT_S_IP      BIT(17)
#define DMINT_N_IP      BIT(16)

#define TCSM		0x2000
#define PEND		0x3800
#define ARGS		0x3b00

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

struct jzmcu_platform_data {
	int irq_base;
	int irq_end;
};

struct jzmcu_master {
	struct device		*dev;
	void __iomem		*iomem;
	void __iomem		*iomem_intc;
	void __iomem		*iomem_gpio;
	struct clk			*clk;
	int					irq_mcu;
	int					irq_pdmam;   /* irq_pdmam for PDMAM irq */
};

struct jzmcu_gpio_chip {
	void __iomem *base;
};

struct mailbox_pend_addr_s {
	unsigned int irq_state;
	unsigned int irq_mask;
	unsigned int cpu_state;
	unsigned int mcu_state;
};

#endif
