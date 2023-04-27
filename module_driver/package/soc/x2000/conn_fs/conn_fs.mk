#-------------------------------------------------------
package_name = soc_conn_fs
package_depends = utils soc_conn
package_module_src = soc/x2000/conn_fs
package_make_hook =
package_init_hook =
package_finalize_hook = conn_fs_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_conn_fs_init_file = output/soc_conn_fs.sh

define conn_fs_finalize_hook
	$(Q)cp soc/x2000/conn_fs/soc_conn_fs.ko output/
	$(Q)echo -n 'insmod soc_conn_fs.ko' > $(soc_conn_fs_init_file)
	$(Q)echo  >> $(soc_conn_fs_init_file)
endef
