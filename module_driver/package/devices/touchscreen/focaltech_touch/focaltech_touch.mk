#-------------------------------------------------------
package_name = focaltech_touch
package_depends = utils
package_module_src = devices/touchscreen/focaltech_touch/
package_make_hook =
package_init_hook =
package_finalize_hook = focaltech_touch_finalize_hook
package_clean_hook =
#-------------------------------------------------------

focaltech_touch_init_file = output/focaltech_touch.sh


define focaltech_touch_finalize_hook
	$(Q)cp devices/touchscreen/focaltech_touch/focaltech_touch.ko output/
	$(Q)echo "insmod focaltech_touch.ko \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_i2c_bus_num=$(MD_FTS_I2C_BUSNUM) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_x_coords_min=$(MD_FTS_X_COORDS_MIN) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_y_coords_min=$(MD_FTS_Y_COORDS_MIN) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_x_coords_max=$(MD_FTS_X_COORDS_MAX) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_y_coords_max=$(MD_FTS_Y_COORDS_MAX) \\" >> $(focaltech_touch_init_file)

	$(Q)echo " fts_x_coords_flip=$(if $(MD_FTS_X_COORDS_FLIP),1,0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_y_coords_flip=$(if $(MD_FTS_Y_COORDS_FLIP),1,0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_x_y_coords_exchange=$(if $(MD_FTS_X_Y_COORDS_EXCHANGE),1,0) \\" >> $(focaltech_touch_init_file)

	$(Q)echo " fts_reset_gpio=$(MD_FTS_RESET_GPIO) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_irq_gpio=$(MD_FTS_IRQ_GPIO) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_max_touch_number=$(MD_FTS_MAX_TOUCH_NUMBER) \\" >> $(focaltech_touch_init_file)

	$(Q)echo " fts_have_key=$(if $(MD_FTS_HAVE_KEY),$(MD_FTS_HAVE_KEY),n) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key_number=$(if $(MD_FTS_KEY_NUMBER),$(MD_FTS_KEY_NUMBER),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key0_code=$(if $(MD_FTS_KEY0_CODE),$(MD_FTS_KEY0_CODE),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key1_code=$(if $(MD_FTS_KEY1_CODE),$(MD_FTS_KEY1_CODE),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key2_code=$(if $(MD_FTS_KEY2_CODE),$(MD_FTS_KEY2_CODE),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key3_code=$(if $(MD_FTS_KEY3_CODE),$(MD_FTS_KEY3_CODE),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key0_x_coords=$(if $(MD_FTS_KEY0_X_COORDS),$(MD_FTS_KEY0_X_COORDS),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key1_x_coords=$(if $(MD_FTS_KEY1_X_COORDS),$(MD_FTS_KEY1_X_COORDS),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key2_x_coords=$(if $(MD_FTS_KEY2_X_COORDS),$(MD_FTS_KEY2_X_COORDS),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key3_x_coords=$(if $(MD_FTS_KEY3_X_COORDS),$(MD_FTS_KEY3_X_COORDS),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key0_y_coords=$(if $(MD_FTS_KEY0_Y_COORDS),$(MD_FTS_KEY0_Y_COORDS),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key1_y_coords=$(if $(MD_FTS_KEY1_Y_COORDS),$(MD_FTS_KEY1_Y_COORDS),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key2_y_coords=$(if $(MD_FTS_KEY2_Y_COORDS),$(MD_FTS_KEY2_Y_COORDS),0) \\" >> $(focaltech_touch_init_file)
	$(Q)echo " fts_key3_y_coords=$(if $(MD_FTS_KEY3_Y_COORDS),$(MD_FTS_KEY3_Y_COORDS),0) \\" >> $(focaltech_touch_init_file)

	$(Q)echo >> $(focaltech_touch_init_file)
endef