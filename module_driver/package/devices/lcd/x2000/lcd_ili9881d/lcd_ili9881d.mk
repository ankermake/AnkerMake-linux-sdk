#-------------------------------------------------------
package_name = lcd_ili9881d
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_ili9881d
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_ili9881d_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_ili9881d_init_file = output/lcd_ili9881d.sh

define lcd_ili9881d_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_ili9881d/lcd_ili9881d.ko output/
	$(Q)echo -n 'insmod lcd_ili9881d.ko' > $(lcd_ili9881d_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_ILI9881D_POWER_EN)' >> $(lcd_ili9881d_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_ILI9881D_RST)' >> $(lcd_ili9881d_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2000_ILI9881D_BACKLIGHT_EN)' >> $(lcd_ili9881d_init_file)
	$(Q)echo -n ' gpio_lcd_regulator_name=$(MD_X2000_ILI9881D_REGULATOR_NAME)' >> $(lcd_ili9881d_init_file)
	$(Q)echo  >> $(lcd_ili9881d_init_file)
endef