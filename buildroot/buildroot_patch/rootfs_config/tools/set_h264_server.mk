ifeq ($(APP_br_h264_server_file),y)
define SYSTEM_CONFIG_H264_SERVER_FILE
	cp -f rootfs_config/file/h264e-nl-server/h264e-nl-server $(TARGET_DIR)/usr/bin/
	cp -f rootfs_config/file/h264e-nl-server/h264_server.sh $(TARGET_DIR)/usr/bin/
	cp -f rootfs_config/file/h264e-nl-server/S16h264_server $(TARGET_DIR)/etc/init.d/
endef
else
define SYSTEM_CONFIG_H264_SERVER_FILE
	rm -f $(TARGET_DIR)/usr/bin/h264e-nl-server
	rm -f $(TARGET_DIR)/usr/bin/h264_server.sh
	rm -f $(TARGET_DIR)/etc/init.d/S16h264_server
endef
endif

define SYSTEM_CONFIG_SET_H264
	$(SYSTEM_CONFIG_H264_SERVER_FILE)
endef
