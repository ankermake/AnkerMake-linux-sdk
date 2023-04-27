include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := pwm_battery
module-y += pwm_battery_data.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(DRIVERS_DIR)/soc/pwm/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)