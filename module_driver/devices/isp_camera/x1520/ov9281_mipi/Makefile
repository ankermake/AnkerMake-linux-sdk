include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := sensor_ov9281_mipi

module-y += ov9281_mipi.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
