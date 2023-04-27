include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := sensor_s5k6a1

module-y += s5k6a1.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
