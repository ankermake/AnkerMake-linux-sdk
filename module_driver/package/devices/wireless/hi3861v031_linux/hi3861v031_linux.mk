#-------------------------------------------------------
package_name = hichannel
package_depends = utils
package_module_src = devices/wireless/hi3861v031_linux/
package_make_hook =
package_init_hook =
package_finalize_hook = hichannel_finalize_hook
package_clean_hook =
#-------------------------------------------------------

hichannel_init_file = output/hichannel.sh

define hichannel_finalize_hook
	$(Q)cp devices/wireless/hi3861v031_linux/hichannel.ko output/hichannel.ko
	$(Q)echo 'insmod hichannel.ko \' > $(hichannel_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_HI3861_MMC_NUM) \' >> $(hichannel_init_file)
	$(Q)echo 'gpio_wifi_int=$(MD_HI3861_WIFI_INT) \' >> $(hichannel_init_file)
	$(Q)echo 'gpio_wifi_power_on=$(MD_HI3861_WIFI_POWER_ON) \' >> $(hichannel_init_file)
	$(Q)echo 'gpio_wifi_power_en=$(MD_HI3861_WIFI_POWER_EN) \' >> $(hichannel_init_file)
	$(Q)echo '' >> $(hichannel_init_file)
endef