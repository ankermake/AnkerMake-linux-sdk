#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <mach/jztcu.h>
#include <soc/base.h>
#include <soc/cpm.h>
#include <linux/jz-pwm-rtc-bypass.h>

#include <board.h>

void *jz_pwm_request_handle(int pwm_id);

int jz_pwm_enable_rtc_bypass_mode(void *handle);

static int im501_init_rtc_clk(void)
{
    void *handle = jz_pwm_request_handle(IM501_RTC_CLK_PWM_ID);

    if (handle)
        jz_pwm_enable_rtc_bypass_mode(handle);

    return 0;
}

module_init(im501_init_rtc_clk);