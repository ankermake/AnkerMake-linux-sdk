
obj-y		+= board_base.o misc.o
obj-y		+= i2c/
obj-y		+= spi/
obj-y		+= wifi-bt/
obj-y		+= led/

obj-$(CONFIG_SND_ASOC_INGENIC)	+= speaker.o
obj-$(CONFIG_JZ_MICCHAR)	+= speaker.o
obj-$(CONFIG_x1800_MIC)		+= sound.o
obj-$(CONFIG_JZMMC_V12)		+= mmc.o
obj-$(CONFIG_KEYBOARD_GPIO)	+= keyboard_gpio.o
obj-$(CONFIG_VIDEO_TX_ISP)	+= sensor_board.o

obj-$(CONFIG_JZ_SPI) 			+= spi_bus.o
obj-$(CONFIG_JZ_SFC) 			+= sfc_bus.o
obj-$(CONFIG_SPI_GPIO) 			+= spi_bus.o

obj-$(CONFIG_BCMDHD_1_141_66)	+= rtc.o bcm_wlan_device.o bcm_1_141_66_wlan_power_control.o bcm_get_mac_from_sys.o
obj-$(CONFIG_BCM_43438_RFKILL)     += bcm_bt_power_control.o
obj-$(CONFIG_ESP8089)   += esp8089_wlan_dev_control.o
