#-------------------------------------------------------
package_name = sensor_sc031_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x1600/sc031_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_sc031_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_sc031_mipi.sh


define sensor_sc031_mipi_finalize_hook
	$(if $(MD_X1600_SENSOR_SC031_MIPI), $(Q)cp devices/camera/x1600/sc031_mipi/sensor_sc031_mipi.ko output/)
	$(if $(MD_X1600_SENSOR_SC031_MIPI), $(Q)echo -n 'insmod sensor_sc031_mipi.ko' > $(shell_sensor_init_file))
	$(if $(MD_X1600_SENSOR_SC031_MIPI), $(Q)echo -n ' power_gpio=$(MD_X1600_SC031_MIPI_GPIO_POWER)' >> $(shell_sensor_init_file))
	$(if $(MD_X1600_SENSOR_SC031_MIPI), $(Q)echo -n ' reset_gpio=$(MD_X1600_SC031_MIPI_GPIO_RESET)' >> $(shell_sensor_init_file))
	$(if $(MD_X1600_SENSOR_SC031_MIPI), $(Q)echo -n ' pwdn_gpio=$(MD_X1600_SC031_MIPI_GPIO_PWDN)' >> $(shell_sensor_init_file))
	$(if $(MD_X1600_SENSOR_SC031_MIPI), $(Q)echo -n ' i2c_bus_num=$(MD_X1600_SC031_MIPI_I2C_BUSNUM)' >> $(shell_sensor_init_file))

	$(Q)echo  >> $(shell_sensor_init_file)

endef

