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


#include <linux/init.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>

#include <asm/irq_cpu.h>
#include <soc/base.h>
#include <soc/irq.h>
#ifdef CONFIG_PCI
#include <soc/pci.h>
#endif


/* arch/mips/xburst/soc-x1000/common/pm_p0.c
 static int soc_pm_enter(suspend_state_t state)
 {
	......
+	jz_pm_update_wakeup_source(0);
 	return 0;
 }
 */



static int wakeup_source_irq;
static int wakeup_source_gpio_level; /* 0: gpio pin low, 1: gpio pin high, -1: not gpio level */


static int get_irq_num(int ipr)
{
	int iii;

	for (iii=0; iii<32; iii++) {
		if (ipr&(1<<iii)) {
			return iii;
		}
	}

	return 0;
}


/* ------------------------- GPIO irq ----------------------- */

#define GPIO_GROUP_MAX 4
#define GPIO_PORT_OFF	0x100

#define PXPIN		0x00   /* PIN Level Register */
#define PXFLG		0x50   /* Port Flag Register */
#define PXMSK		0x20   /* Port Interrupt Mask Reg */

static void __iomem *gpio_base = (void __iomem *)0xB0010000;

static int get_gpio_irq(int group)
{
	int irq;
	unsigned long pend,mask, pin_level;
	void __iomem *group_addr;

	group_addr = gpio_base + GPIO_PORT_OFF*group;
	pin_level = readl(group_addr + PXPIN);
	pend = readl(group_addr + PXFLG);
	mask = readl(group_addr + PXMSK);
	printk("GPIO group=%x, pend=%08lx, mask=%08lx, level=%08lx\n", group, pend, mask, pin_level);

	/*
	 * PXFLG may be 0 because of GPIO's bounce in level triggered mode,
	 * so we ignore it when it occurs.
	 */
	pend = pend & ~mask;

	irq = get_irq_num(pend);

	wakeup_source_gpio_level = (pin_level >> irq ) & 1; /* 0: gpio pin low, 1: gpio pin high, -1: not gpio level */

	return irq;
}

/* ------------------------- INTC ----------------------- */

#define TRACE_IRQ        1
#define PART_OFF	0x20

#define ISR_OFF		(0x00)
#define IMR_OFF		(0x04)
#define IMSR_OFF	(0x08)
#define IMCR_OFF	(0x0c)
#define IPR_OFF		(0x10)

#define GPIO_IRQ_GROUPS  (0xf<<14)
#define INTC_IRQ_MAX     (64)

static void __iomem *intc_base = (void __iomem *)0xB0001000;

extern void __enable_irq(struct irq_desc *desc, unsigned int irq, bool resume);

static int get_intc_irq_num(void)
{
	unsigned long ipr0,ipr1, isr0, isr1;
	int irq_num;
	ipr0 = readl(intc_base + IPR_OFF);
	ipr1 = readl(intc_base + PART_OFF + IPR_OFF);
	isr0 = readl(intc_base + ISR_OFF);
	isr1 = readl(intc_base + PART_OFF + ISR_OFF);
	printk("ipr0, ipr1(%08lx, %08lx)\n", ipr0, ipr1);
	printk("isr0, isr1(%08lx, %08lx)\n", isr0, isr1);

	if (ipr0 & GPIO_IRQ_GROUPS) {
		unsigned long gpio_irqs;
		unsigned long gpio_group;
		int iii;
		gpio_irqs = (ipr0 & GPIO_IRQ_GROUPS) >> 14;
		for (iii=0; iii<GPIO_GROUP_MAX; iii++) {
			if (gpio_irqs&(1<<iii)) {
				gpio_group = GPIO_GROUP_MAX-iii - 1;
				irq_num = get_gpio_irq(gpio_group);
				irq_num += INTC_IRQ_MAX + gpio_group*32;
				break;
			}
		}
	}
	else if (ipr0) {
		irq_num = get_irq_num(ipr0);
	}
	else if (ipr1) {
		irq_num = get_irq_num(ipr1);
		irq_num += 32;
	}

	return irq_num;
}


int jz_pm_update_wakeup_source(int args)
{
	wakeup_source_gpio_level = -1;
	wakeup_source_irq = get_intc_irq_num();


	printk("%s(), wakeup_source_irq=%d\n", __FUNCTION__, wakeup_source_irq);
	return wakeup_source_irq;
}


/* ------------------------- proc interface ----------------------- */
#include <jz_proc.h>
/*
 * # cat /proc/jz/suspend/wakeup_source
 * 127:0
 * #
 * output means: irq:gpio_level
 *
 * irq num:
 * 	0~31: INTC0
 * 	31~63: INTC1
 * 	64+32*0~ 64+32*1: GPIO group 0 (A)
 * 	64+32*1~ 64+32*2: GPIO group 1 (B)
 * 	64+32*2~ 64+32*3: GPIO group 2 (C)
 * 	64+32*3~ 64+32*4: GPIO group 3 (D)
 * gpio_level:
 *      0: gpio pin low, 1: gpio pin high, -1: not gpio level
 */
static int proc_show(struct seq_file *m, void *v)
{
	seq_printf(m,"%d:%d\n", wakeup_source_irq, wakeup_source_gpio_level);
	return 0;
}

static int proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_show, PDE_DATA(inode));
}
static const struct file_operations proc_fops ={
	.read = seq_read,
	.open = proc_open,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init init_suspend_proc(void)
{
	struct proc_dir_entry *p;
	p = jz_proc_mkdir("suspend");
	if (!p) {
		pr_warning("create_proc_entry for common suspend wakeup_source failed.\n");
		return -ENODEV;
	}
	proc_create("wakeup_source", 0600,p,&proc_fops);

	return 0;
}

module_init(init_suspend_proc);
