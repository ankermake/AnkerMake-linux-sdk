package-y += package/drivers/utils/utils.mk

package-$(MD_GPIO_PS2) += package/drivers/ps2_gpio/ps2_gpio.mk
package-$(MD_PWM_AUDIO) += package/drivers/pwm_audio/pwm_audio.mk
package-$(MD_RMEM_MANAGER) += package/drivers/rmem_manager/rmem_manager.mk
package-$(MD_SPI_GPIO) += package/drivers/spi_gpio/spi_gpio.mk
package-$(MD_SPI_FB) += package/drivers/spi_fb/spi_fb.mk
package-$(MD_I2C_GPIO) += package/drivers/i2c_gpio/i2c_gpio.mk
package-$(MD_SPI_DEVICE_HELPER) += package/drivers/spidev_helper/spidev_helper.mk
package-$(MD_SPI_DEVICE) += package/drivers/spidev/spidev.mk
package-$(MD_BT_HCIUART_RTKH5_KERNEL_4_4_94) += package/drivers/hci_uart_h5_kernel_4_4_94/hci_uart_h5_kernel_4_4_94.mk
package-$(MD_BT_HCIUART_RTKH5_KERNEL_3_10_14) += package/drivers/hci_uart_h5_kernel_3_10_14/hci_uart_h5_kernel_3_10_14.mk