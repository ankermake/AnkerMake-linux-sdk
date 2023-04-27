#-------------------------------------------------------
package_name = soc_dmic
package_depends = utils soc_audio_dma
package_module_src = soc/x2000/dmic
package_make_hook =
package_init_hook =
package_finalize_hook = dmic_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_dmic_init_file = output/soc_dmic.sh

define dmic_finalize_hook
	$(Q)cp soc/x2000/dmic/soc_dmic.ko output/
	$(Q)echo -n 'insmod soc_dmic.ko ' > $(soc_dmic_init_file)
	$(Q)echo -n 'dmic_int0=$(if $(MD_X2000_DMIC_INT0),1,0) ' >> $(soc_dmic_init_file)
	$(Q)echo -n 'dmic_int1=$(if $(MD_X2000_DMIC_INT1),1,0) ' >> $(soc_dmic_init_file)
	$(Q)echo -n 'dmic_int2=$(if $(MD_X2000_DMIC_INT2),1,0) ' >> $(soc_dmic_init_file)
	$(Q)echo -n 'dmic_int3=$(if $(MD_X2000_DMIC_INT3),1,0) ' >> $(soc_dmic_init_file)
	$(Q)echo >> $(soc_dmic_init_file)
endef