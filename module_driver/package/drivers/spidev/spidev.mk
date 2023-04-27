#-------------------------------------------------------
package_name = spidev
package_depends = utils
package_module_src = drivers/spidev
package_make_hook =
package_init_hook =
package_finalize_hook = spidev_finalize_hook
package_clean_hook =
#-------------------------------------------------------

spidev_init_file = output/spidev.sh

define spidev_finalize_hook
	$(Q)cp drivers/spidev/spidev.ko output/
	$(Q)echo -n 'insmod spidev.ko ' > $(spidev_init_file)
	$(Q)echo >> $(spidev_init_file)
endef
