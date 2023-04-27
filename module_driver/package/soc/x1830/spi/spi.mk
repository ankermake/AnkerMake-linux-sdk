
#-------------------------------------------------------
package_name = soc_spi
package_depends = utils
package_module_src = soc/x1830/spi
package_make_hook =
package_init_hook =
package_finalize_hook = spi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

spi_init_file = output/soc_spi.sh

define spi_finalize_hook
	$(Q)cp soc/x1830/spi/soc_spi.ko output/
	$(Q)echo "insmod soc_spi.ko \\" > $(spi_init_file)

	$(Q)echo -n "	spi0_is_enable=$(if $(MD_X1830_SPI0),1,0) " >> $(spi_init_file)
	$(Q)echo -n "spi0_miso=$(if $(MD_X1830_SPI0),$(MD_X1830_SPI0_MISO),-1) " >> $(spi_init_file)
	$(Q)echo -n "spi0_mosi=$(if $(MD_X1830_SPI0),$(MD_X1830_SPI0_MOSI),-1) " >> $(spi_init_file)
	$(Q)echo "spi0_clk=$(if $(MD_X1830_SPI0),$(MD_X1830_SPI0_CLK),-1) \\" >> $(spi_init_file)

	$(Q)echo -n "	spi1_is_enable=$(if $(MD_X1830_SPI1),1,0) " >> $(spi_init_file)
	$(Q)echo -n "spi1_miso=$(if $(MD_X1830_SPI1),$(MD_X1830_SPI1_MISO),-1) " >> $(spi_init_file)
	$(Q)echo -n "spi1_mosi=$(if $(MD_X1830_SPI1),$(MD_X1830_SPI1_MOSI),-1) " >> $(spi_init_file)
	$(Q)echo "spi1_clk=$(if $(MD_X1830_SPI1),$(MD_X1830_SPI1_CLK),-1) " >> $(spi_init_file)
endef
