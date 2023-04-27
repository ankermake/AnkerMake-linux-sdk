include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := axp-216

module-y += axp-board.o
module-y += axp-mfd.o
module-y += axp-regu.o
module-y += axp-sply.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)


