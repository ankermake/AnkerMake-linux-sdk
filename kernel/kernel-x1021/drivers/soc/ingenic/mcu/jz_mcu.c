/*
 * JZSOC MCU controller
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/stat.h>
#include <linux/firmware.h>

#include <soc/irq.h>
#include <soc/base.h>
#include <soc/gpio.h>
#include <mach/jzmcu.h>

#define FIRMWARE_NAME "firmware_mcu.hex"

int firmware_loaded = 0;
EXPORT_SYMBOL(firmware_loaded);

#define ICMSR0 (0x08)
#define ICMSR1 (0x28)
#define DMR0   (0x38)
#define DMR1   (0x44)
#define PDMAM  (0x1 << 29)

#define GPIO_PORT_OFF	0x100
#define PXFLG	(0x50)

static void jzmcu_mcu_reset(struct jzmcu_master *mcu)
{
	unsigned long dmcs;
	dmcs = readl(mcu->iomem + DMCS);
	dmcs |= 0x1;
	writel(dmcs, mcu->iomem + DMCS);
}

static int jzmcu_load_firmware(struct jzmcu_master *mcu,
		const u8 *addr, size_t size)
{
	memcpy(mcu->iomem + TCSM, addr, size);
	return 0;
}

static void jzmcu_mcu_init(struct jzmcu_master *mcu)
{
	unsigned long dmcs;
	dmcs = readl(mcu->iomem + DMCS);
	dmcs &= ~0x1;
	writel(dmcs, mcu->iomem + DMCS);
}

static void jzmcu_mcu_mailbox(struct jzmcu_master *mcu)
{
	volatile struct mailbox_pend_addr_s *mailbox_pend_addr =
		(volatile struct mailbox_pend_addr_s *)(mcu->iomem + PEND);
	unsigned int tmp;

	tmp = readl(mcu->iomem + DMINT);
	tmp &= ~1;
	writel(tmp, mcu->iomem + DMINT);

	mailbox_pend_addr->irq_state = 0;
	mailbox_pend_addr->irq_mask = 0xffffffff;
	mailbox_pend_addr->cpu_state = 0;
	mailbox_pend_addr->mcu_state = 0;

	writel(0xf << 14, mcu->iomem_intc + ICMSR0);
	tmp = readl(mcu->iomem_intc + DMR0);
	tmp &= ~(0xf << 14);
	writel(tmp, mcu->iomem_intc + DMR0);

	writel(PDMAM, mcu->iomem_intc + ICMSR1);
	tmp = readl(mcu->iomem_intc + DMR1);
	tmp &= ~PDMAM;
	writel(tmp, mcu->iomem_intc + DMR1);
}

static char *gpio_name[] = {"gpio0","gpio1","gpio2","gpio3"};

static struct jzmcu_gpio_chip jzmcu_gpio_chips[GPIO_NR_PORTS];

irqreturn_t jzmcu_gpio_int_handler(int irq_mcu, void *dev)
{
	unsigned long pend;
	int port;

	port = irq_mcu - IRQ_PEND_GPIO0;
	pend = readl(jzmcu_gpio_chips[port].base + PXFLG);

	if (pend)
		generic_handle_irq(ffs(pend) - 1 + IRQ_GPIO_BASE + port * 32);

	return IRQ_HANDLED;
}

static int jzmcu_gpio_irq_setup(struct jzmcu_master *mcu)
{
	int i, ret;

	for (i = 0; i < GPIO_NR_PORTS; i++) {
		jzmcu_gpio_chips[i].base = mcu->iomem_gpio + i * GPIO_PORT_OFF;
		ret = request_irq(IRQ_PEND_GPIO0 + i, jzmcu_gpio_int_handler,
					IRQF_DISABLED, gpio_name[i], mcu);
		if(ret)
			goto err;
	}

err:
	return 0;
}

irqreturn_t jzmcu_int_handler(int irq_pdmam, void *dev)
{
	unsigned long pending;
	unsigned long pend, mask;
	struct jzmcu_master *master = (struct jzmcu_master *)dev;
	volatile struct mailbox_pend_addr_s *mailbox_pend_addr =
		(volatile struct mailbox_pend_addr_s *)(master->iomem + PEND);
	pending = readl(master->iomem + DMINT);

	if(pending & DMINT_N_IP){
		pending &= ~DMINT_N_IP;
		writel(pending, master->iomem + DMINT);
	}

	pend = mailbox_pend_addr->irq_state;
	mask = mailbox_pend_addr->irq_mask;
	pend = pend & ~mask;
	if(pend)
		generic_handle_irq(ffs(pend) + master->irq_mcu - 1);

	return IRQ_HANDLED;
}

static void jzmcu_irq_ack(struct irq_data *d)
{
	return;
}

static void jzmcu_irq_mask(struct irq_data *d)
{
	struct jzmcu_master *master = d->chip_data;
	volatile struct mailbox_pend_addr_s *mailbox_pend_addr =
		(volatile struct mailbox_pend_addr_s *)(master->iomem + PEND);

	mailbox_pend_addr->irq_mask |= mailbox_pend_addr->irq_state;
}

static void jzmcu_irq_unmask(struct irq_data *d)
{
	struct jzmcu_master *master = d->chip_data;
	volatile struct mailbox_pend_addr_s *mailbox_pend_addr =
		(volatile struct mailbox_pend_addr_s *)(master->iomem + PEND);
	unsigned int tmp;

	mailbox_pend_addr->irq_mask &= ~mailbox_pend_addr->irq_state;
	mailbox_pend_addr->irq_state = 0;

	writel(PDMAM, master->iomem_intc + ICMSR1);
	tmp = readl(master->iomem_intc + DMR1);
	tmp &= ~PDMAM;
	writel(tmp, master->iomem_intc + DMR1);
}

static struct irq_chip jzmcu_irq_chip = {
	.name		= "PDMAM",
	.irq_ack	= jzmcu_irq_ack,
	.irq_mask	= jzmcu_irq_mask,
	.irq_unmask	= jzmcu_irq_unmask,
};

static ssize_t load_fw_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	unsigned long value;
	const struct firmware *fw = NULL;
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct jzmcu_master *mcu = platform_get_drvdata(pdev);

	if (kstrtoul(buf, 10, &value))
		return -EINVAL;

	if (value == 1) {
		ret = request_firmware(&fw, FIRMWARE_NAME, dev);
		if (ret)
			return -ENOENT;

		jzmcu_load_firmware(mcu, fw->data, fw->size);
		jzmcu_mcu_init(mcu);
		jzmcu_mcu_mailbox(mcu);

		release_firmware(fw);
		firmware_loaded = 1;
	}

	return count;
}

static DEVICE_ATTR(load_fw, S_IWUGO, NULL, load_fw_store);

static int __init jzmcu_probe(struct platform_device *pdev)
{
	struct jzmcu_master *mcu;
	struct jzmcu_platform_data *pdata;
	struct resource *iores;
	short irq_pdmam, irq_mcu;
	int i, ret = 0;

	pdata = pdev->dev.platform_data;
	if (!pdata)
		return -ENODATA;

	mcu = kzalloc(sizeof(*mcu), GFP_KERNEL);
	if (!mcu)
		return -ENOMEM;

	mcu->dev = &pdev->dev;

	iores = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pdma");
	mcu->iomem = ioremap(iores->start, resource_size(iores));
	iores = platform_get_resource_byname(pdev, IORESOURCE_MEM, "intc");
	mcu->iomem_intc = ioremap(iores->start, resource_size(iores));
	iores = platform_get_resource_byname(pdev, IORESOURCE_MEM, "gpio");
	mcu->iomem_gpio = ioremap(iores->start, resource_size(iores));
	if (!mcu->iomem || !mcu->iomem_intc || !mcu->iomem_gpio) {
		ret = -ENOMEM;
		goto release_mcu;
	}

	irq_pdmam = platform_get_irq_byname(pdev, "pdmam");
	irq_mcu = platform_get_irq_byname(pdev, "mcu");
	if (irq_pdmam < 0 || irq_mcu < 0) {
		ret = -EINVAL;
		goto release_iomap;
	}

	mcu->clk = clk_get(&pdev->dev, "pdma");
	if (IS_ERR(mcu->clk)) {
		goto release_mcu;
	}

	clk_enable(mcu->clk);

	ret = request_irq(irq_pdmam, jzmcu_int_handler, IRQF_DISABLED,"pdmam", mcu);
	if (ret)
		goto release_clk;

	mcu->irq_pdmam = irq_pdmam;
	mcu->irq_mcu = irq_mcu;

	for (i = pdata->irq_base; i < pdata->irq_end; i++) {
		irq_set_chip_data(i, mcu);
		irq_set_chip_and_handler(i, &jzmcu_irq_chip, handle_level_irq);
	}
	jzmcu_gpio_irq_setup(mcu);

	ret = device_create_file(&pdev->dev, &dev_attr_load_fw);
    if (ret)
		goto release_irq;

	jzmcu_mcu_reset(mcu);

	platform_set_drvdata(pdev, mcu);
	dev_info(mcu->dev, "JZ SoC MCU initialized\n");
	return 0;

release_irq:
	free_irq(irq_pdmam, mcu);
release_clk:
	if (mcu->clk)
		clk_put(mcu->clk);
release_iomap:
	iounmap(mcu->iomem);
	iounmap(mcu->iomem_intc);
release_mcu:
	kfree(mcu);
	return ret;
}

static int __exit jzmcu_remove(struct platform_device *pdev)
{
	struct jzmcu_master *mcu = platform_get_drvdata(pdev);
	kfree(mcu);
	return 0;
}

static int jzmcu_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct jzmcu_master *mcu = platform_get_drvdata(pdev);
	jzmcu_mcu_reset(mcu);
	return 0;
}
static int jzmcu_resume(struct platform_device * pdev)
{
	struct jzmcu_master *mcu = platform_get_drvdata(pdev);
	jzmcu_mcu_init(mcu);
	return 0;
}
static struct platform_driver jzmcu_driver = {
	.driver = {
		   .name = "jz-mcu",
		   },
	.remove = __exit_p(jzmcu_remove),
	.suspend = jzmcu_suspend,
	.resume = jzmcu_resume,
};

static int __init jzmcu_module_init(void)
{
	return platform_driver_probe(&jzmcu_driver, jzmcu_probe);
}
fs_initcall(jzmcu_module_init);

