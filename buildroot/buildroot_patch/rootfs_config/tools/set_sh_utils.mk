ifeq ($(APP_br_sh_utils_wait_file),y)
define SYSTEM_CONFIG_SH_UTILS_WAIT_FILE
	cp -vrf rootfs_config/file/sh_utils/wait_file.sh $(TARGET_DIR)/usr/bin/
endef
else
define SYSTEM_CONFIG_SH_UTILS_WAIT_FILE
	rm -f $(TARGET_DIR)/usr/bin/wait_file.sh
endef
endif

ifeq ($(APP_br_sh_utils_mount_ubifs),y)
define SYSTEM_CONFIG_SH_UTILS_MOUNT_UBIFS
	cp -vrf rootfs_config/file/sh_utils/mount_ubifs.sh $(TARGET_DIR)/usr/bin/
endef
else
define SYSTEM_CONFIG_SH_UTILS_MOUNT_UBIFS
	rm -f $(TARGET_DIR)/usr/bin/mount_ubifs.sh
endef
endif

ifeq ($(APP_br_sh_utils_mount_jffs2),y)
define SYSTEM_CONFIG_SH_UTILS_MOUNT_JFFS2
	cp -vrf rootfs_config/file/sh_utils/mount_jffs2.sh $(TARGET_DIR)/usr/bin/
endef
else
define SYSTEM_CONFIG_SH_UTILS_MOUNT_JFFS2
	rm -f $(TARGET_DIR)/usr/bin/mount_jffs2.sh
endef
endif

ifeq ($(APP_br_sh_utils_mount_mmc_ext4),y)
define SYSTEM_CONFIG_SH_UTILS_MOUNT_MMC_EXT4
	cp -vrf rootfs_config/file/sh_utils/mount_mmc_ext4.sh $(TARGET_DIR)/usr/bin/
endef
else
define SYSTEM_CONFIG_SH_UTILS_MOUNT_MMC_EXT4
	rm -f $(TARGET_DIR)/usr/bin/mount_mmc_ext4.sh
endef
endif

ifeq ($(APP_br_sh_utils_mount_mmc_fat),y)
define SYSTEM_CONFIG_SH_UTILS_MOUNT_MMC_FAT
	cp -vrf rootfs_config/file/sh_utils/mount_mmc_fat.sh $(TARGET_DIR)/usr/bin/
endef
else
define SYSTEM_CONFIG_SH_UTILS_MOUNT_MMC_FAT
	rm -f $(TARGET_DIR)/usr/bin/mount_mmc_fat.sh
endef
endif

define SYSTEM_CONFIG_SET_SH_UTILS
	$(SYSTEM_CONFIG_SH_UTILS_WAIT_FILE)
	$(SYSTEM_CONFIG_SH_UTILS_MOUNT_UBIFS)
	$(SYSTEM_CONFIG_SH_UTILS_MOUNT_JFFS2)
	$(SYSTEM_CONFIG_SH_UTILS_MOUNT_MMC_EXT4)
	$(SYSTEM_CONFIG_SH_UTILS_MOUNT_MMC_FAT)
endef
