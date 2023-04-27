/*
 * Copyright (C) 2016 Ingenic Semiconductor Co., Ltd.
 * Author: cli <chen.li@ingenic.com>
 *
 * The PLL Part Of Clock Generate Uint interface for Ingenic's SOC,
 * such as X1000, and so on. (kernel.4.4)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/debugfs.h>
#include <linux/slab.h>
#include "clk-pll-v2.h"


static unsigned long ingenic_pll_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{

	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_pll_hwdesc *hwdesc = iclk->hwdesc;
	unsigned od0, m, n, od1;
	uint32_t tmp;
	uint32_t tmp2;
	uint32_t rate;

	tmp = clkhw_readl(iclk, hwdesc->regoff);
	tmp2 = clkhw_readl(iclk, hwdesc->regoff + 0x8);


	if (!(tmp & BIT(hwdesc->on_bit)))
		return 0;

	od0 = (tmp >> hwdesc->od0_sft) & GENMASK(hwdesc->od0_width - 1, 0);
	od1 = (tmp >> hwdesc->od1_sft) & GENMASK(hwdesc->od1_width - 1, 0);
	n = (tmp >> hwdesc->n_sft) & GENMASK(hwdesc->n_width - 1, 0);
	m = (tmp >> hwdesc->m_sft) & GENMASK(hwdesc->m_width - 1, 0);

	tmp2 = tmp2 * 125;
	tmp2 = tmp2 / 0x200000 + (((tmp2 % 0x200000) * 2 >= 0x200000) ? 1 : 0);

	rate = ((parent_rate / 4000 ) * m / n / od1 / od0) * 4000
		+ ((parent_rate / 4000) * tmp2 / n / od1 / od0) * 4;

	//printk("od0: %d, od1: %d, m: %d, n: %d, tmp: %x, tmp2: %x, rate:%ld\n", od0, od1, m, n, tmp, tmp2, rate);

	return rate;
}


#ifdef CONFIG_DEBUG_FS
static ssize_t pll_show_hwdesc(struct file *file, char __user *user_buf,
		size_t count, loff_t *ppos)
{
	struct ingenic_clk *iclk = file->private_data;
	struct ingenic_pll_hwdesc *hwdesc = iclk->hwdesc;
	char *buf;
	int len = 0, i;
	ssize_t ret;

#define REGS_BUFSIZE	4096
	buf = kzalloc(REGS_BUFSIZE, GFP_KERNEL);
	if (!buf)
		return 0;
	len += snprintf(buf + len, REGS_BUFSIZE - len, "%s hardware description: \n", clk_hw_get_name(&(iclk->hw)));
	len += snprintf(buf + len, REGS_BUFSIZE - len, "regbase  [0x%08x]\n", (u32)iclk->regbase);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "spinlock [%s]%p\n", iclk->spinlock?"YES":"NO", iclk->spinlock);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "regoff  [0x%03x]\n", hwdesc->regoff);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "m_off   [%d]\n", hwdesc->m_sft);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "n_off   [%d]\n", hwdesc->n_sft);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "od0_off  [%d]\n", hwdesc->od0_sft);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "od1_off  [%d]\n", hwdesc->od1_sft);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "bit_on  [%d]\n", hwdesc->on_bit);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "bit_en  [%d]\n", hwdesc->en_bit);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "bit_bs  [%d]\n", hwdesc->bs_bit);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "m_msk   [0x%08lx]\n", GENMASK(hwdesc->m_width - 1, 0) << hwdesc->m_sft);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "n_msk   [0x%08lx]\n", GENMASK(hwdesc->n_width - 1, 0) << hwdesc->n_sft);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "od0_msk  [0x%08lx]\n", GENMASK(hwdesc->od0_width - 1, 0) << hwdesc->od0_sft);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "od1_msk  [0x%08lx]\n", GENMASK(hwdesc->od1_width - 1, 0) << hwdesc->od1_sft);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "regval  [0x%08x]\n", clkhw_readl(iclk, hwdesc->regoff));
	len += snprintf(buf + len, REGS_BUFSIZE - len, "=========od encode array==========\n");
#if 0
	for (i = 0; i <= GENMASK(hwdesc->od_width - 1, 0); i++) {
		if (hwdesc->od_encode[i] > 0)
			len += snprintf(buf + len, REGS_BUFSIZE - len, "%d:[0x%02x]\n", i, hwdesc->od_encode[i]);
		else
			len += snprintf(buf + len, REGS_BUFSIZE - len, "%d:[@@@-1@@@@]\n", i);
	}
#endif

	ret =  simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);
#undef REGS_BUFSIZE
	return ret;
}

static const struct file_operations pll_debug_fops  =  {
	.open = simple_open,
	.read = pll_show_hwdesc,
	.llseek = default_llseek,
};

static int  ingenic_pll_debug_init(struct clk_hw *hw, struct dentry *dentry)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct dentry *ret;
	ret = debugfs_create_file("hwdesc", 0444, dentry, (void *)iclk, &pll_debug_fops);
	if (IS_ERR(ret) || !ret)
		return -ENODEV;
	return 0;
}
#endif

const struct clk_ops __unused ingenic_pll_v2_ro_ops = {
	.recalc_rate	= ingenic_pll_recalc_rate,
#ifdef CONFIG_DEBUG_FS
	.debug_init	= ingenic_pll_debug_init,
#endif
};
