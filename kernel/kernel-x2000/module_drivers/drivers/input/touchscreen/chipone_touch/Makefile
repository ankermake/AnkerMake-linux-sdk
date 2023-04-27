# Makefile for the chipone touchscreen drivers.
# firmware :/etc/firmware/ICNT8918.bin
# insmod chipone-ts.ko

MODULE_NAME := chipone-ts
DRIVER_TYPE ?= m

TPFILES := cts_i2c_driver.o cts_core.o cts_platform.o cts_tool.o cts_firmware.o cts_emb_flash.o cts_test.o cts_sysfs.o cts_efctrl.o cts_strerror.o cts_csvparse.o

obj-$(CONFIG_TOUCHSCREEN_CHIPONE) += $(MODULE_NAME).o
$(MODULE_NAME)-objs += $(TPFILES)
obj-$(DRIVER_TYPE)   += $(MODULE_NAME).o

