/*
 * Base driver for X-Powers AXP
 *
 * Copyright (C) 2013 X-Powers, Ltd.
 *  Zhang Donglu <zhangdonglu@x-powers.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
//#include <mach/irqs.h>
#include <linux/power_supply.h>
#include <linux/apm_bios.h>
#include <linux/apm-emulation.h>
#include <linux/module.h>

#include "axp-cfg.h"
#include "axp-mfd.h"

struct regulator_init_data axp216_regl_init_data[] = {
    [rtcldo] = {
        .constraints = {
            .name = "RTC",
            .min_uV =  AXP_LDO1_MIN,
            .max_uV =  AXP_LDO1_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 1,
            .apply_uV = 1,
        },
    },

    [aldo1] = {
        .constraints = {
            .name = "ALDO1",
            .min_uV = AXP_ALDO1_MIN,
            .max_uV = AXP_ALDO1_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 1,
            .apply_uV = 1,
        },
    },

    [aldo2] = {
        .constraints = {
            .name = "ALDO2",
            .min_uV =  AXP_ALDO2_MIN,
            .max_uV =  AXP_ALDO2_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 1,
            .apply_uV = 1,
        },
    },

    [aldo3] = {
        .constraints = {
            .name = "ALDO3",
            .min_uV = AXP_ALDO3_MIN,
            .max_uV = AXP_ALDO3_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 1,
            .apply_uV = 1,
        },
    },

    [eldo1] = {
        .constraints = {
            .name = "ELDO1",
            .min_uV = AXP_ELDO1_MIN,
            .max_uV = AXP_ELDO1_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 1,
            .apply_uV = 1,
        },
    },

    [eldo2] = {
        .constraints = {
            .name = "ELDO2",
            .min_uV = AXP_ELDO2_MIN,
            .max_uV = AXP_ELDO2_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 0,
            .apply_uV = 1,
        },
    },

    [dcdc1] = {
        .constraints = {
            .name = "DCDC1",
            .min_uV = AXP_DCDC1_MIN,
            .max_uV = AXP_DCDC1_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 1,
            .apply_uV = 1,
        },
    },

    [dcdc2] = {
        .constraints = {
            .name = "DCDC2",
            .min_uV = AXP_DCDC2_MIN,
            .max_uV = AXP_DCDC2_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 1,
            .apply_uV = 1,
        },
    },

    [dcdc3] = {
        .constraints = {
            .name = "DCDC3",
            .min_uV = AXP_DCDC3_MIN,
            .max_uV = AXP_DCDC3_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 1,
            .apply_uV = 1,
        },
    },

    [dcdc4] = {
        .constraints = {
            .name = "DCDC4",
            .min_uV = AXP_DCDC4_MIN,
            .max_uV = AXP_DCDC4_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 1,
            .apply_uV = 0,
        },
    },

    [dcdc5] = {
        .constraints = {
            .name = "DCDC5",
            .min_uV = AXP_DCDC5_MIN,
            .max_uV = AXP_DCDC5_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 1,
            .apply_uV = 1,
        },
    },

    [gpioldo1] = {
        .constraints = {
            .name = "LDOIO1",
            .min_uV = GPIO_LDO1_MIN,
            .max_uV = GPIO_LDO1_MAX,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .boot_on = 1,
            .apply_uV = 1,
        },
    },

};

static struct power_supply_info axp216_battery_data ={
        .name ="PTI PL336078",
        .technology = POWER_SUPPLY_TECHNOLOGY_LION,
        .voltage_max_design = CHGVOL,
        .voltage_min_design = SHUTDOWNVOL,
        .energy_full_design = BATCAP,
        .use_for_apm = 1,
};

struct axp_supply_init_data axp216_sply_init_data = {
    .battery_info = &axp216_battery_data,
    .chgcur = STACHGCUR,
    .chgvol = CHGVOL,
    .chgend = ENDCHGRATE,
    .chgen = CHGEN,
    .sample_time = ADCFREQ,
    .chgpretime = CHGPRETIME,
    .chgcsttime = CHGCSTTIME,
};
