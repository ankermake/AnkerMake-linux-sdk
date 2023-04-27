
#-------------------------------------------------------
package_name = soc_gpio
package_depends = utils
package_module_src = soc/x1000/gpio
package_make_hook =
package_init_hook =
package_finalize_hook = soc_gpio_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_gpio_init_file = output/soc_gpio.sh

define soc_gpio_finalize_hook
	$(Q)cp soc/x1000/gpio/soc_gpio.ko output/
	$(Q)echo -n 'insmod soc_gpio.ko' > $(soc_gpio_init_file)
	$(Q)echo -n ' debug=0' >> $(soc_gpio_init_file)
	$(Q)echo  >> $(soc_gpio_init_file)

endef
