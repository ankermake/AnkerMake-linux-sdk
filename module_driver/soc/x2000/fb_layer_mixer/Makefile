include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_fb_layer_mixer

module-y += fb_layer_mixer.o
module-y += dpu_hal.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
