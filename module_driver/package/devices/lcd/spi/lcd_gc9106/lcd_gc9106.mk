
#-------------------------------------------------------
package_name = lcd_gc9106
package_depends = utils spi_fb
package_module_src = devices/lcd/spi/lcd_gc9106
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_gc9106_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_gc9106_init_file = output/lcd_gc9106.sh

define lcd_gc9106_finalize_hook
    $(Q)cp devices/lcd/spi/lcd_gc9106/lcd_gc9106.ko output/
    $(Q)echo -n 'insmod lcd_gc9106.ko' > $(lcd_gc9106_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_GC9106_POWER_EN)' >> $(lcd_gc9106_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_GC9106_RST)' >> $(lcd_gc9106_init_file)
    $(Q)echo -n ' spi_bus_num=$(MD_X2000_GC9106_SPI_BUS_NUM)' >> $(lcd_gc9106_init_file)
    $(Q)echo -n ' gpio_spi_cs=$(MD_X2000_GC9106_SPI_CS)' >> $(lcd_gc9106_init_file)
    $(Q)echo -n ' gpio_spi_rs=$(MD_X2000_GC9106_SPI_RS)' >> $(lcd_gc9106_init_file)
    $(Q)echo  >> $(lcd_gc9106_init_file)
endef
