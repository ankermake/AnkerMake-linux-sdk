#-------------------------------------------------------
package_name = sensor_sc301_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/sc301_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_sc301_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_sc301_init_file = output/sensor_sc301_mipi.sh

define sensor_sc301_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/camera/x2000/sc301_mipi/sc301-vis-x2000.bin  $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_sc301_mipi_install_clean_hook
	# $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/sc301.bin
endef

TARGET_INSTALL_HOOKS += sensor_sc301_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_sc301_mipi_install_clean_hook

define sensor_sc301_mipi_finalize_hook
	$(Q)cp devices/camera/x2000/sc301_mipi/sensor_sc301_mipi.ko output/
	$(Q)echo -n 'insmod sensor_sc301_mipi.ko' > $(sensor_sc301_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_SC301_GPIO_POWER)' >> $(sensor_sc301_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_SC301_GPIO_RESET)' >> $(sensor_sc301_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2000_SC301_GPIO_PWDN)' >> $(sensor_sc301_init_file)
	$(Q)echo -n ' regulator_name=$(MD_X2000_SC301_SENSOR_REGULATOR_NAME)' >> $(sensor_sc301_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_SC301_I2C_BUSNUM)' >> $(sensor_sc301_init_file)
	$(Q)echo -n ' i2c_addr=$(MD_X2000_SC301_I2C_ADDR)' >> $(sensor_sc301_init_file)
	$(Q)echo -n ' cam_bus_num=$(MD_X2000_SC301_CAM_BUSNUM)' >> $(sensor_sc301_init_file)
	$(Q)echo -n ' sensor_name=$(MD_X2000_SC301_SENSOR_NAME)' >> $(sensor_sc301_init_file)
	$(Q)echo  >> $(sensor_sc301_init_file)
endef