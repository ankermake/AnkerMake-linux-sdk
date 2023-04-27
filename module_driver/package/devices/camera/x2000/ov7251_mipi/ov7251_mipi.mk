#-------------------------------------------------------
package_name = sensor_ov7251_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2000/ov7251_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov7251_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ov7251_mipi_init_file = output/sensor_ov7251_mipi.sh

define sensor_gc7251_mipi_install_hook
    $(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
    $(if $(MD_X2000_SENSOR_OV7251_MIPI), $(Q)cp -rf package/devices/camera/x2000/ov7251_mipi/ov7251-x2000.bin  $(FS_TARGET_DIR)/etc/sensor/)
endef

define sensor_gc7251_mipi_install_clean_hook
    $(if $(MD_X2000_SENSOR_OV7251_MIPI), $(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/ov7251-x2000.bin)
endef

TARGET_INSTALL_HOOKS += sensor_gc7251_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_gc7251_mipi_install_clean_hook

define sensor_ov7251_mipi_finalize_hook
    $(Q)cp devices/camera/x2000/ov7251_mipi/sensor_ov7251_mipi.ko output/
    $(Q)echo -n 'insmod sensor_ov7251_mipi.ko' > $(sensor_ov7251_mipi_init_file)
    $(Q)echo -n ' camera_index=$(MD_X2000_OV7251_CAMERA_INDEX)' >> $(sensor_ov7251_mipi_init_file)
    $(Q)echo -n ' reset_gpio=$(MD_X2000_OV7251_GPIO_RESET)' >> $(sensor_ov7251_mipi_init_file)
    $(Q)echo -n ' i2c_bus_num=$(MD_X2000_OV7251_I2C_BUSNUM)' >> $(sensor_ov7251_mipi_init_file)
    $(Q)echo -n ' resolution=$(MD_X2000_SENSOR_OV7251_RESOLUTION)' >> $(sensor_ov7251_mipi_init_file)
    $(Q)echo  >> $(sensor_ov7251_mipi_init_file)
endef
