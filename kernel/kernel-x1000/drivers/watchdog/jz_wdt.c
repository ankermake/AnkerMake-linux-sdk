/*
 *  Copyright (C) 2010, Paul Cercueil <paul@crapouillou.net>
 *  JZ Watchdog driver
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/err.h>

#define JZ_REG_WDT_TIMER_DATA     0x0
#define JZ_REG_WDT_COUNTER_ENABLE 0x4
#define JZ_REG_WDT_TIMER_COUNTER  0x8
#define JZ_REG_WDT_TIMER_CONTROL  0xC

#define JZ_WDT_CLOCK_PCLK 0x1
#define JZ_WDT_CLOCK_RTC  0x2
#define JZ_WDT_CLOCK_EXT  0x4

#define JZ_WDT_CLOCK_DIV_SHIFT   3

#define JZ_WDT_CLOCK_DIV_1    (0 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_4    (1 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_16   (2 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_64   (3 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_256  (4 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_1024 (5 << JZ_WDT_CLOCK_DIV_SHIFT)

#define DEFAULT_HEARTBEAT 30
#define MAX_HEARTBEAT     2048

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout,
		 "Watchdog cannot be stopped once started (default="
		 __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static unsigned int heartbeat = DEFAULT_HEARTBEAT;
module_param(heartbeat, uint, 0);
MODULE_PARM_DESC(heartbeat,
		"Watchdog heartbeat period in seconds from 1 to "
		__MODULE_STRING(MAX_HEARTBEAT) ", default "
		__MODULE_STRING(DEFAULT_HEARTBEAT));

struct jz_wdt_drvdata {
	struct watchdog_device wdt;
	void __iomem *base;
	struct clk *wdt_clk;
	unsigned int timeout;
};

static void __iomem *wdt_reg_base;

static int __init feed_dog(void)
{
	writew(0x0, wdt_reg_base + JZ_REG_WDT_TIMER_COUNTER);
	return 0;
}

static int jz_wdt_ping(struct watchdog_device *wdt_dev)
{
	struct jz_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
	writew(0x0, drvdata->base + JZ_REG_WDT_TIMER_COUNTER);
	return 0;
}

static int jz_wdt_set_timeout(struct watchdog_device *wdt_dev,
				    unsigned int new_timeout)
{
	struct jz_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
	unsigned int rtc_clk_rate;
	unsigned int timeout_value;
	unsigned short clock_div = JZ_WDT_CLOCK_DIV_1;

	if (drvdata->timeout  != new_timeout) {
		rtc_clk_rate = clk_get_rate(drvdata->wdt_clk);
		timeout_value = rtc_clk_rate * new_timeout;

		while (timeout_value > 0xffff) {
			if (clock_div == JZ_WDT_CLOCK_DIV_1024) {
				/* Requested timeout too high;
				 * use highest possible value. */
				timeout_value = 0xffff;
				break;
			}
			timeout_value >>= 2;
			clock_div += (1 << JZ_WDT_CLOCK_DIV_SHIFT);
		}
		writeb(0x0, drvdata->base + JZ_REG_WDT_COUNTER_ENABLE);
		writew(clock_div, drvdata->base + JZ_REG_WDT_TIMER_CONTROL);

		writew((u16)timeout_value, drvdata->base + JZ_REG_WDT_TIMER_DATA);
		writew(0x0, drvdata->base + JZ_REG_WDT_TIMER_COUNTER);
		writew(clock_div | JZ_WDT_CLOCK_RTC,
				drvdata->base + JZ_REG_WDT_TIMER_CONTROL);
		drvdata->timeout = new_timeout;
	}
	writeb(0x1, drvdata->base + JZ_REG_WDT_COUNTER_ENABLE);
	return 0;
}

static int jz_wdt_start(struct watchdog_device *wdt_dev)
{
	struct jz_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
	clk_enable(drvdata->wdt_clk);
	jz_wdt_set_timeout(wdt_dev,  wdt_dev->timeout);
	return 0;
}

static int jz_wdt_stop(struct watchdog_device *wdt_dev)
{
	struct jz_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);

	clk_disable(drvdata->wdt_clk);
	writeb(0x0, drvdata->base + JZ_REG_WDT_COUNTER_ENABLE);
	return 0;
}

static const struct watchdog_info jz_wdt_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
	.identity = "jz Watchdog",
};

static const struct watchdog_ops jz_wdt_ops = {
	.owner = THIS_MODULE,
	.start = jz_wdt_start,
	.stop = jz_wdt_stop,
	.ping = jz_wdt_ping,
	.set_timeout = jz_wdt_set_timeout,
};

static int jz_wdt_probe(struct platform_device *pdev)
{
	struct jz_wdt_drvdata *drvdata;
	struct watchdog_device *jz_wdt;
	struct resource	*res;
	int ret;

	drvdata = devm_kzalloc(&pdev->dev, sizeof(struct jz_wdt_drvdata),
			       GFP_KERNEL);
	if (!drvdata) {
		dev_err(&pdev->dev, "Unable to alloacate watchdog device\n");
		return -ENOMEM;
	}

	if (heartbeat < 1 || heartbeat > MAX_HEARTBEAT)
		heartbeat = DEFAULT_HEARTBEAT;

	jz_wdt = &drvdata->wdt;
	jz_wdt->info = &jz_wdt_info;
	jz_wdt->ops = &jz_wdt_ops;
	jz_wdt->timeout = heartbeat;
	jz_wdt->min_timeout = 1;
	jz_wdt->max_timeout = MAX_HEARTBEAT;
	watchdog_set_nowayout(jz_wdt, nowayout);
	watchdog_set_drvdata(jz_wdt, drvdata);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	drvdata->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(drvdata->base)) {
		ret = PTR_ERR(drvdata->base);
		goto err_out;
	}

	drvdata->wdt_clk = clk_get(NULL, "wdt");
	if (IS_ERR(drvdata->wdt_clk)) {
		dev_err(&pdev->dev, "cannot find WDT clock\n");
		ret = PTR_ERR(drvdata->wdt_clk);
		goto err_out;
	}

	ret = watchdog_register_device(&drvdata->wdt);
	if (ret < 0)
		goto err_disable_clk;

	platform_set_drvdata(pdev, drvdata);

	wdt_reg_base = drvdata->base;
	feed_dog();

	return 0;

err_disable_clk:
	clk_put(drvdata->wdt_clk);
err_out:
	return ret;
}

static int jz_wdt_remove(struct platform_device *pdev)
{
	struct jz_wdt_drvdata *drvdata = platform_get_drvdata(pdev);

	jz_wdt_stop(&drvdata->wdt);
	watchdog_unregister_device(&drvdata->wdt);
	clk_put(drvdata->wdt_clk);

	return 0;
}

static struct platform_driver jz_wdt_driver = {
	.probe = jz_wdt_probe,
	.remove = jz_wdt_remove,
	.driver = {
		.name = "jz-wdt",
		.owner	= THIS_MODULE,
	},
};

module_platform_driver(jz_wdt_driver);

late_initcall(feed_dog);

MODULE_AUTHOR("bo liu <bo.liu@ingenic.com>");
MODULE_DESCRIPTION("jz xburst Watchdog Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
MODULE_ALIAS("platform:jz-wdt");
