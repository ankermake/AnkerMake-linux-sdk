

#-------------------------------------------------------
package_name = sensor_ov9732
package_module_src = devices/isp_camera/x1830/ov9732
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov9732_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ov9732_init_file = output/sensor_ov9732.sh

define sensor_ov9732_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/isp_camera/x1830/ov9732/ov9732.bin $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_ov9732_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/ov9732.bin
endef

TARGET_INSTALL_HOOKS += sensor_ov9732_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_ov9732_install_clean_hook

define sensor_ov9732_finalize_hook
	$(Q)cp devices/isp_camera/x1830/ov9732/sensor_ov9732.ko output/
	$(Q)echo -n 'insmod sensor_ov9732.ko' > $(sensor_ov9732_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X1830_OV9732_GPIO_POWER)' >> $(sensor_ov9732_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1830_OV9732_GPIO_RESET)' >> $(sensor_ov9732_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1830_OV9732_GPIO_PWDN)' >> $(sensor_ov9732_init_file)
	$(Q)echo -n ' i2c_sel_gpio=$(MD_X1830_OV9732_GPIO_I2C_SEL)' >> $(sensor_ov9732_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X1830_OV9732_I2C_BUSNUM)' >> $(sensor_ov9732_init_file)
	$(Q)echo -n ' dvp_gpio_func_str=$(MD_X1830_OV9732_DVP_GPIO_FUNC)' >> $(sensor_ov9732_init_file)
	$(Q)echo  >> $(sensor_ov9732_init_file)
endef
