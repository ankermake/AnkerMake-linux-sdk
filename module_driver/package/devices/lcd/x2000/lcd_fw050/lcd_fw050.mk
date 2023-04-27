#-------------------------------------------------------
package_name = lcd_fw050
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_fw050
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_fw050_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_fw050_init_file = output/lcd_fw050.sh

define lcd_fw050_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_fw050/lcd_fw050.ko output/
	$(Q)echo -n 'insmod lcd_fw050.ko' > $(lcd_fw050_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_FW050_POWER_EN)' >> $(lcd_fw050_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_FW050_RST)' >> $(lcd_fw050_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2000_FW050_BACKLIGHT_EN)' >> $(lcd_fw050_init_file)
	$(Q)echo -n ' gpio_lcd_regulator_name=$(MD_X2000_FW050_REGULATOR_NAME)' >> $(lcd_fw050_init_file)
	$(Q)echo  >> $(lcd_fw050_init_file)
endef