

#-------------------------------------------------------
package_name = sensor_gc0308_dvp
package_depends = utils soc_camera
package_module_src = devices/camera/x1600/gc0308_dvp
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_gc0308_dvp_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_gc0308_dvp_init_file = output/sensor_gc0308_dvp.sh

define sensor_gc0308_dvp_finalize_hook
	$(Q)cp devices/camera/x1600/gc0308_dvp/sensor_gc0308_dvp.ko output/
	$(Q)echo -n 'insmod sensor_gc0308_dvp.ko' > $(sensor_gc0308_dvp_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X1600_GC0308_DVP_GPIO_POWER)' >> $(sensor_gc0308_dvp_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1600_GC0308_DVP_GPIO_RESET)' >> $(sensor_gc0308_dvp_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1600_GC0308_DVP_GPIO_PWDN)' >> $(sensor_gc0308_dvp_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X1600_GC0308_DVP_I2C_BUSNUM)' >> $(sensor_gc0308_dvp_init_file)
	$(Q)echo  >> $(sensor_gc0308_dvp_init_file)
endef
