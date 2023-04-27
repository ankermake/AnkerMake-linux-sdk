#-------------------------------------------------------
package_name = gt9xx_touch
package_depends = utils
package_module_src = devices/touchscreen/gt9xx_touch/
package_make_hook =
package_init_hook =
package_finalize_hook = gt9xx_touch_finalize_hook
package_clean_hook =
#-------------------------------------------------------

gt9xx_touch_init_file = output/gt9xx_touch.sh

define gt9xx_touch_finalize_hook
	$(Q)cp devices/touchscreen/gt9xx_touch/gt9xx_touch.ko output/
	$(Q)echo "insmod gt9xx_touch.ko \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_i2c_bus_num=$(MD_GTP_I2C_BUSNUM) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_x_coords_min=$(MD_GTP_X_COORDS_MIN) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_y_coords_min=$(MD_GTP_Y_COORDS_MIN) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_x_coords_max=$(MD_GTP_X_COORDS_MAX) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_y_coords_max=$(MD_GTP_Y_COORDS_MAX) \\" >> $(gt9xx_touch_init_file)

	$(Q)echo " gtp_x_coords_flip=$(if $(MD_GTP_X_COORDS_FLIP),1,0) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_y_coords_flip=$(if $(MD_GTP_Y_COORDS_FLIP),1,0) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_x_y_coords_exchange=$(if $(MD_GTP_X_Y_COORDS_EXCHANGE),1,0) \\" >> $(gt9xx_touch_init_file)

	$(Q)echo " gtp_version=$(MD_GTP_VERSION) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_regulator_name=$(MD_GTP_REGULATOR_NAME) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_power_on_gpio=$(MD_GTP_POWER_EN_GPIO) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_reset_gpio=$(MD_GTP_RESET_GPIO) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_irq_gpio=$(MD_GTP_IRQ_GPIO) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo " gtp_max_touch_number=$(MD_GTP_MAX_TOUCH_NUMBER) \\" >> $(gt9xx_touch_init_file)
	$(Q)echo >> $(gt9xx_touch_init_file)
endef