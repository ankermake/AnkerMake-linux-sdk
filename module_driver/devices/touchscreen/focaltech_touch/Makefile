include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := focaltech_touch
module-y += focaltech_core.o
module-y += focaltech_i2c.o
# module-y += focaltech_ex_fun.o
# module-y += focaltech_ex_mode.o
# module-y += focaltech_gesture.o
# module-y += focaltech_esdcheck.o
# module-y += focaltech_flash.o
# module-y += focaltech_flash/focaltech_upgrade_ft7511.o
# module-y += focaltech_point_report_check.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
