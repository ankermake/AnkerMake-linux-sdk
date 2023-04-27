
#-------------------------------------------------------
package_name = soc_utils
package_depends = utils
package_module_src = soc/x1600_510/utils
package_make_hook =
package_init_hook =
package_finalize_hook = soc_utils_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_utils_init_file = output/soc_utils.sh

define soc_utils_finalize_hook
	$(Q)cp soc/x1600_510/utils/soc_utils.ko output/
	$(Q)echo -n 'insmod soc_utils.ko ' > $(soc_utils_init_file)
	$(if $(MD_X1600_510_UTILS_RTC32K), $(Q)echo -n ' rtc32k_init_on=$(if $(MD_X1600_510_UTILS_RTC32K_INIT_ON),1,0)' >> $(soc_utils_init_file))
	$(if $(MD_X1600_510_UTILS_CLK24M), $(Q)echo -n ' clk24m_init_on=$(if $(MD_X1600_510_UTILS_CLK24M_INIT_ON),1,0)' >> $(soc_utils_init_file))
	$(Q)echo >> $(soc_utils_init_file)

endef
