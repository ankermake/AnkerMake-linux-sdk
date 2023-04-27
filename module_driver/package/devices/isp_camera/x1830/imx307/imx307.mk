

#-------------------------------------------------------
package_name = sensor_imx307
package_module_src = devices/isp_camera/x1830/imx307
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_imx307_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_imx307_init_file = output/sensor_imx307.sh

define sensor_imx307_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/isp_camera/x1830/imx307/imx307.bin $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_imx307_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/imx307.bin
endef

TARGET_INSTALL_HOOKS += sensor_imx307_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_imx307_install_clean_hook

define sensor_imx307_finalize_hook
	$(Q)cp devices/isp_camera/x1830/imx307/sensor_imx307.ko output/
	$(Q)echo -n 'insmod sensor_imx307.ko' > $(sensor_imx307_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X1830_IMX307_GPIO_POWER)' >> $(sensor_imx307_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X1830_IMX307_GPIO_RESET)' >> $(sensor_imx307_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X1830_IMX307_GPIO_PWDN)' >> $(sensor_imx307_init_file)
	$(Q)echo -n ' i2c_sel_gpio=$(MD_X1830_IMX307_GPIO_I2C_SEL)' >> $(sensor_imx307_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X1830_IMX307_I2C_BUSNUM)' >> $(sensor_imx307_init_file)
	$(Q)echo  >> $(sensor_imx307_init_file)
endef
