#-------------------------------------------------------
package_name = esp8089
package_depends = utils
package_module_src = devices/wireless/esp8089_linux/
package_make_hook =
package_init_hook =
package_finalize_hook = esp8089_finalize_hook
package_clean_hook =
#-------------------------------------------------------

esp8089_init_file = output/esp8089.sh

define esp8089_finalize_hook
	$(Q)cp devices/wireless/esp8089_linux/src/esp8089.ko output/esp8089.ko
	$(Q)echo 'insmod esp8089.ko \' > $(esp8089_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_ESP8089_MMC_NUM) \' >> $(esp8089_init_file)
	$(Q)echo 'gpio_wlan_reg_on=$(MD_ESP8089_WLAN_REG_ON) \' >> $(esp8089_init_file)
	$(Q)echo '' >> $(esp8089_init_file)
endef