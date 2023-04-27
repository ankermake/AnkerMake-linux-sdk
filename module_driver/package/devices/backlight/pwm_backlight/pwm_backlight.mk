#-------------------------------------------------------
package_name = pwm_backlight
package_depends = utils soc_pwm
package_module_src = devices/backlight/pwm_backlight/
package_make_hook =
package_init_hook =
package_finalize_hook = pwm_backlight_finalize_hook
package_clean_hook =
#-------------------------------------------------------

pwm_backlight_init_file = output/pwm_backlight.sh

TARGET_INSTALL_HOOKS +=
TARGET_INSTALL_CLEAN_HOOKS +=

define pwm_backlight_finalize_hook
	$(Q)cp devices/backlight/pwm_backlight/pwm_backlight.ko output/
	$(Q)echo '	insmod pwm_backlight.ko \' > $(pwm_backlight_init_file)
	$(Q)echo -n '	backlight_dev_name=$(MD_PWM_BACKLIGHT_DEV_NAME) ' >> $(pwm_backlight_init_file)
	$(Q)echo -n 'default_brightness=$(MD_PWM_BACKLIGHT_DEFAULT_BRIGHTNESS) ' >> $(pwm_backlight_init_file)
	$(Q)echo 'max_brightness=$(MD_PWM_BACKLIGHT0_MAX_BRIGHTNESS) \' >> $(pwm_backlight_init_file)
	$(Q)echo -n '	pwm_gpio=$(MD_PWM_BACKLIGHT_GPIO) ' >> $(pwm_backlight_init_file)
	$(Q)echo -n 'pwm_active_level=$(MD_PWM_BACKLIGHT_ACTIVE_LEVEL) ' >> $(pwm_backlight_init_file)
	$(Q)echo 'power_gpio=$(MD_PWM_BACKLIGHT_POWER_GPIO) \' >> $(pwm_backlight_init_file)
	$(Q)echo -n '	power_gpio_vaild_level=$(MD_PWM_BACKLIGHT_POWER_GPIO_VAILD_LEVEL) ' >> $(pwm_backlight_init_file)
	$(Q)echo -n 'pwm_freq=$(MD_PWM_BACKLIGHT0_FREQ) ' >> $(pwm_backlight_init_file)
	$(Q)echo  >> $(pwm_backlight_init_file)
endef
