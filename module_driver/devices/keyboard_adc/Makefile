include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := keyboard_adc

module-y += adc_keyboard.o
module-y += keyboard-adc-add.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
						$(SOC_DIR)/adc/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
