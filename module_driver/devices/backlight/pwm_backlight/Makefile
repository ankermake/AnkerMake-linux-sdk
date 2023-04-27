include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := pwm_backlight

module-y += pwm-backlight.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(SOC_DIR)/pwm/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
