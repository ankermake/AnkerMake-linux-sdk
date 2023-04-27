#-------------------------------------------------------
package_name = sensor_ov5693_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/ov5693_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov5693_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ov5693_init_file = output/sensor_ov5693_mipi.sh

define sensor_ov5693_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	# $(Q)cp -rf package/devices/camera/x2000/ov5693_mipi/ov5693.bin  $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_ov5693_mipi_install_clean_hook
	# $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/ov5693.bin
endef

TARGET_INSTALL_HOOKS += sensor_ov5693_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_ov5693_mipi_install_clean_hook

define sensor_ov5693_mipi_finalize_hook
	$(Q)cp devices/camera/x2000/ov5693_mipi/sensor_ov5693_mipi.ko output/
	$(Q)echo -n 'insmod sensor_ov5693_mipi.ko' > $(sensor_ov5693_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_OV5693_GPIO_POWER)' >> $(sensor_ov5693_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_OV5693_GPIO_RESET)' >> $(sensor_ov5693_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2000_OV5693_GPIO_PWDN)' >> $(sensor_ov5693_init_file)
	$(Q)echo -n ' regulator_name=$(MD_X2000_OV5693_SENSOR_REGULATOR_NAME)' >> $(sensor_ov5693_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_OV5693_I2C_BUSNUM)' >> $(sensor_ov5693_init_file)
	$(Q)echo -n ' i2c_addr=$(MD_X2000_OV5693_I2C_ADDR)' >> $(sensor_ov5693_init_file)
	$(Q)echo -n ' cam_bus_num=$(MD_X2000_OV5693_CAM_BUSNUM)' >> $(sensor_ov5693_init_file)
	$(Q)echo -n ' sensor_name=$(MD_X2000_OV5693_SENSOR_NAME)' >> $(sensor_ov5693_init_file)
	$(Q)echo  >> $(sensor_ov5693_init_file)
endef