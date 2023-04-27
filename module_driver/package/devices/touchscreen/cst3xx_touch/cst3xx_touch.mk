#-------------------------------------------------------
package_name = cst3xx_touch
package_depends = utils
package_module_src = devices/touchscreen/cst3xx_touch/
package_make_hook =
package_init_hook =
package_finalize_hook = cst3xx_touch_finalize_hook
package_clean_hook =
#-------------------------------------------------------

cst3xx_touch_init_file = output/cst3xx_touch.sh

define cst3xx_touch_finalize_hook
	$(Q)cp devices/touchscreen/cst3xx_touch/cst3xx_touch.ko output/
	$(Q)echo 'insmod cst3xx_touch.ko \\' >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_i2c_bus_num=$(MD_CST_I2C_BUSNUM) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_i2c_addr=$(MD_CST_I2C_ADDR) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_x_coords_min=$(MD_CST_X_COORDS_MIN) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_y_coords_min=$(MD_CST_Y_COORDS_MIN) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_x_coords_max=$(MD_CST_X_COORDS_MAX) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_y_coords_max=$(MD_CST_Y_COORDS_MAX) \\" >> $(cst3xx_touch_init_file)

	$(Q)echo " cst_x_coords_flip=$(if $(MD_CST_X_COORDS_FLIP),1,0) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_y_coords_flip=$(if $(MD_CST_Y_COORDS_FLIP),1,0) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_x_y_coords_exchange=$(if $(MD_CST_X_Y_COORDS_EXCHANGE),1,0) \\" >> $(cst3xx_touch_init_file)

	$(Q)echo " cst_reset_gpio=$(MD_CST_RESET_GPIO) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_irq_gpio=$(MD_CST_IRQ_GPIO) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_power_en_gpio=$(MD_CST_I2C_POWER_ENABLE_GPIO) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_power_en_level=$(MD_CST_I2C_POWER_ENABLE_LEVEL) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_max_touch_number=$(MD_CST_MAX_TOUCH_NUMBER) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo " cst_regulator_name=$(MD_CST_REGULATOR_NAME) \\" >> $(cst3xx_touch_init_file)
	$(Q)echo >> $(cst3xx_touch_init_file)
endef