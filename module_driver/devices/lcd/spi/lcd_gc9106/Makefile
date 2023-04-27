include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := lcd_gc9106

module-y += lcd_gc9106_data.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(DRIVERS_DIR)/drivers/spi_fb/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
