
#-------------------------------------------------------
package_name = soc_spi
package_depends = utils
package_module_src = soc/x2000/spi
package_make_hook =
package_init_hook =
package_finalize_hook = spi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

spi_init_file = output/soc_spi.sh

define spi_finalize_hook
	$(Q)cp soc/x2000/spi/soc_spi.ko output/
	$(Q)echo "insmod soc_spi.ko \\" > $(spi_init_file)

	$(Q)echo -n "	spi0_is_enable=$(if $(MD_X2000_SPI0),1,0) " >> $(spi_init_file)
	$(Q)echo -n "spi0_miso=$(if $(MD_X2000_SPI0),$(MD_X2000_SPI0_MISO),-1) " >> $(spi_init_file)
	$(Q)echo -n "spi0_mosi=$(if $(MD_X2000_SPI0),$(MD_X2000_SPI0_MOSI),-1) " >> $(spi_init_file)
	$(Q)echo "spi0_clk=$(if $(MD_X2000_SPI0),$(MD_X2000_SPI0_CLK),-1) \\" >> $(spi_init_file)

	$(Q)echo -n "	spi1_is_enable=$(if $(MD_X2000_SPI1),1,0) " >> $(spi_init_file)
	$(Q)echo -n "spi1_miso=$(if $(MD_X2000_SPI1),$(MD_X2000_SPI1_MISO),-1) " >> $(spi_init_file)
	$(Q)echo -n "spi1_mosi=$(if $(MD_X2000_SPI1),$(MD_X2000_SPI1_MOSI),-1) " >> $(spi_init_file)
	$(Q)echo "spi1_clk=$(if $(MD_X2000_SPI1),$(MD_X2000_SPI1_CLK),-1) \\" >> $(spi_init_file)

	$(Q)echo "div_ssi_rate=$(if $(MD_X2000_SPI),$(MD_X2000_SPI_CLK_RATE),100000000) " >> $(spi_init_file)
endef
