#-------------------------------------------------------
package_name = soc_watchdog
package_depends = utils
package_module_src = soc/x1600/watchdog
package_make_hook =
package_init_hook =
package_finalize_hook = watchdog_finalize_hook
package_clean_hook =
#-------------------------------------------------------

watchdog_init_file = output/soc_watchdog.sh

define watchdog_finalize_hook
	$(Q)cp soc/x1600/watchdog/soc_watchdog.ko output/
	$(Q)echo 'insmod soc_watchdog.ko ' > $(watchdog_init_file)
endef