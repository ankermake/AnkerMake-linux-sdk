
#-------------------------------------------------------
package_name = utils
package_depends =
package_module_src = drivers/utils/
package_make_hook =
package_init_hook =
package_finalize_hook = utils_finalize_hook
package_clean_hook =
#-------------------------------------------------------

define utils_finalize_hook
	$(Q)echo "insmod utils.ko" > output/utils.sh
	$(Q)cp drivers/utils/utils.ko output/
endef