#-------------------------------------------------------
package_name = sensor_pag7930_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/pag7930_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_pag7930_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_pag7930_init_file = output/sensor_pag7930_mipi.sh

define sensor_pag7930_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	# $(Q)cp -rf package/devices/camera/x2000/pag7930_mipi/pag7930-x2000.bin  $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_pag7930_mipi_install_clean_hook
	# $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/pag7930-x2000.bin
endef

TARGET_INSTALL_HOOKS += sensor_pag7930_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_pag7930_mipi_install_clean_hook

define sensor_pag7930_mipi_finalize_hook
	$(Q)cp devices/camera/x2000/pag7930_mipi/sensor_pag7930_mipi.ko output/
	$(Q)echo -n 'insmod sensor_pag7930_mipi.ko' > $(sensor_pag7930_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_PAG7930_GPIO_POWER)' >> $(sensor_pag7930_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_PAG7930_GPIO_RESET)' >> $(sensor_pag7930_init_file)
	$(Q)echo -n ' regulator_name=$(MD_X2000_PAG7930_SENSOR_REGULATOR_NAME)' >> $(sensor_pag7930_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_PAG7930_I2C_BUSNUM)' >> $(sensor_pag7930_init_file)
	$(Q)echo -n ' i2c_addr=$(MD_X2000_PAG7930_I2C_ADDR)' >> $(sensor_pag7930_init_file)
	$(Q)echo -n ' cam_bus_num=$(MD_X2000_PAG7930_CAM_BUSNUM)' >> $(sensor_pag7930_init_file)
	$(Q)echo -n ' sensor_name=$(MD_X2000_PAG7930_SENSOR_NAME)' >> $(sensor_pag7930_init_file)
	$(Q)echo  >> $(sensor_pag7930_init_file)
endef