include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)/camera/hal
ccflags-y += -I$(SOC_DIR)/camera/

ccflags-y += -Wno-implicit-fallthrough

MODULE_NAME := soc_camera

module-y += camera.o

module-y += hal/camera_gpio.o
module-y += hal/csi.o
module-y += hal/vic.o
module-y += hal/dsys.o

module-y += cim/cim.o

module-y += vic/vic_channel_mem.o

module-y += isp/vic_channel_tiziano.o
module-y += isp/isp.o
module-y += isp/mscaler.o
module-y += isp/isp_sensor.o
module-y += isp/isp_tuning.o
module-$(MD_X2000_CAMERA_ISP_DRIVER_MULTI_CORE)  += isp/isp-core/isp-core-multi.a
module-$(MD_X2000_CAMERA_ISP_DRIVER_SINGLE_CORE) += isp/isp-core/isp-core-single.a

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
