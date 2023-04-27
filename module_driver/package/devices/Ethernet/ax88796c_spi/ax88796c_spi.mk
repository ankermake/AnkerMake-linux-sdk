
#-------------------------------------------------------
package_name = ax88796c-spi
package_depends = utils
package_module_src = devices/Ethernet/ax88796c_spi
package_make_hook =
package_init_hook =
package_finalize_hook = ax88796c_spi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

ax88796c_spi_init_file = output/ax88796c-spi.sh

define ax88796c_spi_finalize_hook
	$(Q)cp devices/Ethernet/ax88796c_spi/ax88796c-spi.ko output/
	$(Q)echo -n 'insmod ax88796c-spi.ko ' > $(ax88796c_spi_init_file)
	$(Q)echo -n 'gpio_cs="$(MD_AX88796C_SPI_GPIO_CS)" ' >> $(ax88796c_spi_init_file)
	$(Q)echo -n 'gpio_irq="$(MD_AX88796C_SPI_GPIO_IRQ)" ' >> $(ax88796c_spi_init_file)
	$(Q)echo 'spi_bus_num=$(MD_AX88796C_SPI_SPI_BUS_NUM) ' >> $(ax88796c_spi_init_file)
endef
