#-------------------------------------------------------
package_name = lcd_kd070d57
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_kd070d57
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_kd070d57_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_kd070d57_init_file = output/lcd_kd070d57.sh

define lcd_kd070d57_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_kd070d57/lcd_kd070d57.ko output/
	$(Q)echo -n 'insmod lcd_kd070d57.ko' > $(lcd_kd070d57_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_KD070D57_POWER_EN)' >> $(lcd_kd070d57_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_KD070D57_RST)' >> $(lcd_kd070d57_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2000_KD070D57_BACKLIGHT_EN)' >> $(lcd_kd070d57_init_file)
	$(Q)echo  >> $(lcd_kd070d57_init_file)
endef