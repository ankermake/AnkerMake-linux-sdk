#-------------------------------------------------------
package_name = sensor_jxh62_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/jxh62_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_jxh62_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_jxh62_init_file = output/sensor_jxh62_mipi.sh

define sensor_jxh62_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	# $(Q)cp -rf package/devices/camera/x2000/jxh62_mipi/jxh62.bin  $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_jxh62_mipi_install_clean_hook
	# $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/jxh62.bin
endef

TARGET_INSTALL_HOOKS += sensor_jxh62_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_jxh62_mipi_install_clean_hook

define sensor_jxh62_mipi_finalize_hook
	$(Q)cp devices/camera/x2000/jxh62_mipi/sensor_jxh62_mipi.ko output/
	$(Q)echo -n 'insmod sensor_jxh62_mipi.ko' > $(sensor_jxh62_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_JXH62_GPIO_POWER)' >> $(sensor_jxh62_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_JXH62_GPIO_RESET)' >> $(sensor_jxh62_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2000_JXH62_GPIO_PWDN)' >> $(sensor_jxh62_init_file)
	$(Q)echo -n ' regulator_name=$(MD_X2000_JXH62_SENSOR_REGULATOR_NAME)' >> $(sensor_jxh62_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_JXH62_I2C_BUSNUM)' >> $(sensor_jxh62_init_file)
	$(Q)echo -n ' i2c_addr=$(MD_X2000_JXH62_I2C_ADDR)' >> $(sensor_jxh62_init_file)
	$(Q)echo -n ' cam_bus_num=$(MD_X2000_JXH62_CAM_BUSNUM)' >> $(sensor_jxh62_init_file)
	$(Q)echo -n ' sensor_name=$(MD_X2000_JXH62_SENSOR_NAME)' >> $(sensor_jxh62_init_file)
	$(Q)echo  >> $(sensor_jxh62_init_file)
endef