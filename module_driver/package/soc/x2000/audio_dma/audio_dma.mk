
#-------------------------------------------------------
package_name = soc_audio_dma
package_depends =
package_module_src = soc/x2000/audio_dma/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_audio_dma_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_audio_dma_init_file = output/soc_audio_dma.sh

define soc_audio_dma_finalize_hook
	$(Q)cp soc/x2000/audio_dma/soc_audio_dma.ko output/
	$(Q)echo 'insmod soc_audio_dma.ko ' > $(soc_audio_dma_init_file)
endef
