

obj-y += board_base.o misc.o

obj-$(CONFIG_KEYBOARD_GPIO)	+= keyboard_gpio.o
obj-$(CONFIG_SOC_CAMERA)     += cim_board.o
obj-$(CONFIG_SPI_JZ47XX) += spi_bus.o
obj-$(CONFIG_SOUND) += sound.o
obj-$(CONFIG_MMC)              += mmc.o
obj-$(CONFIG_USB_OHCI_HCD)     += usb.o

obj-$(CONFIG_I2C_GPIO)          += i2c_bus.o
obj-$(CONFIG_I2C_V12_JZ)       += i2c_bus.o

obj-$(CONFIG_LCD_BYD_8991FTGF)		+= lcd-8991.o

obj-$(CONFIG_BCMDHD_1_141_66)    += 43438_wlan_power_control.o 43438_wlan_device.o
obj-$(CONFIG_BCM_43438_RFKILL)     += 43438_bt_power_control.o 43438_wlan_power_control.o
