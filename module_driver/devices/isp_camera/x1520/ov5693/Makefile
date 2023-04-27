
include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := sensor_ov5693

module-y += ov5693.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
