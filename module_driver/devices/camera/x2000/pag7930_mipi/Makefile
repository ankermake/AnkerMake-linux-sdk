include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)

MODULE_NAME_SENSOR := sensor_pag7930_mipi

module-y += pag7930_mipi.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(SOC_DIR)/camera/Module.symvers

obj-m = $(MODULE_NAME_SENSOR).o

$(MODULE_NAME_SENSOR)-y = $(module-y)
