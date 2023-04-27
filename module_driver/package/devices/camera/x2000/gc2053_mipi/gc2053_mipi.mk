#-------------------------------------------------------
package_name = sensor_gc2053_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/gc2053_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_gc2053_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_gc2053_mipi.sh

define sensor_gc2053_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)cp -rf package/devices/camera/x2000/gc2053_mipi/$(MD_X2000_GC2053_SENSOR0_NAME)-x2000.bin  $(FS_TARGET_DIR)/etc/sensor/)
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)cp -rf package/devices/camera/x2000/gc2053_mipi/$(MD_X2000_GC2053_SENSOR1_NAME)-x2000.bin  $(FS_TARGET_DIR)/etc/sensor/)
endef

define sensor_gc2053_mipi_install_clean_hook
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/$(MD_X2000_GC2053_SENSOR0_NAME)-x2000.bin)
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/$(MD_X2000_GC2053_SENSOR1_NAME)-x2000.bin)
endef

TARGET_INSTALL_HOOKS += sensor_gc2053_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_gc2053_mipi_install_clean_hook



define sensor_gc2053_mipi_finalize_hook
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)cp devices/camera/x2000/gc2053_mipi/sensor0_gc2053_mipi.ko output/)
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)echo -n 'insmod sensor0_gc2053_mipi.ko' > $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)echo -n ' power_gpio=$(MD_X2000_GC2053_GPIO_POWER0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)echo -n ' reset_gpio=$(MD_X2000_GC2053_GPIO_RESET0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)echo -n ' pwdn_gpio=$(MD_X2000_GC2053_GPIO_PWDN0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)echo -n ' regulator_name=$(MD_X2000_GC2053_SENSOR0_REGULATOR_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)echo -n ' i2c_bus_num=$(MD_X2000_GC2053_I2C_BUSNUM0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)echo -n ' i2c_addr=$(MD_X2000_GC2053_I2C_ADDR0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)echo -n ' cam_bus_num=$(MD_X2000_GC2053_CAM_BUSNUM0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_GC2053), $(Q)echo -n ' sensor_name=$(MD_X2000_GC2053_SENSOR0_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)echo  >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)cp devices/camera/x2000/gc2053_mipi/sensor1_gc2053_mipi.ko output/)
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)echo -n 'insmod sensor1_gc2053_mipi.ko' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)echo -n ' power_gpio=$(MD_X2000_GC2053_GPIO_POWER1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)echo -n ' reset_gpio=$(MD_X2000_GC2053_GPIO_RESET1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)echo -n ' pwdn_gpio=$(MD_X2000_GC2053_GPIO_PWDN1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)echo -n ' regulator_name=$(MD_X2000_GC2053_SENSOR1_REGULATOR_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)echo -n ' i2c_bus_num=$(MD_X2000_GC2053_I2C_BUSNUM1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)echo -n ' i2c_addr=$(MD_X2000_GC2053_I2C_ADDR1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)echo -n ' cam_bus_num=$(MD_X2000_GC2053_CAM_BUSNUM1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_GC2053), $(Q)echo -n ' sensor_name=$(MD_X2000_GC2053_SENSOR1_NAME)' >> $(shell_sensor_init_file))

	$(Q)echo  >> $(shell_sensor_init_file)

endef

