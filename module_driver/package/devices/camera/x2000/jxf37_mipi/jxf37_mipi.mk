#-------------------------------------------------------
package_name = sensor_jxf37_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/jxf37_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_jxf37_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_jxf37_mipi_init_file = output/sensor_jxf37_mipi.sh

define sensor_jxf37_mipi_finalize_hook
	$(Q)cp devices/camera/x2000/jxf37_mipi/sensor_jxf37_mipi.ko output/
	$(Q)echo -n 'insmod sensor_jxf37_mipi.ko' > $(sensor_jxf37_mipi_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_JXF37_GPIO_POWER)' >> $(sensor_jxf37_mipi_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_JXF37_GPIO_RESET)' >> $(sensor_jxf37_mipi_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2000_JXF37_GPIO_PWDN)' >> $(sensor_jxf37_mipi_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_JXF37_I2C_BUSNUM)' >> $(sensor_jxf37_mipi_init_file)
	$(Q)echo  >> $(sensor_jxf37_mipi_init_file)
endef
