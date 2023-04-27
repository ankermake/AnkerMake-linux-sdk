

#-------------------------------------------------------
package_name = sensor_gc2375a
package_module_src = devices/isp_camera/x1830/gc2375
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_gc2375a_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_gc2375a_init_file = output/sensor_gc2375a.sh

define sensor_gc2375a_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/isp_camera/x1830/gc2375/gc2375a.bin $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_gc2375a_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/gc2375a.bin
endef

TARGET_INSTALL_HOOKS += sensor_gc2375a_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_gc2375a_install_clean_hook

define sensor_gc2375a_finalize_hook
	$(Q)cp devices/isp_camera/x1830/gc2375/sensor_gc2375a.ko output/
	$(Q)echo -n 'insmod sensor_gc2375a.ko' > $(sensor_gc2375a_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X1830_GC2375A_GPIO_POWER)' >> $(sensor_gc2375a_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1830_GC2375A_GPIO_RESET)' >> $(sensor_gc2375a_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1830_GC2375A_GPIO_PWDN)' >> $(sensor_gc2375a_init_file)
	$(Q)echo -n ' i2c_sel_gpio=$(MD_X1830_GC2375A_GPIO_I2C_SEL)' >> $(sensor_gc2375a_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X1830_GC2375A_I2C_BUSNUM)' >> $(sensor_gc2375a_init_file)
	$(Q)echo  >> $(sensor_gc2375a_init_file)
endef
