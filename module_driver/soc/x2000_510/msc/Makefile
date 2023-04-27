include $(DRIVERS_DIR)/tools/common_module.mk
LINUXINCLUDE +=  -I$(KERNEL_DIR)/drivers/mmc/host/

MODULE_NAME := soc_msc

module-y += msc.o

ccflags-y += -Wno-implicit-fallthrough

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
