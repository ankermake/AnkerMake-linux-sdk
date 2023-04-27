
#-------------------------------------------------------
package_name = soc_adc
package_depends =
package_module_src = soc/x1021/adc/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_adc_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_adc_init_file = output/soc_adc.sh

define soc_adc_finalize_hook
	$(Q)cp soc/x1021/adc/soc_adc.ko output/
	$(Q)echo -n 'insmod soc_adc.ko ' > $(soc_adc_init_file)
	$(Q)echo    'adc_vref=$(MD_X1021_ADC_VREF) ' >> $(soc_adc_init_file)
endef
