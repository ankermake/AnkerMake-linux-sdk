

#-------------------------------------------------------
package_name = sensor_ov9732
package_depends = utils soc_camera
package_module_src = devices/camera/x1520/ov9732
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov9732_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ov9732_init_file = output/sensor_ov9732.sh

define sensor_ov9732_finalize_hook
	$(Q)cp devices/camera/x1520/ov9732/sensor_ov9732.ko output/
	$(Q)echo -n 'insmod sensor_ov9732.ko' > $(sensor_ov9732_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1520_OV9732_GPIO_RESET)' >> $(sensor_ov9732_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1520_OV9732_GPIO_PWDN)' >> $(sensor_ov9732_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X1520_OV9732_I2C_BUSNUM)' >> $(sensor_ov9732_init_file)
	$(Q)echo -n ' i2c_addr=$(MD_X1520_OV9732_I2C_ADDR)' >> $(sensor_ov9732_init_file)
	$(Q)echo  >> $(sensor_ov9732_init_file)
endef
