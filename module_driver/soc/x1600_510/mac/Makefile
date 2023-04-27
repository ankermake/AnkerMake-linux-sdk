include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_mac_driver

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

stmmac-y += stmmac_platform.o mac_get_hwaddr.o

stmmac-y += ingenic_mac_device.o

stmmac-objs:= stmmac_main.o stmmac_ethtool.o stmmac_mdio.o ring_mode.o	\
			chain_mode.o dwmac_lib.o dwmac1000_core.o  dwmac1000_dma.o \
			enh_desc.o  norm_desc.o mmc_core.o stmmac_hwtstamp.o stmmac_ptp.o $(stmmac-y)

$(MODULE_NAME)-y = $(stmmac-objs)

obj-m+= $(MODULE_NAME).o
