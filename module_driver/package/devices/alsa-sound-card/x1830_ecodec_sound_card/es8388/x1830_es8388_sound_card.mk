
#-------------------------------------------------------
package_name = x1830_es8388_sound_card
package_depends =
package_module_src = devices/alsa-sound-card/x1830_ecodec_sound_card/es8388
package_make_hook =
package_init_hook =
package_finalize_hook = x1830_es8388_sound_card_finalize_hook
package_clean_hook =
#-------------------------------------------------------

x1830_es8388_sound_card_init_file = output/x1830_es8388_sound_card.sh

TARGET_INSTALL_HOOKS +=
TARGET_INSTALL_CLEAN_HOOKS +=

define x1830_es8388_sound_card_finalize_hook
	$(Q)cp devices/alsa-sound-card/x1830_ecodec_sound_card/es8388/x1830_es8388_sound_card.ko output/
	$(Q)echo 'insmod x1830_es8388_sound_card.ko ' > $(x1830_es8388_sound_card_init_file)
endef
