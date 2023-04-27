
include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_uart
KBUILD_EXTMOD := $(DRIVERS_DIR)/drivers/utils

module-y += uart_tty.o

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)