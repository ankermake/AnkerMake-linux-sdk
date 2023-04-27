include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_fb

module-y += jzfb.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
						$(DRIVERS_DIR)/drivers/rmem_manager/Module.symvers \
						$(SOC_DIR)/fb_layer_mixer/Module.symvers \
						$(SOC_DIR)/rotator/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
