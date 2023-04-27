include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := esp8089

ccflags-y += -DHAS_FW

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m := $(MODULE_NAME).o
$(MODULE_NAME)-y += esp_wlan_power_control.o
$(MODULE_NAME)-y += esp_debug.o
$(MODULE_NAME)-y += sdio_sif_esp.o
$(MODULE_NAME)-y += esp_io.o
$(MODULE_NAME)-y += esp_file.o
$(MODULE_NAME)-y += esp_main.o
$(MODULE_NAME)-y += esp_sip.o
$(MODULE_NAME)-y += esp_ext.o
$(MODULE_NAME)-y += esp_ctrl.o
$(MODULE_NAME)-y += esp_mac80211.o
$(MODULE_NAME)-y += esp_debug.o
$(MODULE_NAME)-y += esp_utils.o