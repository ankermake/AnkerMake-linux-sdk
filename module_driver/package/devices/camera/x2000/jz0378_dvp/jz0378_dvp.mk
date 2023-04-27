

#-------------------------------------------------------
package_name = sensor_jz0378_dvp
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/jz0378_dvp
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_jz0378_dvp_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_jz0378_dvp_init_file = output/sensor_jz0378_dvp.sh

define sensor_jz0378_dvp_finalize_hook
	$(Q)cp devices/camera/x2000/jz0378_dvp/sensor_jz0378_dvp.ko output/
	$(Q)echo -n 'insmod sensor_jz0378_dvp.ko' > $(sensor_jz0378_dvp_init_file)
	$(Q)echo -n ' camera_index=$(MD_X2000_JZ0378_DVP_CAMERA_INDEX)' >> $(sensor_jz0378_dvp_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_JZ0378_DVP_GPIO_POWER)' >> $(sensor_jz0378_dvp_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_JZ0378_DVP_GPIO_RESET)' >> $(sensor_jz0378_dvp_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2000_JZ0378_DVP_GPIO_PWDN)' >> $(sensor_jz0378_dvp_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_JZ0378_DVP_I2C_BUSNUM)' >> $(sensor_jz0378_dvp_init_file)
	$(Q)echo -n ' dvp_gpio_func_str=$(MD_X2000_JZ0378_DVP_DVP_DATE_FMT)' >> $(sensor_jz0378_dvp_init_file)
	$(Q)echo -n ' resolution=$(MD_X2000_JZ0378_DVP_RESOLUTION)' >> $(sensor_jz0378_dvp_init_file)
	$(Q)echo  >> $(sensor_jz0378_dvp_init_file)
endef
