

#-------------------------------------------------------
package_name = sensor_gc0329
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/gc0329
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_gc0329_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_gc0329_init_file = output/sensor_gc0329.sh

define sensor_gc0329_finalize_hook
	$(Q)cp devices/camera/x2000/gc0329/sensor_gc0329.ko output/
	$(Q)echo -n 'insmod sensor_gc0329.ko' > $(sensor_gc0329_init_file)
	$(Q)echo -n ' camera_index=$(MD_X2000_GC0329_CAMERA_INDEX)' >> $(sensor_gc0329_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_GC0329_GPIO_POWER)' >> $(sensor_gc0329_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_GC0329_GPIO_RESET)' >> $(sensor_gc0329_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2000_GC0329_GPIO_PWDN)' >> $(sensor_gc0329_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_GC0329_I2C_BUSNUM)' >> $(sensor_gc0329_init_file)
	$(Q)echo -n ' dvp_gpio_func_str=$(MD_X2000_GC0329_DVP_DATE_FMT)' >> $(sensor_gc0329_init_file)
	$(Q)echo -n ' resolution=$(MD_X2000_GC0329_RESOLUTION)' >> $(sensor_gc0329_init_file)
	$(Q)echo  >> $(sensor_gc0329_init_file)
endef
