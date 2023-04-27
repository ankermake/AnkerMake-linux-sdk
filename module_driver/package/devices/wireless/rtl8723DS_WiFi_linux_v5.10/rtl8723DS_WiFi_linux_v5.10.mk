#-------------------------------------------------------
package_name = rtl8723ds
package_depends = utils soc_msc
package_module_src = devices/wireless/rtl8723DS_WiFi_linux_v5.10/
package_make_hook =
package_init_hook =
package_finalize_hook = rtl8723ds_finalize_hook
package_clean_hook =
#-------------------------------------------------------

rtl8723ds_init_file = output/rtl8723ds.sh

define rtl8723ds_finalize_hook
	$(Q)cp devices/wireless/rtl8723DS_WiFi_linux_v5.10/8723ds.ko output/rtl8723ds.ko
	$(Q)echo 'insmod rtl8723ds.ko \' > $(rtl8723ds_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_RTL8723DS_MMC_NUM) \' >> $(rtl8723ds_init_file)
	$(Q)echo 'gpio_wl_dis_n=$(MD_RTL8723DS_WLAN_DIS_N) \' >> $(rtl8723ds_init_file)
	$(Q)echo 'gpio_bt_dis_n=$(MD_RTL8723DS_BT_DIS_N) \' >> $(rtl8723ds_init_file)
	$(Q)echo 'gpio_bt_wifi_power_en=$(MD_RTL8723DS_BT_WIFI_POWER_EN) \' >> $(rtl8723ds_init_file)
	$(Q)echo 'bt_wifi_power_valid_level=$(MD_RTL8723DS_BT_WIFI_POWER_VALID_LEVEL) \' >> $(rtl8723ds_init_file)
	$(Q)echo '' >> $(rtl8723ds_init_file)
endef
