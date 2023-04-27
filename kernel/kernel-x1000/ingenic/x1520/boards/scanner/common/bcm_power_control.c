#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mach/jzmmc.h>
#include <linux/bcm_pm_core.h>
#include <linux/delay.h>

#include "board_base.h"


static void enable_clk32k(void)
{
    pwm_enable_clk32k();
}

static void disable_clk32k(void)
{
    pwm_disable_clk32k();
}

static struct bcm_power_platform_data bcm_power_platform_data = {
    .wlan_pwr_en        = BCM_PWR_EN,
    .wlan_pwr_en_level  = BCM_PWR_EN_LEVEL,
    .wlan_pwr_en_level  = BCM_PWR_EN_LEVEL,
    .clk_enable         = enable_clk32k,
    .clk_disable        = disable_clk32k,
};

struct platform_device	bcm_power_platform_device = {
    .name               = "bcm_power",
    .id                 = -1,
    .num_resources      = 0,
    .dev = {
        .platform_data = &bcm_power_platform_data,
    },
};


/*
 * RTC 32K for bt-bluesleep
 */
void rtc32k_enable(void)
{
    enable_clk32k();
}


void rtc32k_disable(void)
{
    disable_clk32k();
}

EXPORT_SYMBOL(rtc32k_enable);
EXPORT_SYMBOL(rtc32k_disable);
