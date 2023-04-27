#--------------------------------------------------
package_name = soc_mac
package_depends =
package_module_src = soc/x2000_510/mac/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_mac_finalize_hook
package_clean_hook =
#--------------------------------------------------

soc_mac_driver_init_file = output/soc_mac_driver.sh

define soc_mac_finalize_hook
	$(Q)cp soc/x2000_510/mac/soc_mac_driver.ko output/

	$(Q)echo -n "insmod soc_mac_driver.ko " > $(soc_mac_driver_init_file)

	$(Q)echo -n "mac0_enable_flag=$(if $(MD_X2000_510_MAC0),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac0_phy_interface=$(if $(MD_X2000_510_MAC0_RGMII_MODE),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac0_enable_crystal_clk=$(if $(MD_X2000_510_MAC0_CRYSTAL_CLK),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac0_enable_tx_clk=$(if $(MD_X2000_510_MAC0_TX_CLK),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac0_tx_clk_rate=$(if $(MD_X2000_510_MAC0_TX_CLK_RATE),$(MD_X2000_510_MAC0_TX_CLK_RATE),0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac0_tx_clk_enable_delay=$(if $(MD_X2000_510_MAC0_TX_CLK_DELAY_US),$(MD_X2000_510_MAC0_TX_CLK_DELAY_US),0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac0_phy_probe_mask=$(if $(MD_X2000_510_MAC0_PHY_PROBE_MASK),$(MD_X2000_510_MAC0_PHY_PROBE_MASK),0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac0_phy_reset_gpio=$(if $(MD_X2000_510_MAC0_PHY_RESET_GPIO),$(MD_X2000_510_MAC0_PHY_RESET_GPIO),-1) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac0_phy_reset_level=$(if $(MD_X2000_510_MAC0_PHY_VALID_LEVEL),$(MD_X2000_510_MAC0_PHY_VALID_LEVEL),0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac0_phy_reset_time_us=$(if $(MD_X2000_510_MAC0_PHY_RESET_TIME_US),$(MD_X2000_510_MAC0_PHY_RESET_TIME_US),0 ) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac0_rx_clk_delay=$(if $(MD_X2000_510_MAC0_RX_CLK_DELAY_UNIT),$(MD_X2000_510_MAC0_RX_CLK_DELAY_UNIT),0 ) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac0_tx_clk_delay=$(if $(MD_X2000_510_MAC0_TX_CLK_DELAY_UNIT),$(MD_X2000_510_MAC0_TX_CLK_DELAY_UNIT),0 ) " >> $(soc_mac_driver_init_file)
	$(Q)echo    "mac0_enable_virtual_phy=$(if $(MD_X2000_510_MAC0_VIRTUAL_PHY),1,0) \\" >> $(soc_mac_driver_init_file)

	$(Q)echo -n "mac1_enable_flag=$(if $(MD_X2000_510_MAC1),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac1_phy_interface=$(if $(MD_X2000_510_MAC1_RGMII_MODE),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac1_enable_crystal_clk=$(if $(MD_X2000_510_MAC1_CRYSTAL_CLK),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac1_enable_tx_clk=$(if $(MD_X2000_510_MAC1_TX_CLK),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac1_tx_clk_rate=$(if $(MD_X2000_510_MAC1_TX_CLK_RATE),$(MD_X2000_510_MAC1_TX_CLK_RATE),0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac1_tx_clk_enable_delay=$(if $(MD_X2000_510_MAC1_TX_CLK_DELAY_US),$(MD_X2000_510_MAC1_TX_CLK_DELAY_US),0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac1_phy_probe_mask=$(if $(MD_X2000_510_MAC1_PHY_PROBE_MASK),$(MD_X2000_510_MAC1_PHY_PROBE_MASK),0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac1_phy_reset_gpio=$(if $(MD_X2000_510_MAC1_PHY_RESET_GPIO),$(MD_X2000_510_MAC1_PHY_RESET_GPIO),-1) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac1_phy_reset_level=$(if $(MD_X2000_510_MAC1_PHY_VALID_LEVEL),$(MD_X2000_510_MAC1_PHY_VALID_LEVEL),0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac1_phy_reset_time_us=$(if $(MD_X2000_510_MAC1_PHY_RESET_TIME_US),$(MD_X2000_510_MAC1_PHY_RESET_TIME_US),0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac1_rx_clk_delay=$(if $(MD_X2000_510_MAC1_RX_CLK_DELAY_UNIT),$(MD_X2000_510_MAC1_RX_CLK_DELAY_UNIT),0 ) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac1_tx_clk_delay=$(if $(MD_X2000_510_MAC1_TX_CLK_DELAY_UNIT),$(MD_X2000_510_MAC1_TX_CLK_DELAY_UNIT),0 ) " >> $(soc_mac_driver_init_file)
	$(Q)echo    "mac1_enable_virtual_phy=$(if $(MD_X2000_510_MAC1_VIRTUAL_PHY),1,0) \\" >> $(soc_mac_driver_init_file)

	$(Q)echo -n "phy_clk_rate=$(if $(MD_X2000_510_PHY_CLK_RATE),$(MD_X2000_510_PHY_CLK_RATE),0 ) " >> $(soc_mac_driver_init_file)
	$(Q)echo    "phy_clk_delay_us=$(if $(MD_X2000_510_PHY_CLK_DELAY_US),$(MD_X2000_510_PHY_CLK_DELAY_US),0 ) " >> $(soc_mac_driver_init_file)

endef
