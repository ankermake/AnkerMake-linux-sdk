#-------------------------------------------------------
package_name = pwm_battery
package_depends = utils soc_pwm
package_module_src = devices/pwm_battery/
package_make_hook =
package_init_hook =
package_finalize_hook = pwm_battery_finalize_hook
package_clean_hook =
#-------------------------------------------------------

pwm_battery_init_file = output/pwm_battery.sh

define pwm_battery_finalize_hook
	$(Q)cp devices/pwm_battery/pwm_battery.ko output/
	$(Q)echo -n 'insmod pwm_battery.ko ' > $(pwm_battery_init_file)
	$(Q)echo -n ' pwm_gpio=$(MD_PWM_BATTERY_PWM_GPIO)' >> $(pwm_battery_init_file)
	$(Q)echo -n ' status_gpio=$(MD_PWM_BATTERY_STATUS_GPIO)'  >> $(pwm_battery_init_file)
	$(Q)echo -n ' status_active=$(MD_PWM_BATTERY_STATUS_GPIO_ACTIVE)' >> $(pwm_battery_init_file)
	$(Q)echo -n ' power_gpio=$(MD_PWM_BATTERY_POWER_GPIO)' >> $(pwm_battery_init_file)
	$(Q)echo -n ' power_active=$(MD_PWM_BATTERY_POWER_GPIO_ACTIVE)' >> $(pwm_battery_init_file)
	$(Q)echo -n ' pwm_max_level=$(MD_PWM_BATTERY_PWM_MAX_LEVEL)' >> $(pwm_battery_init_file)
	$(Q)echo -n ' max_pwm_voltage=$(MD_PWM_BATTERY_PWM_MAX_VOLTAGE)' >> $(pwm_battery_init_file)
	$(Q)echo -n ' res1_value=$(MD_PWM_BATTERY_RES1_VALUE)' >> $(pwm_battery_init_file)
	$(Q)echo -n ' res2_value=$(MD_PWM_BATTERY_RES2_VALUE)' >> $(pwm_battery_init_file)
	$(Q)echo -n ' deviation=$(MD_PWM_BATTERY_DEVIATION)' >> $(pwm_battery_init_file)
	$(Q)echo >> $(pwm_battery_init_file)
endef