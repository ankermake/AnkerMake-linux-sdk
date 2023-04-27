#-------------------------------------------------------
package_name = sensor_imx335_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/imx335_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_imx335_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_imx335_mipi.sh

define sensor_imx335_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)cp -rf package/devices/camera/x2000/imx335_mipi/$(MD_X2000_IMX335_SENSOR_NAME)-x2000.bin  $(FS_TARGET_DIR)/etc/sensor/)
endef

define sensor_imx335_mipi_install_clean_hook
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/$(MD_X2000_IMX335_SENSOR_NAME)-x2000.bin)
endef

TARGET_INSTALL_HOOKS += sensor_imx335_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_imx335_mipi_install_clean_hook



define sensor_imx335_mipi_finalize_hook
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)cp devices/camera/x2000/imx335_mipi/sensor_imx335_mipi.ko output/)
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)echo -n 'insmod sensor_imx335_mipi.ko' > $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)echo -n ' power_gpio=$(MD_X2000_IMX335_GPIO_POWER)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)echo -n ' reset_gpio=$(MD_X2000_IMX335_GPIO_RESET)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)echo -n ' pwdn_gpio=$(MD_X2000_IMX335_GPIO_PWDN)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)echo -n ' regulator_name=$(MD_X2000_IMX335_SENSOR_REGULATOR_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)echo -n ' i2c_bus_num=$(MD_X2000_IMX335_I2C_BUSNUM)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)echo -n ' i2c_addr=$(MD_X2000_IMX335_I2C_ADDR)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)echo -n ' cam_bus_num=$(MD_X2000_IMX335_CAM_BUSNUM)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_IMX335_MIPI), $(Q)echo -n ' sensor_name=$(MD_X2000_IMX335_SENSOR_NAME)' >> $(shell_sensor_init_file))
	$(Q)echo  >> $(shell_sensor_init_file)

endef

