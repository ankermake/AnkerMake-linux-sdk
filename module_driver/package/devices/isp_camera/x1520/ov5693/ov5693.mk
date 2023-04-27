#-------------------------------------------------------
package_name = sensor_ov5693
package_module_src = devices/isp_camera/x1520/ov5693
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov5693_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ov5693_init_file = output/sensor_ov5693.sh

define sensor_ov5693_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	# $(Q)cp -rf package/devices/isp_camera/x1520/ov5693/ov5693.bin  $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_ov5693_install_clean_hook
	# $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/ov5693.bin
endef

TARGET_INSTALL_HOOKS += sensor_ov5693_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_ov5693_install_clean_hook

define sensor_ov5693_finalize_hook
	$(Q)cp devices/isp_camera/x1520/ov5693/sensor_ov5693.ko output/
	$(Q)echo -n 'insmod sensor_ov5693.ko' > $(sensor_ov5693_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1520_OV5693_GPIO_RESET)' >> $(sensor_ov5693_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1520_OV5693_GPIO_PWDN)' >> $(sensor_ov5693_init_file)
	$(Q)echo  >> $(sensor_ov5693_init_file)
endef