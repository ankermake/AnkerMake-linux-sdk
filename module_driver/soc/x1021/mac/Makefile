include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_MAC_NAME := soc_mac_driver

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

stmmac-y += stmmac_platform.o mac_get_hwaddr.o

stmmac-$(MD_X1021_MAC_INNERAL_PHY) += inneral_mac_device.o
stmmac-$(MD_X1021_MAC_EXTERNAL_PHY) += external_mac_device.o

stmmac-objs := stmmac_main.o stmmac_ethtool.o stmmac_mdio.o ring_mode.o	\
		chain_mode.o dwmac_lib.o dwmac1000_core.o  dwmac1000_dma.o \
		enh_desc.o  norm_desc.o mmc_core.o stmmac_hwtstamp.o stmmac_ptp.o $(stmmac-y)

$(MODULE_MAC_NAME)-y = $(stmmac-objs)
obj-m += $(MODULE_MAC_NAME).o

ifeq ($(MD_X1021_MAC_INNERAL_PHY),y)
soc_mac_phy_driver-y = mac_phy.o
obj-m += soc_mac_phy_driver.o
endif
