include $(DRIVERS_DIR)/tools/common_module.mk

EXTRA_CFLAGS +=-Wno-error=date-time
EXTRA_CFLAGS +=-Wno-date-time
MODULE_NAME:= gt9xx_touch

module-y	+=  gt9xx.o
module-y	+=  gt9xx_update.o
# module-y	+=  goodix_tool.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)