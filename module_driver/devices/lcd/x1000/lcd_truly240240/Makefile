include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := lcd_truly240240

module-$(MD_X1000_SLCD_TRULY240240) += truly240240.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(DRIVERS_DIR)/soc/x1000/fb/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)