ifeq ($(APP_br_adb_server),y)

define SYSTEM_CONFIG_ENABLE_ADB
	cp -vrf rootfs_config/file/adb/adb $(TARGET_DIR)/etc/init.d/
	cp -vrf rootfs_config/file/adb/adbserver.sh $(TARGET_DIR)/sbin/
	cp -vrf rootfs_config/file/adb/usb_adb_enable.sh $(TARGET_DIR)/sbin/
endef

ifeq ($(APP_br_adb_server_start),y)
define SYSTEM_CONFIG_ENABLE_ADB_SERVER
	rm -f $(TARGET_DIR)/etc/init.d/T90adb
	cp -vrf rootfs_config/file/adb/S90adb $(TARGET_DIR)/etc/init.d/
endef
else
define SYSTEM_CONFIG_ENABLE_ADB_SERVER
	rm -f $(TARGET_DIR)/etc/init.d/S90adb
	cp -vrf rootfs_config/file/adb/S90adb $(TARGET_DIR)/etc/init.d/T90adb
endef
endif

adb_export_env := tools/export_env.sh
adb_env_file := $(TARGET_DIR)/etc/profile.d/env_setup.sh

define SYSTEM_CONFIG_ADB_SET_ENV
	$(adb_export_env) env_adb_device_name_prefix "$(APP_br_adb_server_name)" $(adb_env_file)
	$(adb_export_env) env_adb_device_use_diffrent_name "$(APP_br_adb_server_add_mac)" $(adb_env_file)
endef
else

define SYSTEM_CONFIG_ENABLE_ADB
	rm -f $(TARGET_DIR)/etc/init.d/S90adb
	rm -f $(TARGET_DIR)/etc/init.d/T90adb
	rm -f $(TARGET_DIR)/sbin/adbserver.sh
	rm -rf $(TARGET_DIR)/etc/init.d/adb
endef

endif

define SYSTEM_CONFIG_SET_ADB
	$(SYSTEM_CONFIG_ENABLE_ADB)
	$(SYSTEM_CONFIG_ENABLE_ADB_SERVER)
	$(SYSTEM_CONFIG_ADB_SET_ENV)
endef