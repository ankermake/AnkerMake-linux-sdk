/*
 * drivers/watchdog/sgm820_wdt.c
 *
 * Watchdog driver for sgm820/Kirkwood processors
 *
 * Author: Sylver Bruneau <sylver.bruneau@googlemail.com>
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>



struct sgm820_watchdog {
	unsigned long wdi;
};


static struct task_struct *task;
static int sgm820_feed_dog(void *pdata)
{
    int ret = 0;
	struct sgm820_watchdog *sgm820_pdata = pdata;

    ret = gpio_request(sgm820_pdata->wdi, "sgm820_wdi");
    if(ret){
        printk(KERN_ERR "sgm820 request wdi error!\n");
        return -1;
    }
    while(1){
        gpio_direction_output(sgm820_pdata->wdi, 1);
        msleep(500);
        gpio_direction_output(sgm820_pdata->wdi, 0);
        msleep(500);
    }
    return 0;
}

static const struct of_device_id sgm820_wdt_of_match_table[] = {
	{
		.compatible = "sgm820-wdt",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sgm820_wdt_of_match_table);

static int sgm820_wdt_probe(struct platform_device *pdev)
{
	struct sgm820_watchdog *dev;
	int ret;
    enum of_gpio_flags flags;

	dev = devm_kzalloc(&pdev->dev, sizeof(struct sgm820_watchdog),
			   GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

    dev->wdi = of_get_named_gpio_flags(pdev->dev.of_node, "ingenic,sgm820-wdi",
               0, &flags);
    if(!gpio_is_valid(dev->wdi)){
        printk("%s: request wdi pin err\n", __func__);
    	return ret;
    }
    task = kthread_run(sgm820_feed_dog, dev, "sgm820");
    if (IS_ERR(task)){
        printk(KERN_ERR "create sgm820 thread failed!\n");
    }
    return 0;
}

static int sgm820_wdt_remove(struct platform_device *pdev)
{
	return 0;
}


static struct platform_driver sgm820_wdt_driver = {
	.probe		= sgm820_wdt_probe,
	.remove		= sgm820_wdt_remove,
	.driver		= {
		.name	= "sgm820_wdt",
		.of_match_table = sgm820_wdt_of_match_table,
	},
};

module_platform_driver(sgm820_wdt_driver);
MODULE_DESCRIPTION("sgm820 Processor Watchdog");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sgm820_wdt");
