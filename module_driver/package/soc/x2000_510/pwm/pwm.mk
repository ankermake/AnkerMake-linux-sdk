
#-------------------------------------------------------
package_name = soc_pwm
package_depends = utils
package_module_src = soc/x2000_510/pwm
package_make_hook =
package_init_hook =
package_finalize_hook = soc_pwm_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_pwm_init_file = output/soc_pwm.sh

define soc_pwm_finalize_hook
	$(Q)cp soc/x2000_510/pwm/soc_pwm.ko output/
	$(Q)echo 'insmod soc_pwm.ko ' > $(soc_pwm_init_file)
endef
