include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := ax88796c-spi

module-y += ax88796c_main.o
module-y += ax88796c_ioctl.o
module-y += ax88796c_spi.o
module-y += ax88796c_spi_dev.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)