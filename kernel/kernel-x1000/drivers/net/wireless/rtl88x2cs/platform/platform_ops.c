/******************************************************************************
 *
 * Copyright(c) 2013 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#ifndef CONFIG_PLATFORM_OPS
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/jzmmc.h>

extern int jzmmc_manual_detect(int index, int on);
/*
 * Return:
 *    0:    power on successfully
 *    others: power on failed
 */
int platform_wifi_power_on(void)
{
#if 0
    jzmmc_manual_detect(1, 1);
#endif

    return 0;
}

void platform_wifi_power_off(void)
{
}
#endif /* !CONFIG_PLATFORM_OPS */
