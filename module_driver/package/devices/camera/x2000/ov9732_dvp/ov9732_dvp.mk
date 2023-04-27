#-------------------------------------------------------
package_name = sensor_ov9732_dvp
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/ov9732_dvp
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov9732_dvp_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_ov9732_dvp.sh

define sensor_ov9732_dvp_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)cp -rf package/devices/camera/x2000/ov9732_dvp/$(MD_X2000_OV9732_DVP_SENSOR_NAME)-x2000.bin  $(FS_TARGET_DIR)/etc/sensor/)
endef

define sensor_ov9732_dvp_install_clean_hook
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/$(MD_X2000_OV9732_DVP_SENSOR_NAME)-x2000.bin)
endef

TARGET_INSTALL_HOOKS += sensor_ov9732_dvp_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_ov9732_dvp_install_clean_hook


define sensor_ov9732_dvp_finalize_hook
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)cp devices/camera/x2000/ov9732_dvp/sensor_ov9732_dvp.ko output/)
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)echo -n 'insmod sensor_ov9732_dvp.ko' > $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)echo -n ' power_gpio=$(MD_X2000_OV9732_DVP_GPIO_POWER0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)echo -n ' reset_gpio=$(MD_X2000_OV9732_DVP_GPIO_RESET0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)echo -n ' pwdn_gpio=$(MD_X2000_OV9732_DVP_GPIO_PWDN0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)echo -n ' regulator_name=$(MD_X2000_OV9732_DVP_SENSOR_REGULATOR_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)echo -n ' i2c_bus_num=$(MD_X2000_OV9732_DVP_I2C_BUSNUM0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)echo -n ' i2c_addr=$(MD_X2000_OV9732_DVP_I2C_ADDR0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)echo -n ' cam_bus_num=$(MD_X2000_OV9732_DVP_CAM_BUSNUM0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)echo -n ' sensor_name=$(MD_X2000_OV9732_DVP_SENSOR_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2000_SENSOR_OV9732_DVP), $(Q)echo  >> $(shell_sensor_init_file))
endef