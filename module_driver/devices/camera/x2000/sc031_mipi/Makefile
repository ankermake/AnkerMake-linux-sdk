include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)

MODULE_NAME_SENSOR0 := sensor0_sc031_mipi
MODULE_NAME_SENSOR1 := sensor1_sc031_mipi

module0-y += sc031_mipi_master.o
module1-y += sc031_mipi_slave.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(SOC_DIR)/camera/Module.symvers

obj-m = $(MODULE_NAME_SENSOR0).o
obj-m += $(MODULE_NAME_SENSOR1).o

$(MODULE_NAME_SENSOR0)-y = $(module0-y)
$(MODULE_NAME_SENSOR1)-y = $(module1-y)
