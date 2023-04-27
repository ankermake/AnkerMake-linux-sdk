
#-------------------------------------------------------
package_name = soc_mscaler
package_depends = utils
package_module_src = soc/x1830/mscaler/
package_make_hook =
package_init_hook = soc_mscaler_init_hook
package_finalize_hook = soc_mscaler_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_mscaler_init_file = output/soc_mscaler.sh

define soc_mscaler_init_hook
	./soc/x1830/mscaler/coef.sh $(MD_X1830_MSCALER_DETAIL)
endef

define soc_mscaler_finalize_hook
	$(Q)cp soc/x1830/mscaler/soc_mscaler.ko output/
	$(Q)echo -n 'insmod soc_mscaler.ko ' > $(soc_mscaler_init_file)
endef
