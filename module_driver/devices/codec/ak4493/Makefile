include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := codec_ak4493

module-y += ak4493.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
