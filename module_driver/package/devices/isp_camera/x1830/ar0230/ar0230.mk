

#-------------------------------------------------------
package_name = sensor_ar0230
package_module_src = devices/isp_camera/x1830/ar0230
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ar0230_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ar0230_init_file = output/sensor_ar0230.sh

define sensor_ar0230_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/isp_camera/x1830/ar0230/ar0230.bin $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_ar0230_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/ar0230.bin
endef

TARGET_INSTALL_HOOKS += sensor_ar0230_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_ar0230_install_clean_hook

define sensor_ar0230_finalize_hook
	$(Q)cp devices/isp_camera/x1830/ar0230/sensor_ar0230.ko output/
	$(Q)echo -n 'insmod sensor_ar0230.ko' > $(sensor_ar0230_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X1830_AR0230_GPIO_POWER)' >> $(sensor_ar0230_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1830_AR0230_GPIO_RESET)' >> $(sensor_ar0230_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1830_AR0230_GPIO_PWDN)' >> $(sensor_ar0230_init_file)
	$(Q)echo -n ' i2c_sel_gpio=$(MD_X1830_AR0230_GPIO_I2C_SEL)' >> $(sensor_ar0230_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X1830_AR0230_I2C_BUSNUM)' >> $(sensor_ar0230_init_file)
	$(Q)echo -n ' dvp_gpio_func_str=$(MD_X1830_AR0230_DVP_GPIO_FUNC)' >> $(sensor_ar0230_init_file)
	$(Q)echo  >> $(sensor_ar0230_init_file)
endef
