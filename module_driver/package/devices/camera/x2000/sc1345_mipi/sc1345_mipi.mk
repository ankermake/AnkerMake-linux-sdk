#-------------------------------------------------------
package_name = sensor_sc1345_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/sc1345_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_sc1345_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_sc1345_mipi.sh

define sensor_sc1345_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/camera/x2000/sc1345_mipi/sc1345-x2000.bin  $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_sc1345_mipi_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/sc1345-x2000.bin
endef

TARGET_INSTALL_HOOKS += sensor_sc1345_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_sc1345_mipi_install_clean_hook

define sensor_sc1345_mipi_finalize_hook
	$(if $(MD_X2000_SENSOR_SC1345_MIPI), $(Q)cp devices/camera/x2000/sc1345_mipi/sensor_sc1345_mipi.ko output/)
	$(if $(MD_X2000_SENSOR_SC1345_MIPI), $(Q)echo -n 'insmod sensor_sc1345_mipi.ko' > $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_SC1345_MIPI), $(Q)echo -n ' avdd_gpio=$(MD_X2000_SC1345_GPIO_AVDD)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_SC1345_MIPI), $(Q)echo -n ' dvdd_gpio=$(MD_X2000_SC1345_GPIO_DVDD)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_SC1345_MIPI), $(Q)echo -n ' dovdd_gpio=$(MD_X2000_SC1345_GPIO_DOVDD)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_SC1345_MIPI), $(Q)echo -n ' reset_gpio=$(MD_X2000_SC1345_GPIO_RESET)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_SC1345_MIPI), $(Q)echo -n ' pwdn_gpio=$(MD_X2000_SC1345_GPIO_PWDN)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_SC1345_MIPI), $(Q)echo -n ' i2c_bus_num=$(MD_X2000_SC1345_I2C_BUSNUM)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_SC1345_MIPI), $(Q)echo -n ' i2c_addr=$(MD_X2000_SC1345_I2C_ADDR)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_SC1345_MIPI), $(Q)echo -n ' cam_bus_num=$(MD_X2000_SC1345_CAM_BUSNUM)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_SC1345_MIPI), $(Q)echo -n ' sensor_name=$(MD_X2000_SC1345_SENSOR_NAME)' >> $(shell_sensor_init_file))

	$(Q)echo  >> $(shell_sensor_init_file)

endef

