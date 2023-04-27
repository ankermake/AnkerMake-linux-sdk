#-------------------------------------------------------
package_name = sensor_gvm8666b_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/gvm8666b_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_gvm8666b_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_gvm8666b_mipi.sh


define sensor_gvm8666b_mipi_finalize_hook
	$(Q)cp devices/camera/x2000/gvm8666b_mipi/sensor_gvm8666b_mipi.ko output/
	$(Q)echo -n 'insmod sensor_gvm8666b_mipi.ko' > $(shell_sensor_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_GVM8666B_GPIO_POWER)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_GVM8666B_GPIO_RESET)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2000_GVM8666B_GPIO_PWDN)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_GVM8666B_I2C_BUSNUM)' >> $(shell_sensor_init_file)
	$(Q)echo  >> $(shell_sensor_init_file)
endef