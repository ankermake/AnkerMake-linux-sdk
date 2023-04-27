
include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_utils

module-y += utils_module.o
module-$(MD_X2000_510_UTILS_RTC32K) += rtc32k_out.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
