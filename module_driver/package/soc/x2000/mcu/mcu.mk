#-------------------------------------------------------
package_name = soc_mcu
package_depends = utils
package_module_src = soc/x2000/mcu
package_make_hook =
package_init_hook =
package_finalize_hook = mcu_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_mcu_init_file = output/soc_mcu.sh

define mcu_finalize_hook
	$(Q)cp soc/x2000/mcu/soc_mcu.ko output/
	$(Q)echo -n 'insmod soc_mcu.ko ' > $(soc_mcu_init_file)
	$(Q)echo >> $(soc_mcu_init_file)
endef