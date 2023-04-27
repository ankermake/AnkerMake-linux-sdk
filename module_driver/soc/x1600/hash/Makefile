include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_hash

module-y += hash.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)