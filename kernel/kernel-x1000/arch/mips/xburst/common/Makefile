
# add object if need

obj-y += pmon.o
obj-y += proc-pmon.o
obj-${CONFIG_JZ_IMEM} += proc-imem.o
obj-y += get-cpu-features.o
#obj-y += trace-exit.o
obj-y += proc-exec.o
obj-y += mxu.o
obj-y += proc.o
obj-y += jz_notifier.o
obj-$(CONFIG_MIPS_MM_HOOKS) += mmu_context.o
obj-$(CONFIG_PCI) += xburst_vpci.o

obj-$(CONFIG_SUPPORT_OTA) += initrd-check.o