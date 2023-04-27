#-------------------------------------------------------
package_name = soc_fb
package_depends = utils
package_module_src = soc/x1600/fb
package_make_hook =
package_init_hook =
package_finalize_hook = soc_fb_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_fb_init_file = output/soc_fb.sh

define soc_fb_finalize_hook
	$(Q)cp soc/x1600/fb/soc_fb.ko output/
	$(Q)echo -n 'insmod soc_fb.ko ' > $(soc_fb_init_file)
	$(Q)echo -n 'lcd_is_inited=$(if $(MD_X1600_RTOS_BOOT_LOGO_FOR_KERNEL),1,0) ' >> $(soc_fb_init_file)
	$(Q)echo -n 'frame_num=$(MD_X1600_FB_FRAME_NUM) ' >> $(soc_fb_init_file)
	$(Q)echo -n 'pan_display_sync=$(if $(MD_X1600_FB_PAN_DISPLAY_SYNC),1,0) ' >> $(soc_fb_init_file)
	$(Q)echo >> $(soc_fb_init_file)
endef
