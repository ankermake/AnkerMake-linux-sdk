include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_mcu

module-y += mcu.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
