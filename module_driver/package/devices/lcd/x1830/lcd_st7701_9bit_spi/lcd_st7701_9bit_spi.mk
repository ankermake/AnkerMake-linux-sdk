
#-------------------------------------------------------
package_name = lcd_st7701_9bit_spi
package_depends = utils soc_fb
package_module_src = devices/lcd/x1830/lcd_st7701_9bit_spi
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_st7701_9bit_spi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_st7701_9bit_spi_init_file = output/lcd_st7701_9bit_spi.sh

define lcd_st7701_9bit_spi_finalize_hook
	$(Q)cp devices/lcd/x1830/lcd_st7701_9bit_spi/lcd_st7701_9bit_spi.ko output/
	$(Q)echo -n 'insmod lcd_st7701_9bit_spi.ko' > $(lcd_st7701_9bit_spi_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X1830_ST7701_9BIT_SPI_POWER_EN)' >> $(lcd_st7701_9bit_spi_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X1830_ST7701_9BIT_SPI_RST)' >> $(lcd_st7701_9bit_spi_init_file)
	$(Q)echo -n ' gpio_lcd_scl=$(MD_X1830_ST7701_9BIT_SPI_SCL)' >> $(lcd_st7701_9bit_spi_init_file)
	$(Q)echo -n ' gpio_lcd_sda=$(MD_X1830_ST7701_9BIT_SPI_SDA)' >> $(lcd_st7701_9bit_spi_init_file)
	$(Q)echo -n ' gpio_lcd_cs=$(MD_X1830_ST7701_9BIT_SPI_CS)' >> $(lcd_st7701_9bit_spi_init_file)
	$(Q)echo  >> $(lcd_st7701_9bit_spi_init_file)
endef
