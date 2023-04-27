usb_storage_conf_file = rootfs_config/file/usb_storage/usb_storage_conf

ifeq ($(APP_br_mount_as_usb),y)
define SYSTEM_CONFIG_MOUNT_AS_USB

	$(Q)echo 'image_file_name=$(APP_br_mount_as_usb_file)' > $(usb_storage_conf_file)
	$(Q)echo 'image_size=$(APP_br_mount_as_usb_file_size)' >> $(usb_storage_conf_file)

	cp -vrf rootfs_config/file/usb_storage/S22mount_as_usb.sh $(TARGET_DIR)/etc/init.d/
	cp -vrf rootfs_config/file/usb_storage/usb_storage_conf $(TARGET_DIR)/etc/

endef
else
define SYSTEM_CONFIG_MOUNT_AS_USB
	rm -f $(TARGET_DIR)/etc/usb_storage_conf
	rm -f $(TARGET_DIR)/etc/init.d/S22mount_as_usb.sh
endef
endif

ifeq ($(APP_br_sdcard_automatic_mount_to_mass_storage),y)
define SYSTEM_CONFIG_SDCARD_AUTOMATIC_MOUNT_TO_MASS_STORAGE
	cp -f rootfs_config/file/usb_storage/sdcard_automatic_mount_to_mass_storage.sh $(TARGET_DIR)/usr/bin/
	rootfs_config/tools/add_config_to_file.sh add "mmcblk[0-9] 0:0 666 * /usr/bin/sdcard_automatic_mount_to_mass_storage.sh" $(TARGET_DIR)/etc/mdev.conf
endef
else
define SYSTEM_CONFIG_SDCARD_AUTOMATIC_MOUNT_TO_MASS_STORAGE
	rm -f $(TARGET_DIR)/usr/bin/sdcard_automatic_mount_to_mass_storage.sh
	rootfs_config/tools/add_config_to_file.sh delete "mmcblk[0-9] 0:0 666 * /usr/bin/sdcard_automatic_mount_to_mass_storage.sh" $(TARGET_DIR)/etc/mdev.conf
endef
endif

ifeq ($(APP_br_mount_as_usb),y)
	copy_usb_dev_mass_storage_sh=y
endif

ifeq ($(APP_br_sdcard_automatic_mount_to_mass_storage),y)
	copy_usb_dev_mass_storage_sh=y
endif

ifeq ($(copy_usb_dev_mass_storage_sh),y)
define SYSTEM_CONFIG_CP_USB_DEV_MASS_STORAGE_SH
	cp -vrf rootfs_config/file/usb_storage/usb_dev_mass_storage.sh $(TARGET_DIR)/usr/bin/
endef
else
define SYSTEM_CONFIG_CP_USB_DEV_MASS_STORAGE_SH
	rm -f $(TARGET_DIR)/usr/bin/usb_dev_mass_storage.sh
endef
endif

define SYSTEM_CONFIG_USB_STORAGE
	$(SYSTEM_CONFIG_MOUNT_AS_USB)
	$(SYSTEM_CONFIG_SDCARD_AUTOMATIC_MOUNT_TO_MASS_STORAGE)
	$(SYSTEM_CONFIG_CP_USB_DEV_MASS_STORAGE_SH)
endef