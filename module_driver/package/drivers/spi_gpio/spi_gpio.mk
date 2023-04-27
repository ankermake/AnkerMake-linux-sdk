
#-------------------------------------------------------
package_name = spi_gpio
package_depends = utils
package_module_src = drivers/spi_gpio/
package_make_hook =
package_init_hook =
package_finalize_hook = spi_gpio_finalize_hook
package_clean_hook =
#-------------------------------------------------------

spi_gpio_init_file = output/spi_gpio_add.sh

define spi_gpio_cmd
	$(Q)echo -n 'echo ' >> $(spi_gpio_init_file)
	$(Q)echo -n 'bus_num=$($(1)_BUS_NUM) ' >> $(spi_gpio_init_file)
	$(Q)echo -n 'sck=$($(1)_SCK) ' >> $(spi_gpio_init_file)
	$(Q)echo -n 'mosi=$($(1)_MOSI) ' >> $(spi_gpio_init_file)
	$(Q)echo -n 'miso=$($(1)_MISO) ' >> $(spi_gpio_init_file)
	$(Q)echo -n 'num_chipselect=$($(1)_NUM_CHIPSELECT) ' >> $(spi_gpio_init_file)
	$(Q)echo ' > spi_bus' >> $(spi_gpio_init_file)
endef

do_spi_gpio_cmd = $(if $($(strip $1)), $(call spi_gpio_cmd,$(strip $1)))

define spi_gpio_finalize_hook
	$(Q)cp drivers/spi_gpio/spi_gpio_add.ko output/
	$(Q)echo "insmod spi_gpio_add.ko" > $(spi_gpio_init_file)
	$(Q)echo "cd /sys/module/spi_gpio_add/parameters/" >> $(spi_gpio_init_file)
	$(call do_spi_gpio_cmd, MD_SPI_GPIO0)
	$(call do_spi_gpio_cmd, MD_SPI_GPIO1)
	$(call do_spi_gpio_cmd, MD_SPI_GPIO2)
	$(call do_spi_gpio_cmd, MD_SPI_GPIO3)
	$(call do_spi_gpio_cmd, MD_SPI_GPIO4)
	$(call do_spi_gpio_cmd, MD_SPI_GPIO5)
	$(call do_spi_gpio_cmd, MD_SPI_GPIO6)
	$(call do_spi_gpio_cmd, MD_SPI_GPIO7)
endef
