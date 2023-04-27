/*
 * Copyright (C) 2016 Ingenic Semiconductor Co., Ltd.
 * Author: cli <chen.li@ingenic.com>
 *
 * The CPU Part Of Clock Generate Uint interface for Ingenic's SOC,
 * such as X1000, and so on. (kernel.4.4)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/regmap.h>
#include "clk-cpccr.h"

static u8 ingenic_cpccr_get_parent(struct clk_hw *hw)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cpccr_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t cpccr = 0;

	regmap_read(iclk->pm_regmap, hwdesc->ctrloff, &cpccr);

	return  (cpccr >> hwdesc->cs_off) & hwdesc->cs_msk;
}

static unsigned long ingenic_cpccr_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct ingenic_clk *iclk = to_ingenic_clk(hw);
	struct ingenic_cpccr_hwdesc *hwdesc = iclk->hwdesc;
	uint32_t cpccr;
	int div;

	regmap_read(iclk->pm_regmap, hwdesc->ctrloff, &cpccr);

	div = ((cpccr >> hwdesc->div_off) & hwdesc->div_msk) + 1;

	return  parent_rate / div;
}

const struct clk_ops ingenic_cpccr_mux_ro_ops = {
	.get_parent = ingenic_cpccr_get_parent,
};

const struct clk_ops ingenic_cpccr_rate_ro_ops = {
	.recalc_rate = ingenic_cpccr_recalc_rate,
};
