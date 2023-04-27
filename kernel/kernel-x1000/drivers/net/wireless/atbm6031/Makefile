#
#makefile for build atbm_wifi.ko
#
###############################################################################
#
# when release to customer ,the CUSTOMER_SUPPORT_USED must set to y!!!!!!!!!!!!!
#
###############################################################################
CUSTOMER_SUPPORT_USED=y
###############################################################################
#PLATFORM_INGENIC    		1

export
platform ?=PLATFORM_INGENIC
#Android
#Linux
sys ?= linux
#arch:arm or arm64
arch ?= mips
#export 
#ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=$(platform)

ifeq ($(CUSTOMER_SUPPORT_USED),y)
MAKEFILE_SUB ?= Makefile.build.customer
else
MAKEFILE_SUB ?= Makefile.build
endif

ifeq ($(KERNELRELEASE),)


ifeq ($(platform),PLATFORM_INGENIC)

KERDIR:=./../../../../
#CROSS_COMPILE:=./../../../../../prebuilts/toolchains/mips-gcc520-32bit/bin
CROSS_COMPILE:=$(CROSS_COMPILE)
export
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=10
endif


ifeq ($(platform),PLATFORM_PCX86)
all:install

install:
	@echo "make PLATFORM_PCX86"
	$(MAKE) all -f Makefile.build.local KDIR=$(KERDIR)
clean:
	$(MAKE) -f Makefile.build.local KDIR=$(KERDIR) clean
else
all:install

install:
	@echo "make PLATFORM_CROSS=$(platform)"
	$(MAKE) all -f $(MAKEFILE_SUB) ARCH=$(arch)  CROSS_COMPILE=$(CROSS_COMPILE) KDIR=$(KERDIR) SYS=$(sys) PLAT=$(platform) -j8
clean:
	$(MAKE) -f $(MAKEFILE_SUB) KDIR=$(KERDIR) ARCH=$(arch) clean
strip:	
	$(MAKE) -f $(MAKEFILE_SUB) KDIR=$(KERDIR) ARCH=$(arch) SYS=$(sys) PLAT=$(platform) strip
endif
else
ifeq ($(platform),PLATFORM_INGENIC)
export 
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=1
endif
ifeq ($(platform),PLATFORM_SUN6I)
export 
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=2
endif
ifeq ($(platform),PLATFORM_SUN6I_64)
export
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=4
endif
ifeq ($(platform),PLATFORM_HI3798M)
export
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=5
endif
ifeq ($(platform),PLATFORM_AMLOGIC)
export
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=8
endif
ifeq ($(platform),PLATFORM_AMLOGICi_905X)
export
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=9
endif
ifeq ($(platform),PLATFORM_ROCKCHIP)
export
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=10
endif
ifeq ($(platform),PLATFORM_MSTAR)
export
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=11
endif
ifeq ($(platform),PLATFORM_CDLINUX)
export
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=12
endif
ifeq ($(platform),PLATFORM_AMLOGIC_S805)
export
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=13
endif
ifeq ($(platform),PLATFORM_ROCKCHIP_3229)
export
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=10
endif
export 


############################################################################
#
#	The Follow Code Of The Makefile Should Not Be Changed 
#
############################################################################
PWD:=$(shell pwd)
WIFI_INSTALL_DIR := $(PWD)/driver_install/

NOSTDINC_FLAGS := -I$(src)/include/ \
	-include $(src)/include/linux/compat-2.6.h \
	-DCOMPAT_STATIC

#####################################################
export
ifeq ($(CONFIG_ATBM601x),y)
CONFIG_NOT_SUPPORT_40M_CHW = y
MODULES_NAME =  atbm6031_wifi
else ifeq ($(CONFIG_ATBM602x),y)
MODULES_NAME = atbm6032_wifi
endif

ifeq ($(CONFIG_ATBM_USB_BUS),y)
CONFIG_ATHENAB=n
CONFIG_ARES=n
CONFIG_ARESB=y
USB_BUS=y
else ifeq ($(CONFIG_ATBM_SDIO_BUS),y)
SDIO_BUS=y
ifeq ($(CONFIG_ATBM_SDIO_24M),y)
CONFIG_ATHENAB_24M=n
CONFIG_ATHENAB=n
CONFIG_ARESB=y
else
CONFIG_ATHENAB=n
endif
else ifeq ($(CONFIG_ATBM_SPI_BUS),y)
SPI_BUS=y
endif
MULT_NAME=y
ATBM_MAKEFILE_SUB=y
#####################################################
export 
ifeq ($(CONFIG_ATBM_APOLLO),)
CONFIG_ATBM_APOLLO=m
endif
################### WIRELESS #########################
ifeq ($(CONFIG_ATBM_APOLLO_DEBUG),)
ATBM_WIFI__EXT_CCFLAGS += -DCONFIG_ATBM_APOLLO_DEBUG=1
CONFIG_ATBM_APOLLO_DEBUG=y
endif
#####################################################
export
ifeq ($(CONFIG_MAC80211_ATBM_RC_MINSTREL),)
ATBM_WIFI__EXT_CCFLAGS += -DCONFIG_MAC80211_ATBM_RC_MINSTREL=1
CONFIG_MAC80211_ATBM_RC_MINSTREL=y
endif
ifeq ($(CONFIG_MAC80211_ATBM_RC_MINSTREL_HT),)
ATBM_WIFI__EXT_CCFLAGS += -DCONFIG_MAC80211_ATBM_RC_MINSTREL_HT=1
CONFIG_MAC80211_ATBM_RC_MINSTREL_HT=y
endif

ifeq ($(USB_BUS),y)
HIF:=usb
endif
ifeq ($(SDIO_BUS),y)
HIF:=sdio
endif
ifeq ($(SPI_BUS),y)
HIF:=spi
endif
 
obj-$(CONFIG_ATBM_APOLLO) += hal_apollo/
endif

