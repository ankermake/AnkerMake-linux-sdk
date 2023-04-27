/* Ingenic gpiolib
 *
 * GPIOlib support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <linux/syscore_ops.h>
#include <linux/delay.h>
#include <irq.h>
#include <linux/seq_file.h>
#include <soc/base.h>
#include <soc/gpio.h>
#include <soc/irq.h>

#if !defined CONFIG_GPIOLIB
#error  "Need GPIOLIB !!!"
#endif

#define GPIO_PORT_OFF	0x1000
#define GPIO_SHADOW_OFF	0x7000
#define GPIO_SIZE		0x10000

#define PXPIN		0x00   /* PIN Level Register */
#define PXINT		0x10   /* Port Interrupt Register */
#define PXINTS		0x14   /* Port Interrupt Set Register */
#define PXINTC		0x18   /* Port Interrupt Clear Register */
#define PXMSK		0x20   /* Port Interrupt Mask Reg */
#define PXMSKS		0x24   /* Port Interrupt Mask Set Reg */
#define PXMSKC		0x28   /* Port Interrupt Mask Clear Reg */
#define PXPAT1		0x30   /* Port Pattern 1 Set Reg. */
#define PXPAT1S		0x34   /* Port Pattern 1 Set Reg. */
#define PXPAT1C		0x38   /* Port Pattern 1 Clear Reg. */
#define PXPAT0		0x40   /* Port Pattern 0 Register */
#define PXPAT0S		0x44   /* Port Pattern 0 Set Register */
#define PXPAT0C		0x48   /* Port Pattern 0 Clear Register */
#define PXFLG		0x50   /* Port Flag Register */
#define PXFLGC		0x58   /* Port Flag clear Register */
#define PXGFCFG0	0x70	/* port Glitch Filter Configure Register0 */
#define PXGFCFG0S	0x74	/* port Glitch Filter Configure Set Register0 */
#define PXGFCFG0C	0x78	/* port Glitch Filter Configure Clear Register0 */
#define PXGFCFG1	0x80	/* port Glitch Filter Configure Register1 */
#define PXGFCFG1S	0x84	/* port Glitch Filter Configure Set Register1 */
#define PXGFCFG1C	0x88	/* port Glitch Filter Configure Clear Register1 */
#define PXGFCFG2	0x90	/* port Glitch Filter Configure Register2 */
#define PXGFCFG2S	0x94	/* port Glitch Filter Configure Set Register2 */
#define PXGFCFG2C	0x98	/* port Glitch Filter Configure Clear Register2 */
#define PXGFCFG3	0xA0	/* port Glitch Filter Configure Register3 */
#define PXGFCFG3S	0xA4	/* port Glitch Filter Configure Set Register3 */
#define PXGFCFG3C	0xA8	/* port Glitch Filter Configure Clear Register3 */
#define PAVDDSEL	0x100	/* port VDDIO-select Register */
#define PAVDDSELS	0x104	/* port VDDIO-select Set Register */
#define PAVDDSELC	0x108	/* port VDDIO-select Clear Register */
#define PXPEL		0x110	/* port Driver disable state Register0 */
#define PXPELS		0x114	/* port Driver disable state Set Register0 */
#define PXPELC		0x118	/* port Driver disable state Clear Register0 */
#define PXPEH		0x120	/* port Driver disable state Register1 */
#define PXPEHS		0x124	/* port Driver disable state Set Register1 */
#define PXPEHC		0x128	/* port Driver disable state Clear Register1 */
#define PXDSL		0x130	/* port Drive Strength Register0 */
#define PXDSLS		0x134	/* port Drive Strength Set Register0 */
#define PXDSLC		0x138	/* port Drive Strength Clear Register0 */
#define PXDSH		0x140	/* port Drive Strength Register1 */
#define PXDSHS		0x144	/* port Drive Strength Set Register1 */
#define PXDSHC		0x148	/* port Drive Strength Clear Register1 */
#define PXSR		0x150	/* port Slew Rate Register */
#define PXSRS		0x154	/* port Slew Rate Set Register */
#define PXSRC		0x158	/* port Slew Rate Clear Register */
#define PXSMT		0x160	/* port Schmitt Trigger Register */
#define PXSMTS		0x164	/* port Schmitt Trigger Set Register */
#define PXSMTC		0x168	/* port Schmitt Trigger Clear Register */
#define PXIE		0x180	/* port Receiver Enable State Register */
#define PXIES		0x184	/* port Receiver Enable State Set Register */
#define PXIEC		0x188	/* port Receiver Enable State Clear Register */
#define PZGID2LD	0xF0   /* GPIOZ Group ID to load */

extern int gpio_ss_table[][2];
#ifdef CONFIG_RECONFIG_SLEEP_GPIO
extern int gpio_ss_table2[][2];
extern bool need_update_gpio_ss(void);
int __init gpio_ss_recheck(void);
#endif

extern void __enable_irq(struct irq_desc *desc, unsigned int irq, bool resume);

struct jzgpio_state {
	unsigned int pxint;
	unsigned int pxmsk;
	unsigned int pxpat1;
	unsigned int pxpat0;
	unsigned int pxpel;
	unsigned int pxpeh;
	unsigned int pxignore;
};

struct jzgpio_chip {
	void __iomem *reg;
	void __iomem *shadow_reg;
	int irq_base;
	spinlock_t gpio_lock;
	DECLARE_BITMAP(dev_map, 32);
	DECLARE_BITMAP(gpio_map, 32);
	DECLARE_BITMAP(irq_map, 32);
	DECLARE_BITMAP(out_map, 32);
	struct gpio_chip gpio_chip;
	struct irq_chip  irq_chip;
	unsigned int wake_map;
	struct jzgpio_state sleep_state;
	unsigned int save[6];
	unsigned int *mcu_gpio_reg;
};

static struct jzgpio_chip jz_gpio_chips[];

static inline struct jzgpio_chip *gpio2jz(struct gpio_chip *gpio)
{
	return container_of(gpio, struct jzgpio_chip, gpio_chip);
}

static inline struct jzgpio_chip *irqdata2jz(struct irq_data *data)
{
	return container_of(data->chip, struct jzgpio_chip, irq_chip);
}

static inline int gpio_pin_level(struct jzgpio_chip *jz, int pin)
{
	return !!(readl(jz->reg + PXPIN) & BIT(pin));
}

static void gpio_set_func(struct jzgpio_chip *chip,
			  enum gpio_function func, unsigned int pins)
{
	int i;
	unsigned long comp1, comp2, flags;
	unsigned long grp;

	spin_lock_irqsave(&chip->gpio_lock,flags);

	/* func option */
	if(func & 0x10) {
		grp = ((unsigned int)chip->reg - (unsigned int)jz_gpio_chips[0].reg) >> 12;
		if(func & 0x8)
			writel(pins, chip->shadow_reg + PXINTS);
		else
			writel(pins, chip->shadow_reg + PXINTC);

		if(func & 0x4)
			writel(pins, chip->shadow_reg + PXMSKS);
		else
			writel(pins, chip->shadow_reg + PXMSKC);

		if(func & 0x2)
			writel(pins, chip->shadow_reg + PXPAT1S);
		else
			writel(pins, chip->shadow_reg + PXPAT1C);

		if(func & 0x1)
			writel(pins, chip->shadow_reg + PXPAT0S);
		else
			writel(pins, chip->shadow_reg + PXPAT0C);

		/* configure PzGID2LD to specify which port group to load */
		writel(grp & 0x7, chip->shadow_reg + PZGID2LD);
	}

	/* pull option */
   	if(func & 0x80) {
		comp1 = readl(chip->reg + PXPEL);
		comp2 = readl(chip->reg + PXPEH);
		for(i = 0; i < 16; i++) {
			if((pins >> i) & 0x01) {
				comp1 &= ~(3 << (i * 2));
				comp1 |= ((func >> 5) & 0x03) << (i * 2);
			}

			if((pins >> (i + 16)) & 0x01) {
				comp2 &= ~(3 << (i * 2));
				comp2 |= ((func >> 5) & 0x03) << (i * 2);
			}
		}
		writel(comp1, chip->reg + PXPEL);
		writel(comp2, chip->reg + PXPEH);
	}

	/* drive strength option */
	if(func & 0x800) {
		comp1 = readl(chip->reg + PXDSL);
		comp2 = readl(chip->reg + PXDSH);
		for(i = 0; i < 16; i++) {
			if((pins >> i) & 0x01) {
				comp1 &= ~(3 << (i * 2));;
				comp1 |= ((func >> 8) & 0x03) << (i * 2);
			}

			if((pins >> (i + 16)) & 0x01) {
				comp2 &= ~(3 << (i * 2));
				comp2 |= ((func >> 8) & 0x03) << (i * 2);
			}
		}
		writel(comp1, chip->reg + PXDSL);
		writel(comp2, chip->reg + PXDSH);
	}

	/* slew rate option */
	if(func & 0x2000) {
		if(func & 0x1000)
			writel(pins, chip->reg + PXSRS);
		else
			writel(pins, chip->reg + PXSRC);
	}

	/* schmitt trigger option */
	if(func & 0x8000) {
		if(func & 0x4000)
			writel(pins, chip->reg + PXSMTS);
		else
			writel(pins, chip->reg + PXSMTC);
	}

	spin_unlock_irqrestore(&chip->gpio_lock,flags);
}

static void gpio_restore_func(struct jzgpio_chip *chip,
			      struct gpio_reg_func *rfunc, unsigned int pins)
{
	unsigned long flags;
	unsigned long grp;

	spin_lock_irqsave(&chip->gpio_lock,flags);

	grp = ((unsigned int)chip->reg - (unsigned int)jz_gpio_chips[0].reg) >> 12;

	writel(pins & rfunc->save[0], chip->shadow_reg + PXINTS);

	writel(pins & rfunc->save[1], chip->shadow_reg + PXMSKS);

	writel(pins & rfunc->save[2], chip->shadow_reg + PXPAT1S);

	writel(pins & rfunc->save[3], chip->shadow_reg + PXPAT0S);


	writel(pins & (~rfunc->save[0]), chip->shadow_reg + PXINTC);

	writel(pins & (~rfunc->save[1]), chip->shadow_reg + PXMSKC);

	writel(pins & (~rfunc->save[2]), chip->shadow_reg + PXPAT1C);

	writel(pins & (~rfunc->save[3]), chip->shadow_reg + PXPAT0C);

	/* configure PzGID2LD to specify which port group to load */
	writel(grp & 0x7, chip->shadow_reg + PZGID2LD);

	writel(pins & rfunc->save[4], chip->reg + PXPELS);
	writel(pins & (~rfunc->save[4]), chip->reg + PXPELC);
	writel(pins & rfunc->save[5], chip->reg + PXPEHS);
	writel(pins & (~rfunc->save[5]), chip->reg + PXPEHC);

	spin_unlock_irqrestore(&chip->gpio_lock,flags);
}
int jz_gpio_save_reset_func(enum gpio_port port, enum gpio_function dst_func,
			    unsigned long pins, struct gpio_reg_func *rfunc)
{
	struct jzgpio_chip *jz = &jz_gpio_chips[port];

	rfunc->save[0] = readl(jz->reg + PXINT);
	rfunc->save[1] = readl(jz->reg + PXMSK);
	rfunc->save[2] = readl(jz->reg + PXPAT0);
	rfunc->save[3] = readl(jz->reg + PXPAT1);
	rfunc->save[4] = readl(jz->reg + PXPEL);
	rfunc->save[5] = readl(jz->reg + PXPEH);

	gpio_set_func(jz, dst_func, pins);

	return 0;
}
EXPORT_SYMBOL(jz_gpio_save_reset_func);

int jz_gpio_restore_func(enum gpio_port port,
			 unsigned long pins, struct gpio_reg_func *rfunc)
{
	struct jzgpio_chip *jz = &jz_gpio_chips[port];

	gpio_restore_func(jz, rfunc, pins);

	return 0;
}
EXPORT_SYMBOL(jz_gpio_restore_func);

int jzgpio_set_func(enum gpio_port port,
		    enum gpio_function func,unsigned long pins)
{
	struct jzgpio_chip *jz = &jz_gpio_chips[port];

	gpio_set_func(jz,func,pins);
	return 0;
}
EXPORT_SYMBOL(jzgpio_set_func);

int jz_gpio_set_func(int gpio, enum gpio_function func)
{
	int port = gpio / 32;
	int pin = BIT(gpio & 0x1f);

	struct jzgpio_chip *jz = &jz_gpio_chips[port];

	gpio_set_func(jz, func, pin);
	return 0;
}
EXPORT_SYMBOL(jz_gpio_set_func);

int jzgpio_set_glitch_filter(enum gpio_port port,
			unsigned long pins, unsigned char period)
{

	struct jzgpio_chip *jz = &jz_gpio_chips[port];

	if(period >= 0x0f) {
		writel(pins, jz->reg + PXGFCFG0S);
		writel(pins, jz->reg + PXGFCFG1S);
		writel(pins, jz->reg + PXGFCFG2S);
		writel(pins, jz->reg + PXGFCFG3S);
	} else {
		if(period & 0x01)
			writel(pins, jz->reg + PXGFCFG0S);
		else
			writel(pins, jz->reg + PXGFCFG0C);

		if(period & 0x02)
			writel(pins, jz->reg + PXGFCFG1S);
		else
			writel(pins, jz->reg + PXGFCFG1C);

		if(period & 0x04)
			writel(pins, jz->reg + PXGFCFG2S);
		else
			writel(pins, jz->reg + PXGFCFG2C);

		if(period & 0x08)
			writel(pins, jz->reg + PXGFCFG3S);
		else
			writel(pins, jz->reg + PXGFCFG3C);
	}

	return 0;
}
EXPORT_SYMBOL(jzgpio_set_glitch_filter);

int jz_gpio_set_glitch_filter(int gpio, unsigned char period)
{
	int port = gpio / 32;
	int pin = BIT(gpio & 0x1f);

	struct jzgpio_chip *jz = &jz_gpio_chips[port];

	if(period >= 0x0f) {
		writel(pin, jz->reg + PXGFCFG0S);
		writel(pin, jz->reg + PXGFCFG1S);
		writel(pin, jz->reg + PXGFCFG2S);
		writel(pin, jz->reg + PXGFCFG3S);
	} else {
		if(period & 0x01)
			writel(pin, jz->reg + PXGFCFG0S);
		else
			writel(pin, jz->reg + PXGFCFG0C);

		if(period & 0x02)
			writel(pin, jz->reg + PXGFCFG1S);
		else
			writel(pin, jz->reg + PXGFCFG1C);

		if(period & 0x04)
			writel(pin, jz->reg + PXGFCFG2S);
		else
			writel(pin, jz->reg + PXGFCFG2C);

		if(period & 0x08)
			writel(pin, jz->reg + PXGFCFG3S);
		else
			writel(pin, jz->reg + PXGFCFG3C);
	}

	return 0;
}
EXPORT_SYMBOL(jz_gpio_set_glitch_filter);

int jz_gpio_get_glitch_filter(int gpio)
{
	int port = gpio / 32;
	int pin = BIT(gpio & 0x1f);
	unsigned int gfcfg0 = 0;
	unsigned int gfcfg1 = 0;
	unsigned int gfcfg2 = 0;
	unsigned int gfcfg3 = 0;
	unsigned int gfcfg = 0;

	struct jzgpio_chip *jz = &jz_gpio_chips[port];
	gfcfg0 = !!(readl(jz->reg + PXGFCFG0) & pin);
	gfcfg1 = !!(readl(jz->reg + PXGFCFG1) & pin);
	gfcfg2 = !!(readl(jz->reg + PXGFCFG2) & pin);
	gfcfg3 = !!(readl(jz->reg + PXGFCFG3) & pin);

	gfcfg = gfcfg3 << 3 | gfcfg2 << 2 | gfcfg1 << 1 | gfcfg0;

	return gfcfg;
}

EXPORT_SYMBOL(jz_gpio_get_glitch_filter);

int jzgpio_ctrl_pull(enum gpio_port port, int enable_pull,unsigned long pins)
{
	struct jzgpio_chip *jz = &jz_gpio_chips[port];

	if (enable_pull)
		gpio_set_func(jz, GPIO_PULL_UP, pins);
	else
		gpio_set_func(jz, GPIO_PULL_HIZ, pins);

	return 0;
}
EXPORT_SYMBOL(jzgpio_ctrl_pull);

/* Functions followed for GPIOLIB */
static int jz_gpio_set_pull(struct gpio_chip *chip,
			    unsigned offset, unsigned pull)
{
	struct jzgpio_chip *jz = gpio2jz(chip);

	if (test_bit(offset, jz->gpio_map)) {
		pr_err("BAD pull to input gpio.\n");
		return -EINVAL;
	}

	if (pull)
		gpio_set_func(jz,GPIO_PULL_UP,BIT(offset));
	else
		gpio_set_func(jz,GPIO_PULL_HIZ,BIT(offset));

	return 0;
}

int jzgpio_phy_reset(struct jz_gpio_phy_reset *gpio_phy_reset)
{
	struct jzgpio_chip *jz = &jz_gpio_chips[gpio_phy_reset->port];
	gpio_set_func(jz, gpio_phy_reset->start_func, 1 << gpio_phy_reset->pin);
	udelay(gpio_phy_reset->delaytime_usec);
	gpio_set_func(jz, gpio_phy_reset->end_func, 1 << gpio_phy_reset->pin);

	return 0;
}

/* Functions followed for GPIOLIB */

static void jz_gpio_set(struct gpio_chip *chip,
			unsigned offset, int value)
{
	struct jzgpio_chip *jz = gpio2jz(chip);

	if (!test_bit(offset, jz->out_map)) {
		pr_err("BAD set to input gpio.\n");
		return;
	}

	if (value)
		writel(BIT(offset), jz->reg + PXPAT0S);
	else
		writel(BIT(offset), jz->reg + PXPAT0C);
}

static int jz_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct jzgpio_chip *jz = gpio2jz(chip);

	return !!(readl(jz->reg + PXPIN) & BIT(offset));
}

static int jz_gpio_input(struct gpio_chip *chip, unsigned offset)
{
	struct jzgpio_chip *jz = gpio2jz(chip);

	/* Can NOT operate gpio which used as irq line */
	if (test_bit(offset, jz->irq_map))
		return -EINVAL;

	clear_bit(offset, jz->out_map);
	gpio_set_func(jz, GPIO_INPUT, BIT(offset));
	return 0;
}

static int jz_gpio_output(struct gpio_chip *chip,
			  unsigned offset, int value)
{
	struct jzgpio_chip *jz = gpio2jz(chip);

	/* Can NOT operate gpio which used as irq line */
	if (test_bit(offset, jz->irq_map))
		return -EINVAL;

	set_bit(offset, jz->out_map);
	gpio_set_func(jz, value? GPIO_OUTPUT1: GPIO_OUTPUT0
		      , BIT(offset));
	return 0;
}

static int jz_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct jzgpio_chip *jz = gpio2jz(chip);

	return jz->irq_base + offset;
}

static int jz_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	struct jzgpio_chip *jz = gpio2jz(chip);

	if(!test_bit(offset, jz->gpio_map)) {
		printk(KERN_WARNING "gpio has conflict\n");
		return -EINVAL;
	}
	if(jz->dev_map[0] & (1 << offset)) {
		printk("gpio:jz->reg = 0x%x\n", (unsigned int)jz->reg);
		printk("gpio pin: 0x%x\n", 1 << offset);
		printk("jz->dev_map[0]: 0x%x\n", (unsigned int)jz->dev_map[0]);
		dump_stack();
		printk("%s:gpio functions has redefinition", __FILE__);
	}
	jz->dev_map[0] |= 1 << offset;

	/* Disable pull up/down as default */
	gpio_set_func(jz,GPIO_PULL_HIZ,BIT(offset));

	clear_bit(offset, jz->gpio_map);
	return 0;
}

static void jz_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	struct jzgpio_chip *jz = gpio2jz(chip);

	/* Enable pull up when release */
	gpio_set_func(jz, GPIO_INPUT | GPIO_PULL_UP, BIT(offset));

	set_bit(offset, jz->gpio_map);

	jz->dev_map[0] &= ~(1 << offset);
}

/* Functions followed for GPIO IRQ */

static void gpio_unmask_irq(struct irq_data *data)
{
	struct jzgpio_chip *jz = irqdata2jz(data);
	int pin  = data->irq - jz->irq_base;

	writel(BIT(pin), jz->reg + PXFLGC);
	writel(BIT(pin), jz->reg + PXMSKC);
}

static void gpio_mask_irq(struct irq_data *data)
{
	struct jzgpio_chip *jz = irqdata2jz(data);
	int pin  = data->irq - jz->irq_base;

	writel(BIT(pin), jz->reg + PXMSKS);
}

static void gpio_mask_and_ack_irq(struct irq_data *data)
{
	struct jzgpio_chip *jz = irqdata2jz(data);
	int pin  = data->irq - jz->irq_base;

	if (irqd_get_trigger_type(data) != IRQ_TYPE_EDGE_BOTH)
		goto end;

	/* emulate double edge trigger interrupt */
	if (gpio_pin_level(jz, pin))
		writel(BIT(pin), jz->reg + PXPAT0C);
	else
		writel(BIT(pin), jz->reg + PXPAT0S);

end:
	writel(BIT(pin), jz->reg + PXMSKS);
	writel(BIT(pin), jz->reg + PXFLGC);
}

static int gpio_set_type(struct irq_data *data, unsigned int flow_type)
{
	struct jzgpio_chip *jz = irqdata2jz(data);
	int pin  = data->irq - jz->irq_base;
	int func;

	if (flow_type & IRQ_TYPE_PROBE)
		return 0;
	switch (flow_type & IRQD_TRIGGER_MASK) {
	case IRQ_TYPE_LEVEL_HIGH:	func = GPIO_INT_HI;	break;
	case IRQ_TYPE_LEVEL_LOW:	func = GPIO_INT_LO;	break;
	case IRQ_TYPE_EDGE_RISING:	func = GPIO_INT_RE;	break;
	case IRQ_TYPE_EDGE_FALLING:	func = GPIO_INT_FE;	break;
	case IRQ_TYPE_EDGE_BOTH:
		if (gpio_pin_level(jz, pin))
			func = GPIO_INT_LO;
		else
			func = GPIO_INT_HI;
		break;
	default:
		return -EINVAL;
	}

	irqd_set_trigger_type(data, flow_type);
	gpio_set_func(jz, func, BIT(pin));
	if (irqd_irq_disabled(data))
		writel(BIT(pin), jz->reg + PXMSKS);
	return 0;
}

static int gpio_set_wake(struct irq_data *data, unsigned int on)
{
	struct jzgpio_chip *jz = irqdata2jz(data);
	int pin  = data->irq - jz->irq_base;

	/* gpio must set be interrupt */
	if(!test_bit(pin, jz->irq_map))
		return -EINVAL;

	if(on)
		jz->wake_map |= 0x1 << pin;
	else
		jz->wake_map &= ~(0x1 << pin);
	return 0;
}

static unsigned int gpio_startup_irq(struct irq_data *data)
{
	struct jzgpio_chip *jz = irqdata2jz(data);
	int pin  = data->irq - jz->irq_base;

	/* gpio must be requested */
	if(test_bit(pin, jz->gpio_map))
		return -EINVAL;

	set_bit(pin, jz->irq_map);
	writel(BIT(pin), jz->reg + PXINTS);
	writel(BIT(pin), jz->reg + PXFLGC);
	writel(BIT(pin), jz->reg + PXMSKC);
	return 0;
}

static void gpio_shutdown_irq(struct irq_data *data)
{
	struct jzgpio_chip *jz = irqdata2jz(data);
	int pin  = data->irq - jz->irq_base;

	clear_bit(pin, jz->irq_map);
	writel(BIT(pin), jz->reg + PXINTC);
	writel(BIT(pin), jz->reg + PXMSKS);
}

static struct jzgpio_chip jz_gpio_chips[] = {
#define ADD_JZ_CHIP(NAME,GRP,OFFSET,NUM)				\
	[GRP] = {							\
		.irq_base = IRQ_GPIO_BASE + OFFSET,			\
		.irq_chip = {						\
			.name 	= NAME,					\
			.irq_startup 	= gpio_startup_irq,		\
			.irq_shutdown 	= gpio_shutdown_irq,		\
			.irq_enable	= gpio_unmask_irq,		\
			.irq_disable	= gpio_mask_and_ack_irq,	\
			.irq_unmask 	= gpio_unmask_irq,		\
			.irq_mask 	= gpio_mask_irq,		\
			.irq_mask_ack	= gpio_mask_and_ack_irq,	\
			.irq_set_type 	= gpio_set_type,		\
			.irq_set_wake	= gpio_set_wake,		\
		},							\
		.gpio_chip = {						\
			.base			= OFFSET,		\
			.label			= NAME,			\
			.ngpio			= NUM,			\
			.direction_input	= jz_gpio_input,	\
			.direction_output	= jz_gpio_output,	\
			.set			= jz_gpio_set,		\
			.set_pull		= jz_gpio_set_pull,	\
			.get			= jz_gpio_get,		\
			.to_irq			= jz_gpio_to_irq,	\
			.request		= jz_gpio_request,	\
			.free			= jz_gpio_free,		\
		}							\
	}
	ADD_JZ_CHIP("GPIO A",0, 0*32, 32),
	ADD_JZ_CHIP("GPIO B",1, 1*32, 32),
	ADD_JZ_CHIP("GPIO C",2, 2*32, 32),
	ADD_JZ_CHIP("GPIO D",3, 3*32, 32),
	ADD_JZ_CHIP("GPIO E",4, 4*32, 32),
	ADD_JZ_CHIP("GPIO F",5, 5*32, 32),
#undef ADD_JZ_CHIP
};

static __init int jz_gpiolib_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(jz_gpio_chips); i++)
		gpiochip_add(&jz_gpio_chips[i].gpio_chip);

	return 0;
}

static irqreturn_t gpio_handler(int irq, void *data)
{
	struct jzgpio_chip *jz = data;
	unsigned long pend,mask;

	pend = readl(jz->reg + PXFLG);
	mask = readl(jz->reg + PXMSK);

	/*
	 * PXFLG may be 0 because of GPIO's bounce in level triggered mode,
	 * so we ignore it when it occurs.
	 */
	pend = pend & ~mask;
	if (pend)
		generic_handle_irq(ffs(pend) -1 + jz->irq_base);

	return IRQ_HANDLED;
}

static irqreturn_t mcu_gpio_handler(int irq, void *data)
{
	struct jzgpio_chip *jz = data;
	unsigned long pend = *(jz->mcu_gpio_reg);
	int i;

	for(i = 0;i < 32;i++)
	{
		if((pend >> i) & 1)
			generic_handle_irq(i + jz->irq_base);
	}
	return IRQ_HANDLED;
}


int mcu_gpio_register(unsigned int reg) {
	int i;
	int ret = -1;
	for(i = 0;i < ARRAY_SIZE(jz_gpio_chips); i++) {
		jz_gpio_chips[i].mcu_gpio_reg =(unsigned int *)reg + i;

		ret = request_irq(IRQ_MCU_GPIO_PORT(i), mcu_gpio_handler, IRQF_DISABLED,"mcu gpio irq",
				  (void*)&jz_gpio_chips[i]);
		if (ret) {
			pr_err("mcu irq[%d] register error!\n",i);
			break;
		}
	}
	return ret;
}
static int __init setup_gpio_irq(void)
{
	int i, j;

	for (i = 0; i < ARRAY_SIZE(jz_gpio_chips); i++) {
		if (request_irq(IRQ_GPIO_PORT(i), gpio_handler, IRQF_DISABLED,
				jz_gpio_chips[i].irq_chip.name,
				(void*)&jz_gpio_chips[i]))
			continue;

		enable_irq_wake(IRQ_GPIO_PORT(i));
		irq_set_handler(IRQ_GPIO_PORT(i), handle_simple_irq);
		for (j = 0; j < 32; j++)
			irq_set_chip_and_handler(jz_gpio_chips[i].irq_base + j,
						 &jz_gpio_chips[i].irq_chip,
						 handle_level_irq);
	}
	return 0;
}

void gpio_suspend_set(struct jzgpio_chip *jz)
{
	unsigned int d,grp;
	unsigned int pxint,pxmsk,pxpat1,pxpat0,pxpel,pxpeh;
	unsigned long flags;
	spin_lock_irqsave(&jz->gpio_lock,flags);

	grp = ((unsigned int)jz->reg - (unsigned int)jz_gpio_chips[0].reg) >> 12;

	d = jz->wake_map | jz->sleep_state.pxignore;
	//printk("grp: %d wake_map:0x%08x pxignore:0x%08x\n",grp,jz->wake_map,jz->sleep_state.pxignore);

	jz->save[0] = readl(jz->reg + PXINT);
	jz->save[1] = readl(jz->reg + PXMSK);
	jz->save[2] = readl(jz->reg + PXPAT1);
	jz->save[3] = readl(jz->reg + PXPAT0);
	jz->save[4] = readl(jz->reg + PXPEL);
	jz->save[5] = readl(jz->reg + PXPEH);

	// gpio ignore will read func init state

	pxint = jz->save[0] & d;
	pxmsk = jz->save[1] & d;
	pxpat1 = jz->save[2] & d;
	pxpat0 = jz->save[3] & d;
	pxpel = (~jz->save[4]) & d;
	pxpeh = (~jz->save[5]) & d;

	//printk("aa grp:%d pxint:0x%08x,pxmsk:0x%08x,pxpat1:0x%08x,pxpat0:0x%08x,pxpen:0x%08x\n",
	//       grp,pxint,pxmsk,pxpat1,pxpat0,pxpen);
	/* printk("grp:%d pxint:0x%08x,pxmsk:0x%08x,pxpat1:0x%08x,pxpat0:0x%08x,pxpen:0x%08x,d:0x%08x\n", */
	/*       grp,jz->sleep_state.pxint,jz->sleep_state.pxmsk,jz->sleep_state.pxpat1,jz->sleep_state.pxpat0, */
	/*        jz->sleep_state.pxpen,d); */

	pxint |= (jz->sleep_state.pxint & (~d));
	pxmsk |= (jz->sleep_state.pxmsk & (~d));
	pxpat1 |= (jz->sleep_state.pxpat1 & (~d));
	pxpat0 |= (jz->sleep_state.pxpat0 & (~d));
	/* pxpen |= (jz->sleep_state.pxpen & (~d));*/ /*TODO: here we DO NOT change pull */

	/* printk("grp:%d pxint:0x%08x,pxmsk:0x%08x,pxpat1:0x%08x,pxpat0:0x%08x,pxpen:0x%08x\n", */
	/*       grp,pxint,pxmsk,pxpat1,pxpat0,pxpen); */
        //set set reg
	writel(pxint,jz->reg + PXINT);
	writel(pxmsk,jz->reg + PXMSK);
	writel(pxpat1,jz->reg + PXPAT1);
	writel(pxpat0,jz->reg + PXPAT0);
	/* writel(~pxpen,jz->reg + PXPEN); */ /*TODO:*/

/* #define output_str(x) \ */
/* 	printk("\t " #x " 0x%08x\n",readl(jz->reg + x)); */
/* 	output_str(PXINT); */
/* 	output_str(PXMSK); */
/* 	output_str(PXPAT1); */
/* 	output_str(PXPAT0); */
/* 	output_str(PXPEN); */
/* #undef output_str */


	spin_unlock_irqrestore(&jz->gpio_lock,flags);
}

int gpio_suspend(void)
{
	int i,j,irq = 0;
	struct irq_desc *desc;
	struct jzgpio_chip *jz;

	for(i = 0; i < GPIO_NR_PORTS; i++) {
		jz = &jz_gpio_chips[i];
		for(j=0;j<32;j++) {
			if (jz->wake_map & (0x1<<j)) {
				irq = jz->irq_base + j;
				desc = irq_to_desc(irq);
				__enable_irq(desc, irq, true);
			}
		}
		gpio_suspend_set(jz);
	}
	return 0;
}

void gpio_resume(void)
{
	int i;
	struct jzgpio_chip *jz;

	for(i = 0; i < GPIO_NR_PORTS; i++) {
		jz = &jz_gpio_chips[i];
		writel(jz->save[0], jz->reg + PXINT);
		writel(jz->save[1], jz->reg + PXMSK);
		writel(jz->save[2], jz->reg + PXPAT1);
		writel(jz->save[3], jz->reg + PXPAT0);
		writel(jz->save[4], jz->reg + PXPEL);
		writel(jz->save[5], jz->reg + PXPEH);
	}
}

struct syscore_ops gpio_pm_ops = {
	.suspend = gpio_suspend,
	.resume = gpio_resume,
};

int __init gpio_ss_check(void)
{
	unsigned int i,state,group,index;
	unsigned int panic_flags[GPIO_NR_PORTS] = {0};
	const unsigned int default_type = GPIO_INPUT | GPIO_PULL_UP;
	enum gpio_function gpio_type = default_type;

	for (i = 0; i < GPIO_NR_PORTS; i++) {
		jz_gpio_chips[i].sleep_state.pxignore = 0xffffffff;
	}

	for(i = 0; gpio_ss_table[i][1] != GSS_TABLET_END;i++) {
		group = gpio_ss_table[i][0] / 32;
		index = gpio_ss_table[i][0] % 32;
		state = gpio_ss_table[i][1];
		if(group >= GPIO_NR_PORTS)
		{
			printk("ERROR: gpio_ss_table[%d] is invalid!\n",i);
			continue;
		}
		if(panic_flags[group] & (1 << index)) {
			printk("\nwarning : (%d line) same gpio already set before this line!\n",i);
			printk("\nwarning : (%d line) same gpio already set before this line!\n",i);
			printk("\nwarning : (%d line) same gpio already set before this line!\n",i);
			printk("\nwarning : (%d line) same gpio already set before this line!\n",i);
			printk("\nwarning : (%d line) same gpio already set before this line!\n",i);
			printk("\nwarning : (%d line) same gpio already set before this line!\n",i);
			printk("\nwarning : (%d line) same gpio already set before this line!\n",i);
			panic("gpio_ss_table has iterant gpio set , system halt\n");
			while(1);
		} else {
			panic_flags[group] |= 1 << index;
		}
		switch(state) {
		case GSS_OUTPUT_HIGH:
			gpio_type = GPIO_OUTPUT1;
			break;
		case GSS_OUTPUT_LOW:
			gpio_type = GPIO_OUTPUT0;
			break;
		case GSS_INPUT_PULL:
			gpio_type = GPIO_INPUT | GPIO_PULL_UP;
			break;
		case GSS_INPUT_NOPULL:
			gpio_type = GPIO_INPUT | GPIO_PULL_HIZ;
			break;
		case GSS_IGNORE:
			gpio_type = -1;
			break;
		}

		if(gpio_type != -1) {
			jz_gpio_chips[group].sleep_state.pxignore &= ~(1 << index);

#define SLEEP_SET_STATE(st,t)	do{					\
				if((gpio_type) & (t))			\
					jz_gpio_chips[group].sleep_state.px##st |= 1 << (index); \
				else					\
					jz_gpio_chips[group].sleep_state.px##st &= ~(1 << (index)); \
			}while(0)

			/*SLEEP_SET_STATE(pen,0x10);*/ /*TODO:here we DO NOT change sleep state*/
			SLEEP_SET_STATE(int,0x8);
			SLEEP_SET_STATE(msk,0x4);
			SLEEP_SET_STATE(pat1,0x2);
			SLEEP_SET_STATE(pat0,0x1);
#undef SLEEP_SET_STATE

		}
	}
	return 0;
}

int __init setup_gpio_pins(void)
{
	int i;
	void __iomem *base;

	base = ioremap(GPIO_IOBASE, GPIO_SIZE);
	for (i = 0; i < GPIO_NR_PORTS; i++) {
		jz_gpio_chips[i].reg = base + i * GPIO_PORT_OFF;
		jz_gpio_chips[i].shadow_reg = base + GPIO_SHADOW_OFF;
		jz_gpio_chips[i].gpio_map[0] = 0xffffffff;
		spin_lock_init(&jz_gpio_chips[i].gpio_lock);
	}

	/* vddio select */
	writel(0x00, base + PAVDDSEL);
#ifdef CONFIG_VDDIO0_V25
	writel(1 << 3, base + PAVDDSELS);
#endif
#ifdef CONFIG_VDDIO0_V18
	writel(1 << 0, base + PAVDDSELS);
#endif

#ifdef CONFIG_VDDIO1_V25
	writel(1 << 4, base + PAVDDSELS);
#endif
#ifdef CONFIG_VDDIO1_V18
	writel(1 << 1, base + PAVDDSELS);
#endif

#ifdef CONFIG_VDDIO2_V25
	writel((1 << 5) || (1 << 7), base + PAVDDSELS);
#endif
#ifdef CONFIG_VDDIO2_V18
	writel((1 << 2) || (1 << 6), base + PAVDDSELS);
#endif

	for (i = 0; i < platform_devio_array_size ; i++) {
		struct jz_gpio_func_def *g = &platform_devio_array[i];
		struct jzgpio_chip *jz;

		if (g->port >= GPIO_NR_PORTS) {
			pr_info("Bad gpio define! i:%d\n",i);
			continue;
		} else if (g->port < 0) {
			/* Wrong defines end */
			break;
		}

		jz = &jz_gpio_chips[g->port];
		if (GPIO_AS_FUNC(g->func)) {
			if(jz->dev_map[0] & g->pins) {
				panic("%s:gpio functions has redefinition of '%s'", __FILE__, g->name);
				while(1);
			}
			jz->dev_map[0] |= g->pins;
		}

		gpio_set_func(jz, g->func | GPIO_PULL_HIZ, g->pins);
	}

	setup_gpio_irq();
	jz_gpiolib_init();
	register_syscore_ops(&gpio_pm_ops);

	gpio_ss_check();
#ifdef CONFIG_RECONFIG_SLEEP_GPIO
	gpio_ss_recheck();
#endif

	return 0;
}

arch_initcall(setup_gpio_pins);
/* -------------------------gpio proc----------------------- */
#include <jz_proc.h>

static int gpio_proc_show(struct seq_file *m, void *v)
{
	int i=0;
	seq_printf(m,"INT\t\tMASK\t\tPAT1\t\tPAT0\n");
	for(i = 0; i < GPIO_NR_PORTS; i++) {
		seq_printf(m,"0x%08x\t0x%08x\t0x%08x\t0x%08x\n",
			   readl(jz_gpio_chips[i].reg + PXINT),
			   readl(jz_gpio_chips[i].reg + PXMSK),
			   readl(jz_gpio_chips[i].reg + PXPAT1),
			   readl(jz_gpio_chips[i].reg + PXPAT0));
	}
	return 0;
}

static int gpio_open(struct inode *inode, struct file *file)
{
	return single_open(file, gpio_proc_show, PDE_DATA(inode));
}
static const struct file_operations gpios_proc_fops ={
	.read = seq_read,
	.open = gpio_open,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init init_gpio_proc(void)
{
	struct proc_dir_entry *p;
	p = jz_proc_mkdir("gpio");
	if (!p) {
		pr_warning("create_proc_entry for common gpio failed.\n");
		return -ENODEV;
	}
	proc_create("gpios", 0600,p,&gpios_proc_fops);

	return 0;
}

module_init(init_gpio_proc);