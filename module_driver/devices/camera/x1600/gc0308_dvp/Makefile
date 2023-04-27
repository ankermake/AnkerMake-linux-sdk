include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)/camera/

MODULE_NAME := sensor_gc0308_dvp

module-y += gc0308_dvp.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(SOC_DIR)/camera/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
