#-------------------------------------------------------
package_name = codec_es8388
package_module_src = devices/codec/es8388
package_make_hook =
package_init_hook =
package_finalize_hook = codec_es8388_finalize_hook
package_clean_hook =
#-------------------------------------------------------

codec_es8388_init_file = output/codec_es8388.sh

TARGET_INSTALL_HOOKS +=
TARGET_INSTALL_CLEAN_HOOKS +=

define codec_es8388_finalize_hook
	$(Q)cp devices/codec/es8388/codec_es8388.ko output/
	$(Q)echo -n 'insmod codec_es8388.ko' > $(codec_es8388_init_file)
	$(Q)echo  >> $(codec_es8388_init_file)
endef
