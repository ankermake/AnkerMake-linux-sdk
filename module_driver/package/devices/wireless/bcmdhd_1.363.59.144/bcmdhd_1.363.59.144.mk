#-------------------------------------------------------
package_name = bcmdhd_144
package_depends = soc_utils utils
package_module_src = devices/wireless/bcmdhd_1.363.59.144/
package_make_hook =
package_init_hook =
package_finalize_hook = bcmdhd_144_finalize_hook
package_clean_hook =
#-------------------------------------------------------

bcmdhd_144_init_file = output/bcmdhd_144.sh

define bcmdhd_144_finalize_hook
	$(Q)cp devices/wireless/bcmdhd_1.363.59.144/bcmdhd_144.ko output/
	$(Q)echo 'insmod bcmdhd_144.ko \' > $(bcmdhd_144_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_BCMDHD_1_363_59_144_MMC_NUM) \' >> $(bcmdhd_144_init_file)
	$(Q)echo 'bt_wifi_power_valid_level=$(MD_BCMDHD_1_363_59_144_BT_WIFI_POWER_VALID_LEVEL) \' >> $(bcmdhd_144_init_file)
	$(Q)echo 'gpio_bt_wifi_power=$(MD_BCMDHD_1_363_59_144_BT_WIFI_POWER) \' >> $(bcmdhd_144_init_file)
	$(Q)echo 'gpio_wlan_reg_on=$(MD_BCMDHD_1_363_59_144_WLAN_REG_ON) \' >> $(bcmdhd_144_init_file)
	$(Q)echo 'gpio_wlan_wake_host=$(MD_BCMDHD_1_363_59_144_WLAN_WAKE_HOST) \' >> $(bcmdhd_144_init_file)
	$(Q)echo 'gpio_bt_rst_n=-1 \' >> $(bcmdhd_144_init_file)
	$(Q)echo 'gpio_bt_reg_on=$(MD_BCMDHD_1_363_59_144_BT_REG_ON) \' >> $(bcmdhd_144_init_file)
	$(Q)echo 'gpio_host_wake_bt=$(MD_BCMDHD_1_363_59_144_HOST_WAKE_BT) \' >> $(bcmdhd_144_init_file)
	$(Q)echo 'gpio_bt_wake_host=$(MD_BCMDHD_1_363_59_144_BT_WAKE_HOST) \' >> $(bcmdhd_144_init_file)
	$(Q)echo 'gpio_bt_uart_rts=-1 ' >> $(bcmdhd_144_init_file)
endef