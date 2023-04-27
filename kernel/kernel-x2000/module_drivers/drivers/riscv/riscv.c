#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>

#include "riscv.h"


static inline void riscv_reg_writel(struct riscv_device *riscv, unsigned int reg, unsigned int val)
{
	writel(val, riscv->iobase + reg);
}

static inline unsigned int riscv_reg_readl(struct riscv_device *riscv, unsigned int reg)
{
	return readl(riscv->iobase + reg);
}


static RAW_NOTIFIER_HEAD(riscv_notifier_chain);

int register_riscv_notifier(struct notifier_block *nb)
{
	return raw_notifier_chain_register(&riscv_notifier_chain, nb);
}
EXPORT_SYMBOL(register_riscv_notifier);

int unregister_riscv_notifier(struct notifier_block *nb)
{
	return raw_notifier_chain_unregister(&riscv_notifier_chain, nb);
}
EXPORT_SYMBOL(unregister_riscv_notifier);

int riscv_notifier_call_chain(unsigned long val, void *v)
{
	return raw_notifier_call_chain(&riscv_notifier_chain, val, v);
}
EXPORT_SYMBOL(riscv_notifier_call_chain);

static irqreturn_t riscv_irq_handler(int irq, void *data)
{
	struct riscv_device *riscv = (struct riscv_device *)data;
	unsigned int message = 0;

	//get message
	message = riscv_reg_readl(riscv, CCU_TO_HOST);
	dev_info(riscv->dev, "receive riscv message : 0x%x\n", message);
	riscv_notifier_call_chain(message, NULL);
	//clear interrupt
	riscv_reg_writel(riscv, CCU_TO_HOST, 0);
	//reply
	riscv_reg_writel(riscv, CCU_FROM_HOST, REPLY);

	return IRQ_HANDLED;
}

static int ingenic_riscv_probe(struct platform_device *pdev)
{

	struct riscv_device *riscv = NULL;
	struct resource *regs = NULL;
	void *vaddr;
	dma_addr_t paddr;
	int ret;

	riscv = kzalloc(sizeof(struct riscv_device), GFP_KERNEL);
	if(!riscv) {
		pr_err("Failed to alloc riscv dev [%s]\n", pdev->name);
		ret = -ENOMEM;
		goto err_alloc;
	}

	riscv->dev = &pdev->dev;
	platform_set_drvdata(pdev, riscv);

	mutex_init(&riscv->mutex);

	riscv->irq = platform_get_irq(pdev, 0);
	if(riscv->irq < 0) {
		dev_warn(&pdev->dev, "No RISCV IRQ specified\n");
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!regs) {
		dev_err(&pdev->dev, "No iomem resource!\n");
		goto err_get_resource;
	}

	riscv->iobase = devm_ioremap_resource(&pdev->dev, regs);
	if(!riscv->iobase) {
		goto err_ioremap;
	}

	ret = devm_request_irq(riscv->dev, riscv->irq, riscv_irq_handler, 0,
			dev_name(riscv->dev), riscv);
	if(ret) {
		dev_err(riscv->dev, "request irq failed!\n");
		goto err_request_irq;
	}


	ret = of_reserved_mem_device_init(riscv->dev);
	if(ret)
		dev_warn(riscv->dev, "failed to init reserved mem\n");

	dev_info(riscv->dev, "riscv_init_done!\n");

	return 0;

err_request_irq:
	devm_iounmap(riscv->dev, riscv->iobase);
err_ioremap:
err_get_resource:
err_alloc:
	return ret;
}



static int ingenic_riscv_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id ingenic_riscv_dt_match[] = {
        { .compatible = "ingenic,x2500-riscv" },
        { }
};

MODULE_DEVICE_TABLE(of, ingenic_riscv_dt_match);

static int __maybe_unused ingenic_riscv_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
        return 0;
}

static int __maybe_unused ingenic_riscv_drv_resume(struct platform_device *pdev)
{
        return 0;
}

static struct platform_driver ingenic_riscv_driver = {
        .probe = ingenic_riscv_probe,
        .remove = ingenic_riscv_remove,
	.suspend = ingenic_riscv_drv_suspend,
	.resume = ingenic_riscv_drv_resume,
        .driver = {
                .name = "ingenic-riscvdrv",
                .of_match_table = ingenic_riscv_dt_match,
        },
};

module_platform_driver(ingenic_riscv_driver);

MODULE_ALIAS("platform:ingenic-riscv");
MODULE_DESCRIPTION("ingenic riscv subsystem");
MODULE_AUTHOR("qipengzhen <aric.pzqi@ingenic.com>");
MODULE_LICENSE("GPL v2");


