/*
 * Ingenic SOC MCU controller
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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/delay.h>


#define FIRMWARE_NAME "libmcu-bare.bin"

int firmware_loaded = 0;
EXPORT_SYMBOL(firmware_loaded);

/* MCU controller Regs */
#define DMCS	0x1030
#define DMNMB	0x1034
#define DMSMB	0x1038
#define DMINT	0x103c

#define DMINT_N_IP	(1 << 16)
#define DMINT_N_IMSK	(1 << 0)


#define TCSM_BASE_ADDR	0xb3422000


#define GPIO_PORT_OFF	0x100
#define PXFLG	(0x50)

#define MCU_STATE_SHUTDOWN	1
#define MCU_STATE_BOOTUP	2

struct ingenic_mcu {
	void __iomem	*reg_base;

	struct device 	*dev;
	struct clk 	*clk;
	int		irq_pdmam;
	unsigned int	tcsm_size;
	int		state;


};

static int mcu_load_fw(struct ingenic_mcu *mcu, const struct firmware *fw)
{
	if(fw->size > mcu->tcsm_size) {
		dev_err(mcu->dev, "firmware too large( fw->size: %d > tcsm_size: %d)\n", fw->size, mcu->tcsm_size);
		return -EINVAL;
	}

	memcpy((void *)TCSM_BASE_ADDR, fw->data, fw->size);

	return 0;
}

static inline void mcu_shutdown(struct ingenic_mcu *mcu)
{
	unsigned int dmcs = 0;

	dmcs = readl(mcu->reg_base + DMCS);
	dmcs |= (1 << 0);
	/*1. keep reset*/
	writel(dmcs, mcu->reg_base + DMCS);
	mcu->state = MCU_STATE_SHUTDOWN;
}

static inline void mcu_bootup(struct ingenic_mcu *mcu)
{
	unsigned int dmcs = 0;

	dmcs = readl(mcu->reg_base + DMCS);
	dmcs &= ~(1);
	writel(dmcs, mcu->reg_base + DMCS);

	mcu->state = MCU_STATE_BOOTUP;
}


static int mcu_reset(struct ingenic_mcu *mcu)
{
	unsigned int dmint = 0;

	mcu_shutdown(mcu);

	udelay(10);

	mcu_bootup(mcu);

	/*3. init mailbox*/
	dmint = readl(mcu->reg_base + DMINT);
	dmint &= ~(DMINT_N_IP | DMINT_N_IMSK);
	writel(dmint, mcu->reg_base + DMINT);

	return 0;
}

#ifdef CONFIG_INGENIC_CDBUS
extern int ingenic_cdbus_irq_handler(void);
#endif

irqreturn_t ingenic_mcu_irq(int irq, void *dev)
{
	struct ingenic_mcu *mcu = (struct ingenic_mcu *)dev;
	unsigned int dmint = 0;
#ifdef CONFIG_INGENIC_CDBUS
	unsigned int dmnmb;
#endif
	/*1. mask mailbox int*/
	/*2. clear mailbox pending*/
	dmint = readl(mcu->reg_base + DMINT);
	dmint &= ~(DMINT_N_IP);
	dmint |= DMINT_N_IMSK;
	writel(dmint, mcu->reg_base + DMINT);

	//printk("received msg from mcu!\n");

#ifdef CONFIG_INGENIC_CDBUS
	dmnmb = readl(mcu->reg_base + DMNMB);
	//printk("%s():%d >>> cdbus info = %d\n", __func__, __LINE__, dmnmb);
	if (dmnmb == 0x66)
		ingenic_cdbus_irq_handler();
#endif

	/*3. unmask mailbox int*/
	dmint &= ~(DMINT_N_IMSK);
	writel(dmint, mcu->reg_base + DMINT);

	return IRQ_HANDLED;
}

static ssize_t load_fw_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct ingenic_mcu *mcu = dev_get_drvdata(dev);
	int ret = 0;
	unsigned long value;
	const struct firmware *fw = NULL;

	if (kstrtoul(buf, 10, &value))
		return -EINVAL;

	if (value == 1) {
		ret = request_firmware(&fw, FIRMWARE_NAME, dev);
		if (ret)
			return -ENOENT;

		print_hex_dump(KERN_INFO, "fw: ", DUMP_PREFIX_ADDRESS, 16, 4, fw->data, 128, true);

		mcu_load_fw(mcu, fw);

		mcu_reset(mcu);

		release_firmware(fw);
		firmware_loaded = 1;
	}

	return count;
}

static ssize_t mcu_shutdown_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct ingenic_mcu *mcu = dev_get_drvdata(dev);
	unsigned long value;

	if (kstrtoul(buf, 10, &value))
		return -EINVAL;

	if (value == 1) {
		mcu_shutdown(mcu);
	}

	return count;
}

static ssize_t mcu_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ingenic_mcu *mcu = dev_get_drvdata(dev);
	int ret = 0;

	if(mcu->state == MCU_STATE_SHUTDOWN) {
		ret = sprintf(buf, "state: shutdown\n");
	} else {
		ret = sprintf(buf, "state: running\n");
	}

	return ret;
}


static DEVICE_ATTR(load_fw, S_IWUSR, NULL, load_fw_store);
static DEVICE_ATTR(shutdown, S_IWUSR, NULL, mcu_shutdown_store);
static DEVICE_ATTR(state, S_IRUSR, mcu_state_show, NULL);

static int __init ingenic_mcu_probe(struct platform_device *pdev)
{
	struct ingenic_mcu *mcu = NULL;
	int irq_pdmam = -1;
	int ret = 0;

	mcu = kzalloc(sizeof(*mcu), GFP_KERNEL);
	if (!mcu)
		return -ENOMEM;

	mcu->dev = &pdev->dev;


	ret = of_property_read_u32(pdev->dev.of_node, "ingenic,tcsm_size", &mcu->tcsm_size);
	if(ret < 0) {
		dev_warn(mcu->dev, "Faile to get property ingenic,tcsm_size, use default 8K size\n");
		mcu->tcsm_size = 16 * 1024 * 1024;
	}

	mcu->reg_base = of_iomap(pdev->dev.of_node, 0);;
	if(IS_ERR(mcu->reg_base)) {
		ret = -ENODEV;
		goto err_ioremap;
	}


	irq_pdmam = platform_get_irq_byname(pdev, "pdmam");
	if (irq_pdmam < 0) {
		ret = -EINVAL;
		goto err_irq;
	}
	mcu->irq_pdmam = irq_pdmam;

	ret = devm_request_irq(mcu->dev, mcu->irq_pdmam, ingenic_mcu_irq, IRQF_TRIGGER_RISING, "xburst-mcu", mcu);
	if (ret)
		goto err_irq;


	mcu->clk = devm_clk_get(&pdev->dev, "gate_pdma");
	if (IS_ERR(mcu->clk)) {
		goto err_clk;
	}

	clk_prepare_enable(mcu->clk);

	ret = device_create_file(&pdev->dev, &dev_attr_load_fw);
	if (ret)
		goto err_sys;
	ret = device_create_file(&pdev->dev, &dev_attr_shutdown);
	if (ret)
		goto err_sys;
	ret = device_create_file(&pdev->dev, &dev_attr_state);
	if (ret)
		goto err_sys;

	platform_set_drvdata(pdev, mcu);

	/*TODO: try to request firmware at probe and bootup mcu.*/
	mcu_shutdown(mcu);

	dev_info(mcu->dev, "Ingenic xburst-mcu initialized\n");
	return 0;

err_sys:
	if (mcu->clk)
		devm_clk_put(mcu->dev, mcu->clk);
err_clk:
	devm_free_irq(mcu->dev, irq_pdmam, mcu);
err_irq:
err_ioremap:
	kfree(mcu);
	return ret;
}

static int __exit ingenic_mcu_remove(struct platform_device *pdev)
{
	struct ingenic_mcu *mcu = platform_get_drvdata(pdev);
	kfree(mcu);
	return 0;
}

static int ingenic_mcu_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}
static int ingenic_mcu_resume(struct platform_device * pdev)
{
	return 0;
}

static const struct of_device_id ingenic_mcu_match[] = {
	{.compatible = "ingenic,x2000-mcu", .data = NULL},
	{.compatible = "ingenic,m300-mcu", .data = NULL},
	{.compatible = "ingenic,x1600-mcu", .data = NULL},
};

MODULE_DEVICE_TABLE(of, ingenic_mcu_match);

static struct platform_driver ingenic_mcu_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-mcu",
		.of_match_table = of_match_ptr(ingenic_mcu_match),
	},
	.remove = __exit_p(ingenic_mcu_remove),
	.suspend = ingenic_mcu_suspend,
	.resume = ingenic_mcu_resume,
};

static int __init ingenic_mcu_module_init(void)
{
	return platform_driver_probe(&ingenic_mcu_driver, ingenic_mcu_probe);
}
fs_initcall(ingenic_mcu_module_init);

