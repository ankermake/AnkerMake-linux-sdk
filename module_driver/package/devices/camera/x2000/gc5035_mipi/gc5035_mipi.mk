#-------------------------------------------------------
package_name = sensor_gc5035_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/gc5035_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_gc5035_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_gc5035_mipi.sh

define sensor_gc5035_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/camera/x2000/gc5035_mipi/gc5035-x2000.bin $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_gc5035_mipi_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/gc5035-x2000.bin
endef

TARGET_INSTALL_HOOKS += sensor_gc5035_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_gc5035_mipi_install_clean_hook

define sensor_gc5035_mipi_finalize_hook
	$(Q)cp devices/camera/x2000/gc5035_mipi/sensor_gc5035_mipi.ko output/
	$(Q)echo -n 'insmod sensor_gc5035_mipi.ko' > $(shell_sensor_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_GC5035_GPIO_POWER)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_GC5035_GPIO_RESET)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2000_GC5035_GPIO_PWDN)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_GC5035_I2C_BUSNUM)' >> $(shell_sensor_init_file)
	$(Q)echo  >> $(shell_sensor_init_file)
endef
