#-------------------------------------------------------
package_name = sensor_sc4238_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/sc4238_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_sc4238_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_sc4238_mipi.sh

define sensor_sc4238_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)cp -rf package/devices/camera/x2000/sc4238_mipi/$(MD_X2000_SC4238_SENSOR0_NAME)-x2000.bin  $(FS_TARGET_DIR)/etc/sensor/)
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)cp -rf package/devices/camera/x2000/sc4238_mipi/$(MD_X2000_SC4238_SENSOR1_NAME)-x2000.bin  $(FS_TARGET_DIR)/etc/sensor/)
endef

define sensor_sc4238_mipi_install_clean_hook
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/$(MD_X2000_SC4238_SENSOR0_NAME)-x2000.bin)
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/$(MD_X2000_SC4238_SENSOR1_NAME)-x2000.bin)
endef

TARGET_INSTALL_HOOKS += sensor_sc4238_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_sc4238_mipi_install_clean_hook



define sensor_sc4238_mipi_finalize_hook
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)cp devices/camera/x2000/sc4238_mipi/sensor0_sc4238_mipi.ko output/)
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)echo -n 'insmod sensor0_sc4238_mipi.ko' > $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)echo -n ' power_gpio=$(MD_X2000_SC4238_GPIO_POWER0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)echo -n ' reset_gpio=$(MD_X2000_SC4238_GPIO_RESET0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)echo -n ' pwdn_gpio=$(MD_X2000_SC4238_GPIO_PWDN0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)echo -n ' regulator_name=$(MD_X2000_SC4238_SENSOR0_REGULATOR_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)echo -n ' i2c_bus_num=$(MD_X2000_SC4238_I2C_BUSNUM0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)echo -n ' i2c_addr=$(MD_X2000_SC4238_I2C_ADDR0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)echo -n ' cam_bus_num=$(MD_X2000_SC4238_CAM_BUSNUM0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC4238), $(Q)echo -n ' sensor_name=$(MD_X2000_SC4238_SENSOR0_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)echo  >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)cp devices/camera/x2000/sc4238_mipi/sensor1_sc4238_mipi.ko output/)
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)echo -n 'insmod sensor1_sc4238_mipi.ko' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)echo -n ' power_gpio=$(MD_X2000_SC4238_GPIO_POWER1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)echo -n ' reset_gpio=$(MD_X2000_SC4238_GPIO_RESET1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)echo -n ' pwdn_gpio=$(MD_X2000_SC4238_GPIO_PWDN1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)echo -n ' regulator_name=$(MD_X2000_SC4238_SENSOR1_REGULATOR_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)echo -n ' i2c_bus_num=$(MD_X2000_SC4238_I2C_BUSNUM1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)echo -n ' i2c_addr=$(MD_X2000_SC4238_I2C_ADDR1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)echo -n ' cam_bus_num=$(MD_X2000_SC4238_CAM_BUSNUM1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC4238), $(Q)echo -n ' sensor_name=$(MD_X2000_SC4238_SENSOR1_NAME)' >> $(shell_sensor_init_file))

	$(Q)echo  >> $(shell_sensor_init_file)

endef

