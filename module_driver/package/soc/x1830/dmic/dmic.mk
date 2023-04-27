#-------------------------------------------------------
package_name = soc_dmic
package_depends = utils
package_module_src = soc/x1830/dmic
package_make_hook =
package_init_hook =
package_finalize_hook = dmic_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_dmic_init_file = output/soc_dmic.sh

define dmic_finalize_hook
	$(Q)cp soc/x1830/dmic/soc_dmic.ko output/
	$(Q)echo 'insmod soc_dmic.ko ' > $(soc_dmic_init_file)
endef