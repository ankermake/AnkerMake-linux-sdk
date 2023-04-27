include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)

#生成两个ko用于双摄
MODULE_NAME_SENSOR0 := sensor0_gc2053_mipi
MODULE_NAME_SENSOR1 := sensor1_gc2053_mipi

module-y += gc2053_mipi.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(SOC_DIR)/camera/Module.symvers

obj-m = $(MODULE_NAME_SENSOR0).o
obj-m += $(MODULE_NAME_SENSOR1).o

$(MODULE_NAME_SENSOR0)-y = $(module-y)
$(MODULE_NAME_SENSOR1)-y = $(module-y)
