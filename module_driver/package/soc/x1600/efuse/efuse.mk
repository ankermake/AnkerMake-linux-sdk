#-------------------------------------------------------
package_name = soc_efuse
package_depends = utils
package_module_src = soc/x1600/efuse
package_make_hook =
package_init_hook =
package_finalize_hook = efuse_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_efuse_init_file = output/soc_efuse.sh

define efuse_finalize_hook
	$(Q)cp soc/x1600/efuse/soc_efuse.ko output/
	$(Q)echo -n 'insmod soc_efuse.ko ' > $(soc_efuse_init_file)
	$(Q)echo    'gpio_efuse_vddq=$(MD_X1600_EFUSE_VDDQ) ' >> $(soc_efuse_init_file)
endef