
obj-y		+= board_base.o
obj-y		+= misc.o
obj-y		+= power_gpio_control.o
obj-$(CONFIG_JZMMC_V12)		+= mmc.o
obj-$(CONFIG_KEYBOARD_GPIO)	+= keyboard_gpio.o
obj-$(CONFIG_STMMAC_PLATFORM) += jz_mac_device.o

obj-$(CONFIG_SOUND_OSS_XBURST)		+= sound.o
obj-$(CONFIG_I2C_GPIO)			+= i2c_bus.o
obj-$(CONFIG_I2C0_V12_JZ)		+= i2c_bus.o
obj-$(CONFIG_I2C1_V12_JZ)		+= i2c_bus.o
obj-$(CONFIG_JZ_SPI) += spi_bus.o
obj-$(CONFIG_JZ_SFC) += sfc_bus.o
obj-$(CONFIG_SPI_GPIO) += spi_bus.o
obj-$(CONFIG_VIDEO_TX_ISP)	+= tx_isp.o
obj-y +=lcd/

ifdef CONFIG_BCMDHD_1_141_66
obj-y += rtc.o 43438_wlan_device.o 43438_wlan_power_control.o bcm_get_mac_from_sys.o
endif
ifdef CONFIG_BCMDHD_1_363_125_7
obj-y += rtc.o 43438_wlan_device.o 43438_wlan_power_control.o bcm_get_mac_from_sys.o
endif
ifdef CONFIG_BCM_43438_RFKILL
obj-y += rtc.o 43438_bt_power_control.o
endif
obj-$(CONFIG_ESP8089)   += esp8089_wlan_dev_control.o
