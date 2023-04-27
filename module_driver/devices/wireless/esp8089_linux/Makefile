
KERNELPATH ?= ../../../../kernel/kernel-x2000
ARCH ?= mips
CROSS_COMPILE ?= mips-linux-gnu-

PDIR := $(shell pwd)

ESP8089_DIR += $(PDIR)/src

all: clean
	make CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) -C $(KERNELPATH) SUBDIRS=$(ESP8089_DIR) modules

clean:
	make ARCH=$(ARCH) -C $(KERNELPATH) SUBDIRS=$(ESP8089_DIR) clean
