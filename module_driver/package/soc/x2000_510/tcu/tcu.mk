#-------------------------------------------------------
package_name = soc_tcu
package_depends = utils
package_module_src = soc/x2000_510/tcu
package_make_hook =
package_init_hook =
package_finalize_hook = tcu_finalize_hook
package_clean_hook =
#-------------------------------------------------------

tcu_init_file = output/soc_tcu.sh

define tcu_finalize_hook
	$(Q)cp soc/x2000_510/tcu/soc_tcu.ko output/
	$(Q)echo "insmod soc_tcu.ko " > $(tcu_init_file)
endef