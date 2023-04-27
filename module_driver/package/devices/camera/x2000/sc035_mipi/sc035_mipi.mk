#-------------------------------------------------------
package_name = sensor_sc035_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/sc035_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_sc035_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_sc035_mipi.sh


define sensor_sc035_mipi_finalize_hook
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)cp devices/camera/x2000/sc035_mipi/sensor0_sc035_mipi.ko output/)
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n 'insmod sensor0_sc035_mipi.ko' > $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n ' sensor_mode=$(MD_X2000_SC035_SENSOR0_MODE)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n ' power_gpio=$(MD_X2000_SC035_GPIO_POWER0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n ' reset_gpio=$(MD_X2000_SC035_GPIO_RESET0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n ' pwdn_gpio=$(MD_X2000_SC035_GPIO_PWDN0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n ' regulator_name=$(MD_X2000_SC035_SENSOR0_REGULATOR_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n ' i2c_bus_num=$(MD_X2000_SC035_I2C_BUSNUM0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n ' i2c_addr=$(MD_X2000_SC035_I2C_ADDR0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n ' cam_bus_num=$(MD_X2000_SC035_CAM_BUSNUM0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n ' sensor_name=$(MD_X2000_SC035_SENSOR0_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n ' sensor_light_ctl=$(MD_X2000_SC035_LIGHT_CTL_GPIO)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR0_SC035), $(Q)echo -n ' sensor_strobe=$(MD_X2000_SC035_SENSOR_STROBE)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)echo  >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)cp devices/camera/x2000/sc035_mipi/sensor1_sc035_mipi.ko output/)
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)echo -n 'insmod sensor1_sc035_mipi.ko' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)echo -n ' sensor_mode=$(MD_X2000_SC035_SENSOR1_MODE)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)echo -n ' power_gpio=$(MD_X2000_SC035_GPIO_POWER1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)echo -n ' reset_gpio=$(MD_X2000_SC035_GPIO_RESET1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)echo -n ' pwdn_gpio=$(MD_X2000_SC035_GPIO_PWDN1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)echo -n ' regulator_name=$(MD_X2000_SC035_SENSOR1_REGULATOR_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)echo -n ' i2c_bus_num=$(MD_X2000_SC035_I2C_BUSNUM1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)echo -n ' i2c_addr=$(MD_X2000_SC035_I2C_ADDR1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)echo -n ' cam_bus_num=$(MD_X2000_SC035_CAM_BUSNUM1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR1_SC035), $(Q)echo -n ' sensor_name=$(MD_X2000_SC035_SENSOR1_NAME)' >> $(shell_sensor_init_file))

	$(Q)echo  >> $(shell_sensor_init_file)

endef

