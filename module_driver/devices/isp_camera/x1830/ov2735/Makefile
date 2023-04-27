include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)/camera/

MODULE_NAME := sensor_ov2735

module-y += ov2735.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
