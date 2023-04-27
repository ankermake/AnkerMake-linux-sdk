#-------------------------------------------------------
package_name = lcd_truly240240
package_depends = utils soc_fb
package_module_src = devices/lcd/x1000/lcd_truly240240/
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_truly240240_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_truly240240_init_file = output/lcd_truly240240.sh

define lcd_truly240240_finalize_hook
	$(Q)cp devices/lcd/x1000/lcd_truly240240/lcd_truly240240.ko output/
	$(Q)echo -n 'insmod lcd_truly240240.ko ' > $(lcd_truly240240_init_file)
	$(Q)echo -n 'gpio_lcd_cs=$(MD_X1000_TRULY240240_CS) ' >> $(lcd_truly240240_init_file)
	$(Q)echo -n 'gpio_lcd_rst=$(MD_X1000_TRULY240240_RST) ' >> $(lcd_truly240240_init_file)
	$(Q)echo -n 'gpio_lcd_rd=$(MD_X1000_TRULY240240_RD) ' >> $(lcd_truly240240_init_file)
	$(Q)echo -n 'gpio_lcd_te=$(MD_X1000_TRULY240240_TE) ' >> $(lcd_truly240240_init_file)
	$(Q)echo >> $(lcd_truly240240_init_file)
endef