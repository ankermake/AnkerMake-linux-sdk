#-------------------------------------------------------
package_name = lcd_st7701s
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_st7701s
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_st7701s_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_st7701s_init_file = output/lcd_st7701s.sh

define lcd_st7701s_finalize_hook
    $(Q)cp devices/lcd/x2000/lcd_st7701s/lcd_st7701s.ko output/
    $(Q)echo -n 'insmod lcd_st7701s.ko' > $(lcd_st7701s_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_ST7701S_POWER_EN)' >> $(lcd_st7701s_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_ST7701S_RST)' >> $(lcd_st7701s_init_file)
    $(Q)echo -n ' gpio_lcd_te=$(MD_X2000_ST7701S_TE)' >> $(lcd_st7701s_init_file)
    $(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2000_ST7701S_LCD_BACKLIGHT_ENABLE)' >> $(lcd_st7701s_init_file)
    $(Q)echo -n ' lcd_regulator_name=$(MD_X2000_ST7701S_REGULATOR_NAME)' >> $(lcd_st7701s_init_file)
    $(Q)echo  >> $(lcd_st7701s_init_file)
endef
