#-------------------------------------------------------
package_name = ft6236_touch
package_depends = utils
package_module_src = devices/touchscreen/ft6236_touch/
package_make_hook =
package_init_hook =
package_finalize_hook = ft6236_touch_finalize_hook
package_clean_hook =
#-------------------------------------------------------

ft6236_touch_init_file = output/ft6236_touch.sh

define ft6236_touch_finalize_hook
	$(Q)cp devices/touchscreen/ft6236_touch/ft6236_touch.ko output/
	$(Q)echo "insmod ft6236_touch.ko \\" > $(ft6236_touch_init_file)
	$(Q)echo " ft6236_i2c_bus_num=$(MD_FT6236_I2C_BUSNUM) \\" >> $(ft6236_touch_init_file)
	$(Q)echo " ft6236_regulator_name=$(MD_FT6236_REGULATOR_NAME) \\" >> $(ft6236_touch_init_file)
	$(Q)echo " ft6236_power_gpio=$(MD_FT6236_GPIO_POWER) \\" >> $(ft6236_touch_init_file)
	$(Q)echo " ft6236_int_gpio=$(MD_FT6236_GPIO_INT) \\" >> $(ft6236_touch_init_file)
	$(Q)echo " ft6236_reset_gpio=$(MD_FT6236_GPIO_RESET) \\" >> $(ft6236_touch_init_file)

	$(Q)echo " ft6236_x_coords_max=$(MD_FT6236_X_COORDS_MAX) \\" >> $(ft6236_touch_init_file)
	$(Q)echo " ft6236_y_coords_max=$(MD_FT6236_Y_COORDS_MAX) \\" >> $(ft6236_touch_init_file)

	$(Q)echo " ft6236_x_coords_flip=$(if $(MD_FT6236_X_COORDS_FLIP),1,0) \\" >> $(ft6236_touch_init_file)
	$(Q)echo " ft6236_y_coords_flip=$(if $(MD_FT6236_Y_COORDS_FLIP),1,0) \\" >> $(ft6236_touch_init_file)
	$(Q)echo " ft6236_x_y_coords_exchange=$(if $(MD_FT6236_X_Y_COORDS_EXCHANGE),1,0) \\" >> $(ft6236_touch_init_file)

	$(Q)echo " ft6236_max_touch_number=$(MD_FT6236_MAX_TOUCH_NUMBER) \\" >> $(ft6236_touch_init_file)

	$(Q)echo >> $(ft6236_touch_init_file)
endef