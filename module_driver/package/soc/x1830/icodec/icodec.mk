
#-------------------------------------------------------
package_name = soc_icodec
package_depends =
package_module_src = soc/x1830/icodec/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_icodec_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_icodec_init_file = output/soc_icodec.sh

define soc_icodec_finalize_hook
	$(Q)cp soc/x1830/icodec/soc_icodec.ko output/
	$(Q)echo "insmod soc_icodec.ko \\" > $(soc_icodec_init_file)

	$(Q)echo -n "	is_bais=$(if $(MD_X1830_ICODEC_BAIS_ON),1,0) " >> $(soc_icodec_init_file)
	$(Q)echo -n "keep_bias_on=$(if $(MD_X1830_ICODEC_KEEP_BIAS_ON),1,0) " >> $(soc_icodec_init_file)
	$(Q)echo -n "bias_on_voltage_level=$(if $(MD_X1830_ICODEC_KEEP_BIAS_ON),$(MD_X1830_SET_BIAS_ON_VOLTAGE_LEVEL),0) " >> $(soc_icodec_init_file)
	$(Q)echo -n "max_hpout_volume=$(MD_X1830_ICODEC_MAX_HPOUT_VAL) " >> $(soc_icodec_init_file)
	$(Q)echo -n "min_hpout_volume=$(MD_X1830_ICODEC_MIN_HPOUT_VAL) " >> $(soc_icodec_init_file)
	$(Q)echo 'speaker_gpio=$(MD_X1830_ICODEC_SPK_GPIO) ' >> $(soc_icodec_init_file)
endef
