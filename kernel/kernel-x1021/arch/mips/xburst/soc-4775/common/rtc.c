#include <linux/module.h>
#include <gpio.h>
#include <asm/atomic.h>
#include <soc/gpio.h>
#include <mach/jzmmc.h>

static int clk_32k = 0;
void rtc32k_init(void)
{
}

void rtc32k_enable(void)
{
	if(!clk_32k)
		jzrtc_enable_clk32k();
	clk_32k++;
}

void rtc32k_disable(void)
{
	clk_32k--;
	if(!clk_32k)
		jzrtc_disable_clk32k();
}

EXPORT_SYMBOL(rtc32k_enable);
EXPORT_SYMBOL(rtc32k_disable);
