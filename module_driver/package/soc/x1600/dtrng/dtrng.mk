
#-------------------------------------------------------
package_name = soc_dtrng
package_depends = utils
package_module_src = soc/x1600/dtrng/
package_make_hook =
package_init_hook =
package_finalize_hook = dtrng_finalize_hook
package_clean_hook =
#-------------------------------------------------------

dtrng_init_file = output/soc_dtrng.sh

define dtrng_finalize_hook

	$(Q)cp soc/x1600/dtrng/soc_dtrng.ko output/
	$(Q)echo    "insmod soc_dtrng.ko " > $(dtrng_init_file)

endef