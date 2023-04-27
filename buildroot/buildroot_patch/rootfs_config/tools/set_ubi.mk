ifeq ($(APP_br_ubi_mount_usr_data),y)
define SYSTEM_CONFIG_MOUNT_USR_DATA_UBI
	cp -vrf rootfs_config/file/ubi/S21mount_ubifs $(TARGET_DIR)/etc/init.d/
	mkdir -p $(TARGET_DIR)/usr/data/
endef
else
define SYSTEM_CONFIG_MOUNT_USR_DATA_UBI
	rm -f $(TARGET_DIR)/etc/init.d/S21mount_ubifs
endef
endif

define SYSTEM_CONFIG_SET_UBI
	$(SYSTEM_CONFIG_MOUNT_USR_DATA_UBI)
endef
