/*
 * drivers/regulator/ricoh619-regulator.c
 *
 * Regulator driver for RICOH R5T619 power management chip.
 *
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 * Author: cljiang <conglong.jiang@ingenic.com>
 *
 * Based on code
 * drivers/regulator/rn5t618-regulator.c
 * drivers/regulator/rn5t567-regulator.c
 *
 *	Copyright (C) 2011 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/ricoh619.h>
#include <linux/regulator/ricoh619-regulator.h>
#include <linux/regulator/of_regulator.h>


static const struct regulator_ops ricoh619_ops = {
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
};

#ifdef CONFIG_REGULATOR_USERSPACE_CONSUMER
static void of_populate_userspace_consumers(struct device *dev)
{
	struct device_node *child = NULL;
	struct device_node *parent = NULL;
	struct platform_device *pdev = NULL;
	struct regulator_userspace_consumer_data pdata;
	static int id = 0;

	parent = of_get_child_by_name(dev->parent->of_node, REGULATOR_NODE_NAME);

	for_each_child_of_node(parent, child) {
		int supplies = 0, index = 0;
		if (!child->type || of_node_cmp(child->type, "reg-userspace-consumer"))
			continue;

		supplies = of_property_count_strings(child, "supplies-name");
		if (supplies <= 0)
			continue;

		pdata.num_supplies = supplies;
		pdata.supplies = devm_kzalloc(dev, supplies * sizeof(struct regulator_bulk_data),
				GFP_KERNEL);
		if (!pdata.supplies)
			continue;

		if (of_property_read_string(child, "bulk-name", &(pdata.name)))
			continue;

		for (index = 0; index < supplies; index++)
			of_property_read_string_index(child, "supplies-name", index,
					&(pdata.supplies[index].supply));

		pdata.init_on = of_property_read_bool(child, "init_on");

		pdev = platform_device_register_data(dev, "reg-userspace-consumer", id++, &pdata,
				sizeof(struct regulator_userspace_consumer_data));
		WARN(IS_ERR(pdev), "%s(%s) register failed: %ld\n", pdata.name,
				pdata.supplies[0].supply, PTR_ERR(pdev));
	}
}
#endif


static int of_parse_rn5t619_cb(struct device_node *nd,
			const struct regulator_desc * desc,
			    struct regulator_config *config) {
	uint32_t pval;
	int sel, ret = 0, us;
	int svsel_reg[RICOH619_ID_NUM] = {
		[RICOH619_ID_DC1] = RICOH619_DC1DAC_SLP,
		[RICOH619_ID_DC2] = RICOH619_DC2DAC_SLP,
		[RICOH619_ID_DC3] = RICOH619_DC3DAC_SLP,
		[RICOH619_ID_DC4] = RICOH619_DC4DAC_SLP,
		[RICOH619_ID_DC5] = RICOH619_DC5DAC_SLP,
		[RICOH619_ID_LDO1] = RICOH619_LDO1DAC_SLP,
		[RICOH619_ID_LDO2] = RICOH619_LDO2DAC_SLP,
		[RICOH619_ID_LDO3] = RICOH619_LDO3DAC_SLP,
		[RICOH619_ID_LDO4] = RICOH619_LDO4DAC_SLP,
		[RICOH619_ID_LDO5] = RICOH619_LDO5DAC_SLP,
		[RICOH619_ID_LDO6] = RICOH619_LDO6DAC_SLP,
		[RICOH619_ID_LDO7] = RICOH619_LDO7DAC_SLP,
        [RICOH619_ID_LDO8] = RICOH619_LDO8DAC_SLP,
        [RICOH619_ID_LDO9] = RICOH619_LDO9DAC_SLP,
        [RICOH619_ID_LDO10] = RICOH619_LDO10DAC_SLP,
        [RICOH619_ID_LDORTC1] = -1,
        [RICOH619_ID_LDORTC2] = -1,
	};

    int slot_reg[RICOH619_ID_NUM] = {
		[RICOH619_ID_DC1] = RICOH619_DC1_SLOT,  
		[RICOH619_ID_DC2] = RICOH619_DC2_SLOT, 
		[RICOH619_ID_DC3] = RICOH619_DC3_SLOT, 
		[RICOH619_ID_DC4] = RICOH619_DC4_SLOT, 
		[RICOH619_ID_DC5] = RICOH619_DC5_SLOT,  
		[RICOH619_ID_LDO1] = RICOH619_LDO1_SLOT,
		[RICOH619_ID_LDO2] = RICOH619_LDO2_SLOT, 
		[RICOH619_ID_LDO3] = RICOH619_LDO3_SLOT,
		[RICOH619_ID_LDO4] = RICOH619_LDO4_SLOT,
		[RICOH619_ID_LDO5] = RICOH619_LDO5_SLOT,
		[RICOH619_ID_LDO6] = RICOH619_LDO6_SLOT,
		[RICOH619_ID_LDO7] = RICOH619_LDO7_SLOT,
        [RICOH619_ID_LDO8] = RICOH619_LDO8_SLOT,
        [RICOH619_ID_LDO9] = RICOH619_LDO9_SLOT,
        [RICOH619_ID_LDO10] = RICOH619_LDO10_SLOT,
    };


	if (!of_property_read_u32(nd, "init_uV" , &pval)) {
		sel = (pval -  desc->min_uV) / desc->uV_step;
		if (desc->id < RICOH619_ID_LDO1)
			sel <<= ffs(desc->vsel_mask) - 1;
		else
			sel <<= ffs(desc->vsel_mask) - 1;
		ret = regmap_update_bits(config->regmap, desc->vsel_reg,
				  desc->vsel_mask, sel);
		if (ret)
			goto out;
		us = (sel - 1) * desc->uV_step / desc->ramp_delay;
		udelay(us);
	}

	if (!of_property_read_u32(nd, "sleep_uV", &pval) && svsel_reg[desc->id] != -1) {
		sel = (pval -  desc->min_uV) / desc->uV_step;
		sel <<= ffs(desc->vsel_mask) - 1;
		ret = regmap_update_bits(config->regmap, svsel_reg[desc->id],
				desc->vsel_mask, sel);
		if (ret)
			goto out;
	}

	if (!of_property_read_u32(nd, "poweron-slot", &pval)) {
		ret = regmap_update_bits(config->regmap, slot_reg[desc->id], 0xf0, pval);
		if (ret)
			goto out;
	}

	if (!of_property_read_u32(nd, "sleep-slot", &pval)) {
		ret = regmap_update_bits(config->regmap, slot_reg[desc->id], 0xf, pval);
		if (ret)
			goto out;
	}
out:
	return ret;
}


#define REGULATOR_NODE_NAME	"regulators"
#define DESC(_rid, _ereg, _emsk, _min_uv, _max_uv, _uV_step, _vsreg, _vsmsk, _ramp_delay, _enable_time)	\
	[RICOH619_##_rid] = {	\
		.name		= #_rid,	\
		.of_match	= of_match_ptr(#_rid),	\
		.regulators_node = of_match_ptr(REGULATOR_NODE_NAME),	\
		.of_parse_cb	=  of_parse_rn5t619_cb,	\
		.id			= RICOH619_##_rid,	\
		.n_voltages	= ((_max_uv) - (_min_uv)) / (_uV_step) + 1,	\
		.ops		= &ricoh619_ops, \
		.type = REGULATOR_VOLTAGE,	\
		.owner		= THIS_MODULE,	\
		.min_uV		= _min_uv,	\
		.uV_step	= _uV_step, \
		.linear_min_sel = 0, \
		.ramp_delay	= _ramp_delay, \
		.vsel_reg	= _vsreg,	\
		.vsel_mask	= _vsmsk, \
		.enable_reg = _ereg,	\
		.enable_mask = _emsk,	\
		.enable_time = _enable_time, \
	}

#define LDO_DESC(_rid, _ereg, _emsk, _min_uv, _max_uv, _uV_step, _vsreg, _vsmsk, _enable_time)  \
	DESC(_rid, _ereg, _emsk, _min_uv, _max_uv, _uV_step, _vsreg, _vsmsk, 0, _enable_time)
static struct regulator_desc ricoh619_regulator_desc[] = {
	DESC(DCDC1, RICOH619_DC1CTL, BIT(0), 600000, 3500000, 12500, RICOH619_DC1DAC, 0xff, 14000, 500), //_vsmsk is the valid of RICOH619_DC1DAC bit

	DESC(DCDC2, RICOH619_DC2CTL, BIT(0), 600000, 3500000, 12500, RICOH619_DC2DAC, 0xff, 14000, 500),

	DESC(DCDC3, RICOH619_DC3CTL, BIT(0), 600000, 3500000, 12500, RICOH619_DC3DAC, 0xff, 14000, 500),

	DESC(DCDC4, RICOH619_DC4CTL, BIT(0), 600000, 3500000, 12500, RICOH619_DC4DAC, 0xff, 14000, 500),

	DESC(DCDC5, RICOH619_DC5CTL, BIT(0), 600000, 3500000, 12500, RICOH619_DC5DAC, 0xff, 14000, 500),

	LDO_DESC(LDO1, RICOH619_LDOEN1, BIT(0), 900000, 3500000, 25000, RICOH619_LDO1DAC, 0xff, 500),

	LDO_DESC(LDO2, RICOH619_LDOEN1, BIT(1), 900000, 3500000, 25000, RICOH619_LDO2DAC, 0xff, 500),

	LDO_DESC(LDO3, RICOH619_LDOEN1, BIT(2), 900000, 3500000, 25000, RICOH619_LDO3DAC, 0xff, 500),

	LDO_DESC(LDO4, RICOH619_LDOEN1, BIT(3), 900000, 3500000, 25000, RICOH619_LDO4DAC, 0xff, 500),

	LDO_DESC(LDO5, RICOH619_LDOEN1, BIT(4), 600000, 3500000, 25000, RICOH619_LDO5DAC, 0xff, 500),

	LDO_DESC(LDO6, RICOH619_LDOEN1, BIT(5), 600000, 3500000, 25000, RICOH619_LDO6DAC, 0xff, 500),

	LDO_DESC(LDO7, RICOH619_LDOEN1, BIT(6), 900000, 3500000, 25000, RICOH619_LDO7DAC, 0xff, 500),

	LDO_DESC(LDO8, RICOH619_LDOEN1, BIT(7), 900000, 3500000, 25000, RICOH619_LDO8DAC, 0xff, 500),

	LDO_DESC(LDO9, RICOH619_LDOEN2, BIT(0), 900000, 3500000, 25000, RICOH619_LDO9DAC, 0xff, 500),

	LDO_DESC(LDO10, RICOH619_LDOEN2, BIT(1), 900000, 3500000, 25000, RICOH619_LDO10DAC, 0xff, 500),

	LDO_DESC(LDORTC1, RICOH619_LDOEN2, BIT(4), 1700000, 3500000, 25000, RICOH619_LDORTC1DAC, 0xff, 500),

	LDO_DESC(LDORTC2, RICOH619_LDOEN2, BIT(5), 900000, 3500000, 25000, RICOH619_LDORTC2DAC, 0xff, 500),
};

static struct regulator_config ricoh619_regulator_config;

static int ricoh619_regulator_probe(struct platform_device *pdev)
{
	struct regmap *regmap = dev_get_drvdata(pdev->dev.parent);
	struct regulator_dev *rdev = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(ricoh619_regulator_desc); i++) {
		ricoh619_regulator_config.dev = pdev->dev.parent;
		ricoh619_regulator_config.regmap = regmap;

		rdev = devm_regulator_register(&pdev->dev, &ricoh619_regulator_desc[i], &ricoh619_regulator_config);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "failed to register %s regulator (errno %ld\n)\n",
					ricoh619_regulator_desc[i].name, PTR_ERR(rdev));
			return PTR_ERR(rdev);
		}
	}

#ifdef CONFIG_REGULATOR_USERSPACE_CONSUMER
    of_populate_userspace_consumers(&pdev->dev);
#endif
	return 0;
}


static struct platform_driver ricoh619_regulator_driver = {
	.driver	= {
		.name	= "ricoh619-regulator",
	},
	.probe		= ricoh619_regulator_probe,
};

static int __init ricoh619_regulator_init(void)
{
	return platform_driver_register(&ricoh619_regulator_driver);
}
subsys_initcall(ricoh619_regulator_init);

static void __exit ricoh619_regulator_exit(void)
{
	platform_driver_unregister(&ricoh619_regulator_driver);
}
module_exit(ricoh619_regulator_exit);

MODULE_DESCRIPTION("RICOH R5T619 regulator driver");
MODULE_ALIAS("platform:ricoh619-regulator");
MODULE_LICENSE("GPL");
