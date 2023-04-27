#-------------------------------------------------------
package_name = cywdhd
package_depends = soc_utils utils
package_module_src = devices/wireless/bcmdhd_1.363.125.19_infineon/
package_make_hook =
package_init_hook =
package_finalize_hook = cywdhd_finalize_hook
package_clean_hook =
#-------------------------------------------------------

cywdhd_init_file = output/cywdhd.sh

define cywdhd_finalize_hook
	$(Q)cp devices/wireless/bcmdhd_1.363.125.19_infineon/cywdhd.ko output/
	$(Q)echo 'insmod cywdhd.ko \' > $(cywdhd_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_BCMDHD_1_363_125_19_MMC_NUM) \' >> $(cywdhd_init_file)
	$(Q)echo 'bt_wifi_power_valid_level=$(MD_BCMDHD_1_363_125_19_BT_WIFI_POWER_VALID_LEVEL) \' >> $(cywdhd_init_file)
	$(Q)echo 'gpio_bt_wifi_power=$(MD_BCMDHD_1_363_125_19_BT_WIFI_POWER) \' >> $(cywdhd_init_file)
	$(Q)echo 'gpio_wlan_reg_on=$(MD_BCMDHD_1_363_125_19_WLAN_REG_ON) \' >> $(cywdhd_init_file)
	$(Q)echo 'gpio_wlan_wake_host=$(MD_BCMDHD_1_363_125_19_WLAN_WAKE_HOST) \' >> $(cywdhd_init_file)
	$(Q)echo 'gpio_bt_rst_n=-1 \' >> $(cywdhd_init_file)
	$(Q)echo 'gpio_bt_reg_on=$(MD_BCMDHD_1_363_125_19_BT_REG_ON) \' >> $(cywdhd_init_file)
	$(Q)echo 'gpio_host_wake_bt=$(MD_BCMDHD_1_363_125_19_HOST_WAKE_BT) \' >> $(cywdhd_init_file)
	$(Q)echo 'gpio_bt_wake_host=$(MD_BCMDHD_1_363_125_19_BT_WAKE_HOST) \' >> $(cywdhd_init_file)
	$(Q)echo 'gpio_bt_uart_rts=-1 ' >> $(cywdhd_init_file)
endef