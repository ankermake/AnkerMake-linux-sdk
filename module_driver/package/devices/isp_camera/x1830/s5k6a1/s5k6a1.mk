#-------------------------------------------------------
package_name = sensor_s5k6a1
package_module_src = devices/isp_camera/x1830/s5k6a1
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_s5k6a1_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_s5k6a1_init_file = output/sensor_s5k6a1.sh

define sensor_s5k6a1_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/isp_camera/x1830/s5k6a1/s5k6a1.bin $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_s5k6a1_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/s5k6a1.bin
endef

TARGET_INSTALL_HOOKS += sensor_s5k6a1_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_s5k6a1_install_clean_hook

define sensor_s5k6a1_finalize_hook
	$(Q)cp devices/isp_camera/x1830/s5k6a1/sensor_s5k6a1.ko output/
	$(Q)echo -n 'insmod sensor_s5k6a1.ko' > $(sensor_s5k6a1_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X1830_S5K6A1_GPIO_POWER)' >> $(sensor_s5k6a1_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1830_S5K6A1_GPIO_RESET)' >> $(sensor_s5k6a1_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1830_S5K6A1_GPIO_PWDN)' >> $(sensor_s5k6a1_init_file)
	$(Q)echo -n ' i2c_sel_gpio=$(MD_X1830_S5K6A1_GPIO_I2C_SEL)' >> $(sensor_s5k6a1_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X1830_S5K6A1_I2C_BUSNUM)' >> $(sensor_s5k6a1_init_file)
	$(Q)echo  >> $(sensor_s5k6a1_init_file)
endef
