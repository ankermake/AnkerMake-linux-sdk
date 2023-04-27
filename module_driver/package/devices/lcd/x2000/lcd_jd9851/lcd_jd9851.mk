#-------------------------------------------------------
package_name = lcd_jd9851
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_jd9851
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_jd9851_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_jd9851_init_file = output/lcd_jd9851.sh

define lcd_jd9851_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_jd9851/lcd_jd9851.ko output/
	$(Q)echo -n 'insmod lcd_jd9851.ko' > $(lcd_jd9851_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_JD9851_POWER_EN)' >> $(lcd_jd9851_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_JD9851_RST)' >> $(lcd_jd9851_init_file)
	$(Q)echo -n ' gpio_lcd_cs=$(MD_X2000_JD9851_CS)' >> $(lcd_jd9851_init_file)
	$(Q)echo -n ' gpio_lcd_rd=$(MD_X2000_JD9851_RD)' >> $(lcd_jd9851_init_file)
	$(Q)echo -n ' lcd_regulator_name=$(MD_X2000_LCD_REGULATOR_NAME)' >> $(lcd_jd9851_init_file)
	$(Q)echo  >> $(lcd_jd9851_init_file)
endef