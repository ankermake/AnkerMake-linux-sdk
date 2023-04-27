# Makefile for true color logo.

obj-y	+= logo.o
obj-y	+= showlogo.o

ifeq ($(CONFIG_FB_JZ_V12),y)
$(obj)/logo.c: scripts/png2logo FORCE
	@$< $(CONFIG_TRUE_COLOR_LOGO_FILE) 32 $(CONFIG_TRUE_COLOR_LOGO_BACKGROUND)> $@
endif

ifeq ($(CONFIG_FB_JZ_V13),y)
$(obj)/logo.c: scripts/png2logo FORCE
	@$< $(CONFIG_TRUE_COLOR_LOGO_FILE) 16 $(CONFIG_TRUE_COLOR_LOGO_BACKGROUND)> $@
endif
