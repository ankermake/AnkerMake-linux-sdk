
#-------------------------------------------------------
package_name = soc_utils
package_depends = utils
package_module_src = soc/x1830/utils
package_make_hook =
package_init_hook =
package_finalize_hook = soc_utils_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_utils_init_file = output/soc_utils.sh

ifeq ($(MD_X1830_UTILS_RTC32K),y)
define X1830_UTILS_RTC
	$(Q)echo "	rtc32k_init_on=$(if $(MD_X1830_UTILS_RTC32K_INIT_ON),1,0) " >> $(soc_utils_init_file)
endef
endif

define soc_utils_finalize_hook
	$(Q)cp soc/x1830/utils/soc_utils.ko output/
	$(Q)echo "insmod soc_utils.ko \\" > $(soc_utils_init_file)

	$(X1830_UTILS_RTC)
endef