KMODULE_NAME = ssv6x5x

ifeq ($(SSV_DRV_PATH),)
SSV_DRV_PATH := $(DRIVERS_DIR)/devices/wireless/ssv6x5x_linux
endif

KBUILD_TOP := $(SSV_DRV_PATH)
ifeq ($(MAKELEVEL),0)
KBUILD_TOP := .
endif

ifeq ($(ARCH),)
ARCH := mips
endif

CROSS_COMPILE ?= mips-linux-gnu-

ifeq ($(INSTALL_PATH),)
INSTALL_PATH := $(SSV_DRV_PATH)
endif

include $(KBUILD_TOP)/config_common.mak
include $(KBUILD_TOP)/config.mak

MOD_DEF_H = include/ssv_mod_conf.h

EXTRA_CFLAGS := -I$(KBUILD_TOP) -I$(KBUILD_TOP)/include
USER_EXTRA_CFLAGS += -I$(SOC_DIR)/include/
DEF_PARSER_H = $(KBUILD_TOP)/include/ssv_conf_parser.h

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers
EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -I$(DRIVERS_DIR) -I$(DRIVERS_DIR)/include/
EXTRA_CFLAGS += -I$(SOC_DIR)/include/
EXTRA_CFLAGS += -include $(DRIVERS_DIR)/config.h

#------------------------------

# ssvdevice/
KERN_SRCS += ssvdevice/ssvdevice.c
KERN_SRCS += ssvdevice/ssv_cmd.c

# hci/
KERN_SRCS += hci/ssv_hci.c

# smac/
KERN_SRCS += smac/init.c
KERN_SRCS += smac/dev.c
KERN_SRCS += smac/regd.c
KERN_SRCS += smac/wow.c
KERN_SRCS += smac/hw_scan.c

KERN_SRCS += smac/lib.c
KERN_SRCS += smac/ap.c

ifeq ($(findstring -DCONFIG_SSV6XXX_DEBUGFS, $(ccflags-y)), -DCONFIG_SSV6XXX_DEBUGFS)
KERN_SRCS += smac/ssv6xxx_debugfs.c
endif

KERN_SRCS += smac/efuse.c
KERN_SRCS += smac/ssv_skb.c
ifeq ($(findstring -DCONFIG_SSV_CTL, $(ccflags-y)), -DCONFIG_SSV_CTL)
KERN_SRCS += smac/ssv_wifi_ctl.c
endif

KERN_SRCS += smac/hal/hal.c
ifeq ($(findstring -DSSV_SUPPORT_SSV6006, $(ccflags-y)), -DSSV_SUPPORT_SSV6006)
KERN_SRCS += smac/hal/ssv6006c/ssv6006_common.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006C_mac.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006_phy.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006_turismoC.c
endif

# hwif/hal/
KERN_SRCS += hwif/hal/hwif_hal.c
ifeq ($(findstring -DSSV_SUPPORT_SSV6006, $(ccflags-y)), -DSSV_SUPPORT_SSV6006)
KERN_SRCS += hwif/hal/ssv6006c/ssv6006C_hwif.c
endif

ifeq ($(findstring -DSSV_SUPPORT_SDIO, $(ccflags-y)), -DSSV_SUPPORT_SDIO)
# hwif/sdio/
KERN_SRCS += hwif/sdio/sdio.c
endif

ifeq ($(findstring -DSSV_SUPPORT_USB, $(ccflags-y)), -DSSV_SUPPORT_USB)
# hwif/usb
KERN_SRCS += hwif/usb/usb.c
endif

#------------------------------
KERN_SRCS += $(KMODULE_NAME)-generic-wlan.c

$(KMODULE_NAME)-y += $(KERN_SRCS_S:.S=.o)
$(KMODULE_NAME)-y += $(KERN_SRCS:.c=.o)

obj-$(CONFIG_SSV6X5X) += $(KMODULE_NAME).o

.PHONY: all ver modules clean

all: $(MOD_DEF_H) modules

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(SSV_DRV_PATH) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

strip:
	$(CROSS_COMPILE)strip $(KMODULE_NAME).ko --strip-unneeded

install:
	$(MAKE) INSTALL_MOD_DIR=$(INSTALL_PATH) -C $(KERNEL_DIR) \
			M=$(SSV_DRV_PATH) modules_install

clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -fr .tmp_versions
	rm -fr Module.symvers
	rm -fr Module.markers
	rm -fr modules.order
	rm -fr image/$(KMODULE_NAME)-wifi.cfg
	cd ssvdevice/; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd hci/; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd smac/; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd hwif/; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd hwif/sdio/; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd crypto/; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	rm -fr lib.a
	rm -rf $(MOD_DEF_H)

$(MOD_DEF_H): config.mak config_common.mak
