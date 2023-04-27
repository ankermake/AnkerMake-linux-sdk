ifeq ($(APP_br_jffs2_mount_usr_data),y)
define SYSTEM_CONFIG_MOUNT_USR_DATA_JFFS2
	cp -vrf rootfs_config/file/jffs2/S21mount_jffs2 $(TARGET_DIR)/etc/init.d/
	mkdir -p $(TARGET_DIR)/usr/data/
endef
else
define SYSTEM_CONFIG_MOUNT_USR_DATA_JFFS2
	rm -f $(TARGET_DIR)/etc/init.d/S21mount_jffs2
endef
endif

define SYSTEM_CONFIG_SET_JFFS2
	$(SYSTEM_CONFIG_MOUNT_USR_DATA_JFFS2)
endef
