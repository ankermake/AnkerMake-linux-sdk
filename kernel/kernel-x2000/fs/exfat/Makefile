#
# Makefile for the linux FAT12/16/32(VFAT)/64(exFAT) filesystem driver.
#

obj-$(CONFIG_EXFAT_FS) += exfat.o

exfat-objs	:= super.o core.o core_exfat.o blkdev.o fatent.o cache.o \
		   nls.o misc.o extent.o xattr.o
