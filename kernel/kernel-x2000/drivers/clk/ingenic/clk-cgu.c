/*
 * Copyright (C) 2016 Ingenic Semiconductor Co., Ltd.
 * Author: cli <chen.li@ingenic.com>
 *
 * The Peripherals Part Of Clock Generate Uint interface for Ingenic's SOC,
 * such as X1000, and so on. (kernel.4.4)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/regmap.h>
#include "clk-cgu.h"

int ingenic_cgu_set_parent(struct clk_hw *hw, u8 index)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_hwdesc *hwdesc = iclk->hwdesc;
	unsigned long flags;
	unsigned int clear_ce = 0;

	CLK_LOCK(iclk, flags);

	/*
	 *	Register reset vaule or after bootloader, ce may be one
	 *	But some register has this feature, eg:
	 *	"If UCS == 0, the clock divider is gated, don't set CE_USB"
	 *	so we clear ce here.
	 */
	if (is_fixclk(hwdesc, index) && hwdesc->bit_ce != (u8)(-1))
		clear_ce = BIT(hwdesc->bit_ce);

	regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
			(hwdesc->cs_msk << hwdesc->cs_off) | clear_ce,
			index << hwdesc->cs_off,
			NULL);

	CLK_UNLOCK(iclk, flags);
	return 0;
}

u8 ingenic_cgu_get_parent(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t xcdr;

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);

	return (xcdr >> hwdesc->cs_off) & hwdesc->cs_msk;
}

unsigned long ingenic_cgu_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_hwdesc *hwdesc = iclk->hwdesc;
	int div;
	uint32_t xcdr;
	u8 cs;

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);
	cs = (xcdr >> hwdesc->cs_off) & hwdesc->cs_msk;

	if (is_fixclk(hwdesc, cs))
		return parent_rate;

	div = (xcdr >> hwdesc->cdr_off) & hwdesc->cdr_msk;
	div += 1;
	div = div * hwdesc->div_step;
	return DIV_ROUND_UP(parent_rate, div);
}

int ingenic_cgu_determine_rate(struct clk_hw *hw,
		struct clk_rate_request *req)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_hwdesc *hwdesc = iclk->hwdesc;
	unsigned long div;

	if (hwdesc->fixclk != NULL) {
		struct clk_hw *fixclk;
		unsigned long fix_rate;
		uint32_t xcdr;
		u8 cs;

		fixclk = clk_hw_get_parent_by_index(hw, hwdesc->fixclk[0]);
		fix_rate = clk_hw_get_rate(fixclk);

		regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);
		cs = (xcdr >> hwdesc->cs_off) & hwdesc->cs_msk;

		if ((fix_rate == req->rate) || is_fixclk(hwdesc, cs)) {
			req->best_parent_rate = fix_rate;
			req->best_parent_hw = fixclk;
			req->rate = fix_rate;
			return 0;
		} else {
			/*FIXME*/
		}
	}

	div = DIV_ROUND_UP(req->best_parent_rate, req->rate * hwdesc->div_step);
	req->rate = DIV_ROUND_UP(req->best_parent_rate, div * hwdesc->div_step);
	return 0;
}

int ingenic_cgu_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t xcdr, cdr, cdr_tmp;
	unsigned long flags;
	u8 cs;

	CLK_LOCK(iclk, flags);

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);
	cs = (xcdr >> hwdesc->cs_off) & hwdesc->cs_msk;

	/*the clock divider is gated, don‘t set CE_*/
	if (is_fixclk(hwdesc, cs))
		goto out;

	cdr_tmp = (xcdr >>  hwdesc->cdr_off) & hwdesc->cdr_msk;
	cdr = DIV_ROUND_UP((parent_rate) , rate * hwdesc->div_step);
	cdr -= 1;
	if (cdr == cdr_tmp)
		goto out;

	if(!(xcdr & BIT(hwdesc->bit_stop))) {
		regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
				(hwdesc->cdr_msk << hwdesc->cdr_off) | BIT(hwdesc->bit_ce),
				(cdr <<  hwdesc->cdr_off) | BIT(hwdesc->bit_ce),
				NULL);
		while (1) {
			regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);
			if (!(xcdr & BIT(hwdesc->bit_busy)))
				break;
			printk("wait stable.[%d][%s] %x\n",__LINE__, clk_hw_get_name(hw), xcdr);
	}
	}
	regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
			(hwdesc->cdr_msk << hwdesc->cdr_off | BIT(hwdesc->bit_ce)),
			cdr << hwdesc->cdr_off,
			NULL);
out:
	CLK_UNLOCK(iclk, flags);
	return 0;
}

int ingenic_cgu_enable(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t xcdr;
	u8 cs;
	unsigned long flags;

	CLK_LOCK(iclk, flags);

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);
	cs = (xcdr >> hwdesc->cs_off) & hwdesc->cs_msk;

	/*the clock divider is gated, don‘t set CE_*/
	if (is_fixclk(hwdesc, cs))
		goto out;

	/*has already enabled*/
	if (!(xcdr & BIT(hwdesc->bit_stop)) &&
			!(xcdr & BIT(hwdesc->bit_ce)))
		goto out;

	regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
			BIT(hwdesc->bit_stop) | BIT(hwdesc->bit_ce),
			BIT(hwdesc->bit_ce),
			NULL);
	while (1) {
		regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);
		if (!(xcdr & BIT(hwdesc->bit_busy)))
			break;
		printk("wait stable.[%d][%s] %x\n",__LINE__, clk_hw_get_name(hw), xcdr);
	}

	regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
			BIT(hwdesc->bit_ce), 0,
			NULL);
out:
	CLK_UNLOCK(iclk, flags);
	return 0;
}

void ingenic_cgu_disable(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t xcdr;
	unsigned long flags;
	u8 cs;

	CLK_LOCK(iclk, flags);

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);
	cs = (xcdr >> hwdesc->cs_off) & hwdesc->cs_msk;

	/*the clock divider is gated, don‘t set CE_*/
	if (is_fixclk(hwdesc, cs))
		goto out;

	/*has already disabled*/
	if (xcdr & BIT(hwdesc->bit_stop) &&
			!(xcdr & BIT(hwdesc->bit_ce)))
		goto out;

	regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
			BIT(hwdesc->bit_stop) | BIT(hwdesc->bit_ce),
			BIT(hwdesc->bit_stop) | BIT(hwdesc->bit_ce),
			NULL);
	regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
			BIT(hwdesc->bit_ce), 0,
			NULL);
out:
	CLK_UNLOCK(iclk, flags);
	return;
}

int ingenic_cgu_is_enabled(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t xcdr, cs;

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);
	cs = (xcdr >> hwdesc->cs_off) & hwdesc->cs_msk;

	if (is_fixclk(hwdesc, cs))
		return 1;
	else
		return (int)(!(xcdr & BIT(hwdesc->bit_stop)));
}

#ifdef CONFIG_INGENIC_CLK_DEBUG_FS
static ssize_t cgu_show_hwdesc(struct file *file, char __user *user_buf,
		size_t count, loff_t *ppos)
{
	struct ingenic_clk *iclk = file->private_data;
	struct ingenic_cgu_hwdesc *hwdesc = iclk->hwdesc;
	char *buf;
	int len = 0, i;
	ssize_t ret;

#define REGS_BUFSIZE	4096
	buf = kzalloc(REGS_BUFSIZE, GFP_KERNEL);
	if (!buf)
		return 0;
	len += snprintf(buf + len, REGS_BUFSIZE - len, "%s hardware description: \n", clk_hw_get_name(&(iclk->hw)));
	len += snprintf(buf + len, REGS_BUFSIZE - len, "regbase  [0x%08x]\n", (u32)iclk->regbase);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "spinlock [%s]%p\n", iclk->spinlock? "YES" : "NO", iclk->spinlock);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "regoff  [0x%03x]\n", hwdesc->regoff);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "cs_msk  [0x%08x]\n", hwdesc->cs_msk << hwdesc->cs_off);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "ce      [0x%08x]\n", (unsigned)BIT(hwdesc->bit_ce));
	len += snprintf(buf + len, REGS_BUFSIZE - len, "stop    [0x%08x]\n", (unsigned)BIT(hwdesc->bit_stop));
	len += snprintf(buf + len, REGS_BUFSIZE - len, "busy    [0x%08x]\n", (unsigned)BIT(hwdesc->bit_busy));
	len += snprintf(buf + len, REGS_BUFSIZE - len, "div_msk [0x%08x]\n", hwdesc->cdr_msk << hwdesc->cdr_off);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "regval  [0x%08x]\n", clkhw_readl(iclk, hwdesc->regoff));
	len += snprintf(buf + len, REGS_BUFSIZE - len, "div_step [%d]\n", hwdesc->div_step);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "fix clk: ");
	for (i = 0; i < hwdesc->fixclk_sz; i++) {
		len += snprintf(buf + len, REGS_BUFSIZE - len, "[0x%x] ", hwdesc->fixclk[i]);
	}
	len += snprintf(buf + len, REGS_BUFSIZE - len, "\n");
	ret =  simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);
#undef REGS_BUFSIZE
	return ret;
}

static const struct file_operations cgu_debug_fops  =  {
	.open = simple_open,
	.read = cgu_show_hwdesc,
	.llseek = default_llseek,
};

int  ingenic_cgu_debug_init(struct clk_hw *hw, struct dentry *dentry)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct dentry *ret;

	ret = debugfs_create_file("hwdesc", 0444, dentry, (void *)iclk, &cgu_debug_fops);

	if (IS_ERR(ret) || !ret)
		return -ENODEV;
	return 0;
}
#endif

const struct clk_ops __unused ingenic_cgu_ops = {
	.set_parent = ingenic_cgu_set_parent,
	.get_parent = ingenic_cgu_get_parent,
	.enable = ingenic_cgu_enable,
	.disable = ingenic_cgu_disable,
	.is_enabled = ingenic_cgu_is_enabled,
	.recalc_rate = ingenic_cgu_recalc_rate,
	.determine_rate = ingenic_cgu_determine_rate,
	.set_rate = ingenic_cgu_set_rate,
#ifdef CONFIG_INGENIC_CLK_DEBUG_FS
	.debug_init = ingenic_cgu_debug_init,
#endif
};
