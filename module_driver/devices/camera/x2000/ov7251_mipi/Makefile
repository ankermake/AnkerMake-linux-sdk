include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)

MODULE_NAME := sensor_ov7251_mipi

module-y += ov7251_mipi.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(SOC_DIR)/camera/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
