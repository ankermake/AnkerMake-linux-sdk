
ifeq ($(APP_br_root_login),y)

ifeq ($(BR2_INIT_SYSV),y)
# In sysvinit inittab, the "id" must not be longer than 4 bytes, so we
# skip the "tty" part and keep only the remaining.
define SYSTEM_CONFIG_SET_LOGIN
	sed -i '/# GENERIC_SERIAL$$/s~^.*#~$(shell echo $(APP_br_root_login_tty_port) | tail -c+4)::respawn:/bin/sh #~' \
		$(TARGET_DIR)/etc/inittab
endef

else ifeq ($(BR2_INIT_BUSYBOX),y)
# Add getty to busybox inittab
define SYSTEM_CONFIG_SET_LOGIN
	sed -i '/# GENERIC_SERIAL$$/s~^.*#~$(APP_br_root_login_tty_port)::respawn:-/bin/sh #~' \
		$(TARGET_DIR)/etc/inittab
endef
endif

endif # APP_br_root_login

ifeq ($(APP_br_root_login_no_console),y)
define SYSTEM_CONFIG_SET_LOGIN
	sed -i '/# GENERIC_SERIAL$$/s~^.*#~#~' \
		$(TARGET_DIR)/etc/inittab
endef
endif

ifeq ($(APP_br_root_login_keep_buildroot),y)
define SYSTEM_CONFIG_SET_LOGIN
endef
endif

