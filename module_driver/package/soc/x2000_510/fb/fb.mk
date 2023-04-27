
#-------------------------------------------------------
package_name = soc_fb
package_depends = utils soc_fb_layer_mixer soc_rotator
package_module_src = soc/x2000_510/fb
package_make_hook =
package_init_hook =
package_finalize_hook = soc_fb_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_fb_init_file = output/soc_fb.sh

define soc_fb_finalize_hook
	$(Q)cp soc/x2000_510/fb/soc_fb.ko output/
	$(Q)echo "insmod soc_fb.ko \\" > $(soc_fb_init_file)
	$(Q)echo -n " lcd_is_inited=$(if $(MD_X2000_510_RTOS_BOOT_LOGO_FOR_KERNEL),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " pan_display_sync=$(if $(MD_X2000_510_FB_PAN_DISPLAY_SYNC),1,0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " is_rotated=$(if $(MD_X2000_510_FB_ROTATOR),1,0) " >> $(soc_fb_init_file)
	$(Q)echo -n " rotator_angle=$(if $(MD_X2000_510_FB_ROTATOR),$(MD_X2000_510_FB_ROTATOR_ANGLE),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " layer0_enable=$(if $(MD_X2000_510_FB_LAYER0_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer0_frames=$(MD_X2000_510_FB_LAYER0_FRAMES)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer0_alpha=$(MD_X2000_510_FB_LAYER0_ALPHA)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " layer1_enable=$(if $(MD_X2000_510_FB_LAYER1_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer1_frames=$(MD_X2000_510_FB_LAYER1_FRAMES)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer1_alpha=$(MD_X2000_510_FB_LAYER1_ALPHA)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " layer2_enable=$(if $(MD_X2000_510_FB_LAYER2_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer2_frames=$(MD_X2000_510_FB_LAYER2_FRAMES)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer2_alpha=$(MD_X2000_510_FB_LAYER2_ALPHA)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " layer3_enable=$(if $(MD_X2000_510_FB_LAYER3_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer3_frames=$(MD_X2000_510_FB_LAYER3_FRAMES)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer3_alpha=$(MD_X2000_510_FB_LAYER3_ALPHA)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " srdma_enable=$(if $(MD_X2000_510_FB_MIXER_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " srdma_frames=$(if $(MD_X2000_510_FB_MIXER_ENABLE),$(MD_X2000_510_FB_SRDMA_FRAMES),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " mixer_enable=$(if $(MD_X2000_510_FB_MIXER_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " tft_underrun_count=0" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " use_default_order=$(if $(MD_X2000_510_FB_USE_DEFAULT_ORDER),1,0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " user_fb0_enable=$(if $(MD_X2000_510_FB_LAYER0_USER_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_width=$(if $(MD_X2000_510_FB_LAYER0_USER_ENABLE),$(MD_X2000_510_FB_LAYER0_USER_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_height=$(if $(MD_X2000_510_FB_LAYER0_USER_ENABLE),$(MD_X2000_510_FB_LAYER0_USER_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_xpos=$(if $(MD_X2000_510_FB_LAYER0_USER_ENABLE),$(MD_X2000_510_FB_LAYER0_USER_XPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_ypos=$(if $(MD_X2000_510_FB_LAYER0_USER_ENABLE),$(MD_X2000_510_FB_LAYER0_USER_YPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_scaling_enable=$(if $(MD_X2000_510_FB_LAYER0_USER_SCALING_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_scaling_width=$(if $(MD_X2000_510_FB_LAYER0_USER_SCALING_ENABLE),$(MD_X2000_510_FB_LAYER0_USER_SCALING_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_scaling_height=$(if $(MD_X2000_510_FB_LAYER0_USER_SCALING_ENABLE),$(MD_X2000_510_FB_LAYER0_USER_SCALING_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " user_fb1_enable=$(if $(MD_X2000_510_FB_LAYER1_USER_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_width=$(if $(MD_X2000_510_FB_LAYER1_USER_ENABLE),$(MD_X2000_510_FB_LAYER1_USER_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_height=$(if $(MD_X2000_510_FB_LAYER1_USER_ENABLE),$(MD_X2000_510_FB_LAYER1_USER_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_xpos=$(if $(MD_X2000_510_FB_LAYER1_USER_ENABLE),$(MD_X2000_510_FB_LAYER1_USER_XPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_ypos=$(if $(MD_X2000_510_FB_LAYER1_USER_ENABLE),$(MD_X2000_510_FB_LAYER1_USER_YPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_scaling_enable=$(if $(MD_X2000_510_FB_LAYER1_USER_SCALING_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_scaling_width=$(if $(MD_X2000_510_FB_LAYER1_USER_SCALING_ENABLE),$(MD_X2000_510_FB_LAYER1_USER_SCALING_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_scaling_height=$(if $(MD_X2000_510_FB_LAYER1_USER_SCALING_ENABLE),$(MD_X2000_510_FB_LAYER1_USER_SCALING_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " user_fb2_enable=$(if $(MD_X2000_510_FB_LAYER2_USER_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_width=$(if $(MD_X2000_510_FB_LAYER2_USER_ENABLE),$(MD_X2000_510_FB_LAYER2_USER_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_height=$(if $(MD_X2000_510_FB_LAYER2_USER_ENABLE),$(MD_X2000_510_FB_LAYER2_USER_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_xpos=$(if $(MD_X2000_510_FB_LAYER2_USER_ENABLE),$(MD_X2000_510_FB_LAYER2_USER_XPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_ypos=$(if $(MD_X2000_510_FB_LAYER2_USER_ENABLE),$(MD_X2000_510_FB_LAYER2_USER_YPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_scaling_enable=$(if $(MD_X2000_510_FB_LAYER2_USER_SCALING_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_scaling_width=$(if $(MD_X2000_510_FB_LAYER2_USER_SCALING_ENABLE),$(MD_X2000_510_FB_LAYER2_USER_SCALING_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_scaling_height=$(if $(MD_X2000_510_FB_LAYER2_USER_SCALING_ENABLE),$(MD_X2000_510_FB_LAYER2_USER_SCALING_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " user_fb3_enable=$(if $(MD_X2000_510_FB_LAYER3_USER_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_width=$(if $(MD_X2000_510_FB_LAYER3_USER_ENABLE),$(MD_X2000_510_FB_LAYER3_USER_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_height=$(if $(MD_X2000_510_FB_LAYER3_USER_ENABLE),$(MD_X2000_510_FB_LAYER3_USER_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_xpos=$(if $(MD_X2000_510_FB_LAYER3_USER_ENABLE),$(MD_X2000_510_FB_LAYER3_USER_XPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_ypos=$(if $(MD_X2000_510_FB_LAYER3_USER_ENABLE),$(MD_X2000_510_FB_LAYER3_USER_YPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_scaling_enable=$(if $(MD_X2000_510_FB_LAYER3_USER_SCALING_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_scaling_width=$(if $(MD_X2000_510_FB_LAYER3_USER_SCALING_ENABLE),$(MD_X2000_510_FB_LAYER3_USER_SCALING_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_scaling_height=$(if $(MD_X2000_510_FB_LAYER3_USER_SCALING_ENABLE),$(MD_X2000_510_FB_LAYER3_USER_SCALING_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo  >> $(soc_fb_init_file)

endef
