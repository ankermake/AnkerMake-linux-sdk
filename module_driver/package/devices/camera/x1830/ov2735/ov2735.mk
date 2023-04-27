

#-------------------------------------------------------
package_name = sensor_ov2735
package_depends = utils soc_camera
package_module_src = devices/camera/x1830/ov2735
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov2735_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ov2735_init_file = output/sensor_ov2735.sh

define sensor_ov2735_finalize_hook
	$(Q)cp devices/camera/x1830/ov2735/sensor_ov2735.ko output/
	$(Q)echo -n 'insmod sensor_ov2735.ko' > $(sensor_ov2735_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X1830_OV2735_GPIO_POWER)' >> $(sensor_ov2735_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1830_OV2735_GPIO_RESET)' >> $(sensor_ov2735_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1830_OV2735_GPIO_PWDN)' >> $(sensor_ov2735_init_file)
	$(Q)echo -n ' i2c_sel_gpio=$(MD_X1830_OV2735_GPIO_I2C_SEL)' >> $(sensor_ov2735_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X1830_OV2735_I2C_BUSNUM)' >> $(sensor_ov2735_init_file)
	$(Q)echo  >> $(sensor_ov2735_init_file)
endef
