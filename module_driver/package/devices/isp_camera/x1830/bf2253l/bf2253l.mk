#-------------------------------------------------------
package_name = sensor_bf2253l
package_module_src = devices/isp_camera/x1830/bf2253l
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_bf2253l_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_bf2253l_init_file = output/sensor_bf2253l.sh

define sensor_bf2253l_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/isp_camera/x1830/bf2253l/bf2253l.bin $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_bf2253l_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/bf2253l.bin
endef

TARGET_INSTALL_HOOKS += sensor_bf2253l_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_bf2253l_install_clean_hook

define sensor_bf2253l_finalize_hook
	$(Q)cp devices/isp_camera/x1830/bf2253l/sensor_bf2253l.ko output/
	$(Q)echo -n 'insmod sensor_bf2253l.ko' > $(sensor_bf2253l_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X1830_BF2253L_GPIO_POWER)' >> $(sensor_bf2253l_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1830_BF2253L_GPIO_RESET)' >> $(sensor_bf2253l_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1830_BF2253L_GPIO_PWDN)' >> $(sensor_bf2253l_init_file)
	$(Q)echo -n ' i2c_sel_gpio=$(MD_X1830_BF2253L_GPIO_I2C_SEL)' >> $(sensor_bf2253l_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X1830_BF2253L_I2C_BUSNUM)' >> $(sensor_bf2253l_init_file)
	$(Q)echo  >> $(sensor_bf2253l_init_file)
endef
