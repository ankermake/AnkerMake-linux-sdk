
#-------------------------------------------------------
package_name = soc_aes
package_depends =
package_module_src = soc/x2000/aes/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_aes_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_aes_init_file = output/soc_aes.sh

define soc_aes_finalize_hook
	$(Q)cp soc/x2000/aes/soc_aes.ko output/
	$(Q)echo 'insmod soc_aes.ko ' > $(soc_aes_init_file)
endef
