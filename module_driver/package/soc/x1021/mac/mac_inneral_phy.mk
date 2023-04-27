#--------------------------------------------------
package_name = soc_mac
package_depends =
package_module_src = soc/x1021/mac/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_mac_finalize_hook
package_clean_hook =
#--------------------------------------------------

soc_mac_driver_init_file = output/soc_mac_driver.sh

define soc_mac_finalize_hook
	$(Q)cp soc/x1021/mac/soc_mac_driver.ko output/
	$(Q)cp soc/x1021/mac/soc_mac_phy_driver.ko output/

	$(Q)echo "insmod soc_mac_phy_driver.ko " > $(soc_mac_driver_init_file)
	$(Q)echo -n "insmod soc_mac_driver.ko " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac_10m_speed_led=$(if $(MD_X1021_MAC_10M_SPEED_LED),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac_100m_speed_led=$(if $(MD_X1021_MAC_100M_SPEED_LED),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac_link_led=$(if $(MD_X1021_MAC_LINK_LED),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac_duplex_led=$(if $(MD_X1021_MAC_DUPLEX_LED),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo -n "mac_tx_led=$(if $(MD_X1021_MAC_TX_LED),1,0) " >> $(soc_mac_driver_init_file)
	$(Q)echo    "mac_rx_led=$(if $(MD_X1021_MAC_RX_LED),1,0) " >> $(soc_mac_driver_init_file)

endef
