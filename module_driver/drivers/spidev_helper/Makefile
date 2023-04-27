include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := spidev_helper

module-y += spidev_helper_data.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)