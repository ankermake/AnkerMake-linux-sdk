
ifeq ($(APP_br_mass_storage_automatic_mount),y)
define SYSTEM_CONFIG_MASS_STORAGE_AUTOMATIC_MOUNT
	cp -f rootfs_config/file/mass_storage/mass_storage_inserting.sh $(TARGET_DIR)/etc/
	cp -f rootfs_config/file/mass_storage/mass_storage_removing.sh $(TARGET_DIR)/etc/
	rootfs_config/tools/add_config_to_file.sh add "sd[a-z][0-9] 0:0 666 @ /etc/mass_storage_inserting.sh" $(TARGET_DIR)/etc/mdev.conf
	rootfs_config/tools/add_config_to_file.sh add "sd[a-z] 0:0 666 $$ /etc/mass_storage_removing.sh" $(TARGET_DIR)/etc/mdev.conf
endef
else
define SYSTEM_CONFIG_MASS_STORAGE_AUTOMATIC_MOUNT
	rm -f $(TARGET_DIR)/etc/mass_storage_inserting.sh
	rm -f $(TARGET_DIR)/etc/mass_storage_removing.sh
	rootfs_config/tools/add_config_to_file.sh delete "sd[a-z][0-9] 0:0 666 @ /etc/mass_storage_inserting.sh" $(TARGET_DIR)/etc/mdev.conf
	rootfs_config/tools/add_config_to_file.sh delete "sd[a-z] 0:0 666 $$ /etc/mass_storage_removing.sh" $(TARGET_DIR)/etc/mdev.conf
endef
endif

define SYSTEM_CONFIG_SET_MASS_STORAGE
	$(SYSTEM_CONFIG_MASS_STORAGE_AUTOMATIC_MOUNT)
endef
