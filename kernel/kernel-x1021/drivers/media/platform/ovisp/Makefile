# Makefile for OVISP video driver

ifeq ($(CONFIG_VIDEO_OVISP_STUB),y)
obj-$(CONFIG_VIDEO_OVISP) += ovisp-videostub.o ovisp-videobuf.o isp-debug.o
else
obj-$(CONFIG_VIDEO_OVISP) += ovisp-base.o ovisp-video.o  ovisp-isp.o ovisp-csi.o isp-i2c.o isp-ctrl.o ovisp-debugtool.o \
	isp-debug.o ovisp-buf-contig.o
obj-$(CONFIG_VIDEO_OV5645) += isp-sensor-ov5645-setting.o
obj-$(CONFIG_VIDEO_OV5645_BYPASS) += isp-sensor-ov5645-setting.o
obj-$(CONFIG_VIDEO_OV9724) += isp-sensor-ov9724-setting.o
obj-$(CONFIG_VIDEO_OV8858) += isp-sensor-ov8858-setting.o
obj-$(CONFIG_DVP_OV9712) += isp-sensor-ov9712-setting.o
obj-$(CONFIG_DVP_BF3116) += isp-sensor-bf3116-setting.o
obj-$(CONFIG_SOC_CAMERA_OV2710) += isp-sensor-ov2710-setting.o
endif
