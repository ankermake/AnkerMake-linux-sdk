#ifndef _JZ_PWM_RTC_BYPASS_H_
#define _JZ_PWM_RTC_BYPASS_H_

void *jz_pwm_request_handle(int pwm_id);

void jz_pwm_free_handle(void *handle);

int jz_pwm_enable_rtc_bypass_mode(void *handle);

void jz_pwm_disable_rtc_bypass_mode(void *handle);

#endif /* _JZ_PWM_RTC_BYPASS_H_ */