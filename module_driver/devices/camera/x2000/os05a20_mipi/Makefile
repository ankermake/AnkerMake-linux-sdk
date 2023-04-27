include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)

MODULE_NAME_SENSOR := sensor_os05a20_mipi

module-y += os05a20_mipi.o

KBUILD_EXTMOD := $(DRIVERS_DIR)/drivers/utils \
                 $(DRIVERS_DIR)/soc/x2000/camera

obj-m = $(MODULE_NAME_SENSOR).o

$(MODULE_NAME_SENSOR)-y = $(module-y)