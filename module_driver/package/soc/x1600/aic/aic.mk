
#-------------------------------------------------------
package_name = soc_aic
package_depends =
package_module_src = soc/x1600/aic/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_aic_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_aic_init_file = output/soc_aic.sh

define soc_aic_finalize_hook
	$(Q)cp soc/x1600/aic/soc_aic.ko output/
	$(Q)echo "insmod soc_aic.ko \\" > $(soc_aic_init_file)
	$(Q)echo "aic_if_send_invalid_data=$(if $(MD_X1600_AIC_IF_SEND_INVALID_DATA),1,0) \\" >> $(soc_aic_init_file)
	$(Q)echo "aic_if_send_invalid_data_everytime=$(if $(MD_X1600_AIC_IF_SEND_INVALID_DATA_EVERYTIME),1,0) \\" >> $(soc_aic_init_file)
	$(Q)echo "aic_send_invalid_data_time_ms=$(MD_X1600_AIC_SEND_INVALID_DATA_TIME_MS) " >> $(soc_aic_init_file)
	$(Q)echo  >> $(soc_aic_init_file)
endef
