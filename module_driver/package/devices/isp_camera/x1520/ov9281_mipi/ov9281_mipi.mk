

#-------------------------------------------------------
package_name = sensor_ov9281_mipi
package_module_src = devices/isp_camera/x1520/ov9281_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov9281_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ov9281_init_file = output/sensor_ov9281_mipi.sh

define sensor_ov9281_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/isp_camera/x1520/ov9281_mipi/ov9281.bin  $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_ov9281_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/ov9281.bin
endef

TARGET_INSTALL_HOOKS += sensor_ov9281_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_ov9281_install_clean_hook

define sensor_ov9281_finalize_hook
	$(Q)cp devices/isp_camera/x1520/ov9281_mipi/sensor_ov9281_mipi.ko output/
	$(Q)echo -n 'insmod sensor_ov9281_mipi.ko' > $(sensor_ov9281_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1520_OV9281_GPIO_RESET)' >> $(sensor_ov9281_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1520_OV9281_GPIO_PWDN)' >> $(sensor_ov9281_init_file)
	$(Q)echo  >> $(sensor_ov9281_init_file)
endef
