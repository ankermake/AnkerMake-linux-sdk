ifeq ($(APP_br_asound_conf_dmix),y)
define SYSTEM_CONFIG_ASOUND_CONF_DMIX_FILE
	cp rootfs_config/file/alsa/asound.conf $(TARGET_DIR)/etc/
	sed -i 's/CODEC_NAME/$(APP_br_asound_conf_codec_name)/g' $(TARGET_DIR)/etc/asound.conf
endef
else
define SYSTEM_CONFIG_ASOUND_CONF_DMIX_FILE
	rm -f $(TARGET_DIR)/etc/asound.conf
endef
endif

define SYSTEM_CONFIG_SET_ASOUND_CONF_DMIX
	$(SYSTEM_CONFIG_ASOUND_CONF_DMIX_FILE)
endef
