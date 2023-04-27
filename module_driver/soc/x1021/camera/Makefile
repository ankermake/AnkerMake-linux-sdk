include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_camera

module-y += camera.o
module-y += vic_gpio.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)