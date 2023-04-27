#include <linux/module.h>
#include <gpio.h>
#include <asm/atomic.h>
#include <soc/gpio.h>

static int rtc32k_initialized = 0;
static atomic_t clk32k_refcount;
#define rtc_gpio GPIO_PB(18)
#define rtc_gpio_func GPIO_FUNC_1

static int rtc32k_init(void)
{
	atomic_set(&clk32k_refcount, 0);
	if (gpio_request(rtc_gpio, "clk32k_out_o")) {
		pr_err("[Error] request rtc32k gpio fail!\n");
		return -ENODEV;
	}
	rtc32k_initialized = 1;
        return 0;
}

void rtc32k_enable(void)
{
	if (!rtc32k_initialized)
		rtc32k_init();

	if (atomic_inc_return(&clk32k_refcount) != 1)
                return;

    pr_err("--> set rtc clk\n");
    jz_gpio_set_func(rtc_gpio, rtc_gpio_func);
}

void rtc32k_disable(void)
{
	if (!rtc32k_initialized)
		rtc32k_init();

	if (atomic_dec_return(&clk32k_refcount) != 0)
                return;

	jz_gpio_set_func(rtc_gpio, GPIO_INPUT);
}

EXPORT_SYMBOL(rtc32k_enable);
EXPORT_SYMBOL(rtc32k_disable);
