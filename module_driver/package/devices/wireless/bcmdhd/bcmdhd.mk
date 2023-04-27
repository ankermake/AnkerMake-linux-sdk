#-------------------------------------------------------
package_name = bcmdhd
package_depends = utils soc_msc soc_utils
package_module_src = devices/wireless/bcmdhd/
package_make_hook =
package_init_hook =
package_finalize_hook = bcmdhd_finalize_hook
package_clean_hook =
#-------------------------------------------------------

bcmdhd_init_file = output/bcmdhd.sh

define bcmdhd_finalize_hook
	$(Q)cp devices/wireless/bcmdhd/bcmdhd.ko output/
	$(Q)echo 'insmod bcmdhd.ko \' > $(bcmdhd_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_BCMDHD_MMC_NUM) \' >> $(bcmdhd_init_file)
	$(Q)echo 'gpio_bt_wifi_power=$(MD_BCMDHD_BT_WIFI_POWER) \' >> $(bcmdhd_init_file)
	$(Q)echo 'gpio_wlan_reg_on=$(MD_BCMDHD_WLAN_REG_ON) \' >> $(bcmdhd_init_file)
	$(Q)echo 'gpio_wlan_wake_host=$(MD_BCMDHD_WLAN_WAKE_HOST) \' >> $(bcmdhd_init_file)
	$(Q)echo 'gpio_bt_rst_n=-1 \' >> $(bcmdhd_init_file)
	$(Q)echo 'gpio_bt_reg_on=$(MD_BCMDHD_BT_REG_ON) \' >> $(bcmdhd_init_file)
	$(Q)echo 'gpio_host_wake_bt=$(MD_BCMDHD_HOST_WAKE_BT) \' >> $(bcmdhd_init_file)
	$(Q)echo 'gpio_bt_wake_host=$(MD_BCMDHD_BT_WAKE_HOST) \' >> $(bcmdhd_init_file)
	$(Q)echo 'gpio_bt_uart_rts=-1 ' >> $(bcmdhd_init_file)
endef