

#-------------------------------------------------------
package_name = sensor_ar0230
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/ar0230
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ar0230_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ar0230_init_file = output/sensor_ar0230.sh

define sensor_ar0230_finalize_hook
	$(Q)cp devices/camera/x2000/ar0230/sensor_ar0230.ko output/
	$(Q)echo -n 'insmod sensor_ar0230.ko' > $(sensor_ar0230_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_AR0230_GPIO_POWER)' >> $(sensor_ar0230_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_AR0230_GPIO_RESET)' >> $(sensor_ar0230_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2000_AR0230_GPIO_PWDN)' >> $(sensor_ar0230_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_AR0230_I2C_BUSNUM)' >> $(sensor_ar0230_init_file)
	$(Q)echo -n ' dvp_gpio_func_str=$(MD_X2000_AR0230_DVP_DATE_FMT)' >> $(sensor_ar0230_init_file)
	$(Q)echo  >> $(sensor_ar0230_init_file)
endef
