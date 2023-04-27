#-------------------------------------------------------
package_name = rmem_manager
package_depends = utils
package_module_src = drivers/rmem_manager
package_make_hook =
package_init_hook =
package_finalize_hook = rmem_manager_finalize_hook
package_clean_hook =
#-------------------------------------------------------

rmem_manager_init_file = output/rmem_manager.sh

define rmem_manager_finalize_hook
	$(Q)cp drivers/rmem_manager/rmem_manager.ko output/
	$(Q)cat package/drivers/rmem_manager/get_rmem.sh > $(rmem_manager_init_file)
	$(Q)echo -n 'insmod rmem_manager.ko ' >> $(rmem_manager_init_file)
	$(Q)echo -n ' rmem_start=$$rmem_start' >> $(rmem_manager_init_file)
	$(Q)echo -n ' rmem_size=$$rmem_size' >> $(rmem_manager_init_file)
	$(Q)echo >> $(rmem_manager_init_file)
endef
