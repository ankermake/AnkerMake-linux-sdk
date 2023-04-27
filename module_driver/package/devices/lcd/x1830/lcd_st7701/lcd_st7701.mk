
#-------------------------------------------------------
package_name = lcd_st7701
package_depends = utils soc_fb
package_module_src = devices/lcd/x1830/lcd_st7701
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_st7701_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_st7701_init_file = output/lcd_st7701.sh

define lcd_st7701_finalize_hook
	$(Q)cp devices/lcd/x1830/lcd_st7701/lcd_st7701.ko output/
	$(Q)echo -n 'insmod lcd_st7701.ko' > $(lcd_st7701_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X1830_ST7701_POWER_EN)' >> $(lcd_st7701_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X1830_ST7701_RST)' >> $(lcd_st7701_init_file)
	$(Q)echo -n ' gpio_spi_cs=$(MD_X1830_ST7701_SPI_CS)' >> $(lcd_st7701_init_file)
	$(Q)echo -n ' spi_bus_num=$(MD_X1830_ST7701_SPI_BUS_NUM)' >> $(lcd_st7701_init_file)
	$(Q)echo  >> $(lcd_st7701_init_file)
endef
