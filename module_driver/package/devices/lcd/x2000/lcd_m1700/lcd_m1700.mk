
#-------------------------------------------------------
package_name = lcd_m1700
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_m1700
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_m1700_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_m1700_init_file = output/lcd_m1700.sh

define lcd_m1700_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_m1700/lcd_m1700.ko output/
	$(Q)echo -n 'insmod lcd_m1700.ko' > $(lcd_m1700_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_M1700_POWER_EN)' >> $(lcd_m1700_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_M1700_RST)' >> $(lcd_m1700_init_file)
	$(Q)echo -n ' gpio_lcd_scl=$(MD_X2000_M1700_SPI_SCL)' >> $(lcd_m1700_init_file)
	$(Q)echo -n ' gpio_lcd_sda=$(MD_X2000_M1700_SPI_SDA)' >> $(lcd_m1700_init_file)
	$(Q)echo -n ' gpio_lcd_cs=$(MD_X2000_M1700_SPI_CS)' >> $(lcd_m1700_init_file)
	$(Q)echo  >> $(lcd_m1700_init_file)
endef
