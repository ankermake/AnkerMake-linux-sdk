
#-------------------------------------------------------
package_name = lcd_spi_gc9106
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_gc9106
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_spi_gc9106_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_spi_gc9106_init_file = output/lcd_spi_gc9106.sh

define lcd_spi_gc9106_finalize_hook
    $(Q)cp devices/lcd/x2000/lcd_gc9106/lcd_spi_gc9106.ko output/
    $(Q)echo -n 'insmod lcd_spi_gc9106.ko' > $(lcd_spi_gc9106_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_LCD_SPI_GC9106_RST)' >> $(lcd_spi_gc9106_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_LCD_SPI_GC9106_POWER_EN)' >> $(lcd_spi_gc9106_init_file)
    $(Q)echo -n ' gpio_spi_cs=$(MD_X2000_LCD_SPI_GC9106_CE)' >> $(lcd_spi_gc9106_init_file)
    $(Q)echo  >> $(lcd_spi_gc9106_init_file)
endef
