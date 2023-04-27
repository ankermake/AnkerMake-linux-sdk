#-------------------------------------------------------
package_name = soc_fb_layer_mixer
package_depends = utils
package_module_src = soc/x2000/fb_layer_mixer
package_make_hook =
package_init_hook =
package_finalize_hook = soc_fb_layer_mixer_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_fb_layer_mixer_init_file = output/soc_fb_layer_mixer.sh

define soc_fb_layer_mixer_finalize_hook
	$(Q)cp soc/x2000/fb_layer_mixer/soc_fb_layer_mixer.ko output/
	$(Q)echo "insmod soc_fb_layer_mixer.ko \\" > $(soc_fb_layer_mixer_init_file)
	$(Q)echo -n " fb_is_srdma=$(if $(MD_X2000_FB_MIXER_ENABLE),1,0)" >> $(soc_fb_layer_mixer_init_file)
	$(Q)echo  >> $(soc_fb_layer_mixer_init_file)

endef