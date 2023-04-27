
#-------------------------------------------------------
package_name = lcd_gl0700
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_gl0700
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_gl0700_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_gl0700_init_file = output/lcd_gl0700.sh

define lcd_gl0700_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_gl0700/lcd_gl0700.ko output/
	$(Q)echo -n 'insmod lcd_gl0700.ko' > $(lcd_gl0700_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_GL0700_POWER_EN)' >> $(lcd_gl0700_init_file)
	$(Q)echo  >> $(lcd_gl0700_init_file)
endef
