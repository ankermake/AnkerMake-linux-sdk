ifeq ($(APP_br_mmc_ext4_mount_usr_data),y)
define SYSTEM_CONFIG_MOUNT_USR_DATA_MMC_EXT4
	cp -vrf rootfs_config/file/mmc/S21mount_mmc_ext4 $(TARGET_DIR)/etc/init.d/
	mkdir -p $(TARGET_DIR)/usr/data/
endef
else
define SYSTEM_CONFIG_MOUNT_USR_DATA_MMC_EXT4
	rm -f $(TARGET_DIR)/etc/init.d/S21mount_mmc_ext4
endef
endif

ifeq ($(APP_br_mmc_fat_mount_usr_rtosdata),y)
define SYSTEM_CONFIG_MOUNT_USR_DATABASE_MMC_FAT
	cp -vrf rootfs_config/file/mmc/S22mount_mmc_fat $(TARGET_DIR)/etc/init.d/
	mkdir -p $(TARGET_DIR)/usr/rtosdata/
endef
else
define SYSTEM_CONFIG_MOUNT_USR_DATABASE_MMC_FAT
	rm -f $(TARGET_DIR)/etc/init.d/S22mount_mmc_fat
endef
endif

define SYSTEM_CONFIG_SET_MMC
	$(SYSTEM_CONFIG_MOUNT_USR_DATA_MMC_EXT4)
	$(SYSTEM_CONFIG_MOUNT_USR_DATABASE_MMC_FAT)
endef
