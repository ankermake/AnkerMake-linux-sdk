include $(DRIVERS_DIR)/tools/common_module.mk


MODULE_NAME := soc_camera

module-y += camera.o
module-y += camera_gpio.o
module-y += csi.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
						$(DRIVERS_DIR)/drivers/rmem_manager/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
