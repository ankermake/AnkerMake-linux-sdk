
#-------------------------------------------------------
package_name = soc_spdif
package_depends = soc_audio_dma
package_module_src = soc/x2000_510/spdif/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_spdif_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_spdif_init_file = output/soc_spdif.sh

define soc_spdif_finalize_hook
	$(Q)cp soc/x2000_510/spdif/soc_spdif.ko output/
	$(Q)echo 'insmod soc_spdif.ko ' > $(soc_spdif_init_file)
endef
