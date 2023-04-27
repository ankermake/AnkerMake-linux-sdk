#-------------------------------------------------------
package_name = pwm_audio
package_depends = utils soc_pwm
package_module_src = drivers/pwm_audio
package_make_hook =
package_init_hook =
package_finalize_hook = pwm_audio_finalize_hook
package_clean_hook =
#-------------------------------------------------------

pwm_audio_init_file = output/pwm_audio.sh

define pwm_audio_finalize_hook
	$(Q)cp drivers/pwm_audio/pwm_audio.ko output/
	$(Q)echo -n 'insmod pwm_audio.ko ' >> $(pwm_audio_init_file)

	$(Q)echo -n "	pwm_gpio_1=$(MD_PWM_AUDIO_PWM_GPIO_1) " >> $(pwm_audio_init_file)
	$(Q)echo -n "	amp_power_gpio_1=$(MD_PWM_AUDIO_AMP_POWER_GPIO_1) " >> $(pwm_audio_init_file)
	$(Q)echo -n "	pwm_gpio_2=$(MD_PWM_AUDIO_PWM_GPIO_2) " >> $(pwm_audio_init_file)
	$(Q)echo -n "	amp_power_gpio_2=$(MD_PWM_AUDIO_AMP_POWER_GPIO_2) " >> $(pwm_audio_init_file)

	$(Q)echo >> $(pwm_audio_init_file)
endef
