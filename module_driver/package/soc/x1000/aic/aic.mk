
#-------------------------------------------------------
package_name = soc_aic
package_depends =
package_module_src = soc/x1000/aic/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_aic_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_aic_init_file = output/soc_aic.sh

define soc_aic_finalize_hook
	$(Q)cp soc/x1000/aic/soc_aic.ko output/
	$(Q)echo "insmod soc_aic.ko " > $(soc_aic_init_file)
endef
