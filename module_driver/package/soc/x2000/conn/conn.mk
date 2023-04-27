#-------------------------------------------------------
package_name = soc_conn
package_depends = utils
package_module_src = soc/x2000/conn
package_make_hook =
package_init_hook =
package_finalize_hook = conn_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_conn_init_file = output/soc_conn.sh

define conn_finalize_hook
	$(Q)cp soc/x2000/conn/soc_conn.ko output/
	$(Q)echo -n 'insmod soc_conn.ko ' > $(soc_conn_init_file)
	$(Q)echo 'conn_max_link_count=$(MD_X2000_CONN_MAX_LINK_COUNT) ' >> $(soc_conn_init_file)
endef