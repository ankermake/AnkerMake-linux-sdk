#--------------------------------------------------
package_name = soc_mac
package_depends =
package_module_src = soc/x1600/mac/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_mac_finalize_hook
package_clean_hook =
#--------------------------------------------------

soc_mac_driver_init_file = output/soc_mac_driver.sh

define soc_mac_finalize_hook
	$(Q)cp soc/x1600/mac/soc_mac_driver.ko output/

	$(Q)echo -n "insmod soc_mac_driver.ko " > $(soc_mac_driver_init_file)

	$(Q)echo -n "phy_probe_mask=$(MD_X1600_PHY_PROBE_MASK) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "phy_power_gpio=$(MD_X1600_PHY_POWER_GPIO) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "phy_power_level=$(MD_X1600_PHY_POWER_LEVEL) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "phy_reset_gpio=$(MD_X1600_PHY_RESET_GPIO) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "phy_reset_level=$(MD_X1600_PHY_VALID_LEVEL) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "phy_reset_time_us=$(MD_X1600_PHY_RESET_TIME_US) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "enable_virtual_phy=$(if $(MD_X1600_VIRTUAL_PHY),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo    "phy_clk_delay_us=$(MD_X1600_PHY_CLK_DELAY_US) " >> $(soc_mac_driver_init_file)

endef
