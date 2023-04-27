#-------------------------------------------------------
package_name = ssv6x5x
package_depends = utils soc_msc
package_module_src = devices/wireless/ssv6x5x_linux/
package_make_hook =
package_init_hook =
package_finalize_hook = ssv6x5x_finalize_hook
package_clean_hook =
#-------------------------------------------------------

ssv6x5x_init_file = output/ssv6x5x.sh

define ssv6x5x_finalize_hook
	$(Q)cp devices/wireless/ssv6x5x_linux/ssv6x5x.ko output/ssv6x5x.ko
	$(Q)cp devices/wireless/ssv6x5x_linux/ssv6x5x-wifi.cfg output/ssv6x5x-wifi.cfg
	$(Q)echo 'insmod ssv6x5x.ko \' > $(ssv6x5x_init_file)
	$(Q)echo 'stacfgpath=$(MD_SSV6X5X_STACFG_PATH) \' >> $(ssv6x5x_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_SSV6X5X_MMC_NUM) \' >> $(ssv6x5x_init_file)
	$(Q)echo 'wifi_power_valid_level=$(MD_SSV6X5X_WIFI_POWER_VALID_LEVEL) \' >> $(ssv6x5x_init_file)
	$(Q)echo 'gpio_wifi_en=$(MD_SSV6X5X_WIFI_POWER_EN) \' >> $(ssv6x5x_init_file)
	$(Q)echo 'gpio_wifi_rst=$(MD_SSV6X5X_WIFI_RESET) \' >> $(ssv6x5x_init_file)

	$(Q)echo '' >> $(ssv6x5x_init_file)
endef