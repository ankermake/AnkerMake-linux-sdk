#-------------------------------------------------------
package_name = sensor_sc132
package_module_src = devices/isp_camera/x1520/sc132
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_sc132_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_sc132_init_file = output/sensor_sc132.sh

define sensor_sc132_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	# $(Q)cp -rf package/devices/isp_camera/x1520/sc132/sc132.bin  $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_sc132_install_clean_hook
	# $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/sc132.bin
endef

TARGET_INSTALL_HOOKS += sensor_sc132_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_sc132_install_clean_hook

define sensor_sc132_finalize_hook
	$(Q)cp devices/isp_camera/x1520/sc132/sensor_sc132.ko output/
	$(Q)echo -n 'insmod sensor_sc132.ko' > $(sensor_sc132_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1520_SC132_GPIO_RESET)' >> $(sensor_sc132_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1520_SC132_GPIO_PWDN)' >> $(sensor_sc132_init_file)
	$(Q)echo  >> $(sensor_sc132_init_file)
endef