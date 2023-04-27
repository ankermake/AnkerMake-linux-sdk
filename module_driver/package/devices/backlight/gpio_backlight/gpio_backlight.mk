#-------------------------------------------------------
package_name = gpio_backlight
package_depends = utils
package_module_src = devices/backlight/gpio_backlight/
package_make_hook =
package_init_hook =
package_finalize_hook = gpio_backlight_finalize_hook
package_clean_hook =
#-------------------------------------------------------

gpio_backlight_init_file = output/gpio_backlight.sh

TARGET_INSTALL_HOOKS +=
TARGET_INSTALL_CLEAN_HOOKS +=

define gpio_backlight_finalize_hook
	$(Q)cp devices/backlight/gpio_backlight/gpio_backlight.ko output/
	$(Q)echo 'insmod gpio_backlight.ko \' > $(gpio_backlight_init_file)
	$(Q)echo -n 'backlight_gpio=$(MD_GPIO_BACKLIGHT0_GPIO) ' >> $(gpio_backlight_init_file)
	$(Q)echo -n 'backlight_dev_name=$(MD_GPIO_BACKLIGHT0_DEV_NAME) ' >> $(gpio_backlight_init_file)
	$(Q)echo -n 'backlight_enabled=$(if $(MD_GPIO_BACKLIGHT0_ENABLE),1,0) ' >> $(gpio_backlight_init_file)
	$(Q)echo -n 'gpio_active_level=$(MD_GPIO_BACKLIGHT0_ACTIVE_LEVEL) ' >> $(gpio_backlight_init_file)
	$(Q)echo  >> $(gpio_backlight_init_file)
endef
