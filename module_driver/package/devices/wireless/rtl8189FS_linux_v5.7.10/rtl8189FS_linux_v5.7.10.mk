#-------------------------------------------------------
package_name = rtl8189fs
package_depends = utils
package_module_src = devices/wireless/rtl8189FS_linux_v5.7.10/
package_make_hook =
package_init_hook =
package_finalize_hook = rtl8189fs_finalize_hook
package_clean_hook =
#-------------------------------------------------------

rtl8189fs_init_file = output/rtl8189fs.sh

define rtl8189fs_finalize_hook
	$(Q)cp devices/wireless/rtl8189FS_linux_v5.7.10/8189fs.ko output/rtl8189fs.ko
	$(Q)echo 'insmod rtl8189fs.ko \' > $(rtl8189fs_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_RTL8189FS_MMC_NUM) \' >> $(rtl8189fs_init_file)
	$(Q)echo 'gpio_wl_dis_n=$(MD_RTL8189FS_WLAN_DIS_N) \' >> $(rtl8189fs_init_file)
	$(Q)echo 'gpio_bt_dis_n=$(MD_RTL8189FS_BT_DIS_N) \' >> $(rtl8189fs_init_file)
	$(Q)echo 'gpio_bt_wifi_power_en=$(MD_RTL8189FS_BT_WIFI_POWER_EN) \' >> $(rtl8189fs_init_file)
	$(Q)echo '' >> $(rtl8189fs_init_file)
endef
