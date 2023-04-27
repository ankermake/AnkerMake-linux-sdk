include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)

MODULE_NAME_SENSOR0 := sensor0_sc4238_mipi
MODULE_NAME_SENSOR1 := sensor1_sc4238_mipi

module-y += sc4238_mipi.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(SOC_DIR)/camera/Module.symvers

obj-m = $(MODULE_NAME_SENSOR0).o
obj-m += $(MODULE_NAME_SENSOR1).o

$(MODULE_NAME_SENSOR0)-y = $(module-y)
$(MODULE_NAME_SENSOR1)-y = $(module-y)
