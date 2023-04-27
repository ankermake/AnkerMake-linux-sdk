#-------------------------------------------------------
package_name = lcd_ma0060
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_ma0060
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_ma0060_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_ma0060_init_file = output/lcd_ma0060.sh

define lcd_ma0060_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_ma0060/lcd_ma0060.ko output/
	$(Q)echo -n 'insmod lcd_ma0060.ko' > $(lcd_ma0060_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_MA0060_POWER_EN)' >> $(lcd_ma0060_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_MA0060_RST)' >> $(lcd_ma0060_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2000_MA0060_BACKLIGHT_EN)' >> $(lcd_ma0060_init_file)
	$(Q)echo -n ' power_valid_level=$(MD_X2000_MA0060_POWER_VALID)' >> $(lcd_ma0060_init_file)
	$(Q)echo  >> $(lcd_ma0060_init_file)
endef