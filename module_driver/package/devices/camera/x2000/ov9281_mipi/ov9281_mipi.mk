#-------------------------------------------------------
package_name = sensor_ov9281_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/ov9281_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov9281_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ov9281_mipi_init_file = output/sensor_ov9281_mipi.sh

define sensor_ov9281_mipi_finalize_hook
	$(Q)cp devices/camera/x2000/ov9281_mipi/sensor_ov9281_mipi.ko output/
	$(Q)echo -n 'insmod sensor_ov9281_mipi.ko' > $(sensor_ov9281_mipi_init_file)
	$(Q)echo -n ' camera_index=$(MD_X2000_OV9281_CAMERA_INDEX)' >> $(sensor_ov9281_mipi_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_OV9281_GPIO_POWER)' >> $(sensor_ov9281_mipi_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_OV9281_GPIO_RESET)' >> $(sensor_ov9281_mipi_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2000_OV9281_GPIO_PWDN)' >> $(sensor_ov9281_mipi_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_OV9281_I2C_BUSNUM)' >> $(sensor_ov9281_mipi_init_file)
	$(Q)echo  >> $(sensor_ov9281_mipi_init_file)
endef
