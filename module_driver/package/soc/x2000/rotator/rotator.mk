
#-------------------------------------------------------
package_name = soc_rotator
package_depends = utils
package_module_src = soc/x2000/rotator/
package_make_hook =
package_init_hook =
package_finalize_hook = rotator_finalize_hook
package_clean_hook =
#-------------------------------------------------------

rotator_init_file = output/soc_rotator.sh

define rotator_finalize_hook

	$(Q)cp soc/x2000/rotator/soc_rotator.ko output/
	$(Q)echo    "insmod soc_rotator.ko " > $(rotator_init_file)

endef