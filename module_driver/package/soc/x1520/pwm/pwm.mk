
#-------------------------------------------------------
package_name = soc_pwm
package_depends = utils
package_module_src = soc/x1520/pwm
package_make_hook =
package_init_hook =
package_finalize_hook = pwm_finalize_hook
package_clean_hook =
#-------------------------------------------------------

pwm_init_file = output/soc_pwm.sh

define pwm_finalize_hook
	$(Q)cp soc/x1520/pwm/soc_pwm.ko output/
	$(Q)echo 'insmod soc_pwm.ko' > $(pwm_init_file)
endef
