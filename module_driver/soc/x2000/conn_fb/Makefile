include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_conn_fb

module-y += conn_fb.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(DRIVERS_DIR)/soc/x2000/conn/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)