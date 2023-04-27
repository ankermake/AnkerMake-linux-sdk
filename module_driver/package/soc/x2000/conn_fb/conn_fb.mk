#-------------------------------------------------------
package_name = soc_conn_fb
package_depends = utils soc_conn
package_module_src = soc/x2000/conn_fb
package_make_hook =
package_init_hook =
package_finalize_hook = conn_fb_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_conn_fb_init_file = output/soc_conn_fb.sh

define conn_fb_finalize_hook
	$(Q)cp soc/x2000/conn_fb/soc_conn_fb.ko output/
	$(Q)echo 'insmod soc_conn_fb.ko ' > $(soc_conn_fb_init_file)
endef