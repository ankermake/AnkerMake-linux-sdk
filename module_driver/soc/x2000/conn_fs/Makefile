include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_conn_fs

module-y += conn_fs.o
module-y += conn_fs_posix.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                        $(DRIVERS_DIR)/soc/x2000/conn/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
