#-------------------------------------------------------
package_name = lcd_st7785m
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_st7785m
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_st7785m_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_st7785m_init_file = output/lcd_st7785m.sh

define lcd_st7785m_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_st7785m/lcd_st7785m.ko output/
	$(Q)echo -n 'insmod lcd_st7785m.ko' > $(lcd_st7785m_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_ST7785M_POWER_EN)' >> $(lcd_st7785m_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_ST7785M_RST)' >> $(lcd_st7785m_init_file)
	$(Q)echo -n ' lcd_regulator_name=$(MD_X2000_LCD_REGULATOR_NAME)' >> $(lcd_st7785m_init_file)
	$(Q)echo  >> $(lcd_st7785m_init_file)
endef