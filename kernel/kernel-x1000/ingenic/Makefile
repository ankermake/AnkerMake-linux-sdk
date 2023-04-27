CFLAGS_KERNEL	+= -I$(srctree)/ingenic/include/ -Wno-switch -Wno-undef -Wno-declaration-after-statement

obj-y    += drivers/
obj-y	 += utils/
obj-$(CONFIG_SOC_X1830_DRIVERS) += x1830/
obj-$(CONFIG_SOC_X1520_DRIVERS) += x1520/
obj-$(CONFIG_INGENIC_MODULE) += for_module/