/*
 * Copyright (C) 2016 Ingenic Semiconductor Co., Ltd.
 * Author: cli <chen.li@ingenic.com>
 *
 * The rtc clock for Ingenic's SOC,
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

#include "clk-rtc.h"

static int ingenic_rtc_set_parent(struct clk_hw *hw, u8 index)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_rtc_hwdesc *hwdesc = iclk->hwdesc;
	unsigned long flags;

	CLK_LOCK(iclk, flags);

	regmap_update_bits_check(iclk->pm_regmap, hwdesc->regoff,
			BIT(hwdesc->bit_cs),
			index ? BIT(hwdesc->bit_cs) : 0,
			NULL);

	CLK_UNLOCK(iclk, flags);
	return 0;
}

static u8 ingenic_rtc_get_parent(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_rtc_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t rtc;

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &rtc);

	return ((rtc & BIT(hwdesc->bit_cs)) >> hwdesc->bit_cs);
}

static unsigned long ingenic_rtc_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_rtc_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t rtc;

	regmap_read(iclk->pm_regmap, hwdesc->regoff, &rtc);

	if (rtc & BIT(hwdesc->bit_cs))
		return parent_rate;
	else
		return DIV_ROUND_UP(parent_rate, hwdesc->div);
}

const struct clk_ops __unused ingenic_rtc_ops = {
	.set_parent = ingenic_rtc_set_parent,
	.get_parent = ingenic_rtc_get_parent,
	.recalc_rate = ingenic_rtc_recalc_rate,
};
