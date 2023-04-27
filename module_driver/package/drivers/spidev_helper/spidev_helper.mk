#-------------------------------------------------------
package_name = spidev_helper
package_depends = utils spidev
package_module_src = drivers/spidev_helper
package_make_hook =
package_init_hook =
package_finalize_hook = spidev_helper_finalize_hook
package_clean_hook =
#-------------------------------------------------------

spidev_helper_init_file = output/spidev_helper.sh

define spidev_helper_finalize_hook
	$(Q)cp drivers/spidev_helper/spidev_helper.ko output/
	$(Q)echo -n 'insmod spidev_helper.ko ' > $(spidev_helper_init_file)
	$(Q)echo >> $(spidev_helper_init_file)
endef