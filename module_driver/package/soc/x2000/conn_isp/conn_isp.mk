#-------------------------------------------------------
package_name = soc_conn_isp
package_depends = utils soc_conn
package_module_src = soc/x2000/conn_isp
package_make_hook =
package_init_hook =
package_finalize_hook = conn_isp_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_conn_isp_init_file = output/soc_conn_isp.sh

define conn_isp_finalize_hook
	$(Q)cp soc/x2000/conn_isp/soc_conn_isp.ko output/
	$(Q)echo 'insmod soc_conn_isp.ko ' > $(soc_conn_isp_init_file)
endef