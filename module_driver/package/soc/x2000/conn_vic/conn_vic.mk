#-------------------------------------------------------
package_name = soc_conn_vic
package_depends = utils soc_conn
package_module_src = soc/x2000/conn_vic
package_make_hook =
package_init_hook =
package_finalize_hook = conn_vic_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_conn_vic_init_file = output/soc_conn_vic.sh

define conn_vic_finalize_hook
	$(Q)cp soc/x2000/conn_vic/soc_conn_vic.ko output/
	$(Q)echo 'insmod soc_conn_vic.ko ' > $(soc_conn_vic_init_file)
endef