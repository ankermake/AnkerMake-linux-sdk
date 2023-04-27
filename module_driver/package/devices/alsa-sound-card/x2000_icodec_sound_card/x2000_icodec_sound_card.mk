
#-------------------------------------------------------
package_name = x2000_icodec_sound_card
package_depends =
package_module_src = devices/alsa-sound-card/x2000_icodec_sound_card/
package_make_hook =
package_init_hook =
package_finalize_hook = x2000_icodec_sound_card_finalize_hook
package_clean_hook =
#-------------------------------------------------------

x2000_icodec_sound_card_init_file = output/x2000_icodec_sound_card.sh

define x2000_icodec_sound_card_finalize_hook
	$(Q)cp devices/alsa-sound-card/x2000_icodec_sound_card/x2000_icodec_sound_card.ko output/
	$(Q)echo 'insmod x2000_icodec_sound_card.ko ' > $(x2000_icodec_sound_card_init_file)
endef