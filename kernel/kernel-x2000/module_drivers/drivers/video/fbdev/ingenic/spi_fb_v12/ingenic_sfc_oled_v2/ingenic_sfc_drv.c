/*
 * SFC controller for SPI protocol, use FIFO and DMA;
 *
 * Copyright (c) 2015 Ingenic
 * Author: <xiaoyang.fu@ingenic.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include "ingenic_sfc_drv.h"
#include "sfc.h"
#include "sfc_flash.h"
#include "ingenic_sfc_common.h"
#if 1
int oled_esd_gpio = -1;
int oled_te_gpio = -1;
int lcd_pwr_gpio = -1;
int lcd_rst_gpio = -1;
int tp_pwr_gpio = -1;



/* include/asm-generic/gpio.h
static inline int gpio_direction_output(unsigned gpio, int value)
static inline void gpio_set_value(unsigned gpio, int value)
static inline void gpio_set_value_cansleep(unsigned gpio, int value)
 */

int oled_swire_gpio=-1;

int swire_power_on_impl(int gpio_swire, int cycles)
{
	int i;
	gpio_swire = oled_swire_gpio;
	printk("%s() gpio_swire=%d, cycles: %d\n", __func__, gpio_swire, cycles);

#define T_INIT (300) 		/* udelay */
#define T_OFF (50) 		/* udelay */
#define T_LOW (10) 		/* udelay */
#define T_HIGH (10) 		/* udelay */
#define T_STORE (55) 		/* udelay */

#if 0
	gpio_direction_output(gpio_swire, 0);
	udelay(100);
	udelay(T_OFF);
	gpio_direction_output(gpio_swire, 1);
#endif
	udelay(T_INIT);

	for(i=0;i<cycles;i++) {
		gpio_direction_output(gpio_swire, 0);
		udelay(T_LOW);
		gpio_direction_output(gpio_swire, 1);
		udelay(T_HIGH);
	}

	udelay(T_STORE);	//gpio_direction_output(gpio_swire, 1);

	return 0;
}

int swire_power_on(int gpio_swire, int on)
{
	printk("===============>>%s:%s() on: %d\n",__FILE__,__func__, on);
	if (on) {
		//swire_power_on(0, 65); // SGM38042 swire: 65, OVSS-3.3V
		//swire_power_on(0, 15); // SGM38042 swire: 15, OVDD3.3V
		swire_power_on_impl(oled_swire_gpio, 65);	
		swire_power_on_impl(oled_swire_gpio, 15);
	}
	else {
		gpio_direction_output(oled_swire_gpio, 0); // 0 failed.
	}
	return;
}
#endif
unsigned int flash_type = -1;
static int __init flash_type_get(char *str)
{
	if(!strcmp(str,"nand"))
		flash_type = NAND;
	if(!strcmp(str,"nor"))
		flash_type = NOR;
	return 0;
}
early_param("flashtype", flash_type_get);

static const struct of_device_id ingenic_sfc_match[];

static int __init ingenic_sfc_probe(struct platform_device *pdev)
{
	printk("===============>>%s:%s:%d\n",__FILE__,__func__,__LINE__);
	struct sfc_flash *flash;
	int ret = 0;
	const struct of_device_id *of_match;
	struct sfc_data *data = NULL;
	enum of_gpio_flags flags;

	flash = kzalloc(sizeof(struct sfc_flash), GFP_KERNEL);
	if (IS_ERR_OR_NULL(flash))
		return -ENOMEM;

	platform_set_drvdata(pdev, flash);
	flash->dev = &pdev->dev;
#if 1
	lcd_pwr_gpio = of_get_named_gpio_flags(pdev->dev.of_node, "ingenic,lcd-pwr-gpio", 0, &flags);
	printk("%s(), lcd_pwr_gpio=%d\n", __func__, lcd_pwr_gpio);
	if(gpio_is_valid(lcd_pwr_gpio)) {
		int active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(lcd_pwr_gpio, GPIOF_DIR_OUT, "lcd-pwr-gpio");
		if(ret < 0) {
		  printk("Failed to request te pin!\n");
			//goto err_request_rst;
		}
		printk("%s(), gpio_direction_output(lcd_pwr_gpio); active_level=%d\n", __func__, active_level);
		//gpio_direction_output(lcd_pwr_gpio, 0); // 0 failed.
		//gpio_direction_output(lcd_pwr_gpio, 0); mdelay(100);
		//gpio_direction_output(lcd_pwr_gpio, 1); mdelay(100);
		gpio_direction_output(lcd_pwr_gpio, 0); mdelay(100);
	} else {
		lcd_pwr_gpio = -1;
		printk("invalid gpio te.gpio: %d\n", lcd_pwr_gpio);
	}

	tp_pwr_gpio = of_get_named_gpio_flags(pdev->dev.of_node, "ingenic,tp-pwr-gpio", 0, &flags);
	printk("%s(), tp_pwr_gpio=%d\n", __func__, tp_pwr_gpio);
	if(gpio_is_valid(tp_pwr_gpio)) {
		int active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(tp_pwr_gpio, GPIOF_DIR_OUT, "tp-pwr-gpio");
		if(ret < 0) {
		  printk("Failed to request te pin!\n");
			//goto err_request_rst;
		}
		printk("%s(), gpio_direction_output(tp_pwr_gpio); active_level=%d\n", __func__, active_level);
		//gpio_direction_output(tp_pwr_gpio, 0); // 0 failed.
		//gpio_direction_output(tp_pwr_gpio, 0); mdelay(100);
		//gpio_direction_output(tp_pwr_gpio, 1); mdelay(100);
		gpio_direction_output(tp_pwr_gpio, 0); mdelay(100);
	} else {
		tp_pwr_gpio = -1;
		printk("invalid gpio te.gpio: %d\n", tp_pwr_gpio);
	}

//printk("%s() %d ...\n", __func__, __LINE__); mdelay(3000) ;
	/* swire: dc-dc power controle */
	oled_swire_gpio = of_get_named_gpio_flags(pdev->dev.of_node, "ingenic,swire-gpio", 0, &flags);
	//printk("%s(), oled_swire_gpio=%d\n", __func__, oled_swire_gpio);
	if(gpio_is_valid(oled_swire_gpio)) {
		int active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(oled_swire_gpio, GPIOF_DIR_OUT, "swire-gpio");
		if(ret < 0) {
		  printk("Failed to request swire pin!\n");
			//goto err_request_rst;
		}
		//printk("%s(), swire_power_on(oled_swire_gpio, 22);\n", __func__);
		printk("%s(), gpio_direction_output(oled_swire_gpio, 0);\n", __func__);
		gpio_direction_output(oled_swire_gpio, 0); // 0 failed.
		//while(1);
		//gpio_direction_output(oled_swire_gpio, 1);
		//swire_power_on(oled_swire_gpio, 65);  // 37: -1.8
		//swire_power_on(oled_swire_gpio, 15);  // 37: -1.8
	} else {
		oled_swire_gpio = -1;
		printk("invalid gpio swire.gpio: %d\n", oled_swire_gpio);
	}

//printk("%s() %d ... gpio_request_one(oled_swire_gpio\n", __func__, __LINE__); mdelay(3000) ;
	/*  snow display after reset  */
	lcd_rst_gpio = of_get_named_gpio_flags(pdev->dev.of_node, "ingenic,lcd-rst-gpio", 0, &flags);
	printk("%s(), lcd_rst_gpio=%d\n", __func__, lcd_rst_gpio);
	if(!gpio_is_valid(lcd_rst_gpio)) {
		lcd_rst_gpio = of_get_named_gpio_flags(pdev->dev.of_node, "ingenic,rst-gpio", 0, &flags);
	}
	printk("%s(), lcd_rst_gpio=%d\n", __func__, lcd_rst_gpio);
	if(gpio_is_valid(lcd_rst_gpio)) {
		int active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(lcd_rst_gpio, GPIOF_DIR_OUT, "rst");
		if(ret < 0) {
		  printk("Failed to request rst pin!\n");
			//goto err_request_rst;
		}
		printk("%s(), gpio_direction_output(rst->gpio, 1) lcd_rst_gpio=%d, active_level=%d\n", __func__, lcd_rst_gpio, active_level);
		gpio_direction_output(lcd_rst_gpio, 1);
		msleep(20);
		gpio_direction_output(lcd_rst_gpio, 0); // 0 failed.
		msleep(20);
		//gpio_direction_output(lcd_rst_gpio, 1);
		msleep(200);	/* reset complete time. */
	} else {
		printk("invalid gpio rst.gpio: %d\n", lcd_rst_gpio);
	}
	
//printk("%s() %d lcd_rst_gpio ...\n", __func__, __LINE__); mdelay(3000);
	oled_te_gpio = of_get_named_gpio_flags(pdev->dev.of_node, "ingenic,te-gpio", 0, &flags);
	printk("%s(), oled_te_gpio=%d\n", __func__, oled_te_gpio);
	if(gpio_is_valid(oled_te_gpio)) {
		int active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(oled_te_gpio, GPIOF_DIR_IN, "te-gpio");
		if(ret < 0) {
		  printk("Failed to request te pin!\n");
			//goto err_request_rst;
		}
		printk("%s(), gpio_direction_input(oled_te_gpio);\n", __func__);
		//gpio_direction_output(oled_te_gpio, 0); // 0 failed.
		gpio_direction_input(oled_te_gpio);
	} else {
		oled_te_gpio = -1;
		printk("invalid gpio te.gpio: %d\n", oled_te_gpio);
	}


	oled_esd_gpio = of_get_named_gpio_flags(pdev->dev.of_node, "ingenic,esd-gpio", 0, &flags);
	printk("%s(), oled_esd_gpio=%d\n", __func__, oled_esd_gpio);
	if(gpio_is_valid(oled_esd_gpio)) {
		int active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(oled_esd_gpio, GPIOF_DIR_IN, "te-gpio");
		if(ret < 0) {
		  printk("Failed to request esd pin!\n");
			//goto err_request_rst;
		}
		printk("%s(), gpio_direction_input(oled_esd_gpio);\n", __func__);
		//gpio_direction_output(oled_esd_gpio, 0); // 0 failed.
		gpio_direction_input(oled_esd_gpio);
	} else {
		oled_esd_gpio = -1;
		printk("invalid gpio te.gpio: %d\n", oled_esd_gpio);
	}


	/* tpint: input */
	int tpint_gpio = of_get_named_gpio_flags(pdev->dev.of_node, "ingenic,tpint-gpio", 0, &flags);
	//printk("%s(), tpint_gpio=%d\n", __func__, tpint_gpio);
	if(gpio_is_valid(tpint_gpio)) {
		int active_level = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		ret = gpio_request_one(tpint_gpio, GPIOF_DIR_IN, "tpint-gpio");
		if(ret < 0) {
		  printk("Failed to request tpint pin!\n");
			//goto err_request_rst;
		}
		printk("%s(), gpio_direction_input(tpint_gpio, pa10);\n", __func__);
		gpio_direction_input(tpint_gpio);
	} else {
		printk("invalid gpio tpint.gpio: %d\n", tpint_gpio);
	}

//printk("%s() %d ...\n", __func__, __LINE__); mdelay(3000) ;
#endif
	of_match = of_match_node( ingenic_sfc_match, pdev->dev.of_node);
	if(IS_ERR_OR_NULL(of_match))
	{
		kfree(flash);
		return -ENODEV;
	}

	data = (struct sfc_data *)of_match->data;

	ret = of_property_read_u32(pdev->dev.of_node, "ingenic,sfc-init-frequency", (unsigned int *)&flash->sfc_init_frequency);
	if (ret < 0) {
		dev_err(flash->dev, "Cannot get sfc init frequency\n");
	}

	ret = of_property_read_u32(pdev->dev.of_node, "ingenic,sfc-max-frequency", (unsigned int *)&flash->sfc_max_frequency);
	if (ret < 0) {
		dev_err(flash->dev, "Cannot get sfc max frequency\n");
		kfree(flash);
		return -ENOENT;
	}

	flash->sfc = sfc_res_init(pdev);
	if(IS_ERR(flash->sfc)) {
		dev_err(flash->dev, "sfc control init error!\n");
		kfree(flash);
		return PTR_ERR(flash->sfc);
	}

	flash->pdata_params = pdev->dev.platform_data;

	mutex_init(&flash->lock);

	if(flash_type == -1){						/*flash type is not declared in bootargs */
		flash_type = data->flash_type_auto_detect(pdev);
	}

	switch(flash_type)
	{
		case NAND:
			ret = ingenic_sfc_nand_probe(flash);
			break;
//		case NOR:
//			ret = ingenic_sfc_nor_probe(flash);
//			break;
		default:
			dev_err(&pdev->dev, "unknown flash type");
			ret = -EINVAL;
	}
	if(ret){
		kfree(flash);
		return ret;
	}

	return 0;
}

static int __exit ingenic_sfc_remove(struct platform_device *pdev)
{
	struct sfc_flash *flash = platform_get_drvdata(pdev);
	struct sfc *sfc = flash->sfc;

	clk_disable_unprepare(sfc->clk_gate);
	clk_put(sfc->clk_gate);
	clk_disable_unprepare(sfc->clk);
	clk_put(sfc->clk);
	free_irq(sfc->irq, flash);
	iounmap(sfc->iomem);
	release_mem_region(sfc->ioarea->start, resource_size(sfc->ioarea));
	platform_set_drvdata(pdev, NULL);
	free_sfc_desc(sfc);

	if(flash_type == NAND){
		dma_free_coherent(flash->dev, flash->mtd.writesize, flash->sfc->tmp_buffer, flash->sfc->tbuff_pyaddr);
#ifdef CONFIG_INGENIC_SFCNAND_FMW
		sysfs_remove_group(&pdev->dev.kobj, flash->attr_group);
#endif
	} else if(flash_type == NOR)
		sysfs_remove_group(&pdev->dev.kobj, flash->attr_group);
	else{
		dev_err(&pdev->dev, "unknown flash type!\n");
		return -EINVAL;
	}
	return 0;
}


static int ingenic_sfc_suspend(struct platform_device *pdev, pm_message_t msg)
{
	struct sfc_flash *flash = platform_get_drvdata(pdev);
	struct sfc *sfc = flash->sfc;

	/* 1.Memory power OFF */
	/*(*(volatile unsigned int *)0xb00000f8) |= (1 << 26);*/

	/* 2.Irq OFF */
	disable_irq(sfc->irq);

	/* 3.Clk OFF */
	clk_disable_unprepare(sfc->clk_gate);
	clk_disable_unprepare(sfc->clk);

	return 0;
}

static int ingenic_sfc_resume(struct platform_device *pdev)
{
	struct sfc_flash *flash = platform_get_drvdata(pdev);
	struct sfc *sfc = flash->sfc;

	/* 1.Clk ON */
	clk_prepare_enable(sfc->clk);
	clk_prepare_enable(sfc->clk_gate);

	/* 2.Irq ON */
	enable_irq(sfc->irq);

	/* 3.Memory power ON */
	/*(*(volatile unsigned int *)0xb00000f8) &= ~(1 << 26);*/

	flash->create_cdt_table(flash->sfc, flash->flash_info, DEFAULT_CDT | UPDATE_CDT);

	return 0;
}

void ingenic_sfc_shutdown(struct platform_device *pdev)
{
	struct sfc_flash *flash = platform_get_drvdata(pdev);
	struct sfc *sfc = flash->sfc;

	disable_irq(sfc->irq);
	clk_disable_unprepare(sfc->clk_gate);
	clk_disable_unprepare(sfc->clk);
	return ;
}

int x2000_m300_flash_type_audo_detect(struct platform_device *pdev)
{
	unsigned char flag;
	int ret;

	flag = *(volatile unsigned int *)(0xb2401008);
	switch(flag)
	{
		case 0x00:
			ret = NAND;
			break;
		case 0xaa:
			ret = NOR;
			break;
		default:
			ret = -EINVAL;
	}
	return ret;
}

struct sfc_data x2000_sfc_priv = {
	.flash_type_auto_detect = x2000_m300_flash_type_audo_detect,
};

struct sfc_data m300_sfc_priv = {
	.flash_type_auto_detect = x2000_m300_flash_type_audo_detect,
};

static const struct of_device_id ingenic_sfc_match[] = {
	{ .compatible = "ingenic,x2000-sfc",
	  .data = &x2000_sfc_priv, },
	{ .compatible = "ingenic,m300-sfc",
	  .data = &m300_sfc_priv, },
	{},
};
MODULE_DEVICE_TABLE(of, ingenic_sfc_match);

static struct platform_driver ingenic_sfc_drv = {
	.driver		= {
		.name	= "ingenic-sfc",
		.owner	= THIS_MODULE,
		.of_match_table = ingenic_sfc_match,
	},
	.remove		= __exit_p(ingenic_sfc_remove),
	.suspend	= ingenic_sfc_suspend,
	.resume		= ingenic_sfc_resume,
	.shutdown	= ingenic_sfc_shutdown,
};
module_platform_driver_probe(ingenic_sfc_drv, ingenic_sfc_probe);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("INGENIC SFC Driver");
