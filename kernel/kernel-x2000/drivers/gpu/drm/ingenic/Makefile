ccflags-y := -Iinclude/linux -Iinclude/drm
ifeq (, $(findstring -W,$(EXTRA_CFLAGS)))

endif

ingenic-y := \
	ingenic_dpu.o \
	ingenic_crtc.o \
	ingenic_drv.o \
	drm_sync_helper.o \
	ingenic_plane.o \
	ingenic_disport.o
obj-y += jz_mipi_dsi/
obj-y += panel/
obj-$(CONFIG_DRM_INGENIC_DISPLAYS_V10) += ingenic.o
