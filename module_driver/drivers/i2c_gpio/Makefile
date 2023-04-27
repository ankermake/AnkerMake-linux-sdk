include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := i2c_gpio_add

module-y += i2c-gpio-add.o
module-y += i2c-gpio.o
module-y += i2c-algo-bit.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
