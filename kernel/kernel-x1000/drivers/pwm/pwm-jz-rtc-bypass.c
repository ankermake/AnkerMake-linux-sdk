#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <mach/jztcu.h>
#include <soc/base.h>
#include <soc/cpm.h>

#include <linux/jz-pwm-rtc-bypass.h>


void *jz_pwm_request_handle(int pwm_id)
{
    struct tcu_device *tcu = tcu_request(pwm_id);
    if (IS_ERR(tcu))
        return NULL;

    return tcu;
}

void jz_pwm_free_handle(void *handle)
{
    struct tcu_device *tcu = handle;

    tcu_free(tcu);
}

int jz_pwm_enable_rtc_bypass_mode(void *handle)
{
    struct tcu_device *tcu = handle;

    cpm_outl(cpm_inl(CPM_OPCR) | OPCR_ERCS, CPM_OPCR);

    tcu->clock = RTC_EN;
	tcu->pwm_flag = 1;
	tcu->count_value = 0;
	tcu->pwm_shutdown = 1;

    tcu->full_num = 3;
    tcu->half_num = 1;
    tcu->divi_ratio = 0;
    tcu->rtc_bypass_mode = 1;

	if(tcu_as_timer_init(tcu) != 0)
        return -1;

    if(tcu_as_timer_config(tcu) != 0)
        return -1;

    return 0;
}

void jz_pwm_disable_rtc_bypass_mode(void *handle)
{
    struct tcu_device *tcu = handle;

    tcu_disable(tcu);
}
