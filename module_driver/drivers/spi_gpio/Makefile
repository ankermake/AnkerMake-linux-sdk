include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := spi_gpio_add

module-y += spi-gpio-add.o
module-y += spi-gpio.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
