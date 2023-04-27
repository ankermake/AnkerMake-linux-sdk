
obj-y		+= board_base.o
obj-y		+= misc.o
obj-y		+= mmc.o
obj-y		+= sensor_board.o
obj-$(CONFIG_KEYBOARD_GPIO)	+= keyboard_gpio.o

#obj-$(CONFIG_BCM_PM_CORE)   += bcm_power_control.o
obj-$(CONFIG_BCM_43438_RFKILL)   += bcm_power_control.o bcm_bluetooth_devcie.o
obj-$(CONFIG_BCMDHD_1_141_66)   += bcm_power_control.o bcm_wlan_device.o
obj-$(CONFIG_ATBM_WIRELESS)     += atbm_wlan_device.o
obj-$(CONFIG_X1520_MIC)		+= sound.o
obj-$(CONFIG_I2C_GPIO)			+= i2c_bus.o
obj-$(CONFIG_I2C0_V12_JZ)		+= i2c_bus.o
obj-$(CONFIG_I2C1_V12_JZ)		+= i2c_bus.o

obj-$(CONFIG_JZ_SPI) 			+= spi_bus.o
obj-$(CONFIG_JZ_SFC) 			+= sfc_bus.o
obj-$(CONFIG_SPI_GPIO) 			+= spi_bus.o

obj-$(CONFIG_SND_ASOC_INGENIC)	+= speaker.o
obj-$(CONFIG_JZ_MICCHAR)	+= speaker.o
