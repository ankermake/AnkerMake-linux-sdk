
#-------------------------------------------------------
package_name = lcd_kwh035
package_depends = utils soc_fb
package_module_src = devices/lcd/x1830/lcd_kwh035
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_kwh035_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_kwh035_init_file = output/lcd_kwh035.sh

define lcd_kwh035_finalize_hook
	$(Q)cp devices/lcd/x1830/lcd_kwh035/lcd_kwh035.ko output/
	$(Q)echo -n 'insmod lcd_kwh035.ko' > $(lcd_kwh035_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X1830_KWH035_POWER_EN)' >> $(lcd_kwh035_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X1830_KWH035_RST)' >> $(lcd_kwh035_init_file)
	$(Q)echo -n ' gpio_lcd_cs=$(MD_X1830_KWH035_CS)' >> $(lcd_kwh035_init_file)
	$(Q)echo -n ' gpio_lcd_rd=$(MD_X1830_KWH035_RD)' >> $(lcd_kwh035_init_file)
	$(Q)echo  >> $(lcd_kwh035_init_file)
endef
