obj-$(CONFIG_SOC_CAMERA)		+= soc_camera.o soc_mediabus.o
obj-$(CONFIG_SOC_CAMERA_PLATFORM)	+= soc_camera_platform.o

# soc-camera host drivers have to be linked after camera drivers
obj-$(CONFIG_VIDEO_ATMEL_ISI)		+= atmel-isi.o
obj-$(CONFIG_VIDEO_MX1)			+= mx1_camera.o
obj-$(CONFIG_VIDEO_MX2)			+= mx2_camera.o
obj-$(CONFIG_VIDEO_MX3)			+= mx3_camera.o
obj-$(CONFIG_VIDEO_OMAP1)		+= omap1_camera.o
obj-$(CONFIG_VIDEO_PXA27x)		+= pxa_camera.o
obj-$(CONFIG_VIDEO_SH_MOBILE_CEU)	+= sh_mobile_ceu_camera.o
obj-$(CONFIG_VIDEO_SH_MOBILE_CSI2)	+= sh_mobile_csi2.o
obj-$(CONFIG_VIDEO_JZ_CIM_HOST_V13)     += jz_camera_v13.o
obj-$(CONFIG_VIDEO_JZ_CIM_HOST_V14)     += jz_camera_v14.o
obj-$(CONFIG_VIDEO_JZ_CIM_HOST_V11)     += jz_camera_v11.o

ccflags-y += -I$(srctree)/drivers/media/i2c/soc_camera
