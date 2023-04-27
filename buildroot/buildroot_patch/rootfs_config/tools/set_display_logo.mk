ifeq ($(APP_br_display_logo),y)
define SYSTEM_CONFIG_SET_JPEG_DISPLAY
	cp -vrf $(APP_br_logo_path) $(TARGET_DIR)/etc/logo.jpeg
	cp -vrf rootfs_config/file/display_logo/S11jpeg_display_shell $(TARGET_DIR)/etc/init.d/
endef
else
define SYSTEM_CONFIG_SET_JPEG_DISPLAY
	rm -f $(TARGET_DIR)/etc/logo.jpeg
	rm -f $(TARGET_DIR)/etc/init.d/S11jpeg_display_shell
endef
endif

define SYSTEM_CONFIG_SET_DISPLAY_LOGO
	$(SYSTEM_CONFIG_SET_JPEG_DISPLAY)
endef