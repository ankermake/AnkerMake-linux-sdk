include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := keyboard_gpio_add
module-y += keyboard-gpio-add.o

module-y += gpio_keys.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
