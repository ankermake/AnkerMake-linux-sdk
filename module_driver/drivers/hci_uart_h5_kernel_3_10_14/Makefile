include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := hci_uart_h5_kernel_3_10_14

module-y += rtk_coex.o
module-y += hci_rtk_h5.o
module-y += hci_ldisc.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
