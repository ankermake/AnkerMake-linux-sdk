include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := lcd_kwh035

module-y += lcd_kwh035_data.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(DRIVERS_DIR)/soc/x1830/fb/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
