
#-------------------------------------------------------
package_name = soc_msc
package_depends = utils
package_module_src = soc/x2000/msc/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_msc_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_msc_init_file = output/soc_msc.sh

define soc_msc_finalize_hook
	$(Q)cp soc/x2000/msc/soc_msc.ko output/
	$(Q)echo "insmod soc_msc.ko \\" > $(soc_msc_init_file)

	$(Q)echo "wifi_power_on=$(MD_X2000_WIFI_POWER_ON) \\" >> $(soc_msc_init_file)
	$(Q)echo "wifi_power_on_level=$(MD_X2000_WIFI_POWER_LEVEL) \\" >> $(soc_msc_init_file)
	$(Q)echo "wifi_reg_on=$(MD_X2000_WIFI_REG_ON) \\" >> $(soc_msc_init_file)
	$(Q)echo "wifi_reg_on_level=$(MD_X2000_WIFI_REG_LEVEL) \\" >> $(soc_msc_init_file)

	$(Q)echo "	msc0_is_enable=$(if $(MD_X2000_MSC0),1,0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_cd_method=$(if $(MD_X2000_MSC0),$(MD_X2000_MSC0_CD_METHOD),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_bus_width=$(if $(MD_X2000_MSC0),$(MD_X2000_MSC0_BUS_WIDTH),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_speed=$(if $(MD_X2000_MSC0),$(MD_X2000_MSC0_SPEED),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_max_frequency=$(if $(MD_X2000_MSC0),$(MD_X2000_MSC0_MAX_FREQUENCY),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_cap_power_off_card=$(if $(MD_X2000_MSC0_CAP_POWER_OFF_CARD),$(MD_X2000_MSC0_CAP_POWER_OFF_CARD),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_cap_mmc_hw_reset=$(if $(MD_X2000_MSC0_CAP_MMC_HW_RESET),$(MD_X2000_MSC0_CAP_MMC_HW_RESET),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_cap_sdio_irq=$(if $(MD_X2000_MSC0_CAP_SDIO_IRQ),$(MD_X2000_MSC0_CAP_SDIO_IRQ),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_full_pwr_cycle=$(if $(MD_X2000_MSC0_FULL_PWR_CYCLE),$(MD_X2000_MSC0_FULL_PWR_CYCLE),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_keep_power_in_suspend=$(if $(MD_X2000_MSC0_KEEP_POWER_IN_SUSPEND),$(MD_X2000_MSC0_KEEP_POWER_IN_SUSPEND),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_enable_sdio_wakeup=$(if $(MD_X2000_MSC0_ENABLE_SDIO_WAKEUP),$(MD_X2000_MSC0_ENABLE_SDIO_WAKEUP),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_dsr=$(if $(MD_X2000_MSC0),$(MD_X2000_MSC0_DSR),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_pio_mode=$(if $(MD_X2000_MSC0_PIO_MODE),$(MD_X2000_MSC0_PIO_MODE),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_enable_autocmd12=$(if $(MD_X2000_MSC0_ENABLE_AUTOCMD12),$(MD_X2000_MSC0_ENABLE_AUTOCMD12),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_enable_cpm_rx_tuning=$(if $(MD_X2000_MSC0_ENABLE_CPM_RX_TUNING),$(MD_X2000_MSC0_ENABLE_CPM_RX_TUNING),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_enable_cpm_tx_tuning=$(if $(MD_X2000_MSC0_ENABLE_CPM_TX_TUNING),$(MD_X2000_MSC0_ENABLE_CPM_TX_TUNING),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc0_sdio_clk=$(if $(MD_X2000_MSC0_SDIO_CLK),$(MD_X2000_MSC0_SDIO_CLK),0) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc0_rst=$(if $(MD_X2000_MSC0_RST_GPIO),$(MD_X2000_MSC0_RST_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc0_rst_enable_level=$(if $(MD_X2000_MSC0_RST_GPIO),$(MD_X2000_MSC0_RST_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc0_wp=$(if $(MD_X2000_MSC0_WP_GPIO),$(MD_X2000_MSC0_WP_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc0_wp_enable_level=$(if $(MD_X2000_MSC0_WP_GPIO),$(MD_X2000_MSC0_WP_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc0_pwr=$(if $(MD_X2000_MSC0_PWR_GPIO),$(MD_X2000_MSC0_PWR_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc0_pwr_enable_level=$(if $(MD_X2000_MSC0_PWR_GPIO),$(MD_X2000_MSC0_PWR_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc0_cd=$(if $(MD_X2000_MSC0_CD_GPIO),$(MD_X2000_MSC0_CD_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc0_cd_enable_level=$(if $(MD_X2000_MSC0_CD_GPIO),$(MD_X2000_MSC0_CD_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc0_sdr=$(if $(MD_X2000_MSC0_SDR_GPIO),$(MD_X2000_MSC0_SDR_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc0_sdr_enable_level=$(if $(MD_X2000_MSC0_SDR_GPIO),$(MD_X2000_MSC0_SDR_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)

	$(Q)echo "	msc1_is_enable=$(if $(MD_X2000_MSC1),1,0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_cd_method=$(if $(MD_X2000_MSC1),$(MD_X2000_MSC1_CD_METHOD),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_bus_width=$(if $(MD_X2000_MSC1),$(MD_X2000_MSC1_BUS_WIDTH),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_speed=$(if $(MD_X2000_MSC1),$(MD_X2000_MSC1_SPEED),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_max_frequency=$(if $(MD_X2000_MSC1),$(MD_X2000_MSC1_MAX_FREQUENCY),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_cap_power_off_card=$(if $(MD_X2000_MSC1_CAP_POWER_OFF_CARD),$(MD_X2000_MSC1_CAP_POWER_OFF_CARD),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_cap_mmc_hw_reset=$(if $(MD_X2000_MSC1_CAP_MMC_HW_RESET),$(MD_X2000_MSC1_CAP_MMC_HW_RESET),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_cap_sdio_irq=$(if $(MD_X2000_MSC1_CAP_SDIO_IRQ),$(MD_X2000_MSC1_CAP_SDIO_IRQ),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_full_pwr_cycle=$(if $(MD_X2000_MSC1_FULL_PWR_CYCLE),$(MD_X2000_MSC1_FULL_PWR_CYCLE),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_keep_power_in_suspend=$(if $(MD_X2000_MSC1_KEEP_POWER_IN_SUSPEND),$(MD_X2000_MSC1_KEEP_POWER_IN_SUSPEND),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_enable_sdio_wakeup=$(if $(MD_X2000_MSC1_ENABLE_SDIO_WAKEUP),$(MD_X2000_MSC1_ENABLE_SDIO_WAKEUP),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_dsr=$(if $(MD_X2000_MSC1),$(MD_X2000_MSC1_DSR),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_pio_mode=$(if $(MD_X2000_MSC1_PIO_MODE),$(MD_X2000_MSC1_PIO_MODE),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_enable_autocmd12=$(if $(MD_X2000_MSC1_ENABLE_AUTOCMD12),$(MD_X2000_MSC1_ENABLE_AUTOCMD12),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_enable_cpm_rx_tuning=$(if $(MD_X2000_MSC1_ENABLE_CPM_RX_TUNING),$(MD_X2000_MSC1_ENABLE_CPM_RX_TUNING),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_enable_cpm_tx_tuning=$(if $(MD_X2000_MSC1_ENABLE_CPM_TX_TUNING),$(MD_X2000_MSC1_ENABLE_CPM_TX_TUNING),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc1_sdio_clk=$(if $(MD_X2000_MSC1_SDIO_CLK),$(MD_X2000_MSC1_SDIO_CLK),0) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc1_rst=$(if $(MD_X2000_MSC1_RST_GPIO),$(MD_X2000_MSC1_RST_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc1_rst_enable_level=$(if $(MD_X2000_MSC1_RST_GPIO),$(MD_X2000_MSC1_RST_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc1_wp=$(if $(MD_X2000_MSC1_WP_GPIO),$(MD_X2000_MSC1_WP_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc1_wp_enable_level=$(if $(MD_X2000_MSC1_WP_GPIO),$(MD_X2000_MSC1_WP_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc1_pwr=$(if $(MD_X2000_MSC1_PWR_GPIO),$(MD_X2000_MSC1_PWR_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc1_pwr_enable_level=$(if $(MD_X2000_MSC1_PWR_GPIO),$(MD_X2000_MSC1_PWR_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc1_cd=$(if $(MD_X2000_MSC1_CD_GPIO),$(MD_X2000_MSC1_CD_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc1_cd_enable_level=$(if $(MD_X2000_MSC1_CD_GPIO),$(MD_X2000_MSC1_CD_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc1_sdr=$(if $(MD_X2000_MSC1_SDR_GPIO),$(MD_X2000_MSC1_SDR_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc1_sdr_enable_level=$(if $(MD_X2000_MSC1_SDR_GPIO),$(MD_X2000_MSC1_SDR_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)

	$(Q)echo "	msc2_is_enable=$(if $(MD_X2000_MSC2),1,0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_cd_method=$(if $(MD_X2000_MSC2),$(MD_X2000_MSC2_CD_METHOD),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_bus_width=$(if $(MD_X2000_MSC2),$(MD_X2000_MSC2_BUS_WIDTH),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_speed=$(if $(MD_X2000_MSC2),$(MD_X2000_MSC2_SPEED),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_max_frequency=$(if $(MD_X2000_MSC2),$(MD_X2000_MSC2_MAX_FREQUENCY),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_cap_power_off_card=$(if $(MD_X2000_MSC2_CAP_POWER_OFF_CARD),$(MD_X2000_MSC2_CAP_POWER_OFF_CARD),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_cap_mmc_hw_reset=$(if $(MD_X2000_MSC2_CAP_MMC_HW_RESET),$(MD_X2000_MSC2_CAP_MMC_HW_RESET),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_cap_sdio_irq=$(if $(MD_X2000_MSC2_CAP_SDIO_IRQ),$(MD_X2000_MSC2_CAP_SDIO_IRQ),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_full_pwr_cycle=$(if $(MD_X2000_MSC2_FULL_PWR_CYCLE),$(MD_X2000_MSC2_FULL_PWR_CYCLE),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_keep_power_in_suspend=$(if $(MD_X2000_MSC2_KEEP_POWER_IN_SUSPEND),$(MD_X2000_MSC2_KEEP_POWER_IN_SUSPEND),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_enable_sdio_wakeup=$(if $(MD_X2000_MSC2_ENABLE_SDIO_WAKEUP),$(MD_X2000_MSC2_ENABLE_SDIO_WAKEUP),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_dsr=$(if $(MD_X2000_MSC2),$(MD_X2000_MSC2_DSR),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_pio_mode=$(if $(MD_X2000_MSC2_PIO_MODE),$(MD_X2000_MSC2_PIO_MODE),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_enable_autocmd12=$(if $(MD_X2000_MSC2_ENABLE_AUTOCMD12),$(MD_X2000_MSC2_ENABLE_AUTOCMD12),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_enable_cpm_rx_tuning=$(if $(MD_X2000_MSC2_ENABLE_CPM_RX_TUNING),$(MD_X2000_MSC2_ENABLE_CPM_RX_TUNING),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_enable_cpm_tx_tuning=$(if $(MD_X2000_MSC2_ENABLE_CPM_TX_TUNING),$(MD_X2000_MSC2_ENABLE_CPM_TX_TUNING),0) \\" >> $(soc_msc_init_file)
	$(Q)echo "		msc2_sdio_clk=$(if $(MD_X2000_MSC2_SDIO_CLK),$(MD_X2000_MSC2_SDIO_CLK),0) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc2_rst=$(if $(MD_X2000_MSC2_RST_GPIO),$(MD_X2000_MSC2_RST_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc2_rst_enable_level=$(if $(MD_X2000_MSC2_RST_GPIO),$(MD_X2000_MSC2_RST_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc2_wp=$(if $(MD_X2000_MSC2_WP_GPIO),$(MD_X2000_MSC2_WP_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc2_wp_enable_level=$(if $(MD_X2000_MSC2_WP_GPIO),$(MD_X2000_MSC2_WP_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc2_pwr=$(if $(MD_X2000_MSC2_PWR_GPIO),$(MD_X2000_MSC2_PWR_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc2_pwr_enable_level=$(if $(MD_X2000_MSC2_PWR_GPIO),$(MD_X2000_MSC2_PWR_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc2_cd=$(if $(MD_X2000_MSC2_CD_GPIO),$(MD_X2000_MSC2_CD_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc2_cd_enable_level=$(if $(MD_X2000_MSC2_CD_GPIO),$(MD_X2000_MSC2_CD_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
	$(Q)echo -n "		msc2_sdr=$(if $(MD_X2000_MSC2_SDR_GPIO),$(MD_X2000_MSC2_SDR_GPIO),-1) " >> $(soc_msc_init_file)
	$(Q)echo "msc2_sdr_enable_level=$(if $(MD_X2000_MSC2_SDR_GPIO),$(MD_X2000_MSC2_SDR_ENABLE_LEVEL),-1) \\" >> $(soc_msc_init_file)
endef
