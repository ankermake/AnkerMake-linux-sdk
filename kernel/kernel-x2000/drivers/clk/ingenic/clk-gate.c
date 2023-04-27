/*
 * Copyright (C) 2016 Ingenic Semiconductor Co., Ltd.
 * Author: cli <chen.li@ingenic.com>
 *
 * The CLock Gate Interface for Ingenic's SOC, such as X1000,
 * and so on. (kernel.4.4)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/regmap.h>
#include "clk-gate.h"

static int ingenic_gate_enable(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_gate_hwdesc *hwdesc = iclk->hwdesc;
	int ret = 0;
	unsigned long flags;

	CLK_LOCK(iclk, flags);
	regmap_update_bits_check(iclk->pm_regmap,
			hwdesc->regoff, BIT(hwdesc->bit),
			hwdesc->invert ? BIT(hwdesc->bit) : ~BIT(hwdesc->bit),
			NULL);

	pr_debug("regmap %s hwdesc->bit = %d gate [%x] %x enable\n", clk_hw_get_name(hw),
			hwdesc->bit, hwdesc->regoff, clkhw_readl(iclk, hwdesc->regoff));
	CLK_UNLOCK(iclk, flags);
	return ret;
}

static void ingenic_gate_disable(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_gate_hwdesc *hwdesc = iclk->hwdesc;
	unsigned long flags;

	CLK_LOCK(iclk, flags);

	regmap_update_bits_check(iclk->pm_regmap,
			hwdesc->regoff, BIT(hwdesc->bit),
			!hwdesc->invert ? BIT(hwdesc->bit) : ~BIT(hwdesc->bit),
			NULL);

	pr_debug("regmap %s hwdesc->bit = %d gate [%x] %x disable\n", clk_hw_get_name(hw),
			hwdesc->bit, hwdesc->regoff, clkhw_readl(iclk, hwdesc->regoff));
	CLK_UNLOCK(iclk, flags);

	return;
}

static int ingenic_gate_is_enabled(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_gate_hwdesc *hwdesc = iclk->hwdesc;
	unsigned gate;

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &gate);
	if (!hwdesc->invert)
		return !(gate & BIT(hwdesc->bit));
	else
		return (gate & BIT(hwdesc->bit));
}

#ifdef CONFIG_INGENIC_CLK_DEBUG_FS
static ssize_t gate_show_hwdesc(struct file *file, char __user *user_buf,
		size_t count, loff_t *ppos)
{
	struct ingenic_clk *iclk = file->private_data;
	struct ingenic_gate_hwdesc *hwdesc = iclk->hwdesc;
	char *buf;
	int len = 0;
	ssize_t ret;

#define REGS_BUFSIZE	4096
	buf = kzalloc(REGS_BUFSIZE, GFP_KERNEL);
	if (!buf)
		return 0;
	len += snprintf(buf + len, REGS_BUFSIZE - len, "%s hardware description: \n", clk_hw_get_name(&(iclk->hw)));
	len += snprintf(buf + len, REGS_BUFSIZE - len, "regbase  [0x%08x]\n", (u32)iclk->regbase);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "spinlock [%s]%p\n", iclk->spinlock?"YES":"NO", iclk->spinlock);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "regoff  [0x%03x]\n", hwdesc->regoff);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "bit_bs  [0x%08x](%d)\n", (unsigned int)BIT(hwdesc->bit), hwdesc->bit);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "regval  [0x%08x]\n", clkhw_readl(iclk, hwdesc->regoff));
	ret =  simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);
#undef REGS_BUFSIZE
	return ret;
}

static const struct file_operations gate_debug_fops  =  {
	.open = simple_open,
	.read = gate_show_hwdesc,
	.llseek = default_llseek,
};

static int  ingenic_gate_debug_init(struct clk_hw *hw, struct dentry *dentry)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct dentry *ret;
	ret = debugfs_create_file("hwdesc", 0444, dentry, (void *)iclk, &gate_debug_fops);
	if (IS_ERR(ret) || !ret)
		return -ENODEV;
	return 0;
}
#endif

const struct clk_ops ingenic_gate_ops = {
	.enable = ingenic_gate_enable,
	.disable = ingenic_gate_disable,
	.is_enabled = ingenic_gate_is_enabled,
#ifdef CONFIG_INGENIC_CLK_DEBUG_FS
	.debug_init = ingenic_gate_debug_init,
#endif
};
