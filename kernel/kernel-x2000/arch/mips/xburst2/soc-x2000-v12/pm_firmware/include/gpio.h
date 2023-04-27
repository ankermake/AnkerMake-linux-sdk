#ifndef GPIO_H
#define GPIO_H

#define GPIO_PA(n)      (0*32 + n)
#define GPIO_PB(n)      (1*32 + n)
#define GPIO_PC(n)      (2*32 + n)
#define GPIO_PD(n)      (3*32 + n)
#define GPIO_PE(n)      (4*32 + n)

enum gpio_function {
	GPIO_FUNC_0     = 0x00,  //0000, GPIO as function 0 / device 0
	GPIO_FUNC_1     = 0x01,  //0001, GPIO as function 1 / device 1
	GPIO_FUNC_2     = 0x02,  //0010, GPIO as function 2 / device 2
	GPIO_FUNC_3     = 0x03,  //0011, GPIO as function 3 / device 3
	GPIO_OUTPUT0    = 0x04,  //0100, GPIO output low  level
	GPIO_OUTPUT1    = 0x05,  //0101, GPIO output high level
	GPIO_INPUT      = 0x06,  //0110, GPIO as input
	GPIO_RISE_EDGE  = 0x0b,  //1011, GPIO as rise edge interrupt
	GPIO_PULL       = 0x10,  //
};

enum gpio_port {
	GPIO_PORT_A,
	GPIO_PORT_B,
	GPIO_PORT_C,
	GPIO_PORT_D,
	GPIO_PORT_E,
	GPIO_PORT_F,
	/* this must be last */
	GPIO_NR_PORTS,
};


#define GPIO_PORT_OFF	0x100
#define GPIO_SHADOW_OFF	0xF00
#define GPIO_SIZE	0x1000

#define PXPIN           0x00   /* PIN Level Register */
#define PXINT           0x10   /* Port Interrupt Register */
#define PXINTS          0x14   /* Port Interrupt Set Register */
#define PXINTC          0x18   /* Port Interrupt Clear Register */
#define PXMSK           0x20   /* Port Interrupt Mask Reg */
#define PXMSKS          0x24   /* Port Interrupt Mask Set Reg */
#define PXMSKC          0x28   /* Port Interrupt Mask Clear Reg */
#define PXPAT1          0x30   /* Port Pattern 1 Set Reg. */
#define PXPAT1S         0x34   /* Port Pattern 1 Set Reg. */
#define PXPAT1C         0x38   /* Port Pattern 1 Clear Reg. */
#define PXPAT0          0x40   /* Port Pattern 0 Register */
#define PXPAT0S         0x44   /* Port Pattern 0 Set Register */
#define PXPAT0C         0x48   /* Port Pattern 0 Clear Register */
#define PXFLG           0x50   /* Port Flag Register */
#define PXFLGC          0x58   /* Port Flag clear Register */
#define PXPE            0x70   /* Port Pull Disable Register */
#define PXPES           0x74   /* Port Pull Disable Set Register */
#define PXPEC           0x78   /* Port Pull Disable Clear Register */

#define GPIO_PXPIN(n)   (GPIO_IOBASE + (PXPIN + (n)*0x100)) /* PIN Level Register */
#define GPIO_PXINT(n)   (GPIO_IOBASE + (PXINT + (n)*0x100)) /* Port Interrupt Register */
#define GPIO_PXINTS(n)  (GPIO_IOBASE + (PXINTS + (n)*0x100)) /* Port Interrupt Set Register */
#define GPIO_PXINTC(n)  (GPIO_IOBASE + (PXINTC + (n)*0x100)) /* Port Interrupt Clear Register */
#define GPIO_PXMSK(n)   (GPIO_IOBASE + (PXMSK + (n)*0x100)) /* Port Interrupt Mask Register */
#define GPIO_PXMSKS(n)  (GPIO_IOBASE + (PXMSKS + (n)*0x100)) /* Port Interrupt Mask Set Reg */
#define GPIO_PXMSKC(n)  (GPIO_IOBASE + (PXMSKC + (n)*0x100)) /* Port Interrupt Mask Clear Reg */
#define GPIO_PXPAT1(n)  (GPIO_IOBASE + (PXPAT1 + (n)*0x100)) /* Port Pattern 1 Register */
#define GPIO_PXPAT1S(n) (GPIO_IOBASE + (PXPAT1S + (n)*0x100)) /* Port Pattern 1 Set Reg. */
#define GPIO_PXPAT1C(n) (GPIO_IOBASE + (PXPAT1C + (n)*0x100)) /* Port Pattern 1 Clear Reg. */
#define GPIO_PXPAT0(n)  (GPIO_IOBASE + (PXPAT0 + (n)*0x100)) /* Port Pattern 0 Register */
#define GPIO_PXPAT0S(n) (GPIO_IOBASE + (PXPAT0S + (n)*0x100)) /* Port Pattern 0 Set Register */
#define GPIO_PXPAT0C(n) (GPIO_IOBASE + (PXPAT0C + (n)*0x100)) /* Port Pattern 0 Clear Register */
#define GPIO_PXFLG(n)   (GPIO_IOBASE + (PXFLG + (n)*0x100)) /* Port Flag Register */
#define GPIO_PXFLGC(n)  (GPIO_IOBASE + (PXFLGC + (n)*0x100)) /* Port Flag clear Register */
#define GPIO_PXPE(n)    (GPIO_IOBASE + (PXPE + (n)*0x100)) /* Port Pull Disable Register */
#define GPIO_PXPES(n)   (GPIO_IOBASE + (PXPES + (n)*0x100)) /* Port Pull Disable Set Register */
#define GPIO_PXPEC(n)   (GPIO_IOBASE + (PXPEC + (n)*0x100)) /* Port Pull Disable Clear Register */


void gpio_set_func(enum gpio_port n, enum gpio_function func, unsigned int pins);
void gpio_port_set_value(int port, int pin, int value);
void gpio_port_direction_input(int port, int pin);
void gpio_port_direction_output(int port, int pin, int value);
void gpio_init(void);
void gpio_enable_pull(unsigned gpio);
void gpio_disable_pull(unsigned gpio);
void gpio_irq_high_level(unsigned gpio);
void gpio_irq_low_level(unsigned gpio);
void gpio_irq_rise_edge(unsigned gpio);
void gpio_irq_fall_edge(unsigned gpio);
void gpio_ack_irq(unsigned gpio);
int gpio_clear_flag(unsigned gpio);
int gpio_get_flag(unsigned int gpio);
void dump_gpio_func( unsigned int gpio);





#endif /* GPIO_H */
