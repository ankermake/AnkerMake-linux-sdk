
ifeq ($(APP_br_sdcard_automatic_mount),y)
define SYSTEM_CONFIG_SDCARD_AUTOMATIC_MOUNT
	cp -f rootfs_config/file/sdcard/sdcard_inserting.sh $(TARGET_DIR)/etc/
	cp -f rootfs_config/file/sdcard/sdcard_removing.sh $(TARGET_DIR)/etc/
	rootfs_config/tools/add_config_to_file.sh add "mmcblk[0-9]p[0-9] 0:0 666 @ /etc/sdcard_inserting.sh" $(TARGET_DIR)/etc/mdev.conf
	rootfs_config/tools/add_config_to_file.sh add "mmcblk[0-9] 0:0 666 $$ /etc/sdcard_removing.sh" $(TARGET_DIR)/etc/mdev.conf
endef
else
define SYSTEM_CONFIG_SDCARD_AUTOMATIC_MOUNT
	rm -f $(TARGET_DIR)/etc/sdcard_inserting.sh
	rm -f $(TARGET_DIR)/etc/sdcard_removing.sh
	rootfs_config/tools/add_config_to_file.sh delete "mmcblk[0-9]p[0-9] 0:0 666 @ /etc/sdcard_inserting.sh" $(TARGET_DIR)/etc/mdev.conf
	rootfs_config/tools/add_config_to_file.sh delete "mmcblk[0-9] 0:0 666 $$ /etc/sdcard_removing.sh" $(TARGET_DIR)/etc/mdev.conf
endef
endif

define SYSTEM_CONFIG_SET_SDCARD
	$(SYSTEM_CONFIG_SDCARD_AUTOMATIC_MOUNT)
endef
