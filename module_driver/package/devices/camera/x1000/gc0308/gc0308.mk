

#-------------------------------------------------------
package_name = sensor_gc0308
package_depends = utils soc_camera
package_module_src = devices/camera/x1000/gc0308
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_gc0308_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_gc0308_init_file = output/sensor_gc0308.sh

define sensor_gc0308_finalize_hook
	$(Q)cp devices/camera/x1000/gc0308/sensor_gc0308.ko output/
	$(Q)echo -n 'insmod sensor_gc0308.ko' > $(sensor_gc0308_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X1000_GC0308_GPIO_POWER_EN)' >> $(sensor_gc0308_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1000_GC0308_GPIO_RESET)' >> $(sensor_gc0308_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1000_GC0308_GPIO_PWDN)' >> $(sensor_gc0308_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X1000_GC0308_I2C_BUSNUM)' >> $(sensor_gc0308_init_file)
	$(Q)echo -n ' use_y8=$(if $(MD_X1000_SENSOR_GC0308_FMT_Y8),1,0)' >> $(sensor_gc0308_init_file)
	$(Q)echo  >> $(sensor_gc0308_init_file)
endef
