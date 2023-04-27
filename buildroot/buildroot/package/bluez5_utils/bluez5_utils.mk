################################################################################
#
# bluez5_utils
#
################################################################################

ifeq ($(BR2_PACKAGE_BLUEZ5_UTILS_V5_47),y)
BLUEZ5_UTILS_INIT_PATCH:=$(shell cp $(TOPDIR)/package/bluez5_utils/patch_5_47_0001-tools-bneptest.c-Remove-include-linux-if_bridge.h-to   $(TOPDIR)/package/bluez5_utils/0001.patch)
include package/bluez5_utils/bluez5_47_mk
endif

ifeq ($(BR2_PACKAGE_BLUEZ5_UTILS_V5_54),y)
BLUEZ5_UTILS_INIT_PATCH:=$(shell rm $(TOPDIR)/package/bluez5_utils/*.patch)
include package/bluez5_utils/bluez5_54_mk
endif
