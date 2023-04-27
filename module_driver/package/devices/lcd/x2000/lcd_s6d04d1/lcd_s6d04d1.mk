
#-------------------------------------------------------
package_name = lcd_s6d04d1
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_s6d04d1
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_s6d04d1_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_s6d04d1_init_file = output/lcd_s6d04d1.sh

define lcd_s6d04d1_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_s6d04d1/lcd_s6d04d1.ko output/
	$(Q)echo -n 'insmod lcd_s6d04d1.ko' > $(lcd_s6d04d1_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_S6D04D1_RST)' >> $(lcd_s6d04d1_init_file)
	$(Q)echo -n ' gpio_lcd_cs=$(MD_X2000_S6D04D1_CS)' >> $(lcd_s6d04d1_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_S6D04D1_POWER_EN)' >> $(lcd_s6d04d1_init_file)
	$(Q)echo  >> $(lcd_s6d04d1_init_file)
endef
