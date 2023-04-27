#-------------------------------------------------------
package_name = sensor_gc2145_dvp
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/gc2145_dvp
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_gc2145_dvp_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file_gc2145_dvp = output/sensor_gc2145_dvp.sh

define sensor_gc2145_dvp_finalize_hook
	$(if $(MD_X2000_SENSOR_GC2145_DVP), $(Q)cp devices/camera/x2000/gc2145_dvp/sensor_gc2145_dvp.ko output/)
	$(if $(MD_X2000_SENSOR_GC2145_DVP), $(Q)echo -n 'insmod sensor_gc2145_dvp.ko' > $(shell_sensor_init_file_gc2145_dvp))
	$(if $(MD_X2000_SENSOR_GC2145_DVP), $(Q)echo -n ' power_gpio=$(MD_X2000_GC2145_DVP_GPIO_POWER0)' >> $(shell_sensor_init_file_gc2145_dvp))
	$(if $(MD_X2000_SENSOR_GC2145_DVP), $(Q)echo -n ' reset_gpio=$(MD_X2000_GC2145_DVP_GPIO_RESET0)' >> $(shell_sensor_init_file_gc2145_dvp))
	$(if $(MD_X2000_SENSOR_GC2145_DVP), $(Q)echo -n ' pwdn_gpio=$(MD_X2000_GC2145_DVP_GPIO_PWDN0)' >> $(shell_sensor_init_file_gc2145_dvp))
	$(if $(MD_X2000_SENSOR_GC2145_DVP), $(Q)echo -n ' regulator_name=$(MD_X2000_GC2145_DVP_SENSOR_REGULATOR_NAME)' >> $(shell_sensor_init_file_gc2145_dvp))
	$(if $(MD_X2000_SENSOR_GC2145_DVP), $(Q)echo -n ' i2c_bus_num=$(MD_X2000_GC2145_DVP_I2C_BUSNUM0)' >> $(shell_sensor_init_file_gc2145_dvp))
	$(if $(MD_X2000_SENSOR_GC2145_DVP), $(Q)echo -n ' i2c_addr=$(MD_X2000_GC2145_DVP_I2C_ADDR0)' >> $(shell_sensor_init_file_gc2145_dvp))
	$(if $(MD_X2000_SENSOR_GC2145_DVP), $(Q)echo -n ' cam_bus_num=$(MD_X2000_GC2145_DVP_CAM_BUSNUM0)' >> $(shell_sensor_init_file_gc2145_dvp))
	$(if $(MD_X2000_SENSOR_GC2145_DVP), $(Q)echo -n ' sensor_name=$(MD_X2000_GC2145_DVP_SENSOR_NAME)' >> $(shell_sensor_init_file_gc2145_dvp))
	$(if $(MD_X2000_SENSOR_GC2145_DVP), $(Q)echo  >> $(shell_sensor_init_file_gc2145_dvp))
endef