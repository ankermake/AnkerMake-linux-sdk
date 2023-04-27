#-------------------------------------------------------
package_name = lcd_cc0702
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_cc0702
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_cc0702_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_cc0702_init_file = output/lcd_cc0702.sh

define lcd_cc0702_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_cc0702/lcd_cc0702.ko output/
	$(Q)echo -n 'insmod lcd_cc0702.ko' > $(lcd_cc0702_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_CC0702_POWER_EN)' >> $(lcd_cc0702_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_CC0702_RST)' >> $(lcd_cc0702_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2000_CC0702_BACKLIGHT_EN)' >> $(lcd_cc0702_init_file)
	$(Q)echo  >> $(lcd_cc0702_init_file)
endef