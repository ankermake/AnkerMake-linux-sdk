#-------------------------------------------------------
package_name = soc_conn_pcm_playback
package_depends = utils soc_conn
package_module_src = soc/x2000/conn_pcm/conn_pcm_playback
package_make_hook =
package_init_hook =
package_finalize_hook = conn_pcm_playback_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_conn_pcm_playback_init_file = output/soc_conn_pcm_playback.sh

define conn_pcm_playback_finalize_hook
	$(Q)cp soc/x2000/conn_pcm/conn_pcm_playback/soc_conn_pcm_playback.ko output/
	$(Q)echo 'insmod soc_conn_pcm_playback.ko ' > $(soc_conn_pcm_playback_init_file)
endef