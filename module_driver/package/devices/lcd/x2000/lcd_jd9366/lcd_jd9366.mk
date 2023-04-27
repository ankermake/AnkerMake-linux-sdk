#-------------------------------------------------------
package_name = lcd_jd9366
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_jd9366
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_jd9366_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_jd9366_init_file = output/lcd_jd9366.sh

define lcd_jd9366_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_jd9366/lcd_jd9366.ko output/
	$(Q)echo -n 'insmod lcd_jd9366.ko' > $(lcd_jd9366_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_JD9366_POWER_EN)' >> $(lcd_jd9366_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_JD9366_RST)' >> $(lcd_jd9366_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2000_JD9366_BACKLIGHT_EN)' >> $(lcd_jd9366_init_file)
	$(Q)echo  >> $(lcd_jd9366_init_file)
endef