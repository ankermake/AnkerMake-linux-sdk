ifeq ($(APP_usb_mtp_server),y)
define SYSTEM_CONFIG_SET_USB_MTP_SERVER
	mkdir -p $(TARGET_DIR)/etc/umtprd/
	cp -vrf $(APP_usb_mtp_conf_path) $(TARGET_DIR)/etc/umtprd/umtprd.conf
	cp -vrf rootfs_config/file/usb_mtp/usb_mtp_server.sh $(TARGET_DIR)/etc/
	cp -vrf rootfs_config/file/usb_mtp/S99usb_mtp_server $(TARGET_DIR)/etc/init.d/
endef
else
define SYSTEM_CONFIG_SET_USB_MTP_SERVER
	rm -f $(TARGET_DIR)/etc/umtprd/umtprd.conf
	rm -f $(TARGET_DIR)/etc/usb_mtp_server.sh
	rm -f $(TARGET_DIR)/etc/init.d/S99usb_mtp_server
	rm -rf $(TARGET_DIR)/etc/umtprd/
endef
endif

define SYSTEM_CONFIG_SET_USB_MTP
	$(SYSTEM_CONFIG_SET_USB_MTP_SERVER)
endef