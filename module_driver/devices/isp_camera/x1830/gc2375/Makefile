include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := sensor_gc2375a

module-y += gc2375a.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
