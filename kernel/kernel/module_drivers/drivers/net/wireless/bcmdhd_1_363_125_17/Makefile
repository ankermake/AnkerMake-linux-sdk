# cywdhd
#################
# Usage and porting guide for this Makefile
#################
# 1 Default to the module external build, that is, no need to copy the driver into the kernel source tree
#  Build with:
#       make KDIR=/path/to/your/kernel
# 2 Default to x86 build: Build the x86 driver without any modification outside of kernel
# 3 Just change the 'DHD_PLATFORM=imx' and the IMx OOB version can work!
# 4 Copy DHD_PLATFORM=imx as the start point and create your own name

#####################
# SDIO Basic feature
#####################
MODULE_NAME := bcmdhd_1_363_125_17
DHDCFLAGS += -Wall -Wstrict-prototypes -Dlinux -DLINUX -DBCMDRIVER            \
	-DBCMDONGLEHOST -DUNRELEASEDCHIP -DBCMDMA32 -DBCMFILEIMAGE            \
	-DSHOW_EVENTS -DBCMDBG -DWLP2P                                        \
	-DWIFI_ACT_FRAME -DARP_OFFLOAD_SUPPORT                                \
	-DKEEP_ALIVE -DCSCAN -DPKT_FILTER_SUPPORT                             \
	-DEMBEDDED_PLATFORM -DPNO_SUPPORT                                     \
	-DDHD_DONOT_FORWARD_BCMEVENT_AS_NETWORK_PKT                           \
	-DCUSTOMER_HW2
#	-Idrivers/net/wireless/bcmdhd_1_363_125_17 -Idrivers/net/wireless/bcmdhd_1_363_125_17/include

#################
# Common feature
#################
DHDCFLAGS += -DWL_CFG80211
# Print out kernel panic point of file and line info when assertion happened
DHDCFLAGS += -DBCMASSERT_LOG

# keepalive
DHDCFLAGS += -DCUSTOM_KEEP_ALIVE_SETTING=28000

DHDCFLAGS += -DVSDB

# Fix __DATE__ & __TIME__ error on gcc version 4.9 or later
GCCVERSION = $(shell $(CC) -dumpversion | cut -f1-2 -d.)
ifeq ($(shell expr $(GCCVERSION) \>= 4.9),1)
  DHDCFLAGS += -Wno-error=date-time
endif

# For p2p connection issue
DHDCFLAGS += -DWL_SCB_TIMEOUT=10

# TDLS enable
DHDCFLAGS += -DWLTDLS
# For TDLS tear down inactive time 40 sec
DHDCFLAGS += -DCUSTOM_TDLS_IDLE_MODE_SETTING=40000
# for TDLS RSSI HIGH for establishing TDLS link
DHDCFLAGS += -DCUSTOM_TDLS_RSSI_THRESHOLD_HIGH=-60
# for TDLS RSSI HIGH for tearing down TDLS link
DHDCFLAGS += -DCUSTOM_TDLS_RSSI_THRESHOLD_LOW=-70

# Roaming/
DHDCFLAGS += -DROAM_AP_ENV_DETECTION
DHDCFLAGS += -DROAM_ENABLE -DROAM_CHANNEL_CACHE -DROAM_API
DHDCFLAGS += -DENABLE_FW_ROAM_SUSPEND
# Roaming trigger
DHDCFLAGS += -DCUSTOM_ROAM_TRIGGER_SETTING=-75
DHDCFLAGS += -DCUSTOM_ROAM_DELTA_SETTING=10
# Voice Enterprise
DHDCFLAGS += -DWLFBT
# Set PM 2 always regardless suspend/resume
DHDCFLAGS += -DSUPPORT_PM2_ONLY

# For special PNO Event keep wake lock for 10sec
DHDCFLAGS += -DCUSTOM_PNO_EVENT_LOCK_xTIME=10
DHDCFLAGS += -DMIRACAST_AMPDU_SIZE=8

#Gscan
# DHDCFLAGS += -DGSCAN_SUPPORT
#RTT
DHDCFLAGS += -DRTT_SUPPORT -DRTT_DEBUG


#Link Statistics
DHDCFLAGS += -DLINKSTAT_SUPPORT

# Early suspend
DHDCFLAGS += -DDHD_USE_EARLYSUSPEND

# For Scan result patch
DHDCFLAGS += -DESCAN_RESULT_PATCH

# For Static Buffer
ifeq ($(CONFIG_DHD_USE_STATIC_BUF),y)
  DHDCFLAGS += -DENHANCED_STATIC_BUF
  DHDCFLAGS += -DSTATIC_WL_PRIV_STRUCT
endif
ifneq ($(CONFIG_DHD_USE_SCHED_SCAN),)
DHDCFLAGS += -DWL_SCHED_SCAN
endif

# Ioctl timeout 5000ms
DHDCFLAGS += -DIOCTL_RESP_TIMEOUT=5000

# Prevent rx thread monopolize
DHDCFLAGS += -DWAIT_DEQUEUE

# idle count
DHDCFLAGS += -DDHD_USE_IDLECOUNT

# SKB TAILPAD to avoid out of boundary memory access
DHDCFLAGS += -DDHDENABLE_TAILPAD

# Wi-Fi Direct
DHDCFLAGS += -DWL_CFG80211_VSDB_PRIORITIZE_SCAN_REQUEST
DHDCFLAGS += -DWL_CFG80211_STA_EVENT
DHDCFLAGS += -DWL_IFACE_COMB_NUM_CHANNELS
DHDCFLAGS += -DWL_ENABLE_P2P_IF
#SCAN time
DHDCFLAGS += -DCUSTOM_SET_SHORT_DWELL_TIME
DHDCFLAGS += -DCUSTOM_FORCE_NODFS_FLAG


# SoftAP
DHDCFLAGS += -DSET_RANDOM_MAC_SOFTAP
DHDCFLAGS += -DWL_CFG80211_ACL

##########################
# driver type
# m: module type driver
# y: built-in type driver
##########################
DRIVER_TYPE ?= m

#########################
# Chip dependent feature
#########################

DHDCFLAGS += -DUSE_WL_TXBF
# DHDCFLAGS += -DUSE_WL_FRAMEBURST
DHDCFLAGS += -DCUSTOM_DPC_CPUCORE=0
DHDCFLAGS += -DPROP_TXSTATUS_VSDB
DHDCFLAGS += -DCUSTOM_PSPRETEND_THR=30
# New Features
DHDCFLAGS += -DWL11U
DHDCFLAGS += -DMFP
# The LPC would have the Tx 5G tput drop around 10Mbps but saving powers
# DHDCFLAGS += -DDHD_ENABLE_LPC
DHDCFLAGS += -DCUSTOM_COUNTRY_CODE

### Platform dependent START
#DHD_PLATFORM ?= linux
DHD_PLATFORM ?= ingenic
ifeq ($(DHD_PLATFORM), ingenic)
  CONFIG_BCMDHD_1_363_125_17_SDIO = y
  ## We only add here for the flags in our generic code
  DHDCFLAGS += -DANDROID_SDIO_RESET
  DHDCFLAGS += -DOOB_INTR_ONLY -DHW_OOB
  DHDCFLAGS += -DCUSTOM_SDIO_F2_BLKSIZE=128
  DHDCFLAGS += -DDHD_OF_SUPPORT -DCUSTOMER_OOB
  DHDCFLAGS += -DLOAD_DHD_WITH_FW_ALIVE
  FW_ALIVE = 1
endif


ifeq ($(DHD_PLATFORM), imx_android)
  CONFIG_BCMDHD_1_363_125_17_SDIO = y
# Config DTS support
  DHDCFLAGS += -DCONFIG_DTS
# for iMX platform
  DHDCFLAGS += -DCUSTOMER_IMX
  DHDCFLAGS += -DOOB_INTR_ONLY -DHW_OOB
  DHDCFLAGS += -DANDROID_SDIO_RESET
# The flags only worked for supported host
# Remove it if you found the dhdsdio_rxglom erros
  DHDCFLAGS += -DCUSTOM_RXCHAIN=1
endif

ifeq ($(DHD_PLATFORM), imx)
  CONFIG_BCMDHD_1_363_125_17_SDIO = y
# Config DTS support
  DHDCFLAGS += -DCONFIG_DTS
# for iMX platform
  DHDCFLAGS += -DCUSTOMER_IMX
  DHDCFLAGS += -DOOB_INTR_ONLY -DHW_OOB
# The flags only worked for supported host
# Remove it if you found the dhdsdio_rxglom erros
  DHDCFLAGS += -DCUSTOM_RXCHAIN=1
endif

ifeq ($(CONFIG_BCMDHD_1_363_125_17_PCIE), y)
  CONFIG_BCMDHD_1_363_125_17_PCIE = y
else
ifeq ($(CONFIG_BCMDHD_1_363_125_17_USB), y)
  CONFIG_BCMDHD_1_363_125_17_USB = y
else
  CONFIG_BCMDHD_1_363_125_17_SDIO = y
endif
endif

### Platform dependent END

ifneq ($(CONFIG_BCMDHD_1_363_125_17_SDIO),)
  DHDCFLAGS += -DBDC -DTOE -DDHD_BCMEVENTS -DMMC_SDIO_ABORT
  DHDCFLAGS += -DBCMSDIO -DBCMLXSDMMC -DUSE_SDIOFIFO_IOVAR
  DHDCFLAGS += -DPROP_TXSTATUS
  DHDCFLAGS += -DCUSTOM_AMPDU_MPDU=16
  DHDCFLAGS += -DCUSTOM_AMPDU_BA_WSIZE=64
# tput enhancement
  DHDCFLAGS += -DCUSTOM_GLOM_SETTING=8
  DHDCFLAGS += -DBCMSDIOH_TXGLOM -DCUSTOM_TXGLOM=1
  DHDCFLAGS += -DDHDTCPACK_SUPPRESS
  DHDCFLAGS += -DRXFRAME_THREAD
  DHDCFLAGS += -DCUSTOM_MAX_TXGLOM_SIZE=31
  DHDCFLAGS += -DMAX_HDR_READ=128
  DHDCFLAGS += -DDHD_FIRSTREAD=128
  # bcn_timeout
  DHDCFLAGS += -DCUSTOM_BCN_TIMEOUT=5
 endif

ifneq ($(CONFIG_BCMDHD_1_363_125_17_PCIE),)
  DHDCFLAGS += -DPCIE_FULL_DONGLE -DBCMPCIE -DCUSTOM_DPC_PRIO_SETTING=-1
# tput enhancement
  DHDCFLAGS += -DCUSTOM_AMPDU_BA_WSIZE=64
  DHDCFLAGS += -DCUSTOM_AMPDU_MPDU=32
  DHDCFLAGS += -DCUSTOM_AMPDU_RELEASE=16
  DHDCFLAGS += -DPROP_TXSTATUS_VSDB
# Disable watchdog thread
  DHDCFLAGS += -DCUSTOM_DHD_WATCHDOG_MS=0
  DHDCFLAGS += -DMAX_CNTL_TX_TIMEOUT=1
  DHDCFLAGS += -DSUPPORT_LINKDOWN_RECOVERY
endif

ifneq ($(CONFIG_BCMDHD_1_363_125_17_USB),)
  DHDCFLAGS += -DBDC -DTOE
  DHDCFLAGS += -DDHD_BCMEVENTS -DSHOW_EVENTS -DSHOW_LOGTRACE
  DHDCFLAGS += -DBCMDHDUSB -DBCMDBUS -DBCMTRXV2 -DDBUS_USB_LOOPBACK
  DHDCFLAGS += -DPROP_TXSTATUS
  DHDCFLAGS += -DLIMIT_BORROW
endif

ifeq ($(CONFIG_DHD_USE_STATIC_BUF),y)
  DHDCFLAGS += -DDHD_USE_STATIC_IOCTLBUF
endif
# Print 802.1X packets
DHDCFLAGS += -DDHD_8021X_DUMP
# Print DHCP packets
DHDCFLAGS += -DDHD_DHCP_DUMP


#EXTRA_LDFLAGS += --strip-debug

ifeq ($(DRIVER_TYPE),y)
  DHDCFLAGS += -DENABLE_INSMOD_NO_FW_LOAD
  DHDCFLAGS += -DUSE_LATE_INITCALL_SYNC
endif

## Attend to fix the suspend/resume -16 error issue, refer to SWDHD-409
DHDCFLAGS += -DSUSPEND16


## Flags merged from Linux build
DHDCFLAGS += -DCUSTOMER_IPCAM -DWL11ULB -DWL_NAN -DWL_SUPPORT_AUTO_CHANNEL -DWL_VIRTUAL_APSTA

## Flags that may depends on Linux kernel versions
DHDCFLAGS += -DTSQ_MULTIPLIER=10

## Flags maybe useful but disable it

# The WLFC AC queue credit borrow
# DHDCFLAGS += -DLIMIT_BORROW

# Advanced IBSS features
# DHDCFLAGS += -DWLAIBSS -DWLAIBSS_PS

# p2P service discovery offload
# DHDCFLAGS += -DWL_SDO

EXTRA_CFLAGS += $(DHDCFLAGS) -DDHD_DEBUG
EXTRA_CFLAGS += -DSRCBASE=\"$(src)\"
EXTRA_CFLAGS += -I$(srctree)/$(src)/include/ -I$(srctree)/$(src)/
#EXTRA_CFLAGS += -I$(srctree)/$(src)/include/ -I$(srctree)/$(src)/
#KBUILD_CFLAGS += -I$(LINUXDIR)/include -I$(shell pwd)

DHDOFILES := dhd_pno.o dhd_common.o dhd_ip.o dhd_custom_gpio.o \
	dhd_linux.o dhd_linux_sched.o dhd_cfg80211.o dhd_linux_wq.o aiutils.o bcmevent.o \
	bcmutils.o bcmwifi_channels.o hndpmu.o linux_osl.o sbutils.o siutils.o \
	wl_android.o wl_cfg80211.o wl_cfgp2p.o wl_cfg_btcoex.o wldev_common.o wl_linux_mon.o  \
	dhd_linux_platdev.o dhd_pno.o dhd_rtt.o dhd_linux_wq.o wl_cfg_btcoex.o \
        hnd_pktq.o hnd_pktpool.o wl_cfgvendor.o dhd_debug_linux.o bcmxtlv.o dhd_debug.o bcm_app_utils.o \
	wl_cfgnan.o

ifeq ($(FW_ALIVE), 1)
  DHDCFLAGS += -DLOAD_DHD_WITH_FW_ALIVE
  DHDOFILES += dhd_chip_info.o
endif


ifneq ($(CONFIG_BCMDHD_1_363_125_17_SDIO),)
  DHDOFILES += bcmsdh.o bcmsdh_linux.o bcmsdh_sdmmc.o bcmsdh_sdmmc_linux.o
  DHDOFILES += dhd_cdc.o dhd_wlfc.o dhd_sdio.o
endif

ifneq ($(CONFIG_BCMDHD_1_363_125_17_PCIE),)
  DHDOFILES += dhd_pcie.o dhd_pcie_linux.o dhd_msgbuf.o dhd_flowring.o
  DHDOFILES += pcie_core.o
endif

ifneq ($(CONFIG_BCMDHD_1_363_125_17_USB),)
  DHDOFILES += dbus.o dbus_usb.o dbus_usb_linux.o
  DHDOFILES += dhd_cdc.o dhd_wlfc.o
endif

#cywdhd-objs := $(DHDOFILES)
obj-$(CONFIG_BCMDHD_1_363_125_17) += $(MODULE_NAME).o
$(MODULE_NAME)-objs += $(DHDOFILES)
obj-$(DRIVER_TYPE)   += $(MODULE_NAME).o

all:
	@echo "$(MAKE) --no-print-directory -C $(KDIR) SUBDIRS=$(CURDIR) modules"
	@$(MAKE) --no-print-directory -C $(KDIR) SUBDIRS=$(CURDIR) modules

clean:
	rm -rf *.o *.ko *.mod.c *~ .*.cmd *.o.cmd .*.o.cmd \
	Module.symvers modules.order .tmp_versions modules.builtin

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install
