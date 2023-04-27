
#-------------------------------------------------------
package_name = x1600_510_dummy_sound_card
package_depends = utils
package_module_src = devices/alsa-sound-card/x1600_510_dummy_sound_card/
package_make_hook =
package_init_hook =
package_finalize_hook = x1600_510_dummy_sound_card_finalize_hook
package_clean_hook =
#-------------------------------------------------------

x1600_510_dummy_sound_card_init_file = output/x1600_510_dummy_sound_card.sh

define x1600_510_dummy_sound_card_finalize_hook
	$(Q)cp devices/alsa-sound-card/x1600_510_dummy_sound_card/x1600_510_dummy_sound_card.ko output/
	$(Q)echo 'insmod x1600_510_dummy_sound_card.ko ' > $(x1600_510_dummy_sound_card_init_file)
endef
