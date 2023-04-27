
#-------------------------------------------------------
package_name = soc_hash
package_depends = utils
package_module_src = soc/x1600/hash/
package_make_hook =
package_init_hook =
package_finalize_hook = hash_finalize_hook
package_clean_hook =
#-------------------------------------------------------

hash_init_file = output/soc_hash.sh

define hash_finalize_hook

	$(Q)cp soc/x1600/hash/soc_hash.ko output/
	$(Q)echo    "insmod soc_hash.ko " > $(hash_init_file)

endef