/*
 * Copyright (C) 2016 Ingenic Semiconductor Co., Ltd.
 * Author: cli <chen.li@ingenic.com>
 *
 * The Peripherals Part Of Clock Generate Uint interface for Ingenic's SOC,
 * such as X2000, and so on. (kernel.4.4)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/regmap.h>
#include "clk-x2000-cgu-audio.h"

int ingenic_cgu_audio_set_parent(struct clk_hw *hw, u8 index)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_audio_hwdesc *hwdesc = iclk->hwdesc;
	unsigned long flags;

	if (iclk->spinlock)
		spin_lock_irqsave(iclk->spinlock, flags);
	else
		__acquire(iclk->spinlock);

	regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
			(hwdesc->cs_msk << hwdesc->cs_off),
			index << hwdesc->cs_off,
			NULL);

	if (iclk->spinlock)
		spin_unlock_irqrestore(iclk->spinlock, flags);
	else
		__release(iclk->spinlock);
	return 0;
}

u8 ingenic_cgu_audio_get_parent(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_audio_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t xcdr;

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);

	return (xcdr >> hwdesc->cs_off) & hwdesc->cs_msk;
}

unsigned long ingenic_cgu_audio_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_audio_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t xcdr;
	uint32_t m,n;
	uint32_t rate;

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);
	m = (xcdr >> hwdesc->m_off) & hwdesc->m_msk;
	n = (xcdr >> hwdesc->n_off) & hwdesc->n_msk;
	rate = div_u64((u64)parent_rate*m, n);

	return rate;
}

/* 2<=N<=1048575 20bits */
/* 1<=M<=511 9bits */
/* N >= 2*M */
/* rate = parent_rate*m/n */
static uint32_t calculate_rate(uint32_t in, uint32_t out, uint32_t *M, uint32_t *N)
{
	unsigned long long n,m;
	unsigned long long tmp;
	unsigned long long saven, savem;
	uint32_t min;

	savem = 0;
	saven = 0;
	if((in%out == 0) && ((saven=in/out) > 1)) {
		savem = 1;
		goto end;
	}

	min = out;
	for(m = 511; m >=1; m--) {
		n = in*m;
		tmp = do_div(n, out);
		if(n < 2*m || n >1048575)
			continue;

		if(tmp == 0) {
			saven = n;
			savem = m;
			break;
		}
		if(min > tmp) {
			saven = n;
			savem = m;
			min = tmp;
		}
	}

end:
	*N = saven;
	*M = savem;

	tmp = in*savem;
	do_div(tmp, saven);
	return tmp;
}

int ingenic_cgu_audio_determine_rate(struct clk_hw *hw,
		struct clk_rate_request *req)
{
	uint32_t m, n;

	req->rate = calculate_rate(req->best_parent_rate, req->rate, &m, &n);

	return 0;
}

int ingenic_cgu_audio_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_audio_hwdesc *hwdesc = iclk->hwdesc;
	unsigned long flags;
	uint32_t c_rate;
	uint32_t m, n;

	if(parent_rate < 2*rate) {
		printk("audio source clk is must double target!, src: %ld, tar: %ld\n", parent_rate, rate);
		return -EINVAL;
	}
	c_rate = calculate_rate(parent_rate, rate, &m, &n);

	if (iclk->spinlock)
		spin_lock_irqsave(iclk->spinlock, flags);
	else
		__acquire(iclk->spinlock);

	regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
			(hwdesc->m_msk << hwdesc->m_off | hwdesc->n_msk << hwdesc->n_off),
			(m << hwdesc->m_off | n << hwdesc->n_off),
			NULL);

	if (iclk->spinlock)
		spin_unlock_irqrestore(iclk->spinlock, flags);
	else
		__release(iclk->spinlock);
	return 0;
}

int ingenic_cgu_audio_enable(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_audio_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t xcdr;
	unsigned long flags;

	if (iclk->spinlock)
		spin_lock_irqsave(iclk->spinlock, flags);
	else
		__acquire(iclk->spinlock);


	regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);

	/*has already enabled*/
	if (xcdr & BIT(hwdesc->bit_ce))
		goto out;

	regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
			BIT(hwdesc->bit_ce), BIT(hwdesc->bit_ce),
			NULL);
out:
	if (iclk->spinlock)
		spin_unlock_irqrestore(iclk->spinlock, flags);
	else
		__release(iclk->spinlock);
	return 0;
}

void ingenic_cgu_audio_disable(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_audio_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t xcdr;
	unsigned long flags;

	if (iclk->spinlock)
		spin_lock_irqsave(iclk->spinlock, flags);
	else
		__acquire(iclk->spinlock);

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);

	/*has already disabled*/
	if (!(xcdr & BIT(hwdesc->bit_ce)))
		goto out;

	regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
			BIT(hwdesc->bit_ce), 0,
			NULL);
out:
	if (iclk->spinlock)
		spin_unlock_irqrestore(iclk->spinlock, flags);
	else
		__release(iclk->spinlock);

	return;
}

int ingenic_cgu_audio_is_enabled(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cgu_audio_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t xcdr;

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &xcdr);
	return (int)(xcdr & BIT(hwdesc->bit_ce));
}


#ifdef CONFIG_INGENIC_CLK_DEBUG_FS
static ssize_t cgu_show_hwdesc(struct file *file, char __user *user_buf,
		size_t count, loff_t *ppos)
{
	struct ingenic_clk *iclk = file->private_data;
	struct ingenic_cgu_audio_hwdesc *hwdesc = iclk->hwdesc;
	char *buf;
	int len = 0;
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
	len += snprintf(buf + len, REGS_BUFSIZE - len, "m_msk	[0x%08x]\n", hwdesc->m_msk << hwdesc->m_off);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "n_msk	[0x%08x]\n", hwdesc->n_msk << hwdesc->n_off);
	len += snprintf(buf + len, REGS_BUFSIZE - len, "regval  [0x%08x]\n", clkhw_readl(iclk, hwdesc->regoff));
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

int  ingenic_cgu_audio_debug_init(struct clk_hw *hw, struct dentry *dentry)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct dentry *ret;

	ret = debugfs_create_file("hwdesc", 0444, dentry, (void *)iclk, &cgu_debug_fops);

	if (IS_ERR(ret) || !ret)
		return -ENODEV;
	return 0;
}
#endif

const struct clk_ops __unused ingenic_cgu_i2s_ops = {
	.set_parent = ingenic_cgu_audio_set_parent,
	.get_parent = ingenic_cgu_audio_get_parent,
	.enable = ingenic_cgu_audio_enable,
	.disable = ingenic_cgu_audio_disable,
	.is_enabled = ingenic_cgu_audio_is_enabled,
	.recalc_rate = ingenic_cgu_audio_recalc_rate,
	.determine_rate = ingenic_cgu_audio_determine_rate,
	.set_rate = ingenic_cgu_audio_set_rate,
#ifdef CONFIG_INGENIC_CLK_DEBUG_FS
	.debug_init	= ingenic_cgu_audio_debug_init,
#endif
};

const struct clk_ops __unused ingenic_cgu_audio_ops = {
	.set_parent = ingenic_cgu_audio_set_parent,
	.get_parent = ingenic_cgu_audio_get_parent,
#ifdef CONFIG_INGENIC_CLK_DEBUG_FS
	.debug_init	= ingenic_cgu_audio_debug_init,
#endif
};
