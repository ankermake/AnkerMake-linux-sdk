#-------------------------------------------------------
package_name = lcd_otm8018b
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_otm8018b
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_otm8018b_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_otm8018b_init_file = output/lcd_otm8018b.sh

define lcd_otm8018b_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_otm8018b/lcd_otm8018b.ko output/
	$(Q)echo -n 'insmod lcd_otm8018b.ko' > $(lcd_otm8018b_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_OTM8018B_POWER_EN)' >> $(lcd_otm8018b_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_OTM8018B_RST)' >> $(lcd_otm8018b_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2000_OTM8018B_BACKLIGHT_EN)' >> $(lcd_otm8018b_init_file)
	$(Q)echo  >> $(lcd_otm8018b_init_file)
endef