#-------------------------------------------------------
package_name = sensor_ov9282_dvp
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/ov9282_dvp
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov9282_dvp_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_ov9282_dvp.sh

define sensor_ov9282_dvp_finalize_hook
	$(Q)cp devices/camera/x2000/ov9282_dvp/sensor_ov9282_dvp.ko output/
	$(Q)echo -n 'insmod sensor_ov9282_dvp.ko' > $(shell_sensor_init_file)
	$(Q)echo -n ' sensor_name=$(MD_X2000_OV9282_SENSOR_NAME)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2000_OV9282_GPIO_POWER)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2000_OV9282_GPIO_RESET)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2000_OV9282_GPIO_PWDN)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_OV9282_I2C_BUSNUM)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' i2c_addr=$(MD_X2000_OV9282_I2C_ADDR)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' camera_index=$(MD_X2000_OV9282_CAMERA_INDEX)' >> $(shell_sensor_init_file)
	$(Q)echo  >> $(shell_sensor_init_file)
endef
