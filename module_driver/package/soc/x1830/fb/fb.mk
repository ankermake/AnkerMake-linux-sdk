
#-------------------------------------------------------
package_name = soc_fb
package_depends = utils
package_module_src = soc/x1830/fb
package_make_hook =
package_init_hook =
package_finalize_hook = soc_fb_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_fb_init_file = output/soc_fb.sh

define soc_fb_finalize_hook
	$(Q)cp soc/x1830/fb/soc_fb.ko output/
	$(Q)echo -n 'insmod soc_fb.ko' > $(soc_fb_init_file)
	$(Q)echo -n ' layer0_alpha=$(MD_X1830_FB_LAYER0_ALPHA)' >> $(soc_fb_init_file)
	$(Q)echo -n ' layer0_frames=$(MD_X1830_FB_LAYER0_FRAMES)' >> $(soc_fb_init_file)
	$(Q)echo -n ' layer1_enable=$(if $(MD_X1830_FB_LAYER1_ENABLE),1,0)' >> $(soc_fb_init_file)
	$(Q)echo -n ' layer1_frames=$(MD_X1830_FB_LAYER1_FRAMES)' >> $(soc_fb_init_file)
	$(Q)echo -n ' pan_display_sync=$(if $(MD_X1830_FB_PAN_DISPLAY_SYNC),1,0)' >> $(soc_fb_init_file)
	$(Q)echo  >> $(soc_fb_init_file)

endef
